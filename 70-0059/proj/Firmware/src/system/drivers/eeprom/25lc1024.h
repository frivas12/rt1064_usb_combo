// 25lc1024.h

#ifndef SRC_SYSTEM_DRIVERS_EEPROM_25LC1024_H_
#define SRC_SYSTEM_DRIVERS_EEPROM_25LC1024_H_


#include "slots.h"
#include "defs.h"
#include "user_spi.h"
// #include "lut.h"
#include "UsbCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define EEPROM_SPI_MODE 	_SPI_MODE_0
#define SPI_EEPROM_NO_READ 	EEPROM_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, CS_FLASH
#define SPI_EEPROM_READ 	EEPROM_SPI_MODE, SPI_READ, CS_NO_TOGGLE, CS_FLASH

/** Time-out value (number of attempts). */
#define EEPROM_25LC1024_TIMEOUT       15000

//#define HIGH_BYTE(x) ((x&0xff00)>>8)
//#define LOW_BYTE(x) (x&0xff)

#define EEPROM_25LC1024_READ		0x03	// Read data from memory array beginning at selected address
#define EEPROM_25LC1024_WRITE		0x02	// Write data to memory array beginning at selected address
#define EEPROM_25LC1024_WREN		0x06	// Set the write enable latch (enable write operations)
#define EEPROM_25LC1024_WRDI		0x04	// Reset the write enable latch (disable write operations)
#define EEPROM_25LC1024_RDSR		0x05	// Read STATUS register
#define EEPROM_25LC1024_WRSR		0x01	// Write STATUS register
#define EEPROM_25LC1024_PE			0x42	// Page Erase � erase one page in memory array
#define EEPROM_25LC1024_SE			0xD8	// Sector Erase � erase one sector in memory array
#define EEPROM_25LC1024_CE			0xC7	// Chip Erase � erase all sectors in memory array
#define EEPROM_25LC1024_RDID		0xAB	// Release from Deep power-down and read electronic signature
#define EEPROM_25LC1024_DPD			0xB9	// Deep Power-Down mode


#define EEPROM_25LC1024_WRITE_PROTECT_NONE      (uint8_t)(0x00 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_QUARTER   (uint8_t)(0x01 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_HALF      (uint8_t)(0x02 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_ALL       (uint8_t)(0x03 << 2)
/** 25LC1024 has  1Mbit of memory and can write page with auto erase and save the
 *  values that are not overwritten
 */

/*Data types defines.  This should be in the first byte of every page.  This serves as a check
 * to see if the correct type of data is there but also to see if the data has not be set.*/
