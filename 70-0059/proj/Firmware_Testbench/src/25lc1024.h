#pragma once

//Fake EEPROM file for testing the LUT code.

#include <stdint.h>
// #include "slots.h"
#include "user_spi.h"

#define EEPROM_BOARD_INFO_ADDRESS	(uint32_t) EEPROM_START_ADDRESS
#define EEPROM_BOARD_END			(uint32_t) (EEPROM_BOARD_INFO_ADDRESS + EEPROM_25LC1024_PAGE_SIZE)

#define SLOT_EEPROM_START			(uint32_t) EEPROM_BOARD_END
#define SLOT_EEPROM_PAGES			(uint32_t) 10	/*Number of pages reserved for each slot */
#define SLOT_EEPROM_END				(uint32_t) (SLOT_EEPROM_START + (SLOT_EEPROM_PAGES * \
											EEPROM_25LC1024_PAGE_SIZE * NUMBER_OF_BOARD_SLOTS))

#define EEPROM_25LC1024_WRITE_PROTECT_NONE      (uint8_t)(0x00 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_QUARTER   (uint8_t)(0x01 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_HALF      (uint8_t)(0x02 << 2)
#define EEPROM_25LC1024_WRITE_PROTECT_ALL       (uint8_t)(0x03 << 2)


/*Used to calculate address
 * 10 Pages per slot
 * First page is for params
 * Last page is for slot save stuct*/
#define SLOT_EEPROM_ADDRESS(slot, page) 	(uint32_t) (SLOT_EEPROM_START + \
											(SLOT_EEPROM_PAGES * EEPROM_25LC1024_PAGE_SIZE * slot) + \
											(EEPROM_25LC1024_PAGE_SIZE * page))
#define SLOT_SAVE_PAGE 						(SLOT_EEPROM_PAGES - 1)


#define EEPROM_25LC1024_PAGE_SIZE	(uint32_t) 256
#define EEPROM_25LC1024_SIZE		(uint32_t)(131072)
#define EEPROM_25LC1024_SECTOR_SIZE     (uint32_t) (EEPROM_25LC1024_SIZE / 4)
#define EEPROM_START_ADDRESS		(uint32_t) 0x00000000
#define EEPROM_SECTOR_START_ADDRESS(sector) \
                                        (uint32_t) (0x8000 * sector)

#define EFS_EEPROM_START               EEPROM_SECTOR_START_ADDRESS(2)
#define EFS_EEPROM_END                 EEPROM_25LC1024_SIZE
#define EFS_PAGE_SIZE                  EEPROM_25LC1024_PAGE_SIZE

typedef uint8_t LUT_EEPROM_CRC_t;


void eeprom_25LC1024_clear_sector(uint32_t sector); /// \todo Implement.
void eeprom_25LC1024_set_write_protect(uint8_t protection); /// \todo Implement.
void eeprom_25LC1024_read(uint32_t startAdr, uint32_t len, uint8_t* rx_data);
inline void eeprom_25LC1024_read_safe(uint32_t startAdr, uint32_t len, uint8_t* rx_data)
{
	eeprom_25LC1024_read(startAdr, len, rx_data);
}
uint8_t eeprom_25LC1024_write(uint32_t startAdr, uint32_t len, const uint8_t *data);
inline uint8_t eeprom_25LC1024_write_safe(uint32_t startAdr, uint32_t len, const uint8_t *data)
{
	return eeprom_25LC1024_write(startAdr, len, data);
}
LUT_EEPROM_CRC_t CRC(const char* p_data, uint32_t length);
LUT_EEPROM_CRC_t CRC_split(const char* p_data, uint32_t length, uint8_t crc);

//For emulator
void eeprom_init(void);
void example_write_to_LUT(void);
void example_write_mid_id(void);
