/**
 * @file usr_limits.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include <usr_limits.h>
#include "usr_interrupt.h"
#include <cpld.h>
#include <encoder.h>
#include <pins.h>
#include "Debugging.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void limits_handler(uint8_t slot, void*);
static void check_interrupt_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits);
static void check_soft_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits);
static void check_abs_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits, uint8_t type);
static void check_abs_rotation_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits,
		uint8_t type);
static uint8_t read_limits(uint8_t slot);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
/**
 * Interrupt handler for the interrupt coming in from CPLD.
 * @param slot
 */
static void limits_handler(uint8_t slot, void*)
{
	// set flag to read the interrupt reg from cpld
	// cannot read from inside interrupt handler because
	// read could be in progress from other device
	slots[slot].interrupt_flag_cpld = 1;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
static void check_interrupt_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits)
{
	/* If the limit interrupt pin changed state*/
	if (slots[slot].interrupt_flag_cpld)
	{

		// for homing t0 index
		if(limits->homed == HOMING_STATUS)
		{
			limits->index = 1;
		}


		/*Debounce wait 20ms before reading.  The task is serviced every 10ms so wait 2 times */
		if (limits->debounce_cnt >= 2)  // TODO temp no debounce
		{
			slots[slot].interrupt_flag_cpld = false;
			/*reset debounce count*/
//			limits->debounce_cnt = 0;
			uint8_t limits_val = read_limits(slot);

			/*Index is a very short pulse which will be missed because we read after 10 - 20ms later.
			 * but we still get the interrupt here.  Rather than the index which will always be false,
			 * we can check if limits_val = 0 then we assume the interrupt was triggered by the index.*/


			/* get all the limit data*/
			limits->index = (bool) Tst_bits(limits_val, INDEX);
			limits->cw = (bool) Tst_bits(limits_val, CW_LIMIT);
			limits->ccw = (bool) Tst_bits(limits_val, CCW_LIMIT);

			/*polarity reversed ?*/
			if ((limits_saved->cw_hard_limit & 0x007F) == BREAKS_ON_CONTACT)
				limits->cw = !limits->cw;
			if ((limits_saved->ccw_hard_limit & 0x007F) == BREAKS_ON_CONTACT)
				limits->ccw = !limits->ccw;

			/*ignore limit ?*/
			if ((limits_saved->cw_hard_limit & 0x007F) == NO_LIMIT)
				limits->cw = 0;
			if ((limits_saved->ccw_hard_limit & 0x007F) == NO_LIMIT)
				limits->ccw = 0;

			/*reverse limit ?*/
			bool temp_cw = limits->cw;
			if ((limits_saved->cw_hard_limit & 0x0080) == REVERSE_LIMITS)
				limits->cw = limits->ccw;
			if ((limits_saved->ccw_hard_limit & 0x0080) == REVERSE_LIMITS)
				limits->ccw = temp_cw;
		}

		/*Increment the de-bounce counter*/
		limits->debounce_cnt++;
	}
	else
	{
		/*reset debounce count*/
		limits->debounce_cnt = 0;
	}
}

static void check_soft_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits)
{
	if(!limits->encoder_reveres_flag)
	{
        limits->soft_cw = *limits->compare_val > limits_saved->cw_soft_limit;

        limits->soft_ccw = *limits->compare_val < limits_saved->ccw_soft_limit;
	}
	else  /* encoder counts in reverse*/
	{
        limits->soft_cw = *limits->compare_val < limits_saved->ccw_soft_limit;

        limits->soft_ccw = *limits->compare_val > limits_saved->cw_soft_limit;
	}
}

static void check_abs_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits, uint8_t type)
{
	/*compare_val is the raw encoder value.  Setup as a pointer because this function can be
	 * used by different card slot types*/
	/*High limit*/
	if (*limits->compare_val > limits_saved->abs_high_limit)
	{
		limits->cw = true;
		return;
	}
	else
	{
		limits->cw = false;
	}

	/*Low limit*/
	if (*limits->compare_val < limits_saved->abs_low_limit)
	{
		limits->ccw = true;
		return;
	}
	else
	{
		limits->ccw = false;
	}
}

