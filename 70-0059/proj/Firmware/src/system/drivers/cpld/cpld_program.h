/**
 * @file cpld_program.h
 *
 * @brief Header file for CPLD programming jed file.
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_CPLD_CPLD_PROGRAM_H_
#define SRC_SYSTEM_DRIVERS_CPLD_CPLD_PROGRAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "user_spi.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
#define CPLD_PROGRAM_SPI_MODE 		_SPI_MODE_3
#define CPLD_PROGRAM_NO_READ 		CPLD_PROGRAM_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, CS_CPLD_PROGRAM
#define CPLD_PROGRAM_READ 			CPLD_PROGRAM_SPI_MODE, SPI_READ, CS_NO_TOGGLE, CS_CPLD_PROGRAM

#define CPLD_PROG_OK	0
#define CPLD_PROG_ERROR	1

/*Status register*/
#define REG_BUSY	1<<12
#define REG_FAIL	1<<13

/* read flag*/
#define READ_FLASH		0
#define NO_READ_FLASH	1

#define OPCODES_2	2
#define OPCODES_3	3

#define CONFIGURATION_FLASH_MEM		0x00
#define UFM_MEM						0x40

#define EARSE_ALL			0b00001111
#define EARSE_CONFIG_ONLY	0b00000100

#define KEEP_CS_LOW			0
#define NO_KEEP_CS_LOW		1


#define BYTES_PER_LINE	16

#define READ_STATUS_BUSY	(1<<4)	// of 2nd byte
#define READ_STATUS_FAIL	(1<<5)	// of 2nd byte

#define IDCODE_PUB				0xE0,0,0,0		// Read Device ID
#define ISC_ENABLE_X			0x74,8,0,0		// Enable Configuration Interface (Transparent Mode)
#define ISC_ENABLE				0xC6,8,0,0		// Enable Configuration Interface (Off line Mode)

#define LSC_CHECK_BUSY			0xF0,0,0,0		// Read Busy Flag
												// Bit 1 0
												// 7 Busy Ready

#define LSC_READ_STATUS			0x3C,0,0,0		// Read Status Register // return YY YY YY YY
												// Bit 	1 			0
												// 12 	Busy 	  Ready
												// 13 	Fail 	    OK

#define ISC_ERASE_CF_UFM_FR(x)		0x0E,x,0,0	// Erase CF/UFM/FR
												// Y = Memory space to erase
												// Y is a bitwise OR
												// Bit 1=Enable
												// 16 Erase SRAM
												// 17 Erase Feature Row
												// 18 Erase Configuration Flash
												// 19 Erase UFM

#define LSC_ERASE_TAG			0xCB,0,0,0		// Erase UFM

#define LSC_INIT_ADDRESS		0x46,0,0,0		// Reset Configuration Flash Address
												// Set Page Address pointer to the beginning
												// of the Configuration Flash sector

#define LSC_WRITE_ADDRESS		0xB4,0,0,0		// Set Address
												// Set the Page Address pointer to the Flash
												// page specified by the least significant 14
												// bits of the PP PP field.
												// The �M� field defines the Flash memory
												// space to access.
												// Field 0x0 0x4
												// M Configuration Flash UFM

#define LSC_PROG_INCR_NV		0x70,0,0,1		// Program Page	//write  YY * 16
												// Program one Flash page. Can be used to
												// program the Configuration Flash, or UFM.

#define LSC_INIT_ADDR_UFM		0x47,0,0,0		// Set the Page Address Pointer to the beginning of the UFM sector
#define LSC_PROG_TAG			0xC9,0,0,0		// Program one UFM page	//write  YY * 16
#define ISC_PROGRAM_USERCODE	0xC2,0,0,0		// Program the USERCODE.	//write  YY * 4
#define USERCODE				0xC0,0,0,0		// Retrieves the 32-bit USERCODE value
#define LSC_PROG_FEATURE		0xE4,0,0,0		// Program the Feature Row bits	//write  YY * 8
#define LSC_READ_FEATURE		0xE7,0,0,0		// Retrieves the Feature Row bits
#define LSC_PROG_FEABITS		0xF8,0,0,0		// Program the FEABITS	//write  YY * 2
#define LSC_READ_FEABITS		0xFB,0,0,0		// Read the FEABITS	//write  YY * 2