#define EEPROM_DATA_NOT_VALID		0xFF   /*This is the initial value on fresh EEPROM so why
 	 	 	 	 	 `						not make it the define for invalid data.  This should
 	 	 	 	 	 	 	 	 	 	 	 be the erase value.*/
#define USB_HID_DATA_TYPE			0x01
#define STEPPER_DATA_TYPE			0x02
#define STEPPER_STORE_TYPE			0x03
#define SERVO_DATA_TYPE				0x04
#define BOARD_DATA_TYPE				0x05
#define SLOT_DATA_TYPE				0x06
#define SM_DATA_TYPE				0x07  // Synchronized motion
#define SHUTTER_DATA_TYPE			8
#define IO_DATA_TYPE				9
#define PIEZO_ELLIPTEC_DATA_TYPE	10
#define PIEZO_DATA_TYPE				11


/****************************************************************************
 * Memory Map
 ****************************************************************************/
/**
 * EEPROM STRUCTURE
 *
 *
 *      Sector 1 (Pages 000-127):
 *      EEPROM_BOARD_INFO_ADDRESS                               Page 0
 *      SLOT_EEPROM_START                                       Page 1 - 70     10 pages per slot
 *      HID_EEPROM_START                                        Page 71 - 77    1 page per HID
 *      POWER_BAD_RESTART_CHECK_EEPROM_START                    Page 78
 *      BOARD_SAVE_EEPROM_START                                 Page 79
 *      SM_SAVE_EEPROM_START                                    Page 80
 *
 *      Sector 2 (Pages 128-255):
 *
 *      Sector 3 (Pages 256-383):
 *      EEPROM_LUT_START                                        Page 0 - 127
 *
 *      Sector 4 (Pages 384-511):
 *      *Reserved by LUT                                        Page 0 - 127
 */

#define EEPROM_25LC1024_SIZE		(uint32_t) 131072 ///> Previous value was 1048576.  Why?  See datasheet, page 6 for byte size.
#define EEPROM_25LC1024_PAGE_SIZE	(uint32_t) 256
#define EEPROM_25LC1024_SECTOR_SIZE     (uint32_t) EEPROM_25LC1024_SIZE / 4
#define EEPROM_START_ADDRESS		(uint32_t) 0x0000
#define EEPROM_SECTOR_START_ADDRESS(sector) \
                                        (uint32_t) (0x8000 * sector)

#define EEPROM_BOARD_INFO_ADDRESS	(uint32_t) EEPROM_START_ADDRESS
#define EEPROM_BOARD_END			(uint32_t) (EEPROM_BOARD_INFO_ADDRESS + EEPROM_25LC1024_PAGE_SIZE)

/**************************************************************************
 *
 * Slots EEPROM Area  (0 to 70 (10*7)  10 pages per slot
 * 70 pages
 **************************************************************************/
#define SLOT_EEPROM_START			(uint32_t) EEPROM_BOARD_END
#define SLOT_EEPROM_PAGES			(uint32_t) 10	/*Number of pages reserved for each slot */
#define SLOT_EEPROM_END				(uint32_t) (SLOT_EEPROM_START + (SLOT_EEPROM_PAGES * \
											EEPROM_25LC1024_PAGE_SIZE * NUMBER_OF_BOARD_SLOTS))

/*Used to calculate address
 * 10 Pages per slot
 * First page is for params (managed by LUT)
 * Last page is for slot save stuct*/
#define SLOT_EEPROM_ADDRESS(slot, page) 	(uint32_t) (SLOT_EEPROM_START + \
											(SLOT_EEPROM_PAGES * EEPROM_25LC1024_PAGE_SIZE * slot) + \
											(EEPROM_25LC1024_PAGE_SIZE * page))
#define SLOT_SAVE_PAGE 						(SLOT_EEPROM_PAGES - 1)

/**************************************************************************
 *
 * USB HID (in) mapping information USB_NUM_PORTS = 7+1 (7 on hub)
 * 7 pages
 **************************************************************************/
#define HID_MAP_IN_EEPROM_START				(uint32_t) SLOT_EEPROM_END
#define HID_MAP_IN_EEPROM_PAGES				(uint32_t) USB_NUM_PORTS	/* number of USB devices not counting hubs or port
																	zero which is used only for enumeration*/
#define HID_MAP_IN_EEPROM_END				(uint32_t) (HID_MAP_IN_EEPROM_START + \
											(HID_MAP_IN_EEPROM_PAGES * EEPROM_25LC1024_PAGE_SIZE))
/*Used to calculate address*/
#define HID_MAP_IN_EEPROM_ADDRESS(port) 	(uint32_t) (HID_MAP_IN_EEPROM_START + (EEPROM_25LC1024_PAGE_SIZE * port))

/**************************************************************************
 *
 * USB HID (out) mapping information USB_NUM_PORTS = 7
 * 7 pages
 **************************************************************************/
#define HID_MAP_OUT_EEPROM_START			(uint32_t) HID_MAP_IN_EEPROM_END
#define HID_MAP_OUT_EEPROM_PAGES			(uint32_t) USB_NUM_PORTS	/* number of USB devices not counting hubs or port
																	zero which is used only for enumeration*/
#define HID_MAP_OUT_EEPROM_END				(uint32_t) (HID_MAP_OUT_EEPROM_START + \
											(HID_MAP_OUT_EEPROM_PAGES * EEPROM_25LC1024_PAGE_SIZE))
/*Used to calculate address*/
#define HID_MAP_OUT_EEPROM_ADDRESS(port) 	(uint32_t) (HID_MAP_OUT_EEPROM_START + (EEPROM_25LC1024_PAGE_SIZE * port))

/**************************************************************************
 *
 * Power check area
 * 1 pages
 **************************************************************************/
/*Power in checks the high voltage, if it is low it will restart the system.  To avoid multiple restarts
 * while power is not good (ie when the emergency stop is hit) we can set a value in eeprom to indicate we have
 * already restarted and just wait until power is good to continue with task
 *
 * */
#define POWER_BAD_RESTART_CHECK_EEPROM_START		(uint32_t) HID_MAP_OUT_EEPROM_END
#define POWER_BAD_RESTART_CHECK_EEPROM_END			(uint32_t) (POWER_BAD_RESTART_CHECK_EEPROM_START + EEPROM_25LC1024_PAGE_SIZE)

/**************************************************************************
 *
 * Board save area
 * 1 pages
 **************************************************************************/
/*Area used for board save info in board.c (1 page 256 bytes)*/
#define BOARD_SAVE_EEPROM_START		(uint32_t) POWER_BAD_RESTART_CHECK_EEPROM_END
#define BOARD_SAVE_EEPROM_END		(uint32_t) (BOARD_SAVE_EEPROM_START + EEPROM_25LC1024_PAGE_SIZE)

/**************************************************************************
 *
 * Synchronized Motion area
 * 1 pages
 **************************************************************************/
#define SM_SAVE_EEPROM_START		(uint32_t) BOARD_SAVE_EEPROM_END
#define SM_SAVE_EEPROM_END			(uint32_t) (SM_SAVE_EEPROM_START + EEPROM_25LC1024_PAGE_SIZE)

/**************************************************************************
 *
 * Embedded File System (EFS)
 * Note that this is written in a general format to allow for swapping EEPROMs.
 * 256 pages
 **************************************************************************/
#define LUT_EEPROM_CRC_INIT             0xFF

#define EFS_EEPROM_START               EEPROM_SECTOR_START_ADDRESS(2)
#define EFS_EEPROM_END                 EEPROM_25LC1024_SIZE
#define EFS_PAGE_SIZE                  EEPROM_25LC1024_PAGE_SIZE

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
bool eeprom_25LC1024_clear_mem(void);
bool eeprom_25LC1024_clear_page(uint32_t startAdr);
bool eeprom_25LC1024_clear_sector(uint32_t sector); /// \todo Implement.
bool eeprom_25LC1024_set_write_protect(uint8_t protection); /// \todo Implement

void eeprom_25LC1024_read(uint32_t startAdr, uint32_t len, uint8_t* rx_data);
/**
 * Thread-safe reading from EEPROM.
 */
inline void eeprom_25LC1024_read_safe(uint32_t startAdr, uint32_t len, uint8_t* rx_data)
{
	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
	eeprom_25LC1024_read(startAdr, len, rx_data);
	xSemaphoreGive(xSPI_Semaphore);
}

bool eeprom_25LC1024_write(uint32_t startAdr, uint32_t len, const uint8_t *data);
/**
 * Thread-safe reading from EEPROM.
 */
inline bool eeprom_25LC1024_write_safe(uint32_t startAdr, uint32_t len, const uint8_t *data)
{
	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
	const bool RT = eeprom_25LC1024_write(startAdr, len, data);
	xSemaphoreGive(xSPI_Semaphore);
	return RT;
}

void test_25lc1024(void);
CRC8_t CRC(const char* p_data, uint32_t length);
CRC8_t CRC_split(const char* p_data, uint32_t length, uint8_t crc);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_EEPROM_25LC1024_H_ */
