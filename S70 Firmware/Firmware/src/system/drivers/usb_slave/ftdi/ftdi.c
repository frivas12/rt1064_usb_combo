/**
 * @file ftdi.c
 *
 * @brief Functions for Initialize the FT234XD-R chip and create the ftdi task.
 * In Windows, you need to make sure the FTDI driver latency timer is set to 1/
 *
 */

#include "ftdi.h"

#include <string.h>

#include "Debugging.h"
#include "FreeRTOSConfig.h"
#include "apt.h"
#include "apt_parse.h"
#include "pins.h"
#include "portmacro.h"
#include "projdefs.h"
#include "queue.h"
#include "sams70n21.h"
#include "task.h"
#include "uart.h"
#include "usb_slave.h"
#include "usr_interrupt.h"

typedef struct ftdi_tx_buffer {
    uint8_t buffer[FTDI_TX_BUFFER_SIZE];
    size_t length;

    // Note--LL needs to be atomic
    struct ftdi_tx_buffer* next;
} ftdi_tx_buffer_t;

typedef struct {
    uint8_t buffer[FTDI_RX_BUFFER_SIZE];
    uint8_t* tail;
    SemaphoreHandle_t bytes_in_buffer;
} ftdi_isr_buffer_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
/*FTDI Task Handle*/
static TaskHandle_t xFTDIHandle;

static ftdi_isr_buffer_t isr_buffers[2];
static ftdi_isr_buffer_t* atomic_buffer_ptr      = isr_buffers;
static ftdi_isr_buffer_t* application_buffer_ptr = isr_buffers + 1;
static uint8_t* application_buffer_head          = isr_buffers[1].buffer;

static ftdi_tx_buffer_t ftdi_tx_buffers[FTDI_TX_BUFFER_COUNT];
static SemaphoreHandle_t available_tx_buffers;
static uint8_t* tx_head = NULL;

// atomic
static ftdi_tx_buffer_t* enqueued_tx_buffers_head = NULL;
static ftdi_tx_buffer_t* enqueued_tx_buffers_tail = NULL;
static ftdi_tx_buffer_t* free_tx_buffers          = NULL;

// Large buffers moved off task.
static USB_Slave_Message local_slave_message;
static uint8_t local_builder_buffer[FTDI_RX_BUFFER_SIZE];
static USB_Slave_Message_Builder local_builder;

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Declare a binary Semaphore flag for the USB slave Bus. To ensure only single
 * access to USB Slave Bus. */
SemaphoreHandle_t xFTDI_Slave_TxSemaphore =
    NULL;  // removed STATIC to allow other processes to use same semaphore.

typedef struct {
    bool got_error;
} builder_context;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

static void configure_usart(void);

static void ftdi_isr_buffer_init(ftdi_isr_buffer_t* obj,
                                 SemaphoreHandle_t semaphore);
static bool ftdi_isr_buffer_enqueue(ftdi_isr_buffer_t* buffer, uint8_t value,
                                    BaseType_t* pxHigherPriorityTaskAwoken);

/// \brief Reads up to the specified number of bytes from the application
/// buffer.
/// \returns The number of bytes written to dest.  Guaranteed to be in the range
/// [0, bytes_to_read].
static size_t read_from_application_buffer(void* _dest, size_t bytes_to_read);

/// \brief Returns the number of bytes read into dest within the timeout.
static size_t read_inbound_bytes(void* dest, size_t bytes_to_read,
                                 TickType_t timeout);

static void atomic_swap_buffers(void);
static void consume_tx_buffer_from_isr(BaseType_t* pxHigherPriorityTaskAwoken);
static size_t enqueue_tx_message(const void* _src, size_t length,
                                 TickType_t timeout);

static size_t builder_context_read(void * ctx, void * dest, size_t bytes_to_read, TickType_t timeout);
static void builder_context_on_error(void * ctx, USB_Slave_Message_Builder_State state, void * param);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
/**
 * @brief All the RX data from the FT234XD uart coming in here into a fifo which
 * is later parsed in the ftdi task.
 */
