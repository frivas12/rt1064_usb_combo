/*
 * shutter_log.h
 *
 *  Created on: Jul 29, 2020
 *      Author: frivas
 */

#ifndef SRC_SYSTEM_CARDS_SHUTTER_LOG_H_
#define SRC_SYSTEM_CARDS_SHUTTER_LOG_H_

/****************************************************************************
 * Defines
 ****************************************************************************/
typedef enum
{
	SHUTTER_NO_LOG 							= 0,
	// Device detection events
	SHUTTER_DEVICE_MISMATCH					= 1,

	// Interrupt
	SHUTTER_EXTERNAL_TRIGGER				= 2,

	// USB parser events
	SHUTTER_SET_SHUTTERPARAMS				= 3,
	SHUTTER_SET_SOL_STATE					= 4,
	SHUTTER_UNSUPPORTED_COMMAND				= 5,

	// shutter controller events
	SHUTTER_STOP_LOG						= 6,
	SHUTTER_IDLE_LOG						= 7,
	SHUTTER_OPEN_LOG						= 8,
	SHUTTER_CLOSE_LOG						= 9,
	SHUTTER_HOLD_PULSE_LOG					= 10,
	SHUTTER_HOLD_VOLTGAE_LOG				= 11,

	// shutter joystick events
	SHUTTER_JOYSTICK_BTN_POS_TOGGLE			= 12,


	// shutter emergency stop event




	SHUTTER_END_LOG_IDS			 			= 0xff	// make this enum 8bit
} shutter_log_ids;



#endif /* SRC_SYSTEM_CARDS_SHUTTER_LOG_H_ */
