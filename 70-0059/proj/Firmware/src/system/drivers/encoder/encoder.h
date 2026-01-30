/**
 * @file encoder.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_H_
#define SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_H_

#include "stdint.h"

#ifndef _DESKTOP_BUILD // Used in the LUT compiler to ignore this import.
#include "arm_math.h"
#else
typedef float float32_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
// Type of encoder
#define ENCODER_TYPE_NONE								0
#define ENCODER_TYPE_QUAD_LINEAR						1
#define ENCODER_TYPE_ABS_INDEX_LINEAR					2
#define ENCODER_TYPE_ABS_BISS_LINEAR					3
#define ENCODER_TYPE_ABS_MAGNETIC_ROTATION				4	// for Veneto light path plate
#define ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME	5	// for Veneto EPI with motor
#define ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW			6	// this is for the inverted NDD switcher
#define ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL		7	// for Veneto EPI with NO motor

#define NO_ENCODER                      0
#define HAS_ENCODER                     (1 << 0)
#define ENCODER_REVERSED                (1 << 1)
#define ENCODER_HAS_INDEX               (1 << 2)
#define STEPPER_REVERSED                (1 << 3)
#define USE_PID                         (1 << 4)
#define PID_KICKOUT                     (1 << 5)
#define ROTATIONAL_STATE                (1 << 6)
#define PREFERS_SOFT_STOP               (1 << 7)

#define NUM_OF_STORED_POS	10

#define FAGOR_ENCODER_ERROR_LIMIT	4

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	uint8_t encoder_type;
	uint16_t index_delta_min;
	uint16_t index_delta_step;
    /**
     * For encoders on linear stages, this value is in units of nm per encoder count.
     * For encoders on rotational stages, this value is in units of milli-degrees
     *  (1/1000th of a degree) per encoder count.
     */
	float32_t nm_per_count;
} Encoder_Save;

typedef struct  __attribute__((packed))
{
	uint8_t type;
	int32_t enc_pos;
	int32_t enc_pos_raw;
	int32_t enc_pos_prev_raw;
	int32_t enc_zero;

	// ABS BISS encoder
	bool error;
	bool warn;
	uint8_t error_cnt;	/* The fagor encoder has errors while booting so we count
					 them and if the exceed a limit then the error exist,
					 this counter should be reset when the error is cleared
					 by the encoder*/

	// ABS rotary magnetic encoder
	bool mhi;
	bool mlo;
	bool m_ready;
	bool mag_good;
	uint8_t homed;
	uint8_t home_delay_cnt;
} Encoder;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
int32_t convert_signed32_to_twos_comp(int32_t value);
int32_t convert_twos_comp_to_signed32(uint32_t value);

/**
 * \warning Steppers using non-absolute encounders should shift their soft limits *before* calling this function.
 */
void set_encoder_position(uint8_t slot, uint8_t encoder_flags, Encoder *enc, int32_t counts);
void get_encoder_counts(uint8_t slot, uint8_t encoder_flags, uint32_t max_pos, Encoder *enc);
//void encoder_reset_nsl(uint8_t slot, bool state);
void setup_encoder(uint8_t slot, uint8_t encoder_flags, uint32_t max_pos, Encoder *enc);

/**
 * Translates two bounded values (i.e. min and max, high and low, etc.) in raw encoder counts to encoder counts.
 * \param[inout]	p_bimodal_low The low value of the bounded range.
 * \param[inout]	p_bimodal_high The high value in the bounded range.
 * \param[in]		encoder_reversed If the encoder is reversed.
 */
void raw_encoder_position_pair_to_encoder_position_pair(int32_t *const p_bimodal_low, int32_t *const p_bimodal_high, const bool encoder_reversed);

/**
 * Translates two bounded values (i.e. min and max, high and low, etc.) in encoder counts to raw encoder counts.
 * \param[inout]	p_bimodal_low The low value of the bounded range.
 * \param[inout]	p_bimodal_high The high value in the bounded range.
 * \param[in]		encoder_reversed If the encoder is reversed.
 */
void encoder_position_pair_to_raw_encoder_position_pair(int32_t *const p_bimodal_low, int32_t *const p_bimodal_high, const bool encoder_reversed);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_H_ */
