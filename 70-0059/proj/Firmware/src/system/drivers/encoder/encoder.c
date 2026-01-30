/**
 * @file encoder.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include <encoder.h>
#include <encoder_quad_linear.h>
#include <encoder_abs_index_linear.h>
#include <encoder_abs_biss_linear.h>
#include <encoder_abs_magnetic_rotation.h>
#include <stepper.h>
#include <cpld.h>
#include <pins.h>
#include <string.h>
#include <delay.h>
#include "helper.h"
#include "usr_limits.h"

// TODO temp
int32_t Debug_new_counts = 0;
int32_t Debug_t = 0;


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static int32_t average_filter(Encoder *enc, int32_t new_counts, int32_t max_counts);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int32_t reverse_encoder_value(const int32_t encoder_value);
static int32_t average_filter(Encoder *enc, int32_t new_counts, int32_t max_counts)
{
	uint8_t filter_coef = 0;
	int32_t enc_pos = enc->enc_pos_prev_raw;
	uint32_t delta = abs(new_counts - enc_pos);
	Debug_new_counts = new_counts;

	// check for wrap around
	if(delta > (uint32_t) max_counts)
	{
		delta = max_counts - delta;

		if(new_counts  > max_counts)
		{
			enc_pos = max_counts - enc_pos;
		}
		else
		{
			enc_pos = new_counts - (max_counts - enc_pos);
		}
	}

	if(delta < 40)
	{
		filter_coef = 20;
	}
	else if(delta < 100)
	{
		filter_coef = 4;
	}
	else
	{
		filter_coef = 0;
	}

	int32_t temp = (new_counts + (enc_pos * filter_coef));
	float t = temp / (filter_coef + 1);
	Debug_t = (int32_t) t;

	/* Store this raw value to compute the delta next time*/
	enc->enc_pos_prev_raw = (int32_t) t;

	return (int32_t) t;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int32_t convert_signed32_to_twos_comp(int32_t value)
{
	if (value < 0)
		return value + 0x400000;
	else
		return value;
}

int32_t convert_twos_comp_to_signed32(uint32_t value)
{
	int32_t temp;

	if (value >> 21)
		temp = value | 0xFFC00000;
	else
		temp = value;
	return temp;
}

// MARK:  SPI Mutex Required
void set_encoder_position(uint8_t slot, uint8_t encoder_flags, Encoder *enc, int32_t counts)
{
	/* if counting is reversed multiple by -1*/
	if (encoder_flags & ENCODER_REVERSED)
		counts *= -1;

	int32_t counts_in_twos_format = 0;

	switch (enc->type)
	{
		case ENCODER_TYPE_NONE:
			break;

		case ENCODER_TYPE_QUAD_LINEAR:
			counts_in_twos_format = counts;
			set_quad_linear_encoder_position(slot, counts_in_twos_format);
			enc->enc_pos = counts;
			// for quad copy the value to enc_pos_raw because it is used elsewhere
			enc->enc_pos_raw = counts;
			break;

		case ENCODER_TYPE_ABS_INDEX_LINEAR:
			counts_in_twos_format = convert_signed32_to_twos_comp(counts);
			set_quad_linear_encoder_position(slot, counts_in_twos_format);
			enc->enc_pos = counts;
			// for quad copy the value to enc_pos_raw because it is used elsewhere
			enc->enc_pos_raw = counts;
			// TODO:  Check the changes to soft limits.
			break;

		case ENCODER_TYPE_ABS_BISS_LINEAR:
			enc->enc_zero = enc->enc_pos_raw + counts;
			enc->enc_pos = enc->enc_pos_raw - enc->enc_zero;
			// TODO:  Check the changes to soft limits.
			break;

		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL:
			enc->enc_zero = 0;
			// TODO:  Check the changes to soft limits.
			//			set_abs_magnetic_rotation_encoder_position(slot, counts_in_twos_format);
			break;

		default:
			break;
	}
}

/**
 * @brief Reads the encoder counts from the CPLD.  The count register is 22 bits.
 * @param slot
 * @param encoder : These are where encoder flags are stored
 * @param enc : Pointer to encoder structure
  */
