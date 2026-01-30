/**
 * @file log.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_LOG_LOG_
#define SRC_SYSTEM_DRIVERS_LOG_LOG_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/

#include "usb_slave.h"
#include "../../cards/stepper/stepper_log.h"
#include "slots.h"

typedef struct __attribute__((packed))
{
//	uint8_t slot;
	uint8_t type;
	uint8_t id;
	uint32_t var1;
	uint32_t var2;
	uint32_t var3;
} Log;

// use to n=make sure we don't send the same type repetitively
// +5 for the MOTHERBOARD_ID and other as sources
extern uint8_t slot_prev_type[NUMBER_OF_BOARD_SLOTS + 5];
extern uint8_t slot_prev_id[NUMBER_OF_BOARD_SLOTS + 5];

#define LOG_REPEAT		0
#define LOG_NO_REPEAT	1

typedef enum
{
	NO_LOG_TYPE 			= 0,
	SYSTEM_LOG_TYPE 		= 1,
	STEPPER_ERROR_TYPE 		= 2,
	STEPPER_LOG_TYPE 		= 3,
	SLIDER_IO_ERROR_TYPE 	= 4,
	SLIDER_IO_LOG_TYPE 		= 5,
	SHUTTER_ERROR_TYPE		= 6,
	SHUTTER_LOG_TYPE 		= 7,
	SERVO_ERROR_TYPE 		= 8,
	SERVO_LOG_TYPE 			= 9,
	PIEZO_LOG_TYPE 			= 10,

	END_LOG_TYPES = 0xff	// make this enum 8bit
} slider_io_log_types;

typedef enum
{
	SYSTEM_NO_LOG = 0,
	SYSTEM_HW_START_LOG = 1,
	SYSTEM_SET_DIM_LOG = 2,
	SYSTEM_SET_JOYSTICK_MAP_OUT_LOG = 3,
	SYSTEM_SET_JOYSTICK_MAP_IN_LOG = 4,
	SYSTEM_SET_ERASE_EEPROM_LOG = 5,
	SYSTEM_RESTART_PROCESSOR_LOG = 6,
	SYSTEM_SET_DEVICE_BOARD_LOG = 7,
	SYSTEM_SET_DEVICE_LOG = 8,
	SYSTEM_SET_CARD_TYPE_LOG = 9,
	SYSTEM_SET_HW_REV_LOG = 10,
	SYSTEM_UPDATE_CPLD_LOG = 11,
	SYSTEM_UPDATE_FIRMWARE_LOG = 12,
	SYSTEM_ENABLE_LOG = 13,

	SYSTEM_END_LOG_IDS = 0xff	// make this enum 8bit
} system_log_ids;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void setup_and_send_log(USB_Slave_Message *slave_message, uint8_t slot,
		bool no_repeat, uint8_t type, uint8_t id, uint32_t var1, uint32_t var2,
		uint32_t var3);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_LOG_LOG_ */
