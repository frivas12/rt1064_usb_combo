/**
 * @file usr_limits.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USR_LIMITS_USR_LIMITS_H_
#define SRC_SYSTEM_DRIVERS_USR_LIMITS_USR_LIMITS_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/* Interrupt bits for limits from cpld*/
#define CW_LIMIT		1<<0
#define CCW_LIMIT		1<<1
#define INDEX			1<<2

/* Hard Limit from LIMSWITCHPARAMS*/
#define NO_LIMIT			0
#define MAKES_ON_CONTACT	1
#define BREAKS_ON_CONTACT	2
#define REVERSE_LIMITS		0x80 //or with other limit setting on both CW and CCW

/*Limit mode from ATP LIMSWITCHPARAMS*/
#define IGNORE_LIMIT		1
#define HARD_STOP_LIMIT		2
#define SOFT_STOP_LIMIT		3	// TODO may want to implement

/*Param 1 for MGMSG_MOT_CLEAR_SOFT_LIMITS*/
#define SET_CCW_SOFT_LIMIT	1
#define SET_CW_SOFT_LIMIT	2
#define CLEAR_SOFT_LIMITS	3

/*Param 1 for MGMSG_MCM_SET_ABS_LIMITS*/
#define SET_ABS_LOW_LIMIT	1
#define SET_ABS_HIGH_LIMIT	2
#define CLEAR_ABS_LIMITS	3


// Homed status
#define NOT_HOMED_STATUS		0
#define IS_HOMED_STATUS			1
#define HOMING_STATUS			2
#define HOMING_FAILED_STATUS	3

#define CW_UNSET				INT32_MAX
#define CCW_UNSET				INT32_MIN

/****************************************************************************
 * Public Data
 ****************************************************************************/
/*These are all settings we need to store in EEPROM*/
typedef struct  __attribute__((packed))
{
	/*Limit switch mode flags from ATP LIMSWITCHPARAMS*/
	uint16_t cw_hard_limit;
	uint16_t ccw_hard_limit;

	/*soft limits from ATP LIMSWITCHPARAMS*/
	int32_t cw_soft_limit;
	int32_t ccw_soft_limit;

	int32_t abs_high_limit;
	int32_t abs_low_limit;

	/*limit mode from ATP LIMSWITCHPARAMS*/
	uint16_t limit_mode;
} Limits_save;

typedef struct  __attribute__((packed))
{
	bool cw;	// can also be used for absolute encoder where the limit is a set value above abs_hi_limit
	bool ccw;
	bool index;
	bool soft_cw;
	bool soft_ccw;
	bool hard_stop;
	bool cur_dir;
	int8_t homed;
	bool interlock;
	int32_t *compare_val;	/*Pointer for the compare value for soft limits */
	int8_t debounce_cnt;
	uint32_t buffer_index;
	bool limit_flag_for_logging; // if a limit is hit set this flag to dispatch a log event
	bool encoder_reveres_flag;
} Limits;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
//void limits_handler(uint8_t slot);
void service_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits, uint8_t type);
/**
 * Shifts the soft limits (if any are set) by the indicated amount.
 * \param p_limits_saved The limit structure whose soft limits should be adjusted.
 * \param offset The value to add to the soft limits.
 */
void shift_soft_limits(Limits_save * const p_limits_saved, const int32_t offset);
void setup_limits(uint8_t slot, Limits *limits, int32_t *compare_val, bool encoder_reveres_flag);

//setup_limits();

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USR_LIMITS_USR_LIMITS_H_ */
