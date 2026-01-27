/**
 * @file
 *
 * @brief USB slave device handling functions
 *
 * USB data received flow diagram
 * ======================
 *
 *@dot
 digraph G {
 node [shape=parallelogram]; "verify"; "parse general"; "usb tx"; "parse
 stepper"; "parse servo"; "parse shutter"; "init"; node [shape=doublecircle];
 "vTaskDelay"; node [shape=diamond]; "slot type"; node [shape=ellipse]; "receive
 data"; "data to stepper task"; "data to servo task"; "data to shutter task";
 "init"->"vTaskDelay"->"receive data";
 "receive data"->"verify"->"parse general"->"usb tx"->"vTaskDelay";
 "parse general"->"slot type";
 "slot type"->"parse stepper"->"data to stepper task"->"vTaskDelay";
 "slot type"->"parse servo"->"data to servo task"->"vTaskDelay";
 "slot type"->"parse shutter"->"data to shutter task"->"vTaskDelay";
 { rank=min; "vTaskDelay" }
 }
 * @enddot
 */

#include "usb_slave.h"

#include <asf.h>

#include "25lc1024.h"
#include "Debugging.h"
#include "FreeRTOSConfig.h"
#include "apt.h"
#include "apt_parse.h"
#include "board.h"
#include "compiler.h"
#include "cpld_program.h"
#include "rt_program.h"
#include "hid.h"
#include "pio.h"
#include "portmacro.h"
#include "projdefs.h"
#include "slots.h"
#include "string.h"
#include "sys_task.h"
#include "udi_cdc.h"
#include "usb_host.h"
#include "usb_slave.h"

#define USE_NOTIFY 0

#if !USE_NOTIFY
#include "semphr.h"
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Declare a binary Semaphore flag for the USB slave Bus. To ensure only single
 * access to USB Slave Bus. */
SemaphoreHandle_t xUSB_Slave_TxSemaphore;  // removed STATIC to allow other
                                           // processes to use same semaphore.

typedef struct {
    bool got_error;
} builder_context;

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* Holds the handle of a USB slave task performing Rx data transfers on the PC
 * USB port*/
static TaskHandle_t xUSB_Rx_Task = NULL;

#if !USE_NOTIFY
static SemaphoreHandle_t g_data_ready;
#endif

// Thread buffers allocated statically to prevent stack overflow problems.
static uint8_t builder_buffer[USB_SLAVE_BUFFER_SIZE];

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static bool usb_slave_check_alignment(const USB_Slave_Message_Builder* builder);
static bool usb_slave_try_realign(USB_Slave_Message_Builder* builder);

static size_t read_cdc_into_buffer(void* buffer, size_t max_read);
static bool wait_for_data(TickType_t timeout);
static size_t builder_context_read(void* ctx, void* _dest, size_t bytes_to_read,
                                   TickType_t timeout);
static void builder_context_on_error(void* ctx,
                                     USB_Slave_Message_Builder_State state,
                                     void* param);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

