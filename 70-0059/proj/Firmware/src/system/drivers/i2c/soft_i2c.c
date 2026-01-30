// soft_i2c.c

#include <m24c08.h>
#include "soft_i2c.h"

/*
 * This implements a software i2c.  Software I2C is used because all 3 of the
 * hardware are used for UART, debug, or hardware SPI chip select
 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void soft_i2c_init(void)
{
	i2c_high(PIN_I2C_SDA);
	i2c_high(PIN_I2C_SCL);
}

/*
 * I2C Start condition, data line goes low when clock is high
 */
void i2c_start(void)
{
	i2c_high(PIN_I2C_SDA);
	i2c_high(PIN_I2C_SCL);
	i2c_low(PIN_I2C_SDA);
	i2c_low(PIN_I2C_SCL);
}

/*
 * I2C Stop condition, clock goes high when data is low
 */
void i2c_stop(void)
{
	i2c_low(PIN_I2C_SDA);
	i2c_high(PIN_I2C_SCL);
	i2c_high(PIN_I2C_SDA);
}

/**
 * Read from a i2c slave
 *
 * @param ack acknowledge, ack (1) or not (0).
 *
 * @return 1 byte from a slave, with most significant bit first
 */
uint8_t i2c_read(uint8_t ack)
{
	uint8_t bit, data = 0;
	i2c_high(PIN_I2C_SDA);
	for (bit = 0; bit < 8; bit++)
	{
		data <<= 1;
		i2c_high(PIN_I2C_SCL);
		if (get_pin_level(PIN_I2C_SDA))
			data |= 1;
		i2c_low(PIN_I2C_SCL);
	}
	if (ack)
	{
		i2c_low(PIN_I2C_SDA);
	}
	else
	{
		i2c_high(PIN_I2C_SDA);
	}
	i2c_high(PIN_I2C_SCL);
	i2c_low(PIN_I2C_SCL);
	i2c_high(PIN_I2C_SDA);

	return data;
}

bool i2c_write(uint8_t data)
{
	uint8_t bit;
	bool b;

	for (bit = 8; bit; bit--)
	{
		if (data & 0x80)
		{
			i2c_high(PIN_I2C_SDA);
		}
		else
		{
			i2c_low(PIN_I2C_SDA);
		}
		i2c_high(PIN_I2C_SCL);
		data <<= 1;
		i2c_low(PIN_I2C_SCL);
	}

	i2c_high(PIN_I2C_SDA);
	i2c_high(PIN_I2C_SCL);
	b = get_pin_level(PIN_I2C_SDA);          // possible ACK bit
	i2c_low(PIN_I2C_SCL);
	return b;
}

/**
 *  @brief Sets the channel on the PCA9548A I2c switch.  Some board don't have this
 * 		chip because they have static hardware so their is no need to query PCA9548A's
 * 		on the slot.  This is always called even it the board doesn't have this chip
 * 		because it dosen't hurt anything and this way we don't have to check if the
 * 		chip exist on the board.
 * @param channel	This is the channel number on the PCA9548A I2c switch
 */
void set_channel(uint8_t channel)
{
	taskENTER_CRITICAL();
	i2c_start();
	i2c_write(I2C_8CH_SWITCH_ADDRESS);
	i2c_write(channel);
	i2c_stop();
	taskEXIT_CRITICAL();
}

