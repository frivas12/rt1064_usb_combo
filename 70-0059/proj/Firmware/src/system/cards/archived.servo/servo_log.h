/*
 * servo_log.h
 *
 *  Created on: Aug 8, 2020
 *      Author: frivas
 */

#ifndef SRC_SYSTEM_CARDS_SERVO_LOG_H_
#define SRC_SYSTEM_CARDS_SERVO_LOG_H_

/****************************************************************************
 * Defines
 ****************************************************************************/
typedef enum
{
	SERVO_NO_LOG 							= 0,
	// Device detection events
	//SERVO_DEVICE_MISMATCH					= 1, // cable does not have eeprom

	// Interrupt
	SERVO_EXTERNAL_TRIGGER					= 2,

	// USB parser events
	SERVO_MOVE_STOP							= 3,
	SERVO_MOVE_VELOCITY						= 4,
	SERVO_SET_MFF_OPERPARAMSP				= 5,
	SERVO_SET_BUTTONPARAMS					= 6,
	SERVO_SET_SOL_STATE						= 7,
	SERVO_SET_SHUTTERPARAMS					= 8,
	SERVO_UNSUPPORTED_COMMAND				= 9,

	// servo controller events
	SERVO_CNTRL_STOP						= 10,
	SERVO_CNTRL_IDLE						= 11,
	SERVO_CNTRL_VEL_MOVE					= 12,
	SERVO_CNTRL_GOTO						= 13,
	SERVO_CNTRL_POSITION_1					= 14,
	SERVO_CNTRL_POSITION_2					= 15,
	SERVO_CNTRL_OPEN						= 16,
	SERVO_CNTRL_CLOSE						= 17,
	SERVO_CNTRL_HOLD_PULSE					= 18,
	SERVO_CNTRL_HOLD_VOLTGAE				= 19,

	// servo joystick events

	// servo emergency stop event

	SERVO_END_LOG_IDS			 			= 0xff	// make this enum 8bit
} servo_log_ids;



#endif /* SRC_SYSTEM_CARDS_SERVO_LOG_H_ */
