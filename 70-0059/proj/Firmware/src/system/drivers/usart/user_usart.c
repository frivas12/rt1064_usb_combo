/**
 * @file user_usart.c
 *
 * @brief Functions for ???
 *
 */
#include <asf.h>
#include "user_usart.h"
#include "Debugging.h"
#include <cpld.h>
#include <delay.h>
#include "user_spi.h"
#include <string.h>

fifo_t *uart0_fifo_rx;
Uart0 uart0;

/* Declare a binary Semaphore flag for the SPI Bus. To ensure only single access to UART0
 * Bus. */
SemaphoreHandle_t xUART0_Semaphore = NULL; // removed STATIC to allow other processes to use same semaphore.

SemaphoreHandle_t xUART0_DataReady = NULL;              // Signal. Signals when data of the specified length has been recorded.
static SemaphoreHandle_t xUART0_ReadyToWrite = NULL;    // Semaphore.  Free when the device an accept more data to write.
static SemaphoreHandle_t xUART0_WriteFinished = NULL;   // Semaphore.  Free when the device is not writing.
static uint8_t g_uart0_in_data[8];
static uint8_t g_uart0_out_data[8];
static uint8_t g_uart0_in_index;
static uint8_t g_uart0_in_size;
static uint8_t g_uart0_out_index;
static uint8_t g_uart0_out_size;

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
#if 0
/**
 * @brief All the RX data from the ADM3101EACPZ-250R7 (RS232 IC) uart coming in here into a
 * fifo which is later parsed in the rs232 task.
 */
ISR(UART0_Handler)
{
	uint8_t incoming_byte;

	if ((uart_get_status(UART0) & UART_SR_RXRDY) == UART_SR_RXRDY)
	{
		//Read in the byte
		uart_read(UART0, &incoming_byte);
//		debug_print("read  %d\r\n", rs232_fifo_rx->head);
		/*write the data to the fifo*/
		if (fifo_write(uart0_fifo_rx, &incoming_byte, 1) != 1)
		{
			debug_print("ERROR: The uart0 buffer full./r/n");
			/*Reset the fifo*/
			fifo_reset(uart0_fifo_rx, uart0_fifo_rx->buf, UART0_BUFFER_SIZE);
		}
	}
}
#endif

