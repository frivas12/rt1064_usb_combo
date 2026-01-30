/**
 * @file stepper_control.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_STEPPER_STEPPER_CONTROL_H_
#define SRC_SYSTEM_CARDS_STEPPER_STEPPER_CONTROL_H_

#include "stepper.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "slots.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
// Directions
#define DIR_FORWARD			1
#define DIR_REVERSE			0

/* Main control motor states*/
#define STOP			0
#define IDLE			1
#define GOTO			2
#define RUN				3
#define JOG				4
#define PID				5
#define HOMING			6
#define SOFT_STOP_START 7
#define SOFT_STOP_WAIT  8

/* goto states*/
#define GOTO_IDLE	0
#define GOTO_CLEAR	1
#define GOTO_START	2
#define GOTO_STEP	3
#define GOTO_VEL	4
#define GOTO_VERIFY	5

/*Homing mode */
#define  HOME_MODE_NORMAL		0

/*Homing direction (limit_switch)*/
#define  HOME_CW				0
#define  HOME_CCW				1
#define  HOME_CW_FIRST			2	// use for home to index
#define  HOME_CCW_FIRST			3	// use for home to index

/*Homing to what (home_dir)*/
#define  HOME_TO_INDEX			0
#define  HOME_TO_LIMIT			1
#define  HOME_TO_HARD_STOP		2

// define homing states
#define HOME_IDLE				0
#define HOME_START				1
#define HOME_CW_DIR_FIRST		2
#define HOME_CCW_DIR			3
#define HOME_CCW_DIR_FIRST		4
#define HOME_CW_DIR				5
#define HOME_RETURN_CW			6
#define HOME_RETURN_CCW			7
#define HOME_INDEX_NOT_FOUND	8
#define HOME_CLEAR				9
#define HOME_WAIT_LIMIT			10
#define HOME_WAIT_SETTEL		11

// INTERNAL-ONLY homing state
#define HOME_EXIT_LIMIT         249
#define HOME_DELAYED_START      250

// define homing states for rotational magnetic encoder on inverted af switcher
//#define HOME_IDLE							0	// From above
//#define HOME_START						1	// From above
#define HOME_MAG_SW_MOVE_TO_POSITION_LOW	2
#define HOME_MAG_SW_WAIT_1					3
#define HOME_MAG_SW_MOVE_TO_POSITION_HIGH	4
#define HOME_MAG_SW_WAIT_2					5
#define HOME_MAG_SW_HOMING_COMPLETE			6
#define HOME_MAG_SW_HOMING_FAILED			7


// define homing states for rotational magnetic encoder on inverted epi turret
//#define HOME_IDLE							0	// From above
//#define HOME_START						1	// From above
#define HOME_MAG_SPIN_CCW_FAST				2
#define HOME_MAG_WAIT_FOR_FIRST_EDGE		3
#define HOME_MAG_WAIT_1						4
#define HOME_MAG_CHECK_IF_PASSED_SLOT		5
#define HOME_MAG_GET_IN_SLOT				6
#define HOME_MAG_STOP_BACK_TO_FIRST_EDGE	7
#define HOME_MAG_WAIT_2						8
#define HOME_MAG_GOTO_NEXT_POSTION			9
#define HOME_MAG_SAVE_POSTION				10
#define HOME_MAG_WAIT_3						11
#define HOME_MAG_COMPLETE					12
#define HOME_MAG_FAILED						13
#define HOME_MAG_CHECK_NO_COLLISION			14
#define HOME_START_1						15


// define jog states
#define JOG_IDLE	0
#define JOG_CLEAR	1
#define JOG_START	2
#define JOG_WAIT	3

/* Joystick DirSense*/
#define DIRSENSE_POS		1
#define DIRSENSE_NEG		2

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void write_flash(Stepper_info *info);
void magnetic_sensor_auto_homing_check(Stepper_info *info);
bool stepper_store_not_default(Stepper_Store * const p_store);
void stepper_goto_stored_pos(Stepper_info *info, uint8_t position_num);
void service_stepper(Stepper_info *info, USB_Slave_Message *slave_message);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_CARDS_STEPPER_STEPPER_CONTROL_H_ */
