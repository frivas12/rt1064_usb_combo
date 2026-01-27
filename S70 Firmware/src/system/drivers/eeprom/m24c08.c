/**
 * @file m24c08.c
 *
 * @brief Functions for writing and reading the eeprom
 *
 */

/****************************************************************************
 * Private Data
 ****************************************************************************/
#include <m24c08.h>
#include <soft_i2c.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief The Page Read mode allows up to 16 byte to be read in a single Read cycle,
 * 		provided that they are all located in the same page in the memory.  If more bytes are
 * 		read than will fit up to the end of the page, a “roll-over” occurs,
 * @param channel 			: PCA9548A I2c switch channel
 * @param block	  			: The M24C08 has 4 block to address
 * @param address			: This is the address within a block
 * @param NumByteToRead		: Number of bytes to read
 * @param pBuffer			: Pointer to the buffer that is being written
 * @return : 	EEPROM_OK (0) if operation is correctly performed, else return value
 *         		different from EEPROM_OK (0) or the timeout user callback.
 */
uint8_t m24c08_read_page(uint8_t channel, uint8_t block, uint8_t address, uint8_t NumByteToRead,
		uint8_t *pBuffer)
{
	uint8_t status = EEPROM_FAIL;
	uint8_t i, wait = 0;

// first we need to set the channel on the PCA9548A I2c switch
	set_channel(1 << channel);

	// Dev Select
	// check to make sure device is ready
	while (status != EEPROM_OK)
	{
		delay_ms(1);
		i2c_start();
		status = i2c_write(block);
		wait++;
		if (wait > 8)
		{
			status = EEPROM_FAIL;
			goto fail;
		}
	}

	// Byte address
	status |= i2c_write(address);

	i2c_start();

	// Dev Select
	status |= i2c_write(block + 1);

	// Data read
	for (i = 0; i < NumByteToRead - 1; i++)
	{
		pBuffer[i] = i2c_read(1);
	}
	pBuffer[i] = i2c_read(0);	// last one no ack

	fail: i2c_stop();

	return status;
}

/**
 * @brief The Page Write mode allows up to 16 byte to be written in a single Write cycle,
 * 		provided that they are all located in the same page in the memory.  If more bytes are
 * 		sent than will fit up to the end of the page, a “roll-over” occurs, i.e. the bytes
 * 		exceeding the page end are written on the same page, from location 0.
 * @param channel 			: PCA9548A I2c switch channel
 * @param block	  			: The M24C08 has 4 block to address
 * @param address			: This is the address within a block
 * @param NumByteToWrite	: Number of bytes to write
 * @param pBuffer			: Pointer to the buffer that is being written
 * @return : 	EEPROM_OK (0) if operation is correctly performed, else return value
 *         		different from EEPROM_OK (0) or the timeout user callback.
 */
uint8_t m24c08_write_page(uint8_t channel, uint8_t block, uint8_t address, uint8_t NumByteToWrite,
		uint8_t *pBuffer)
{
	uint8_t status = EEPROM_OK;

	// first we need to set the channel on the PCA9548A I2c switch
	set_channel(1 << channel);

	i2c_start();

	//Dev Select
	status |= i2c_write(block);

	// Byte address
	status |= i2c_write(address);

	// Data write
	for (uint8_t i = 0; i < NumByteToWrite; i++)
	{
		status |= i2c_write(pBuffer[i]);
	}

	i2c_stop();

	return status;
}

