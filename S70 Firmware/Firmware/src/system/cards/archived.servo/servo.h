/**
 * @file servo.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_SERVO_SERVO_H_
#define SRC_SYSTEM_CARDS_SERVO_SERVO_H_

#include "servo_saves.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define SLEEP_ON				0
#define SLEEP_DISABLED			1

/* Main control motor states*/
#define SERVO_STOP				0
#define SERVO_IDLE				1

/*For servo mode*/
#define SERVO_VEL_MOVE			2
#define SERVO_GOTO				3
#define SERVO_POSITION_TOGGLE	4
/*For shutter mode*/
#define SHUTTER_OPEN			5
#define SHUTTER_CLOSE			6
#define SHUTTER_HOLD_PULSE		7
#define SHUTTER_HOLD_VOLTGAE	8

/* Servo goto states*/
#define SERVO_GOTO_IDLE			0
#define SERVO_SART_MOVE			1
#define SERVO_WAIT_TIME			2

// Servo Store Save Latency
#define SERVO_STORE_DELAY               100 // 10 s

/*Mesoscope shutter voltage is 7V for 20ms then drops down to 5V and holds this voltage,
 * so duty cycle should be (1900 * 7V)/12V = 1108 for pulse and ((1900 * 5V)/12V = 792*/


/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	shutter_positions position;
	uint8_t on_time_counter;
} Shutter;

typedef struct  __attribute__((packed))
{
	uint8_t mode;
	uint8_t goto_mode;
	bool cmd_dir;
	uint8_t cmd_position;
	uint32_t lTransitTime_counter;
} Servo_Control;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void setup_io(slot_nums slot);
void servo_init(uint8_t slot, slot_types type);

#ifdef __cplusplus
}
/************************************************************************************
 * C++ Only Data Types
 ************************************************************************************/
typedef struct
{
        slot_nums slot;
        slot_types type;
        Servo_Control  ctrl;
        Servo_Save save;
        Shutter shutter;
        uint32_t store_counter;
} Servo_info;

#endif

#endif /* SRC_SYSTEM_CARDS_SERVO_SERVO_H_ */
