/*
 * piezo_log.h
 *
 *  Created on: Sep 24, 2020
 *      Author: frivas
 */

#ifndef SRC_SYSTEM_CARDS_PIEZO_PIEZO_LOG_H_
#define SRC_SYSTEM_CARDS_PIEZO_PIEZO_LOG_H_

/****************************************************************************
 * Defines
 ****************************************************************************/
typedef enum
{
	PIEZO_NO_LOG 							= 0,
	// Device detection  events

	// USB parser  events
	PIEZO_SET_PRAMS							= 1,
	PIEZO_UNSUPPORTED_COMMAND				= 2,

	// piezo controller events
	PIEZO_ADC_SET_RANGE_ERROR				= 3,
	PIEZO_ENCODER_SLOT_NOT_SET				= 4,

	// piezo joystick events

	// piezo emergency stop event

	// piezo limit events

	// piezo synchronized motion events

	PIEZO_END_LOG_IDS			 			= 0xff	// make this enum 8bit
} piezo_log_ids;



#endif /* SRC_SYSTEM_CARDS_PIEZO_PIEZO_LOG_H_ */