ISR(UART0_Handler)
{
    // Clear interrupt
    // NVIC_ClearPendingIRQ(UART0_IRQn);
    BaseType_t need_to_schedule = pdFALSE;

    // Check if data is ready.
    if (uart_is_rx_ready(UART0))
    {
        uint8_t tmp;
        uart_read(UART0, &tmp);
        if (g_uart0_in_index < 8)
        {
            g_uart0_in_data[g_uart0_in_index++] = tmp;
        }

        if (g_uart0_in_index >= g_uart0_in_size)
        {
            // If so, signal the flag.
            xSemaphoreGiveFromISR(xUART0_DataReady, &need_to_schedule);
        }
    }

    // Check if more data is needed to be written.
    if ((uart_get_interrupt_mask(UART0) & UART_IER_TXRDY) && uart_is_tx_ready(UART0))
    {
        if (g_uart0_out_index < g_uart0_out_size)
        {
            if (! uart_write(UART0, g_uart0_out_data[g_uart0_out_index]))
            {
                ++g_uart0_out_index;
            }
        } else {
            uart_disable_interrupt(UART0, UART_IER_TXRDY);
            uart_enable_interrupt(UART0, UART_IER_TXEMPTY);

            xSemaphoreGiveFromISR(xUART0_ReadyToWrite, &need_to_schedule);
        }
    }

    // Check if the data has finished writing.
    if ((uart_get_interrupt_mask(UART0) & UART_IER_TXEMPTY) && uart_is_tx_empty(UART0))
    {
        uart_disable_interrupt(UART0, UART_IER_TXEMPTY);

        xSemaphoreGiveFromISR(xUART0_WriteFinished, &need_to_schedule);
    }


    // Call the scheduler if required.
    portYIELD_FROM_ISR(need_to_schedule);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void uart0_settings(uint32_t ul_baudrate);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void uart0_settings(uint32_t ul_baudrate)
{
	const sam_uart_opt_t uartSettings =
	{ sysclk_get_peripheral_hz(), ul_baudrate, UART_MR_PAR_NO };
	uart_init(UART0, &uartSettings);

	/*Clear uart by reading*/
	uint8_t temp;
	uart_read(UART0, &temp);

	uart0.baudrate = ul_baudrate;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void int2Hex(int32_t intnumber, uint8_t *txt)
{
	unsigned char _s4 = '0';
	char i = 4;

	do
	{
		i--;
		_s4 = (unsigned char) ((intnumber >> i * 4) & 0x000f);
		if (_s4 < 10)
			_s4 = _s4 + 48;
		else
			_s4 = _s4 + 55;

		*txt++ = _s4;
	}
	while (i);
}
/**
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
uint32_t hex2int(uint8_t *hex)
{
	uint32_t val = 0;
	while (*hex)
	{
		// get current character then increment
		char byte = *hex++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9')
			byte = byte - '0';
		else if (byte >= 'a' && byte <= 'f')
			byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <= 'F')
			byte = byte - 'A' + 10;
		else
			break;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
	}
	return val;
}

/**
 * @brief Start the UART0 clock and setup pins
 */
void uart0_init(void)
{
	sysclk_enable_peripheral_clock(ID_UART0);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA10);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA9);
	uart0.baudrate = 0;
	uart0.address = NO_SLOT;
	uart0.task_using = 0;
	g_uart0_in_index = 0;
	g_uart0_out_index = 0;
	g_uart0_in_size = 0;
	g_uart0_out_size = 0;

	/**
	 * Create a Mutex for sending data on the UART0 port
	 * Check to see if the semaphore has not been created.
	 */
	if (xUART0_Semaphore == NULL)
	{
		/* Then create the SPI bus mutex semaphore */
		xUART0_Semaphore = xSemaphoreCreateMutex();
		if ((xUART0_Semaphore) != NULL)
		{
			xSemaphoreGive((xUART0_Semaphore)); /* make the UART bus available */
		        NameQueueObject(xUART0_Semaphore, "UART0 L");
		}
	}
	if (xUART0_DataReady == NULL)
	{
	    // Create the signalling flag
	    xUART0_DataReady = xSemaphoreCreateBinary();
	    NameQueueObject(xUART0_DataReady, "UART0DR");
	}
        if (xUART0_ReadyToWrite == NULL)
        {
            xUART0_ReadyToWrite = xSemaphoreCreateBinary();
                if ((xUART0_ReadyToWrite) != NULL)
                {
                        xSemaphoreGive((xUART0_ReadyToWrite));
                        NameQueueObject(xUART0_ReadyToWrite, "UART0RW");
                }
        }
        if (xUART0_WriteFinished == NULL)
        {
            // Create the signalling flag
            xUART0_WriteFinished = xSemaphoreCreateBinary();
            if (xUART0_WriteFinished != NULL)
            {
                xSemaphoreGive(xUART0_WriteFinished);
                NameQueueObject(xUART0_WriteFinished, "UART0WF");
            }
        }

	/* Allocate structure for uart rx fifo.*/
	uart0_fifo_rx = pvPortMalloc(sizeof(fifo_t));
	/* Allocate fifo buffer*/
	uart0_fifo_rx->buf = pvPortMalloc(sizeof(uart0_fifo_rx->buf) * UART0_BUFFER_SIZE);
	fifo_reset(uart0_fifo_rx, (uint8_t *) uart0_fifo_rx->buf, UART0_BUFFER_SIZE);

	/* Enable Rx Ready Interrupt on UART1*/
        uart_enable_interrupt(UART0, UART_IER_RXRDY);
        /* must set the interrupt priority lower priority than
         * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY*/
        irq_register_handler(UART0_IRQn,
                        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
}

/**
 * @brief Set the buadrate and address of the uart channel (slot) if it needs to be changed.
 * The calling function must have the SPI semaphore xSPI_Semaphore before calling this function.
 * @param baudrate
 * @param address
 */
void uart0_set_channel(uint8_t slot, uint32_t baudrate, uint8_t address)
{
    // If something has changed, wait until the transmission is done until changing.
    if (uart0.address != address || uart0.baudrate != baudrate)
    {
        uart0_wait_until_tx_done();
    }
	if (uart0.address != address)
	{
		/* Check if card in slot, if no card is installed we don't have a pullup and
		 * we get framing errors*/
		if (slots[slot].card_type > NO_CARD_IN_SLOT)
		{
			xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
			set_reg(C_SET_UART_SLOT, UART_CONTROLLER_ADDRESS, 0, address);
			xSemaphoreGive(xSPI_Semaphore);
			delay_us(10);
			uart0.address = address;
		}
	}

	if ((uart0.baudrate != baudrate))
	{
		uart0_settings(baudrate);
		delay_us(10);
	}
}

bool uart0_write(TickType_t delay, const uint8_t * p_data, uint8_t bytes_to_write)
{
    if (bytes_to_write <= 8)
    {
        if (xSemaphoreTake(xUART0_ReadyToWrite, delay) == pdPASS)
        {
            while (xSemaphoreTake(xUART0_WriteFinished, 0) == pdPASS) {}
            g_uart0_out_index = 0;
            g_uart0_out_size = bytes_to_write;
            memcpy(g_uart0_out_data, p_data, bytes_to_write);
            uart_enable_interrupt(UART0, UART_IER_TXRDY);
            return true;
        }
    }

    return false;
}

/**
 * Reads in data from UART and outputs it to the buffer.
 * \param[in]           delay
 * \param[out]          p_data
 * \param[in]           bytes_to_read
 * \return All of the selected bytes were read.
 */
bool uart0_read(TickType_t delay, uint8_t * p_data)
{
    if (xSemaphoreTake(xUART0_DataReady, delay) != pdPASS)
    {
        return false;
    }
    memcpy(p_data, g_uart0_in_data, g_uart0_in_size);

    return true;
}

/**
 * Clears any available data.
 */
void uart0_setup_read(uint8_t bytes_to_read)
{
    while (xSemaphoreTake(xUART0_DataReady, 0) == pdPASS) {}
    g_uart0_in_index = 0;
    if (bytes_to_read <= 8)
    {
        g_uart0_in_size = bytes_to_read;
    }
}

void uart0_wait_until_tx_done(void)
{
    xSemaphoreTake(xUART0_WriteFinished, portMAX_DELAY);
    xSemaphoreGive(xUART0_WriteFinished);
}
