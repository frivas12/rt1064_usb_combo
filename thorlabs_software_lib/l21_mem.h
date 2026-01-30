// l21_mem.h

#ifndef THORLABS_SOFTWARE_LIB_L21_MEM_H_
#define THORLABS_SOFTWARE_LIB_L21_MEM_H_

/****************************************************************************
 * Defines
 ****************************************************************************/
#define NVM_MEMORY        ((volatile uint16_t *)FLASH_ADDR)
#define MAX_PACKET_SIZE 64

#define NVM_PAGE_SIZE			64
#define NVM_ROW_SIZE			256
#define NVM_SIZE				0x00040000
#define NVM_BASE_ADDRESS		0x00000000
#define APP_START_ADDRESS 		0x00008000
#define RWWEE_ADDRESS_OFFSET	0x00400000


#define EPROM_START_ADDRESS		NVM_BASE_ADDRESS + NVM_SIZE



//ADD YOUR ADDRESSES OVER HERE AND MAINTAIN MEMORY MAP
// EEPROM USER SPACE STARTS FROM 3FC00 Upwards and this row space is empty.. access each row in firmware by this method
//Eg. #define MY_DATA1_ADDRESS            EEPROM_USER_SPACE_ADDRESS
//    #define MY_DATA2_ADDRESS			  MY_DATA1_ADDRESS - NVM_ROW_SIZE
//    #define MY_DATA3_ADDRESS			  MY_DATA2_ADDRESS - NVM_ROW_SIZE
//and so on..
//modify ROM and EEPROM Length in flash.ld file accordingly.
//For generic applications, eeprom is 1KB and ROM is till address 0x0003FC00

#define EEPROM_USER_SPACE_ADDRESS		  BOOTLOADER_COUNT_ADDRESS + NVM_ROW_SIZE //040300

#define BOOTLOADER_COUNT_ADDRESS		  SERIAL_NUMBER_ADDRESS + NVM_ROW_SIZE //040200

#define SERIAL_NUMBER_ADDRESS			  THORLABS_IDS_TO_FLASH_ADDRESS + NVM_ROW_SIZE //040100

#define THORLABS_IDS_TO_FLASH_ADDRESS    NVM_BASE_ADDRESS + RWWEE_ADDRESS_OFFSET //this is the beginning of RWWEE




#endif /* THORLABS_SOFTWARE_LIB_L21_MEM_H_ */