static void check_abs_rotation_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits, uint8_t type)
{
	/*For the EPI turret on the inverted the counts go up CW dir from the top.
	 * For the light path on the inverted the counts go down CW dir from the top.
	 * Because the encoder is absolute we cannot change the count direction*/

	int32_t pos = *limits->compare_val;
	int32_t high = limits_saved->abs_high_limit;
	int32_t low = limits_saved->abs_low_limit;

	if(!Tst_bits(limits_saved->cw_hard_limit, REVERSE_LIMITS))
	{
		// are we between the limits?
		if ((pos > low) && (pos < high))
		{
			limits->cw = false;
			limits->ccw = false;
		}
		else /*We are in the area outside the limits*/
		{
			/*Let's find out which one*/
			if (pos < low)
				limits->cw = true;
			else
				limits->ccw = true;
		}
	}
	else	// this is the reverse part of the area in the circle.  We use the swap bit to control
	{
		// are we outside the limits?
		if ((pos < low) || (pos > high))
		{
			limits->cw = false;
			limits->ccw = false;
		}
		else /*We are outside the area in the limits*/
		{
			/*Let's find out which one*/
			if (pos > low)
				limits->cw = true;
			else
				limits->ccw = true;
		}
	}
}

// MARK:  SPI Mutex Required
static uint8_t read_limits(uint8_t slot)
{
	/*Read the interrupt to get the limit data and clear the limit*/
	uint32_t limits_val = 0;

	read_reg(C_READ_INTERUPTS, slot, NULL, &limits_val);

	return limits_val;

}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
void service_limits(uint8_t slot, Limits_save *limits_saved, Limits *limits, uint8_t type)
{
	// no interrupt from these so check them manually
	check_soft_limits(slot, limits_saved, limits);

	switch (type)
	{
		case ENCODER_TYPE_ABS_INDEX_LINEAR:
		case ENCODER_TYPE_ABS_BISS_LINEAR:
			check_abs_limits(slot, limits_saved, limits, type);
			break;
		/*Put all rotational encoders here to deal roll over*/
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW:
		case ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL:
			check_abs_rotation_limits(slot, limits_saved, limits, type);
			break;

		case ENCODER_TYPE_QUAD_LINEAR:
		default:
		        check_interrupt_limits(slot, limits_saved, limits);
			break;
	}

#if 1  //dev work area for MMA decoding
	if (slots[slot].interrupt_flag_cpld && type == ENCODER_TYPE_ABS_INDEX_LINEAR)
	{
		//clear the interrupt
		// uint8_t limits_val = read_limits(slot);
		read_limits(slot);

		//read in the buffered value from the CPLD
		uint32_t copy = limits->buffer_index;
		read_reg(C_READ_QUAD_BUFFER, slot, NULL, &copy);
		limits->buffer_index = copy;
	}
#endif
}

void shift_soft_limits(Limits_save * const p_limits_saved, const int32_t offset)
{
	p_limits_saved->cw_soft_limit  += (p_limits_saved->cw_soft_limit  ==  CW_UNSET) ? 0 : offset;
	p_limits_saved->ccw_soft_limit += (p_limits_saved->ccw_soft_limit == CCW_UNSET) ? 0 : offset;
}

void setup_limits(uint8_t slot, Limits *limits, int32_t *compare_val, bool encoder_reveres_flag)
{
	/*Setup the pointer for the compare value for soft limits*/
	limits->compare_val = compare_val;

	/*set the encoder reverse flag from calling task*/
	limits->encoder_reveres_flag = encoder_reveres_flag;

	// Set the debounce count to 0.
	limits->debounce_cnt = 0;

	slots[slot].p_interrupt_cpld_handler = &limits_handler;
	setup_slot_interrupt_CPLD(slot);
}

