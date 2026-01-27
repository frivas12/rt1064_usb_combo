/**
 * @file synchronized_motion.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_STEPPER_SYNCHRONIZED_MOTION_H_
#define SRC_SYSTEM_CARDS_STEPPER_SYNCHRONIZED_MOTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../drivers/math/matrix_math.h"
#include "slots.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
/*Controller types*/
typedef enum
{
	DISABLED_SM_TYPE = 0,
	VIRTUAL_AXIS_SM_TYPE,
	VIRTUAL_PLANE_SM_TYPE,
	HEXAPOD_SM_TYPE,
} Stepper_sm_types;

#define MAX_NUM_SM_AXIS		6

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	int32_t position;
	float counts_per_unit;
	uint32_t max_speed;
	uint8_t step_mode;
	uint16_t nm_per_count;
} stepper_sm_Rx_data;

typedef struct  __attribute__((packed))
{
	uint32_t target;
	float max_velocity;  //0-100%
} stepper_sm_Tx_data;


typedef struct  __attribute__((packed))
{
	int32_t x;	/*In encoder counts*/
	int32_t y;
	int32_t z;
} Point;


typedef struct  __attribute__((packed))
{
	Point p[3];
} Matrix_3x3;

typedef struct  __attribute__((packed))
{
	Stepper_sm_types sm_type;
	uint8_t x_slot;
	uint8_t y_slot;
	uint8_t z_slot;
	uint32_t VirtualMotionDelta;

} Sm_config_params;

typedef struct  __attribute__((packed))
{
	Sm_config_params config;
	//	Matrix_3x3 rotation_matrix;
	PmRotationMatrix RMatrix;
	float angle_x;	// angle about x in radians
	float angle_y;	// angle about y  in radians
	float angle_z;	// angle about z  in radians
} Sm_params;

typedef struct  __attribute__((packed))
{
	uint8_t eprom_data_type; /*type of data should be SERVO_DATA_TYPE, used as a check*/
	Sm_params params;
} Sm_Save_Params;

typedef struct  __attribute__((packed))
{
	PmCartesian tran;
	float32_t a;
	float32_t b;
	float32_t c;
} EmcPose;

typedef struct  __attribute__((packed))
{
	EmcPose pose_current;
	EmcPose pose_command;
	float32_t struts[6];  //this is in units of encoder counts
	float32_t null_positions[6];  //this is in stage units (mm)
	float32_t latest_joystick_update[6];

} Sm_data;

typedef struct  __attribute__((packed))
{
	Sm_data sm_data;
	Sm_Save_Params save;
	stepper_sm_Rx_data sm_rx[NUMBER_OF_BOARD_SLOTS];
	stepper_sm_Tx_data sm_tx[NUMBER_OF_BOARD_SLOTS];
	PmRotationMatrix R_inverseMatrix;
	PmRotationMatrix PointsMatrix;
	float32_t latest_joystick_update[6];
} Sm_info;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void stepper_synchronized_motion_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_CARDS_STEPPER_SYNCHRONIZED_MOTION_H_ */
