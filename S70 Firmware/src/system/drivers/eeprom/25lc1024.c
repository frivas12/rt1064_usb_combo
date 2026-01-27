/**
 * @file 25lc1024.c
 *
 * @brief Functions for ??? TODO.
 *
 */

#include "25lc1024.h"
#include <asf.h>
#include "Debugging.h"
#include "string.h"

#include "../../boards/board.h"
#include "delay.h"
#include "user_spi.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
// Look-up table for polynomial 0x31, initial value 0x00, final xor 0x00
static unsigned char CRC_LUT[256] = {
    0x00,0x31,0x62,0x53,0xC4,0xF5,0xA6,0x97,0xB9,0x88,0xDB,0xEA,0x7D,0x4C,0x1F,0x2E,
    0x43,0x72,0x21,0x10,0x87,0xB6,0xE5,0xD4,0xFA,0xCB,0x98,0xA9,0x3E,0x0F,0x5C,0x6D,
    0x86,0xB7,0xE4,0xD5,0x42,0x73,0x20,0x11,0x3F,0x0E,0x5D,0x6C,0xFB,0xCA,0x99,0xA8,
    0xC5,0xF4,0xA7,0x96,0x01,0x30,0x63,0x52,0x7C,0x4D,0x1E,0x2F,0xB8,0x89,0xDA,0xEB,
    0x3D,0x0C,0x5F,0x6E,0xF9,0xC8,0x9B,0xAA,0x84,0xB5,0xE6,0xD7,0x40,0x71,0x22,0x13,
    0x7E,0x4F,0x1C,0x2D,0xBA,0x8B,0xD8,0xE9,0xC7,0xF6,0xA5,0x94,0x03,0x32,0x61,0x50,
    0xBB,0x8A,0xD9,0xE8,0x7F,0x4E,0x1D,0x2C,0x02,0x33,0x60,0x51,0xC6,0xF7,0xA4,0x95,
    0xF8,0xC9,0x9A,0xAB,0x3C,0x0D,0x5E,0x6F,0x41,0x70,0x23,0x12,0x85,0xB4,0xE7,0xD6,
    0x7A,0x4B,0x18,0x29,0xBE,0x8F,0xDC,0xED,0xC3,0xF2,0xA1,0x90,0x07,0x36,0x65,0x54,
    0x39,0x08,0x5B,0x6A,0xFD,0xCC,0x9F,0xAE,0x80,0xB1,0xE2,0xD3,0x44,0x75,0x26,0x17,
    0xFC,0xCD,0x9E,0xAF,0x38,0x09,0x5A,0x6B,0x45,0x74,0x27,0x16,0x81,0xB0,0xE3,0xD2,
    0xBF,0x8E,0xDD,0xEC,0x7B,0x4A,0x19,0x28,0x06,0x37,0x64,0x55,0xC2,0xF3,0xA0,0x91,
    0x47,0x76,0x25,0x14,0x83,0xB2,0xE1,0xD0,0xFE,0xCF,0x9C,0xAD,0x3A,0x0B,0x58,0x69,
    0x04,0x35,0x66,0x57,0xC0,0xF1,0xA2,0x93,0xBD,0x8C,0xDF,0xEE,0x79,0x48,0x1B,0x2A,
    0xC1,0xF0,0xA3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1A,0x2B,0xBC,0x8D,0xDE,0xEF,
    0x82,0xB3,0xE0,0xD1,0x46,0x77,0x24,0x15,0x3B,0x0A,0x59,0x68,0xFF,0xCE,0x9D,0xAC,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * This instruction must be set before any write operation will be completed internally.
 */
// MARK:  SPI Mutex Required
static void eeprom_25LC1024_write_enable(void)
{
	uint8_t spi_tx_data[1];

	spi_tx_data[0] = EEPROM_25LC1024_WREN;
	spi_transfer(SPI_EEPROM_NO_READ, spi_tx_data, 1);
}

/**
 * The Read Status Register instruction (RDSR) provides access to the STATUS register.
 * The STATUS register may be read at any time, even during a write cycle. The STATUS
 * register is formatted as follows:
 *
 *	 	 6	 	5 		4 		3 		2 		1 		0
 * W/R 	 � 		� 		� 	   W/R 	   W/R 		R 		R
 * WPEN  X 		X 		X 	   BP1 	   BP0 		WEL 	WIP
 *
 * W/R = writable/readable. R = read-only.
 *
 * Write-In-Process (WIP)
 * Write Enable Latch (WEL)
 * Block Protection (BP0 and BP1)
 *
 * @return	returns the status byte
 */
// MARK:  SPI Mutex Required
static uint8_t eeprom_25LC1024_read_status(void)
{
	uint8_t spi_tx_data[2];

	spi_tx_data[0] = EEPROM_25LC1024_RDSR;
	spi_tx_data[1] = DUMMY_HIGH;
	spi_transfer(SPI_EEPROM_READ, spi_tx_data, 2);
	return spi_tx_data[1];
}

/**
 * @brief Checks the Write-In-Process (WIP) bit to see whether the 25LC1024 is busy with a
 * write operation. When set to a �1�, a write is in progress, when set to a �0�, no write
 * is in progress.
 *
 * @return	0 if no error, 1 if timeout error.
 */
// MARK:  SPI Mutex Required
static bool eeprom_25LC1024_wait_for_write(void)
{
	uint8_t status;

	uint32_t timeout = EEPROM_25LC1024_TIMEOUT;

	while (1)
	{
		if (!timeout--)
		{
			debug_print("ERROR: EEPROM 25LC1024 write timeout.\r\n");
			return 1;
		}

		status = eeprom_25LC1024_read_status();
		if (0 == (status & 1))
			return 0;
	}
}

// MARK:  SPI Mutex Required
static bool eeprom_25LC1024_write_page(uint32_t startAdr, uint32_t len, const uint8_t *data)
{
	eeprom_25LC1024_write_enable();

	uint8_t header[4];

	/* send write command*/
	header[0] = EEPROM_25LC1024_WRITE;
	// send address
	header[1] = startAdr >> 16;
	header[2] = startAdr >> 8;;
	header[3] = (uint8_t)startAdr;


    // (sbenish)  The SPI driver only does things byte-by-byte.
    //            Instead of allocating a possibly huge amount
    //            of memory to buffer, we're going to do it byte-by-byte.
    spi_start_transfer(EEPROM_SPI_MODE, false, CS_FLASH);
    spi_partial_write_array(header, 4);

    for(const uint8_t* END = data + len; data < END; ++data) {
        spi_partial_write(*data);
    }

    spi_end_transfer();
	if (eeprom_25LC1024_wait_for_write())
		return 1;
	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

CRC8_t CRC_split(const char* p_data, uint32_t length, uint8_t crc)
{
    // static const uint8_t POLY = 0x31;

    for(uint32_t i = 0; i < length; ++i)
    {
        // (sbenish) lut-based CRC calcuation.
        crc = CRC_LUT[crc ^ p_data[i]];
        // for (uint8_t j = 0; j < 8; ++j)
        // {
        //     if ((crc & 0x80) == 0)
        //     {
        //         crc <<= 1;
        //     } else {
        //         crc = (uint8_t)((crc << 1) ^ POLY);
        //     }
        // }
    }

    return crc;
}

/**
 * Calculates the CRC8 for a given data stream.
 * \param p_data Input data stream.
 * \param length Length of the data stream (in bytes).
 * \return The data stream's CRC8.
 */
CRC8_t CRC(const char* p_data, uint32_t length)
{
    //Invert bits and return
    return CRC_split(p_data, length, 0);
}

// MARK:  SPI Mutex Required
bool eeprom_25LC1024_clear_mem(void)
{
	bool ret = 0;

	/*Save the board type to write back to EEPROM*/
	enum board_types current_board = board_type;
	// Save the serial number too.
	char sn[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
	memcpy(sn, usb_serial_number, USB_DEVICE_GET_SERIAL_NAME_LENGTH);

	eeprom_25LC1024_write_enable();

	uint8_t spi_tx_data[1];

	/* send write command*/
	spi_tx_data[0] = EEPROM_25LC1024_CE;
	spi_transfer(SPI_EEPROM_NO_READ, spi_tx_data, 1);
	if (eeprom_25LC1024_wait_for_write())
		ret = 1;

	set_board_type(current_board);
	set_board_serial_number(sn);
	return ret;
}

// MARK:  SPI Mutex Required
bool eeprom_25LC1024_clear_page(uint32_t startAdr)
{
	eeprom_25LC1024_write_enable();
	uint8_t spi_tx_data[4];

	/* send write command*/
	spi_tx_data[0] = EEPROM_25LC1024_PE;
	/* setup send address*/
	spi_tx_data[1] = startAdr >> 16;
	spi_tx_data[2] = startAdr >> 8;;
	spi_tx_data[3] = (uint8_t)startAdr;

	/* send Page Erase command*/
	spi_transfer(SPI_EEPROM_NO_READ, spi_tx_data, 4);

	if (eeprom_25LC1024_wait_for_write())
		return 1;
	return 0;
}

// MARK:  SPI Mutex Required
void eeprom_25LC1024_read(uint32_t startAdr, uint32_t len, uint8_t* rx_data)
{
	// assertion
	if (startAdr + len > EEPROM_25LC1024_SIZE)
		return;

	uint8_t header[4];

	// setup send read command
	header[0] = EEPROM_25LC1024_READ;

	// setup send address
	header[1] = (startAdr >> 16);
	header[2] = (startAdr >> 8);
	header[3] = (uint8_t)startAdr;


    // (sbenish)  The SPI driver only does things byte-by-byte.
    //            Instead of allocating a possibly huge amount
    //            of memory to buffer, we're going to do it byte-by-byte.
    spi_start_transfer(EEPROM_SPI_MODE, false, CS_FLASH);
    spi_partial_write_array(header, 4);
    for(const uint8_t* END = rx_data + len; rx_data < END; ++rx_data) {
        uint8_t byte = DUMMY_HIGH;
        spi_partial_transfer(&byte);
        *rx_data = byte;
    }
    spi_end_transfer();
}

// MARK:  SPI Mutex Required
bool eeprom_25LC1024_write(uint32_t startAdr, uint32_t len, const uint8_t *data)
{
	/* Entire page is automatically erased before write and data is saved in other cells*/
	if (startAdr + len > EEPROM_25LC1024_SIZE)
	{
		debug_print("ERROR: EEPROM 25LC1024 write size too large.\r\n");
		return 1;
	}

	uint16_t ofs = 0;
	while (ofs < len)
	{
		/* calculate amount of data to write into current page*/
		uint16_t pageLen = EEPROM_25LC1024_PAGE_SIZE
				- ((startAdr + ofs) % EEPROM_25LC1024_PAGE_SIZE);
		if (ofs + pageLen > len)
			pageLen = len - ofs;

		/* write single page*/
		bool b = eeprom_25LC1024_write_page(
            startAdr + ofs,
            pageLen,
            data + ofs);
		if (!b)
		{
			return 1;
		}
		/* and switch to next page*/
		ofs += pageLen;
	}
	return 0;
}

// MARK:  SPI Mutex Required
void test_25lc1024(void)
{
#define TEST_LEN	50

	uint8_t buf[TEST_LEN];
	uint8_t buf2[TEST_LEN];

	for (uint8_t i = 0; i < TEST_LEN; ++i)
	{
		buf[i] = i;
	}

	eeprom_25LC1024_write(0, TEST_LEN, buf);
	delay_ms(10);
	eeprom_25LC1024_read(0, TEST_LEN, buf2);
	delay_ms(10);
}

// MARK:  SPI Mutex Required
bool eeprom_25LC1024_clear_sector(uint32_t sector)
{
    eeprom_25LC1024_write_enable();
    uint8_t spi_tx_data[4];

    uint32_t sector_start_address = EEPROM_SECTOR_START_ADDRESS(sector);

    /* send write command*/
    spi_tx_data[0] = EEPROM_25LC1024_SE;
    /* setup send address*/
    spi_tx_data[1] = sector_start_address >> 16;
    spi_tx_data[2] = sector_start_address >> 8;;
    spi_tx_data[3] = (uint8_t)sector_start_address;

    /* send Page Erase command*/
    spi_transfer(SPI_EEPROM_NO_READ, spi_tx_data, 4);

    return eeprom_25LC1024_wait_for_write();
}

// MARK:  SPI Mutex Required
bool eeprom_25LC1024_set_write_protect(uint8_t protection)
{
    uint8_t spi_tx_data[2];

    // Setup and read status register.
    spi_tx_data[0] = protection;
    spi_tx_data[1] = DUMMY_HIGH;

    spi_transfer(SPI_EEPROM_READ, spi_tx_data, 2);

    // Setup and send status register.

    return eeprom_25LC1024_wait_for_write();

}