#define LSC_READ_INCR_NV		0x73,0x10,0,1 // M0 PP PP		// Read Flash
												// Retrieves PPPP count pages. Only the least
												// significant 14 bits of PP PP are used.
												// The �M� field must be set based on the configuration
												// port being used to read the Flash
												// memory.
												// 0x0 I2C
												// 0x1 JTAG/SSPI/WB

#define LSC_READ_UFM			0xCA // M0 PP PP		// Read UFM Flash
												// Retrieves PPPP count UFM pages. Only the
												// least significant 14 bits of PP PP are used
												// for the page count.
												// The �M� field must be set based on the configuration
												// port being used to read the UFM.
												// 0x0 I2C
												// 0x1 JTAG/SSPI/WB

#define ISC_PROGRAM_DONE		0x5E,0,0,0		// Program DONE, Program the DONE status bit enabling SDM
#define LSC_PROG_OTP			0xF9,0,0,0		// Program OTP Fuses	//write  UCFSUCFS
												// Makes the selected memory space One
												// Time Programmable. Matching bits must be
												// set in unison to activate the OTP feature.
												// Bit 1 0
												// 0, 4 SRAM OTP SRAM Writable
												// 1, 5 Feature Row Feature Row
												// OTP Writable
												// 2, 6 CF OTP CF Writable
												// 3, 7 UFM OTP UFM Writable

#define LSC_READ_OTP			0xFA,0,0,0		// Read OTP Fuses
												// Read the state of the One Time Programmable
												// fuses.
												// Bit 1 0
												// 0, 4 SRAM OTP SRAM Writable
												// 1, 5 Feature Row Feature Row
												// OTP Writable
												// 2, 6 CF OTP CF Writable
												// 3, 7 UFM OTP UFM Writable

#define ISC_NOOP				0xFF,0xFF,0xFF,0xFF	// Bypass, No Operation and Device Wakeup

#define ISC_PROGRAM_SECURITY	0xCE,0,0,0		// Program SECURITY
												// Program the Security bit (Secures CFG
												// Flash sector).

#define ISC_PROGRAM_SECPLUS		0xCF,0,0,0		// Program SECURITY PLUS
												// Program the Security Plus bit (Secures CFG
												// and UFM Sectors)

#define UIDCODE_PUB				0x19,0,0,0		//Read TraceID code, Read 64-bit TraceID.
												// read YY*8

// below are different opcode length

#define ISC_DISABLE				0x26,0,0,0	// only 2 opcode but fill with 3
												// Disable Configuration Interface
												// Exit Offline or Transparent programming
												// mode. ISC_DISABLE causes the MachXO2
												// to automatically reconfigure when leaving
												// Offline programming mode. Thus, when
												// leaving Offline programming mode the Configuration
												// SRAM must be explicitly cleared
												// using ISC_ERASE (0x0E) prior to transmitting
												// ISC_DISABLE. The recommended exit
												// command from Offline programming mode
												// is LSC_REFRESH (0x79), wherein
												// ISC_ERASE and ISC_DISABLE are not
												// necessary. See Figure 14-18.

#define LSC_REFRESH				0x79,0,0,0	// only 2 opcode but fill with 3
												// Refresh
												// Force the MachXO2 to reconfigure. Transmitting
												// a REFRESH command reconfigures
												// the MachXO2 in the same fashion as
												// asserting PROGRAMN.


#define TRANSPARENT_MODE		ISC_ENABLE_X
#define OFFLINE_MODE			ISC_ENABLE


#define CONFIGURATION_FLASH_START			0
#define CONFIGURATION_FLASH_PROGRAM			1
#define WRITE_FEATURE_ROW					2
#define WRITE_FEABITS						3


/****************************************************************************
 * Public Data
 ****************************************************************************/
extern bool load_lattice_jed;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
/**
 * \return The number of bytes consumed from the buffer to
 *         parse the next part of the state machine.
 */
size_t cpld_update_data_recieved(const uint8_t * buffer, size_t length);
void lattice_firmware_update(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_CPLD_CPLD_PROGRAM_H_ */