ISR(UART1_Handler) {
    uint8_t incoming_byte;
    BaseType_t higherPriorityTaskAwoken = pdFALSE;

    // Check if data is ready to be read.
    if (uart_read(UART1, &incoming_byte) == 0) {
        // debug_print("FTDI RX: %d", incoming_byte);

        // Push data into the buffer
        if (!ftdi_isr_buffer_enqueue(atomic_buffer_ptr, incoming_byte,
                                     &higherPriorityTaskAwoken)) {
            usb_ftdi_print("ERROR: The FTDI USB Slave buffer full. ISR/r/n");
        }
    }

    if (uart_is_tx_ready(UART1)) {
        // Try to send the next byte, or turn off this interrupt.
        consume_tx_buffer_from_isr(&higherPriorityTaskAwoken);
    }

    uart_reset_status(UART1);
    portYIELD_FROM_ISR(higherPriorityTaskAwoken);
}
/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void configure_usart(void) {
    sysclk_enable_peripheral_clock(ID_UART1);
    pio_set_peripheral(PIOA, PIO_PERIPH_C, PIO_PA4);  // FTDI_RX
    pio_set_peripheral(PIOA, PIO_PERIPH_C, PIO_PA5);  // FTDI_TX
    // PIOA, PIO_PA27 is our CTS but not really
    // PIOD, PIO_PD14 is our RTS but not really
    const sam_uart_opt_t uartSettings = {sysclk_get_peripheral_hz(),
                                         FTDI_BUADRATE, UART_MR_PAR_NO};
    const uint32_t ERROR              = uart_init(UART1, &uartSettings);
    configASSERT(ERROR == 0);

    /* Enable Rx Ready Interrupt on UART1*/
    uart_enable_interrupt(UART1, UART_IER_RXRDY);
    /* must set the interrupt priority lower priority than
     * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY*/
    irq_register_handler(UART1_IRQn,
                         configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
}

/****************************************************************************
 * Task
 ****************************************************************************/
/**
 * @brief This is the FTDI task.  It uses two ping pong buffers swapped at the
 * task service start between the task and uart interrupt.  The task then adds
 * the new data to a third buffer which is used to parse the data.
 *
 * @param pvParameters	: Not used.
 */
static void task_ftdi(void* pvParameters) {
    UNUSED(pvParameters);

    /* Create the structure for holding the USB data*/
    local_slave_message.ftdi_flag              = true;
    local_slave_message.write                  = ftdi_write;
    local_slave_message.xUSB_Slave_TxSemaphore = xFTDI_Slave_TxSemaphore;

    usb_slave_message_builder_init(&local_builder, local_builder_buffer,
                                   FTDI_RX_BUFFER_SIZE, 5);

    configure_usart();

    builder_context context;

    for (;;) {
        context.got_error = false;
        const bool GOT_MESSAGE = usb_slave_message_builder_reading_service(
            &local_builder, &local_slave_message, APT_PARSER_TIMEOUT_PERIOD,
            &context, builder_context_read, builder_context_on_error);
        if (GOT_MESSAGE) {
            apt_parse(&local_slave_message);
        } else {
            atomic_swap_buffers();

            if (!context.got_error) {
                usb_ftdi_print("timeout reached: %d bytes dropped", local_builder.buffer_length);
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

iram_size_t ftdi_write(const void* type_erased_buf, iram_size_t nbytes) {
    iram_size_t rt = 0;

    while (nbytes > 0) {
        const size_t ENQUEUED =
            enqueue_tx_message(type_erased_buf, nbytes, portMAX_DELAY);
        nbytes -= ENQUEUED;
        rt += ENQUEUED;
        type_erased_buf = (const uint8_t*)type_erased_buf + ENQUEUED;
    }

    return rt;
}

void ftdi_init(void) {
    /**
     * Create a Mutex for sending data on the USBsalve port
     * Check to see if the semaphore has not been created.
     */
    if (xFTDI_Slave_TxSemaphore == NULL) {
        /* Then create the FTDI USB Tx bus mutex semaphore */
        xFTDI_Slave_TxSemaphore = xSemaphoreCreateMutex();
        if ((xFTDI_Slave_TxSemaphore) != NULL) {
            xSemaphoreGive((xFTDI_Slave_TxSemaphore)); /* make the USB slave Tx
                                                          bus available */
            NameQueueObject(xFTDI_Slave_TxSemaphore, "FTDI L");
        }
    }

    // Create the RX queues.
    SemaphoreHandle_t bytes_in_buffer_semaphores[2] = {0};
    for (int i = 0; i < 2; ++i) {
        bytes_in_buffer_semaphores[i] =
            xSemaphoreCreateCounting(FTDI_RX_BUFFER_SIZE, 0);
        ftdi_isr_buffer_init(isr_buffers + i, bytes_in_buffer_semaphores[i]);
    }
    configASSERT(bytes_in_buffer_semaphores[0] != NULL &&
                 bytes_in_buffer_semaphores[1] != NULL);

    // Create the FTDI message queue.
    available_tx_buffers =
        xSemaphoreCreateCounting(FTDI_TX_BUFFER_COUNT, FTDI_TX_BUFFER_COUNT);
    configASSERT(available_tx_buffers != NULL);

    // The setup of the free list needs to be here
    // since the TxSemaphore is available externally.
    ftdi_tx_buffers[0].next = NULL;
    for (size_t i = 1; i < FTDI_TX_BUFFER_COUNT; ++i) {
        ftdi_tx_buffers[i].next = ftdi_tx_buffers + i - 1;
    }
    free_tx_buffers = ftdi_tx_buffers + FTDI_TX_BUFFER_COUNT - 1;

    /* Create task to for the FTDI slave */
    if (xTaskCreate(task_ftdi, "FTDI", TASK_FTDI_STACK_SIZE, NULL,
                    TASK_FTDI_STACK_PRIORITY, &xFTDIHandle) != pdPASS) {
        error_print("Failed to create FTDI task\r\n");
    }
}

static void ftdi_isr_buffer_init(ftdi_isr_buffer_t* obj,
                                 SemaphoreHandle_t semaphore) {
    obj->tail            = obj->buffer;
    obj->bytes_in_buffer = semaphore;
}

static bool ftdi_isr_buffer_enqueue(ftdi_isr_buffer_t* buffer, uint8_t value,
                                    BaseType_t* pxHigherPriorityTaskAwoken) {
    if (buffer->tail - buffer->buffer == FTDI_RX_BUFFER_SIZE) {
        return false;
    }
    *(buffer->tail++) = value;
    xSemaphoreGiveFromISR(buffer->bytes_in_buffer, pxHigherPriorityTaskAwoken);
    return true;
}

static size_t read_from_application_buffer(void* _dest, size_t bytes_to_read) {
    uint8_t* dest = (uint8_t*)_dest;
    const size_t bytes_in_application_space =
        application_buffer_ptr->tail - application_buffer_head;
    const size_t BYTES_READ = Min(bytes_to_read, bytes_in_application_space);
    memcpy(dest, application_buffer_head, BYTES_READ);
    application_buffer_head += BYTES_READ;

    return BYTES_READ;
}

static size_t read_inbound_bytes(void* _dest, size_t bytes_to_read,
                                 TickType_t timeout) {
    uint8_t* dest = (uint8_t*)_dest;

    // Read out data from the current application buffer.
    size_t rt = read_from_application_buffer(dest, bytes_to_read);
    bytes_to_read -= rt;
    dest += rt;

    // If more data needs to be read, wait for it or the timeout.
    for (size_t cntr = 0; cntr < bytes_to_read; ++cntr) {
        const TickType_t START = xTaskGetTickCount();
        if (xSemaphoreTake(atomic_buffer_ptr->bytes_in_buffer, timeout) !=
            pdTRUE) {
            break;
        }
        const TickType_t DELTA = xTaskGetTickCount() - START;

        timeout = DELTA >= timeout ? 0 : (timeout - DELTA);
    }

    // Only do the swap if the loop was executed.
    if (bytes_to_read > 0) {
        // Move any bytes available into the application.
        atomic_swap_buffers();
    }

    // If more data needs to be read, read out what is available in the
    // application buffer.
    rt += read_from_application_buffer(dest, bytes_to_read);
    return rt;
}

static void atomic_swap_buffers(void) {
    // Reset the application buffer.
    // We wait on the semaphore to ensure that it is cleared.
    application_buffer_ptr->tail = application_buffer_ptr->buffer;
    while (xSemaphoreTake(application_buffer_ptr->bytes_in_buffer, 0) ==
           pdTRUE);

    irqflags_t flags = cpu_irq_save();
    ptrdiff_t delta  = atomic_buffer_ptr == isr_buffers ? 1 : -1;
    atomic_buffer_ptr += delta;
    cpu_irq_restore(flags);
    application_buffer_ptr += -delta;

    application_buffer_head = application_buffer_ptr->buffer;
}

static void consume_tx_buffer_from_isr(BaseType_t* pxHigherPriorityTaskAwoken) {
reloaded_tx_buffer:
    // If the head buffer is does not exist, stop the ISR.
    if (enqueued_tx_buffers_head == NULL) {
        uart_disable_interrupt(UART1, UART_IER_TXRDY);
        return;
    }

    if (tx_head == NULL) {
        tx_head = enqueued_tx_buffers_head->buffer;
    }

    // If the head buffer is empty, go to the next buffer and restart.
    if (tx_head - enqueued_tx_buffers_head->buffer ==
        (ptrdiff_t)enqueued_tx_buffers_head->length) {
        ftdi_tx_buffer_t* const just_freed = enqueued_tx_buffers_head;
        enqueued_tx_buffers_head           = enqueued_tx_buffers_head->next;
        just_freed->next                   = free_tx_buffers;
        free_tx_buffers                    = just_freed;
        xSemaphoreGiveFromISR(available_tx_buffers, pxHigherPriorityTaskAwoken);
        tx_head = NULL;
        goto reloaded_tx_buffer;
    }

    // Otherwise, send over the next byte
    uart_write(UART1, *tx_head);
    ++tx_head;
}

static size_t enqueue_tx_message(const void* src, size_t length,
                                 TickType_t timeout) {
    if (length == 0 ||
        xSemaphoreTake(available_tx_buffers, timeout) != pdTRUE) {
        return 0;
    }

    // Atomically dequeue the available queue.
    ftdi_tx_buffer_t* buffer;
    irqflags_t FLAGS = cpu_irq_save();
    buffer           = free_tx_buffers;
    free_tx_buffers  = free_tx_buffers->next;
    cpu_irq_restore(FLAGS);
    configASSERT(buffer != NULL);

    // Setup the buffer object outside of the ISR.
    const size_t LENGTH = Min(length, FTDI_TX_BUFFER_SIZE);
    memcpy(buffer->buffer, src, LENGTH);
    buffer->length = LENGTH;
    buffer->next   = NULL;
    length -= LENGTH;

    // Append to the end of the queue and enable the interrupt.
    FLAGS                     = cpu_irq_save();
    const bool NOTHING_QUEUED = enqueued_tx_buffers_head == NULL;
    if (NOTHING_QUEUED) {
        enqueued_tx_buffers_head = buffer;
        uart_enable_interrupt(UART1, UART_IER_TXRDY);
    } else {
        enqueued_tx_buffers_tail->next = buffer;
    }
    enqueued_tx_buffers_tail = buffer;
    cpu_irq_restore(FLAGS);

    return LENGTH;
}

static size_t builder_context_read(void * ctx, void * dest, size_t bytes_to_read, TickType_t timeout) {
    (void)ctx;
    return read_inbound_bytes(dest, bytes_to_read, timeout);
}

static void builder_context_on_error(void * ctx, USB_Slave_Message_Builder_State state, void * param) {
    builder_context * const context = (builder_context*)ctx;

    switch (state)
    {
    case USBSMBS_COMMAND_OVERFLOWED:
        context->got_error = true;
        usb_ftdi_print("USB Slave large command rejection:  extended length %d", *(size_t*)param);
        break;

    case USBSMBS_REALIGN_UNAVAILABLE:
        context->got_error = true;
        break;

    // Normal states.
    default:
        break;
    }
}
