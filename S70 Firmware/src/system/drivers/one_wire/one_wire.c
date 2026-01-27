/**
 * @file one_wire.c
 *
 * @brief Functions for ???
 *
 */
#include <asf.h>
#include "slots.h"
#include "one_wire.h"
#include "user_usart.h"
#include <cpld.h>
#include "../buffers/fifo.h"
#include "delay.h"
#include "Debugging.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/


/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if 0
static uint8_t ow_bit(uint8_t b)
{
	uint8_t data_write;
	uint8_t data_read;
	uint8_t ret_val = 1;
	uint8_t times = 0;

	if (b)
		data_write = 0xff;	//  Write 1
	else
		data_write = 0x00;	//  Write 0

	uart_write(UART0, data_write);
	delay_us(10);

	/* Read */
	while ((ret_val == 1) && (times < 200))
	{
		ret_val = uart0_read(0, &data_read);
		times++;
		delay_us(1);
	}
	if(times >= 200)
	{
		device_print("ERROR: ow_bit read byte failed/r/n");
	}

	if (data_read == 255)
		return 1;
	else
		return 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void ow_byte_wr(uint8_t b)
{
	uint8_t buffer[8];

	for (uint8_t i = 0; i < 8; b >>= 1, ++i)
	{
	    buffer[i] = (b & 1) ? 0xFF : 0x00;
	}


	uart0_write(portMAX_DELAY, buffer, 8);
}

uint8_t ow_byte_rd(bool * const p_timed_out)
{
    uint8_t b = 0;

    uint8_t buffer[8] = {
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
    };

    uart0_wait_until_tx_done();
    uart0_setup_read(8);
    uart0_write(portMAX_DELAY, buffer, 8);
    if (p_timed_out == NULL)
    {
        uart0_read(portMAX_DELAY, buffer);
    } else {
        *p_timed_out = !uart0_read(pdMS_TO_TICKS(8), buffer);
    }

    for (uint8_t i = 0; i < 8; ++i)
    {
        b <<= 1;
        b |= ((buffer[7 - i] == 0xFF) ? 1 : 0);
    }

    return b;
}

