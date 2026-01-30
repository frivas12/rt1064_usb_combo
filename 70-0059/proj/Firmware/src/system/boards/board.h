/**
 * \file
 *
 * \brief Standard board header file.
 *
 * This file includes the appropriate board header file according to the
 * defined board (parameter BOARD).
 *
 * Copyright (c) 2009-2016 Atmel Corporation. All rights reserved.
 *
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * \defgroup group_common_boards Generic board support
 *
 * The generic board support module includes board-specific definitions
 * and function prototypes, such as the board initialization function.
 *
 * \{
 */

#include "compiler.h"
// #include "slots.h" /// \todo Check that this comment does not bug up anything else
#include "user_board.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/**
 * EEPROM for board and type of slot card are stored in 2 bytes in the M24C08
 * EEPROM chip in block 0, address 0.
 * Board types have the most significant bit set to 1 and therefore start 0x8000
 * Slot card type discussed in slots.c start at zero and must always be less than
 * 0x8000.
 */
enum board_types {
	NO_BOARD = 0x8000,//!< NO_BOARD
	BOARD_NOT_PROGRAMMED = 0x8001,
	MCM6000_REV_002,
	MCM6000_REV_003,
	OTM_BOARD_REV_002,
	HEXAPOD_BOARD_REV_001,
	MCM6000_REV_004,
	MCM_41_0117_001,	// CPLD 7000 new version
	MCM_41_0134_001,	// MCM301, Revision 1
	MCM_41_0117_RT1064, // new rt1064 USB CDC and HID host
	END_BOARD_TYPES
};

extern enum board_types board_type;

#define MIN_PWR_VOLTAGE		0 //1400 /* 1400 is about 13.5V on Vin PWR*/

/*Power in defines for the high voltage rail*/
#define POWER_NOT_GOOD		0
#define POWER_GOOD			1

#define DEFAULT_SYSTEM_DIM_VALUE		100 /* 0 - 100 %  100 is full brightness*/

/** Watchdog period 3000ms */
#define WDT_PERIOD                        3000
/** Watchdog restart 2000ms */
#define WDT_RESTART_PERIOD                2000

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	uint8_t board_data_type;
	bool enable_log;
} Board_save;

typedef struct  __attribute__((packed))
{
	uint16_t temperature_sensor_val; 	/*12 bit max 4096*/
	uint16_t vin_monitor_val; 			/*12 bit max 4096*/
	uint16_t cpu_temp_val; 			/*12 bit max 4096*/
	uint8_t error;
	volatile bool power_good; // Declared as volatile, as this value is polled by multiple threads.
	uint8_t clpd_em_stop;	// used to signal the cpld didn't receive a check in by the uC
							// each slot should clear this bit once the uC has started checking
							// in again
	Board_save save;
	uint8_t dim_value;		// The actual dim value.  Used to control all LED brightness.
	uint8_t dim_bound;		// The upper bound for dimming (0 to 100%).
	bool send_log_ready;	/*Flag to let logger know PC is connected and ready to send log info*/
} Boards;
extern Boards board;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void board_save_info_eeprom(void);
void board_get_info_eeprom(void);
void main_power_init(void);
void main_power( bool state);
void board_init(void);
void set_board_type(uint16_t board_type);
void set_board_serial_number(char * const p_sn);
void restart_board(void);
void set_firmware_load_count(void);

#ifdef __cplusplus
}
#endif

/**
 * \}
 */

#endif  // _BOARD_H_
