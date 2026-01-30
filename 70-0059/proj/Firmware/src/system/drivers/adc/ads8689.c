/**
 * @file ads8689.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "ads8689.h"
#include "user_spi.h"

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/
 /*Internal reference is 4.096V => 2.5 * 4.096 = 10.24V, set range to 0 - 10.24V and
 and to use the internal reference
 */
// MARK:  SPI Mutex Required
bool adc_init(uint8_t slot)
{
	uint8_t cnt = 0;
	uint16_t val = 0;

	do
	{
		cnt++;
		adc_write(slot, WRITE, RANGE_SEL_REG_7_0, 0x0009);
		// read it back
		adc_write(slot, READ_HWORD, RANGE_SEL_REG_7_0, 0x0000);
		// get the value
		val = adc_write(slot, NOP, 0, 0x0000);
		if (val == 9)
			return 0;

	} while ((val != 9) && (cnt < 20));

	return 1;
}

// MARK:  SPI Mutex Required
uint16_t adc_read(uint8_t slot)
{
	// ADS8689IPWR 16 bit adc
	//you will actually get the value of the adc on any command
	//but nop makes sure it doesn't execute anything and you just get the value
	//it's best to send all configuration commands before sampling

	return adc_write(slot, NOP, 0x00, 0x0000);
}

// MARK:  SPI Mutex Required
uint16_t adc_write(uint8_t slot, uint8_t op, uint8_t address, uint16_t data)
{
	// ADS8689IPWR 16 bit adc
	uint8_t spi_tx_data[4] =
	{ 0 };

	spi_tx_data[0] = op /*(op | (address >> 7))*/;
	spi_tx_data[1] = address /*(address << 1)*/;
	spi_tx_data[2] = (data >> 8);
	spi_tx_data[3] = data;

	spi_transfer(SPI_PIEZO_ADC_READ, spi_tx_data, 4);

	return ((spi_tx_data[0] << 8) | spi_tx_data[1]);
}