void prvUSB_slave_Rx_Handler(uint8_t port) {
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Notify the sending that that the Tx has completed. */
    if (xUSB_Rx_Task != NULL) {
#if USE_NOTIFY
        vTaskNotifyGiveFromISR(xUSB_Rx_Task, &xHigherPriorityTaskWoken);
#else
        xSemaphoreGiveFromISR(g_data_ready, &xHigherPriorityTaskWoken);
#endif
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Task
 ****************************************************************************/

static void task_usb_slave_rx(void* pvParameters) {
    /* Remove compiler warnings about unused parameters. */
    (void)pvParameters;

    /* Create the structure for holding the USB data*/
    USB_Slave_Message_Builder builder;
    USB_Slave_Message slave_message;
    slave_message.ftdi_flag              = false;
    slave_message.write                  = udi_cdc_write_buf;
    slave_message.xUSB_Slave_TxSemaphore = xUSB_Slave_TxSemaphore;

    usb_slave_message_builder_init(&builder, builder_buffer,
                                   USB_SLAVE_BUFFER_SIZE, 4);

    builder_context context;
    size_t lattice_buffer_length = 0;
    size_t rt_buffer_length = 0;
    for (;;) {
        if (load_lattice_jed) {
            // (sbenish)  Since we're not using the builder buffer when loading
            // the lattice,
            //            let's reuse the builder buffer.

            const bool data_read = wait_for_data(pdMS_TO_TICKS(1));
            // When the CPLD is programming, have the I/O thread clear the
            // WDT.  The supervisor is shut down during this operation.
            if (!data_read) {
                wdt_restart(WDT);
                continue;
            }

            size_t bytes_read = read_cdc_into_buffer(
                builder_buffer + lattice_buffer_length,
                USB_SLAVE_BUFFER_SIZE - lattice_buffer_length);
            lattice_buffer_length += bytes_read;

            uint8_t* reader_head       = builder_buffer;
            uint8_t* const reader_tail = builder_buffer + lattice_buffer_length;

            do {
                const size_t BYTES_AVAILABLE = reader_tail - reader_head;

                xSemaphoreTake(xUSB_Slave_TxSemaphore, portMAX_DELAY);
                xSemaphoreTake(xSPI_Semaphore,
                               portMAX_DELAY);  ///> Have to check that this
                                                /// doesn't deadlock

                /* read data into buffer but skip the head in cpld programming
                 * because this is just data*/
                bytes_read =
                    cpld_update_data_recieved(builder_buffer, BYTES_AVAILABLE);
                xSemaphoreGive(xSPI_Semaphore);
                xSemaphoreGive(xUSB_Slave_TxSemaphore);
                reader_head += bytes_read;
            } while (bytes_read != 0);

            // After parsing, shift the read buffer.
            lattice_buffer_length = reader_tail - reader_head;
            memmove(builder_buffer, reader_head, lattice_buffer_length);

            if (!load_lattice_jed) {
                // Reseting to initial conditions
                lattice_buffer_length = 0;
            }
        } else if (load_rt_hex) {
            const bool data_read = wait_for_data(pdMS_TO_TICKS(1));
            if (!data_read) {
                wdt_restart(WDT);
                continue;
            }

            size_t bytes_read = read_cdc_into_buffer(
                builder_buffer + rt_buffer_length,
                USB_SLAVE_BUFFER_SIZE - rt_buffer_length);
            rt_buffer_length += bytes_read;

            uint8_t *reader_head = builder_buffer;
            uint8_t *reader_tail = builder_buffer + rt_buffer_length;

            while (reader_head < reader_tail) {
                const size_t bytes_available = reader_tail - reader_head;
                size_t consumed = rt_firmware_update_consume(
                    reader_head, bytes_available, &slave_message);
                if (consumed == 0U) {
                    break;
                }
                reader_head += consumed;
            }

            rt_buffer_length = reader_tail - reader_head;
            memmove(builder_buffer, reader_head, rt_buffer_length);

            if (!load_rt_hex) {
                rt_buffer_length = 0;
            }
        } else {
            context.got_error = false;
            const bool COMMAND_READY =
                usb_slave_message_builder_reading_service(
                    &builder, &slave_message, APT_PARSER_TIMEOUT_PERIOD,
                    &context, builder_context_read, builder_context_on_error);
            if (COMMAND_READY) {
                apt_parse(&slave_message);
            } else if (!context.got_error) {
                debug_print("timeout reached: %d bytes dropped",
                            builder.buffer_length);
            }

            // If lattice is now loading, make sure to forward any remaining
            // data read to the lattice buffer before resetting the builder.
            if (load_lattice_jed) {
                lattice_buffer_length = builder.buffer_length;

                usb_slave_message_builder_reset(&builder);
            }
            if (load_rt_hex) {
                rt_buffer_length = builder.buffer_length;

                usb_slave_message_builder_reset(&builder);
            }
        }
    }
}

static bool usb_slave_check_alignment(
    const USB_Slave_Message_Builder* builder) {
    // \pre builder->buffer_length >= APT_COMMAND_SIZE

    const uint8_t DESTINATION = builder->buffer[4] & 0x7F;
    const uint8_t SOURCE      = builder->buffer[5];

    // check the destination is within range 0x11 (motherboard id), or 0x21
    // to 0x28 (slots) strip the 0x80 for extended data just in case
    switch (DESTINATION) {
    case 0x11:  // motherboard id
    case 0x21:  // slot 1
    case 0x22:  // slot 2
    case 0x23:  // slot 3
    case 0x24:  // slot 4
    case 0x25:  // slot 5
    case 0x26:  // slot 6
    case 0x27:  // slot 7
    case 0x28:  // slot 8
        break;

    default:
        debug_print("ERROR: Destination error\r\n");
        return false;
        break;
    }

    // check the 6th byte should be equal to HOST_ID
    if (SOURCE != HOST_ID) {
        debug_print("ERROR: Source error\r\n");
        return false;
    }

    return true;
}

static bool usb_slave_try_realign(USB_Slave_Message_Builder* builder) {
    if (builder->realigned_try_cnt == builder->max_realigned_attempts) {
        return false;
    }
    ++builder->realigned_try_cnt;
    debug_print("realign data (#%i) %x (%x %x %x %x %x ?). /r/n",
                builder->realigned_try_cnt, builder->buffer[0],
                builder->buffer[1], builder->buffer[2], builder->buffer[3],
                builder->buffer[4], builder->buffer[5]);
    memmove(builder->buffer, builder->buffer + 1, builder->buffer_length - 1);
    return true;
}

static size_t read_cdc_into_buffer(void* buffer, size_t max_read) {
    const size_t BYTES_AVAILABLE = udi_cdc_get_nb_received_data();
    const size_t TO_READ         = Min(BYTES_AVAILABLE, max_read);

    // (sbenish) Passing 0 to udi_cdc_read_buf will hang.
    if (TO_READ != 0) {
        udi_cdc_read_buf(buffer, TO_READ);
    }
    return TO_READ;
}

static bool wait_for_data(TickType_t timeout) {
#if USE_NOTIFY
    return ulTaskNotifyTake(pdTRUE, timeout) != 0;
#else
    return xSemaphoreTake(g_data_ready, timeout) == pdTRUE;
#endif
}

static size_t builder_context_read(void* ctx, void* _dest, size_t bytes_to_read,
                                   TickType_t timeout) {
    (void)ctx;
    uint8_t* dest = (uint8_t*)_dest;

    size_t rt = 0;

    while (true) {
        const size_t BYTES_READ = read_cdc_into_buffer(dest, bytes_to_read);
        dest += BYTES_READ;
        bytes_to_read -= BYTES_READ;
        rt += BYTES_READ;

        if (bytes_to_read == 0 || timeout == 0) {
            break;
        }

        const TickType_t START_TIME = xTaskGetTickCount();
        wait_for_data(timeout);
        const TickType_t DURATION = xTaskGetTickCount() - START_TIME;
        timeout = DURATION > timeout ? 0 : (timeout - DURATION);
    }

    return rt;
}

static void builder_context_on_error(void* ctx,
                                     USB_Slave_Message_Builder_State state,
                                     void* param) {
    builder_context* const context = (builder_context*)ctx;

    switch (state) {
    case USBSMBS_COMMAND_OVERFLOWED:
        context->got_error = true;
        debug_print("USB Slave large command rejection:  extended length %d",
                    *(size_t*)param);
        break;

    case USBSMBS_REALIGN_UNAVAILABLE:
        context->got_error = true;
        break;

    // Normal states.
    default:
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void usb_slave_init(void) {
    // Start USB stack to authorize VBus monitoring
    udc_start();

    /**
     * Create a Mutex for sending data on the USBsalve port
     * Check to see if the semaphore has not been created.
     */
    if (xUSB_Slave_TxSemaphore == NULL) {
        /* Then create the USB slave Tx bus mutex semaphore */
        xUSB_Slave_TxSemaphore = xSemaphoreCreateMutex();
        if ((xUSB_Slave_TxSemaphore) != NULL) {
            xSemaphoreGive((xUSB_Slave_TxSemaphore)); /* make the USB slave
                                                         Tx bus available */
            NameQueueObject(xUSB_Slave_TxSemaphore, "USBSTXL");
        }
    }

#if !USE_NOTIFY
    g_data_ready = xSemaphoreCreateBinary();
#endif

    /**
     *  Create a USB slave task to handle all the data coming in from a PC
     * or Tablet
     */
    if (xTaskCreate(task_usb_slave_rx, "USB Slave", TASK_USB_SLAVE_STACK_SIZE,
                    NULL, TASK_USB_SLAVE_PRIORITY, &xUSB_Rx_Task) != pdPASS) {
        debug_print("ERROR: Failed to create test led task\r\n");
    }
}

void usb_slave_message_builder_init(USB_Slave_Message_Builder* out,
                                    uint8_t* buffer, size_t buffer_size,
                                    uint8_t max_realign_attempts) {
    out->buffer                 = buffer;
    out->buffer_capacity        = buffer_size;
    out->max_realigned_attempts = max_realign_attempts;

    usb_slave_message_builder_reset(out);
}

void usb_slave_message_builder_reset(USB_Slave_Message_Builder* out) {
    out->buffer_length     = 0;
    out->counter           = 0;
    out->state             = USBSMBS_IDLE;
    out->realigned_try_cnt = 0;
}

size_t usb_slave_message_builder_read(USB_Slave_Message_Builder* builder,
                                      const void* src, size_t length) {
    const size_t MAX_TO_READ =
        builder->buffer_capacity - builder->buffer_length;
    const size_t TO_READ = Min(MAX_TO_READ, length);
    memcpy(builder->buffer + builder->buffer_length, src, TO_READ);
    builder->buffer_length += TO_READ;
    return TO_READ;
}

size_t usb_slave_message_builder_bytes_needed(
    const USB_Slave_Message_Builder* builder) {
    switch (builder->state) {
    case USBSMBS_IDLE:
    case USBSMBS_BUILDING:
        return builder->buffer_length < APT_COMMAND_SIZE
                   ? (APT_COMMAND_SIZE - builder->buffer_length)
                   : 0;
    case USBSMBS_GOT_COMMAND:
    case USBSMBS_COMMAND_OVERFLOWED:
        return builder->counter;
    case USBSMBS_REALIGN_UNAVAILABLE:
    default:
        return 0;
    }
}

bool usb_slave_message_builder_service(USB_Slave_Message_Builder* builder,
                                       USB_Slave_Message* out,
                                       size_t* overflowed_size) {
    bool message_complete        = false;
    bool reset                   = false;
    size_t bytes_to_consume      = 0;
    size_t local_overflowed_size = 0;

    bool retry = true;
    while (retry) {
        retry = false;
        switch (builder->state) {
        case USBSMBS_IDLE:
            if (builder->buffer_length != 0) {
                builder->state = USBSMBS_BUILDING;
                retry          = true;
            }
            break;

        case USBSMBS_BUILDING:
            // Does not have command.
            if (builder->buffer_length < APT_COMMAND_SIZE) {
                break;
            }

            if (!usb_slave_check_alignment(builder)) {
                builder->state = usb_slave_try_realign(builder)
                                     ? USBSMBS_BUILDING
                                     : USBSMBS_REALIGN_UNAVAILABLE;
                retry          = true;
            } else {
                builder->realigned_try_cnt = 0;
                out->ucMessageID =
                    builder->buffer[0] | (builder->buffer[1] << 8);
                out->param1      = builder->buffer[2];
                out->param2      = builder->buffer[3];
                out->destination = builder->buffer[4] & 0x7F;
                out->source      = builder->buffer[5];
                out->ExtendedData_len =
                    builder->buffer[2] | (builder->buffer[3] << 8);
                out->bHasExtendedData = builder->buffer[4] & 0x80;
                builder->counter      = out->ExtendedData_len;

                if (out->bHasExtendedData) {
                    if ((size_t)(APT_COMMAND_SIZE + out->ExtendedData_len) >
                        builder->buffer_capacity) {
                        builder->state        = USBSMBS_COMMAND_OVERFLOWED;
                        local_overflowed_size = out->ExtendedData_len;
                    } else {
                        builder->state = USBSMBS_GOT_COMMAND;
                    }
                    retry = true;
                } else {
                    bytes_to_consume = APT_COMMAND_SIZE;
                    message_complete = true;
                    reset            = true;
                }
            }
            break;

        case USBSMBS_GOT_COMMAND:
            if (builder->buffer_length >=
                (APT_COMMAND_SIZE + builder->counter)) {
                memcpy(out->extended_data_buf,
                       builder->buffer + APT_COMMAND_SIZE, builder->counter);
                bytes_to_consume = APT_COMMAND_SIZE + builder->counter;
                message_complete = true;
                reset            = true;
            }
            break;
        case USBSMBS_COMMAND_OVERFLOWED:
            // A command was too large and overflowed.  We need to consume
            // the command's number of bytes without actually reading the
            // command.
            if (builder->counter == 0) {
                bytes_to_consume = 0;
                reset            = true;
            } else {
                if (builder->counter > builder->buffer_length) {
                    // Buffer does not have enough data -> decrement
                    // the counter and reset the buffer length.
                    builder->counter -= builder->buffer_length;
                    builder->buffer_length = 0;
                } else {
                    // Buffer has valid data -> execute the reset
                    // routine and consume the remaining bytes.
                    bytes_to_consume = builder->counter;
                    reset            = true;
                }
            }
            break;

        // Do nothing
        case USBSMBS_REALIGN_UNAVAILABLE:
        default:
            break;
        }

        // Resets the state.
        if (reset) {
            // In the optimal case, don't move memory.
            if (builder->buffer_length == bytes_to_consume) {
                builder->state         = USBSMBS_IDLE;
                builder->buffer_length = 0;
            } else {
                // Shift the remaining data.
                memmove(builder->buffer, builder->buffer + bytes_to_consume,
                        builder->buffer_length - bytes_to_consume);

                builder->state = USBSMBS_BUILDING;
                builder->buffer_length -= bytes_to_consume;
            }

            // If the message wasn't completed on reset, try again.
            retry = !message_complete;
        }
    }

    if (overflowed_size) {
        *overflowed_size = local_overflowed_size;
    }

    return message_complete;
}

bool usb_slave_message_builder_reading_service(
    USB_Slave_Message_Builder* const builder, USB_Slave_Message* message,
    TickType_t timeout, void* const callback_context,
    size_t (*callback_read)(void*, void*, size_t, TickType_t),
    void (*callback_on_error_state)(void*, USB_Slave_Message_Builder_State,
                                    void*)) {
    configASSERT(builder != NULL);
    configASSERT(message != NULL);
    configASSERT(callback_read != NULL);

    TickType_t local_timeout = portMAX_DELAY;
    while (true) {
        const bool STARTED_IDLE = builder->state == USBSMBS_IDLE;
        if (!STARTED_IDLE && local_timeout == 0) {
            usb_slave_message_builder_reset(builder);
            return false;
        }

        // Reading bytes directly into the builder.
        const size_t CAPACITY =
            builder->buffer_capacity - builder->buffer_length;
        configASSERT(CAPACITY != 0);

        // Read 1 byte to start the timeout.
        const size_t TO_READ =
            STARTED_IDLE ? 1
                         : Min(CAPACITY,
                               usb_slave_message_builder_bytes_needed(builder));
        if (STARTED_IDLE) {
            local_timeout = portMAX_DELAY;
        }

        // This is fine in a overflow context b/c the builder will set the
        // buffer length to 0.
        const TickType_t READ_START = xTaskGetTickCount();
        const size_t ACTUALLY_READ  = callback_read(
            callback_context, builder->buffer + builder->buffer_length, TO_READ,
            local_timeout);
        const TickType_t READ_DURATION = xTaskGetTickCount() - READ_START;

        if (STARTED_IDLE) {
            // Setup the timeout.
            local_timeout = timeout;
        } else {
            // Adjust the timeout.
            local_timeout = (READ_DURATION > local_timeout)
                                ? 0
                                : (local_timeout - READ_DURATION);
        }

        builder->buffer_length += ACTUALLY_READ;
        size_t overflow_size;
        if (usb_slave_message_builder_service(builder, message,
                                              &overflow_size)) {
            return true;
        }

        if (callback_on_error_state && overflow_size != 0) {
            callback_on_error_state(callback_context, builder->state,
                                    &overflow_size);
        }

        if (builder->state == USBSMBS_REALIGN_UNAVAILABLE) {
            if (callback_on_error_state) {
                callback_on_error_state(callback_context, builder->state, NULL);
            }

            // Realignment failed -> wait until timeout is over.
            vTaskDelay(local_timeout);
            local_timeout = 0;
        }
    }
}
