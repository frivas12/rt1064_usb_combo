/*
 * piezo.h
 *
 *  Created on: Sep 21, 2020
 *      Author: frivas
 */

#ifndef SRC_SYSTEM_CARDS_PIEZO_PIEZO_H_
#define SRC_SYSTEM_CARDS_PIEZO_PIEZO_H_

#include "piezo_saves.h"

#include "compiler.h"
#include "user_spi.h"
#include "slots.h"
#include "encoder.h"
#include "piezo_log.h"
#include "piezo_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define HYSTERESIS	0	// define if you need to run the HYSTERESIS ploting
#define HYSTERESIS_TRACKING		0

/*DAC*/
#define DAC_MAX_VALUE		65535
#define DAC_MIN_VALUE		0
#define DAC_ZERO_VALUE		0	// this is the value where the voltage to the piezo is zero
#define DAC_MAX_SLEW_RATE	500 // max delta value for updating the DAC output
#define DAC_SLOW_SLEW_RATE	20 	//  max delta value for updating the DAC output
#define DAC_VOLTS_PER_COUNT		0.00007629510948f
#define DAC_COUNT_PER_ENC_COUNT		0.07f //0.152590219f

/*.5V we leave at the bottom (this is the negative voltage and
 *  we need to leave head room here because of creep in dac counts*/
#define DAC_LOWER_HEADROOM 	6554	// .5V/(5/65535)
//#define DAC_LOWER_HEADROOM 	3277	// .25V/(5/65535)

/*Analog input*/
#define ANALOG_INPUT_COUNTS_PER_UM				655 	/* 640 this is the conversion of the input
														to encoder um */
#define ANALOG_INPUT_COUNTS_PER_ENC_COUNTS		6.40f //6.554f  /* 6.40f this is the conversion of the input
														//to encoder counts */

#define FILTER_VAL_CNT		20	// average x/4 => ms 20/4= 5ms
#define KP_SCALE			1000.0f

/*DAC ramp voltage states*/
#define DAC_RAMP_START		0
#define DAC_RAMP			1
#define DAC_RAMP_DONE		2

/* Main control piezo states*/
#define PZ_STOP_MODE			0
#define PZ_ANALOG_INPUT_MODE	1
#define SET_DAC_VOLTS			2
#define PZ_CONTROLER_MODE		3
#define PZ_PID_MODE				4
#define PZ_HYSTERESIS_MODE		5

/* Analog input states*/
#define PZ_AIN_START					0
#define PZ_AIN_STEPPER_HOLD_POS			1
#define PZ_AIN_STEPPER_WAIT				2
#define PZ_AIN_PIEZO_SYNC				3
#define PZ_AIN_STEPPER_WAIT_2			4
#define PZ_AIN_STEPPER_WAIT_3			5
#define PZ_AIN_RUN						6
#define PZ_AIN_STEPPER_MOVING			7
#define PZ_AIN_STEPPER_WAIT_4			8

/*displacement states*/
#define DISPLACEMENT_STATE_START			0
#define DISPLACEMENT_STATE_WAIT				1
#define DISPLACEMENT_STATE_COMPLETE			2
#define DISPLACEMENT_STATE_OVERSHOOT		3
#define DISPLACEMENT_STATE_OVERSHOOT_DONE	4

/*Overshoot*/
#define OVERSHOOT_ZERO_CROSSING_CNT			10	// required times to get zero error in overshoot calc

/*Queue state defines*/
#define PIEZO_QUEUE_NO_SEND			0
#define PIEZO_QUEUE_SEND_DATA		1
#define PIEZO_QUEUE_SENT_DATA		2





/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct __attribute__((packed))
{
	float kp;
	float gain_1um;
	float gain_3um;
	float gain_10um;
	float gain_30um;
	float gain_60um;

	int32_t error;
	int32_t error_window;

	int32_t cmnd_pos;
	int32_t delta_pos;

	uint16_t displacement_cnt;
	uint16_t displacement_time;  // cnt = 4 = 1ms
	uint8_t displacement_state;
	bool displacement_complete;

	bool overshoot_complete;
	uint16_t overshoot_time;  // cnt = 4 = 1ms
	uint16_t overshoot; // in encoder counts

	uint8_t filter_cnt;
	float kp_factor;
	bool factor_adjustment_complete;
	int32_t pos_filter_vals[FILTER_VAL_CNT * 2];

	uint8_t clalibration_state;
	uint16_t clalibration_cnt;
	uint8_t clalibration_steps_taken;
	bool clalibration_complete;
	uint8_t overshoot_zero_crossing_cnt;

	// temp for pid
	float iterm; //!< Accumulator for integral term
	float lastin; //!< Last input value for differential term

} Piezo_Controller;

#define HYSTERESIS_DOWN	0
#define HYSTERESIS_UP	1

typedef struct __attribute__((packed))
{
	bool dir;
	uint16_t max;
	uint16_t min;

	bool dir_change;
	uint16_t change_delta;
	uint8_t skip;
} Hysetresis;

typedef struct __attribute__((packed))
{
	uint16_t val;
	uint16_t cmd_val;
	uint8_t ramp_voltage_state;
} Dac;

typedef struct __attribute__((packed))
{
	uint16_t val;
//	uint8_t input_mode;
	uint8_t input_state;
} Adc;

typedef struct __attribute__((packed))
{
	int32_t pos;
	int32_t pos_filtered;
	int32_t pos_zero; // only used for output_pos_on_dac
	int32_t pos_delta;
	int32_t start_val;
	int32_t before_stepper_move_val;

	// ABS BISS encoder
	bool error;
	bool warn;
	uint8_t error_cnt;	/* The fagor encoder has errors while booting so we count
					 them and if the exceed a limit then the error exist,
					 this counter should be reset when the error is cleared
					 by the encoder*/

} Enc;

typedef struct __attribute__((packed)) // from stepper to piezo task
{
	bool stepper_moving;
} piezo_sm_Rx_data; // data being received from the synchronized stepper task queues

typedef struct __attribute__((packed)) // from piezo to stepper task
{
	bool stepper_hold_pos;	// signal stepper to get into pid without kickout to perform a sync
} piezo_sm_Tx_data; //data being sent to the synchronized stepper task queues


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void piezo_init(uint8_t slot);

#ifdef __cplusplus
}

/************************************************************************************
 * C++ Only Data Types
 ************************************************************************************/
typedef struct __attribute__((packed))
{
        slot_nums slot;
        uint8_t mode;
        Piezo_Save save;
        Piezo_Controller cntlr;
#if HYSTERESIS_TRACKING
        Hysetresis hyst;
#endif
        Dac dac;
        Adc adc;
        Enc enc;
        bool enable_plot;
        uint8_t queue_service_cnt;
        uint8_t queue_send_flag;
        uint16_t analog_in_wait_cnt;
} Piezo_info;

#endif

#endif /* SRC_SYSTEM_CARDS_PIEZO_PIEZO_H_ */