// MARK:  SPI Mutex Required
void get_encoder_counts(uint8_t slot, uint8_t encoder_flags, uint32_t max_pos, Encoder *enc)
{
	uint32_t counts;
	int32_t counts_in_twos_format;
	// int32_t temp;

	// float x,t,y = 0;
	// int32_t z = 0;

	switch (enc->type)
	{
		case ENCODER_TYPE_NONE:
			break;

		case ENCODER_TYPE_QUAD_LINEAR:
			counts = get_quad_linear_encoder_counts(slot);
			enc->enc_pos = counts;
			enc->enc_pos_raw = enc->enc_pos;
			break;

		case ENCODER_TYPE_ABS_INDEX_LINEAR:
			counts = get_quad_linear_encoder_counts(slot);
			counts_in_twos_format = convert_twos_comp_to_signed32(counts);
			enc->enc_pos = counts_in_twos_format;
			enc->enc_pos_raw = enc->enc_pos;
			break;

		case ENCODER_TYPE_ABS_BISS_LINEAR:
			enc->enc_pos_raw = get_abs_biss_linear_encoder_counts(slot, enc);
			enc->enc_pos = enc->enc_pos_raw - enc->enc_zero;

		break;

		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL:
			/*Counts go up in CCW direction*/
			counts = get_abs_magnetic_rotation_encoder_counts(slot, enc);

#if 1 // use filter
			enc->enc_pos_raw = average_filter(enc, counts, max_pos);
#else
			enc->enc_pos_raw = counts;
#endif

#if 1 // we don't use enc_zero for magnetic encoders
			enc->enc_pos = enc->enc_pos_raw;

#else
		/* Calculate enc_pos from offset.  Because it is rotational we have to check if the
		 * offset is in the wrap around region*/
		if (enc->enc_pos_raw < enc->enc_zero) /*Wrap around region*/
		{
			enc->enc_pos = max_pos - (enc->enc_zero - enc->enc_pos_raw);
		}
		else
			enc->enc_pos = enc->enc_pos_raw - enc->enc_zero;
#endif
		break;
		default:
			break;
	}

	/* if counting is reversed multiple by -1*/
	if (encoder_flags & ENCODER_REVERSED)
		enc->enc_pos *= -1;
}

#if 0
void encoder_reset_nsl(uint8_t slot, bool state)
{
	/* Need to send cpld reset for NSL because it needs to be held hi for 3m after power up
	 * for the magnetic encoder but also need to send it for the Fagor BISS becuase in the cpld
	 * it is also part of the protocol
	 * in the CPLD after reset reset_nsl = 0 need to set to reset_nsl = 1 to get the protocol going.
	 * in the magnetic sensor is powered off, we need to reset this to 0 and then back to 1*/
	set_reg(C_SET_STEPPER_ENC_NSL_DI, slot, 0, state);
}
#endif

/**
 * @brief Setup the encoder for this slot
 * @param slot
 * @param encoder_flags
 * @param enc
 */
// MARK:  SPI Mutex Required
void setup_encoder(uint8_t slot, uint8_t encoder_flags, uint32_t max_pos, Encoder *enc)
{
	enc->error = 0;
	enc->warn = 0;
	enc->mhi = 0;
	enc->mlo = 0;
	enc->m_ready = 1;

	if (Tst_bits(encoder_flags, HAS_ENCODER))
	{
		switch (enc->type)
		{
			case ENCODER_TYPE_NONE:
				break;

			case ENCODER_TYPE_QUAD_LINEAR:
				/*Disable encoder zero on limit mode*/
				set_reg(C_SET_HOMING, slot, 0, (uint32_t)C_HOMING_DISABLE);
				set_encoder_position(slot, encoder_flags, enc, enc->enc_pos);
				break;

			case ENCODER_TYPE_ABS_INDEX_LINEAR:
				set_reg(C_SET_HOMING, slot, 0, (uint32_t)C_HOMING_DISABLE);
				set_encoder_position(slot, encoder_flags, enc, enc->enc_pos);
				enc->homed = NOT_HOMED_STATUS;
				break;

			case ENCODER_TYPE_ABS_BISS_LINEAR:
//				encoder_reset_nsl(slot, 1);
				break;

			case ENCODER_TYPE_ABS_MAGNETIC_ROTATION:
			case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME:
			case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW:
			case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL:

//				abs_magnetic_rotation_encoder_power(slot, MAG_PWR_ON);
//				encoder_reset_nsl(slot, 1);
//				get_encoder_counts(slot, encoder_flags,	max_pos, enc);
				enc->homed = NOT_HOMED_STATUS;
				break;

			default:
				break;
		}

	}
}


void raw_encoder_position_pair_to_encoder_position_pair(int32_t *const p_bimodal_low, int32_t *const p_bimodal_high, const bool encoder_reversed)
{
	if (encoder_reversed)
	{
		const int32_t copy = *p_bimodal_low;
		*p_bimodal_low = reverse_encoder_value(*p_bimodal_high);
		*p_bimodal_high = reverse_encoder_value(copy);
	}
}
void encoder_position_pair_to_raw_encoder_position_pair(int32_t *const p_bimodal_low, int32_t *const p_bimodal_high, const bool encoder_reversed)
{
	// They are the same underlying operation (reverse and swap)
	raw_encoder_position_pair_to_encoder_position_pair(p_bimodal_low, p_bimodal_high, encoder_reversed);
}


static int32_t reverse_encoder_value(const int32_t encoder_value)
{
	// Using the min and one higher to guarantee flipping the value will never
	// cause the encoder value to be interpreted as the sentinel values.
	if (encoder_value <= (INT32_MIN + 1)) {
		return INT32_MAX;
	} else if (encoder_value == INT32_MAX) {
		return INT32_MIN;
	} else {
		return -encoder_value;
	}
}

//
