/**
 * @file shutter.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_H_
#define SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_H_

#include "shutter_saves.h"

#include "mcp23s09.h"


#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
 * Defines
 ****************************************************************************/
/*GPIO pins*/
#define TRIG_IN_1 	1 << 0
#define TRIG_IN_2 	1 << 1
#define TRIG_IN_3 	1 << 2
#define TRIG_IN_4 	1 << 3

#define EN_DRIVE_1 	1 << 4
#define EN_DRIVE_2 	1 << 5
#define EN_DRIVE_3 	1 << 6
#define EN_DRIVE_4 	1 << 7

// External trigger for shutters in all of our systems are
// Low = Closed ; High = Open

typedef enum
{
	SHUTTER_4_CHAN_1 = 0,
	SHUTTER_4_CHAN_2,
	SHUTTER_4_CHAN_3,
	SHUTTER_4_CHAN_4,
	SHUTTER_4_CHAN_MAX,
} shutter_4_channels;

typedef enum
{
	SHUTTER_4_STOP_STATE = 0,
	SHUTTER_4_IDLE_STATE,
	SHUTTER_4_OPEN_STATE,
	SHUTTER_4_CLOSE_STATE,
	SHUTTER_4_HOLD_PULSE_STATE,
	SHUTTER_4_HOLD_VOLTGAE_STATE
} shutter_states;

#define SLEEP_ON				0
#define SLEEP_DISABLED			1


/****************************************************************************
 * Public Data
 ****************************************************************************/




/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void shutter_init(uint8_t slot);

#ifdef __cplusplus
}

/************************************************************************************
 * C++ Only Data Types
 ************************************************************************************/
typedef struct
{
	Shutter_Save save;
	shutter_4_positions position;	/*Current position opened or closed*/
	uint8_t on_time_counter;		/*Counter for pulse time*/
	shutter_states mode;
} Shutter_4;

typedef struct
{
	slot_nums slot;
	Shutter_4 shutter[SHUTTER_MAX_CHAN];
	mcp23s09_info mcp_info;
	Bool interlock_state;
} Shutter_info;

#endif

#endif /* SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_H_ */
