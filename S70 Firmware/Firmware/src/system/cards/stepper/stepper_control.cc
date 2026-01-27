/**
 * @file stepper_control.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "stepper.h"
#include "stepper_control.h"
#include <cpld.h>
#include "25lc1024.h"
#include "Debugging.h"
#include "hid_out.h"
#include <usr_limits.h>
#include <algorithm>
#include "encoder_abs_magnetic_rotation.h"
#include "log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static bool read_limit(slot_nums slot);
static int32_t find_min_saved_position(Stepper_info *info);
static int32_t find_max_saved_position(Stepper_info *info);
static void homing_ctl_mag(Stepper_info *info);
static void homing_ctl_mag_epi_turret(Stepper_info *info);
static void homing_ctl(Stepper_info *info);
static void run_ctl(Stepper_info *info);
static void pid_ctrl(Stepper_info *info);
static void position_save_check(Stepper_info *info);
static bool check_em_stop(Stepper_info *info);
static void check_save_position(Stepper_info *info);
static bool check_for_collision(Stepper_info *info);

/**
 * Configures the cmd_dir and cmd_vel fields of stepper_control
 * while applying any stepper reversal and speed limitations.
 * \param[in] enable_reversal Optional (default: true)
 *            Handles the direction-reversal flag.
 */
static void configure_movement(Stepper_info* info, bool direction, uint16_t target_speed, bool enable_reversal = true);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * Checks if the optical slot sensor is in the slot.
 * @param slot
 * @return	0 if not in slot, 1 if in slot
 */
// MARK:  SPI Mutex Required
static bool read_limit(slot_nums slot)
{
	uint32_t temp = 0;

	/*Read the interrupt to get the limit data and clear the limit*/
	read_reg(C_READ_INTERUPTS, slot, NULL, &temp);

	/*Clear the interrupt flag so the limit service will fire*/
	slots[slot].interrupt_flag_cpld = false;

	/*Check to see if we are in the slot for the optical slot index, */
	if ((bool) Tst_bits(temp, INDEX)) /* Not in the slot*/
	{
		return 0;
	}
	else /* In the slot*/
	{
		return 1;
	}
}

static int32_t find_min_saved_position(Stepper_info *info)
{
	int32_t temp = (int32_t) info->save.config.params.config.max_pos;
	for (uint8_t i = 0; i < 6; ++i)
	{
		if(info->save.store.stored_pos[i] < temp)
			temp = info->save.store.stored_pos[i];
	}
	return temp;
}

static int32_t find_max_saved_position(Stepper_info *info)
{
	int32_t temp = 0;
	for (uint8_t i = 0; i < 6; ++i)
	{
		if ((info->save.store.stored_pos[i] > temp)
				&& (info->save.store.stored_pos[i]
						< (int32_t) info->save.config.params.config.max_pos))
			temp = info->save.store.stored_pos[i];
	}
	return temp;
}

// MARK:  SPI Mutex Required
static void homing_ctl_mag(Stepper_info *info)
{
//#define MAG_SW_P1	1500
//#define MAG_SW_P2	56500

#define MAG_SW_CYCLE_POSITION_DEADBAND	50

#define MAG_SW_CYCLE_REPEAT_COUNT	3
#define MAG_SW_POWER_CYCLE_DELAY	100  	// 10's of ms

	int32_t min_pos, max_pos;

	/*The magnet needs to be in a position so that when the stage is moves to the detect on the
	 * side of the motor, position 1, the encoder counts are roughly 1,000 counts
	 *
	 * NOT USED, must rotate a few times to calibrate
	 * The homing needs to power the magnetic sensor off at a particular position and then turn it
	 * back on and cycle the stage in order to remove the offset drift of the encoder. Then the
	 * stage goes to position 1
	 * */

	min_pos = find_min_saved_position(info);
	// must be a fresh board and homing position have not been set
	if((min_pos < 0) || (min_pos > (int32_t) info->save.config.params.config.max_pos))
		info->ctrl.homing_mode = HOME_MAG_SW_HOMING_FAILED;

	max_pos = find_max_saved_position(info);
	// must be a fresh board and homing position have not been set
	if((max_pos < 0) || (max_pos > (int32_t) info->save.config.params.config.max_pos))
		info->ctrl.homing_mode = HOME_MAG_SW_HOMING_FAILED;

	switch (info->ctrl.homing_mode)
	{
	case HOME_IDLE:
		break;

	case HOME_START:
		/*Move the stage close to position 1 */
		info->pid.max_velocity = 1;
		info->counter.cmnd_pos = min_pos;
		info->ctrl.homing_control = 0;
		sync_stepper_steps_to_encoder_counts(info);
		info->ctrl.homing_mode = HOME_MAG_SW_MOVE_TO_POSITION_LOW;
		break;

	case HOME_MAG_SW_MOVE_TO_POSITION_LOW:
		pid_ctrl(info);

		if(info->collision)
			info->ctrl.homing_mode = HOME_MAG_SW_HOMING_FAILED;

		else if (abs(info->pid.error) <= MAG_SW_CYCLE_POSITION_DEADBAND)
		{
			/* ***** This caused problems ***** */
			/* If its the first time going to position 1, turn off the the power to the encoder to
			 * calibrate.*/
//			if(info->ctrl.homing_control == 0)
//				abs_magnetic_rotation_encoder_power(info->slot, MAG_PWR_OFF);

			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_SW_WAIT_1;
		}
		break;

	case HOME_MAG_SW_WAIT_1:
		if (info->counter.homing_counter >= MAG_SW_POWER_CYCLE_DELAY)
		{
			/* ***** This caused problems ***** */
//			if(info->ctrl.homing_control == 0)
//			{
//				/*Turn power back on*/
//				abs_magnetic_rotation_encoder_power(info->slot, MAG_PWR_ON);
//			}

			/*Move the stage close to position 2 */
			info->counter.cmnd_pos = max_pos;
			info->ctrl.homing_control++;;
			info->ctrl.homing_mode = HOME_MAG_SW_MOVE_TO_POSITION_HIGH;
		}
		break;

	case HOME_MAG_SW_MOVE_TO_POSITION_HIGH:
		pid_ctrl(info);

		if(info->collision)
			info->ctrl.homing_mode = HOME_MAG_SW_HOMING_FAILED;

		else if (abs(info->pid.error) <= MAG_SW_CYCLE_POSITION_DEADBAND)
		{
			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_SW_WAIT_2;
		}
		break;

	case HOME_MAG_SW_WAIT_2:
		if (info->counter.homing_counter >= MAG_SW_POWER_CYCLE_DELAY)
		{
			if(info->ctrl.homing_control >= MAG_SW_CYCLE_REPEAT_COUNT)
			{
				// finished with cycles
				info->ctrl.homing_mode = HOME_MAG_SW_HOMING_COMPLETE;
			}
			else
			{
				/*Move the stage close to position 1 */
				info->counter.cmnd_pos = min_pos;
				info->ctrl.homing_control++;
				info->ctrl.homing_mode = HOME_MAG_SW_MOVE_TO_POSITION_LOW;
			}
		}
		break;

	case HOME_MAG_SW_HOMING_COMPLETE:
		info->enc.homed = IS_HOMED_STATUS;
		info->limits.homed = IS_HOMED_STATUS;
		stepper_goto_stored_pos(info, 0); // go to position 1
		break;

	case HOME_MAG_SW_HOMING_FAILED:
		info->enc.homed = HOMING_FAILED_STATUS;
		info->limits.homed = NOT_HOMED_STATUS;
		info->ctrl.mode = STOP;
		break;
	}

	info->counter.homing_counter++;
}

// MARK:  SPI Mutex Required
static void homing_ctl_mag_epi_turret(Stepper_info *info)
{
    const uint16_t HOMING_SPEED = stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_HOMING);

	/*	Homing procedure:
	 - magnets must be installed with a jig to keep them all relatively the same angle because of
	 linearity error
	 */
#define MAG_SLOT_TO_P1_COUNTS			3300
#define MAG_COUNTS_BETWEEN_POSITIONS	10923

#define MAG_MED_SPEED_DIV			20
#define MAG_SLOW_SPEED_DIV			100
#define MAG_FIRST_EDGE_CYCLE_DELAY	100  	// 10's of ms
#define MAG_SECOND_EDGE_CYCLE_DELAY	100  	// 10's of ms
#define MAG_SAVE_POSITION_DELAY		150  	// 10's of ms

	/* read limits on our own because we need to look at both edges
	Read the interrupt to get the limit data and clear the limit
	*/
	bool in_slot = read_limit(info->slot);

	switch (info->ctrl.homing_mode)
	{
	case HOME_IDLE:
		break;

	case HOME_START:
		sync_stepper_steps_to_encoder_counts(info);
		info->pid.max_velocity = 1;
		info->counter.cmnd_pos = info->enc.enc_pos - 30000;
		info->ctrl.homing_mode = HOME_MAG_CHECK_NO_COLLISION;
		break;

	case HOME_MAG_CHECK_NO_COLLISION:
		pid_ctrl(info);

		if(info->collision)
			info->ctrl.homing_mode = HOME_MAG_FAILED;

		else if (info->pid.error == 0)
			info->ctrl.homing_mode = HOME_START_1;
		break;

	case HOME_START_1:
		/* set homing_control to 20 to let us know we have not spun the turret once to calibrate,
		 we pick 20 because this is also used to keep count of the position we are calibrating.
		 Calibrating each position using the stepper counts is required because of non-linear errors
		 when re inserting or using a different turret */
		info->ctrl.homing_control = 20;

		/*Turn on the opto switch*/
		set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 1);

		/* ***** This caused problems ***** */
		/* Turn off the encoder so we can turn it back on when we see the first edge of the slot.
		 Putting the turret in at different angles creates problems, powering up at same location
		 make it consistent.*/
//		abs_magnetic_rotation_encoder_power(info->slot, MAG_PWR_OFF);

		info->ctrl.homing_mode = HOME_MAG_SPIN_CCW_FAST;
		break;

	case HOME_MAG_SPIN_CCW_FAST:
		configure_movement(info, DIR_REVERSE, HOMING_SPEED);
		run_stepper(info->slot, info->ctrl.cmd_dir, info->ctrl.cmd_vel);

		if(info->ctrl.homing_control == 20)
			info->ctrl.homing_mode = HOME_MAG_WAIT_FOR_FIRST_EDGE;
		else if(info->ctrl.homing_control == 21) // wait before checking for edge because we are in the slot
		{
			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_WAIT_1;
		}
		break;

	case HOME_MAG_WAIT_FOR_FIRST_EDGE:
		if (in_slot)
		{
			hard_stop_stepper(info->slot);
			info->ctrl.mode = HOMING; /*Reset the mode to homing because check_run_limits set to STOP*/
			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_WAIT_1;
		}
		break;

	case HOME_MAG_WAIT_1:
		if (info->counter.homing_counter >= MAG_FIRST_EDGE_CYCLE_DELAY)
		{
			/*First time we want to spin the turret one complete revolution  to calibrate*/
			if(info->ctrl.homing_control == 20)
			{
				/* ***** This caused problems ***** */
				/*Turn on the encoder*/
//				abs_magnetic_rotation_encoder_power(info->slot, MAG_PWR_ON);
				info->ctrl.homing_control = 21;
				info->ctrl.homing_mode = HOME_MAG_SPIN_CCW_FAST;
			}
			else if(info->ctrl.homing_control == 21)
			{
				info->ctrl.homing_control = 0;
				info->ctrl.homing_mode = HOME_MAG_WAIT_FOR_FIRST_EDGE;
			}
			else
			{
				info->ctrl.homing_mode = HOME_MAG_CHECK_IF_PASSED_SLOT;
			}
		}
		break;

	case HOME_MAG_CHECK_IF_PASSED_SLOT:
		/*Check to see if we are in the slot for the optical slot index.  On the first fast spin we
		 * could overshoot the slot or fall in it.  We need to be inside the slot so that we don't
		 * see the double interrupt from the other side of the slot*/
		if (!in_slot)
		{
			info->ctrl.homing_mode = HOME_MAG_GET_IN_SLOT;
		}
		else
		{
			info->ctrl.homing_mode = HOME_MAG_STOP_BACK_TO_FIRST_EDGE;
		}
		info->ctrl.mode = HOMING; /*Reset the mode to homing because check_run_limits set to STOP*/
        configure_movement(info, DIR_FORWARD, HOMING_SPEED / (in_slot ? MAG_SLOW_SPEED_DIV : MAG_MED_SPEED_DIV));
		run_stepper(info->slot, info->ctrl.cmd_dir, info->ctrl.cmd_vel);
		break;

	case HOME_MAG_GET_IN_SLOT:
		/*Stop when we see the edge of the slot, this will put us inside the slot*/
		if (in_slot)
		{
			hard_stop_stepper(info->slot);
			info->ctrl.mode = HOMING; /*Reset the mode to homing because check_run_limits set to STOP*/
            configure_movement(info, DIR_FORWARD, HOMING_SPEED / MAG_SLOW_SPEED_DIV);
			run_stepper(info->slot, info->ctrl.cmd_dir, info->ctrl.cmd_vel);
			slots[info->slot].interrupt_flag_cpld = true;
			info->ctrl.homing_mode = HOME_MAG_STOP_BACK_TO_FIRST_EDGE;
		}
		break;

	case HOME_MAG_STOP_BACK_TO_FIRST_EDGE:
		if (!in_slot)
		{
			hard_stop_stepper(info->slot);
			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_WAIT_2;
		}
		break;

	case HOME_MAG_WAIT_2:
		if (info->counter.homing_counter >= MAG_SECOND_EDGE_CYCLE_DELAY)
		{
			/* get the encoder value which is the offset*/
			info->enc.enc_zero = 0;
			sync_stepper_steps_to_encoder_counts(info);
			/*Turn off the encoder temporarily so we can run the PID using the step counts instead
			 * of the encoder counts.  The step counts are very accurate and so we use them to
			 * calibrate the turret*/
			info->ctrl.prev_flags = info->save.config.params.flags.flags; // save old flags
			Clr_bits(info->save.config.params.flags.flags, HAS_ENCODER);

			info->pid.max_velocity = 1;
			info->counter.cmnd_pos = info->enc.enc_pos - MAG_SLOT_TO_P1_COUNTS;
			info->ctrl.homing_mode = HOME_MAG_GOTO_NEXT_POSTION;
		}
		break;

	case HOME_MAG_GOTO_NEXT_POSTION:
		pid_ctrl(info);
		if(info->pid.error == 0)
		{
			info->save.config.params.flags.flags = info->ctrl.prev_flags;
			info->counter.homing_counter = 0;
			info->ctrl.homing_mode = HOME_MAG_WAIT_3;
		}
		break;

	case HOME_MAG_WAIT_3:
		if (info->counter.homing_counter >= MAG_SAVE_POSITION_DELAY)
		{
			info->ctrl.homing_mode = HOME_MAG_SAVE_POSTION;
		}
		break;

	case HOME_MAG_SAVE_POSTION:
		sync_stepper_steps_to_encoder_counts(info);
		// save the position to ram
		info->save.store.stored_pos[info->ctrl.homing_control] =	info->enc.enc_pos;
		/*Setup next position*/
		info->ctrl.homing_control++;
		if(info->ctrl.homing_control >= 6)
		{
			info->ctrl.homing_mode = HOME_MAG_COMPLETE;
		}
		else
		{
			info->ctrl.prev_flags = info->save.config.params.flags.flags; // save old flags
			Clr_bits(info->save.config.params.flags.flags, HAS_ENCODER);

			sync_stepper_steps_to_encoder_counts(info);
			info->pid.max_velocity = 1;
			info->counter.cmnd_pos = info->enc.enc_pos - MAG_COUNTS_BETWEEN_POSITIONS;
			info->ctrl.homing_mode = HOME_MAG_GOTO_NEXT_POSTION;
		}
		break;

	case HOME_MAG_COMPLETE:
		// Power off the optical switch
		set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 0);
		info->enc.homed = IS_HOMED_STATUS;
		info->limits.homed = IS_HOMED_STATUS;
		stepper_goto_stored_pos(info, 0); // go to position 1
		break;

	case HOME_MAG_FAILED:
		// Power off the optical switch
		set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 0);
		info->enc.homed = HOMING_FAILED_STATUS;
		info->limits.homed = NOT_HOMED_STATUS;
		info->ctrl.mode = STOP;
		break;

	}
	info->counter.homing_counter++;
}

// MARK:  SPI Mutex Required
static void homing_ctl(Stepper_info *info)
{
    // 7 b(its) + 10 b < 32 b
    const uint16_t HOMING_SPEED = stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_HOMING);
    // bool stepper_reversed = Tst_bits(info->save.config.params.flags.flags, STEPPER_REVERSED);

	if (info->ctrl.homing_mode == HOME_START
	 	&& (info->save.config.params.home.limit_switch == HOME_TO_LIMIT
			|| info->save.config.params.home.limit_switch == HOME_TO_HARD_STOP)
		&& ((info->save.config.params.home.home_dir == HOME_CW
				&& info->limits.ccw)
			|| (info->save.config.params.home.home_dir == HOME_CCW
				&& info->limits.cw))
	) {
		// The homing procedure is trying to start, but the device in on the limit,
		// so transitions to the delayed register mode.
		info->ctrl.homing_mode = HOME_DELAYED_START;
	}

	switch (info->ctrl.homing_mode)
	{
	case HOME_IDLE:
		break;

	case HOME_START:
		/*Put encoder in zero on limit mode*/
		set_reg(C_SET_HOMING, info->slot, 0, (uint32_t) C_HOMING_ENABLE);

		// check method of homing
		if (info->save.config.params.home.limit_switch == HOME_TO_LIMIT
				|| info->save.config.params.home.limit_switch
						== HOME_TO_HARD_STOP)
		{
			sync_stepper_steps_to_encoder_counts(info);
			if (info->save.config.params.home.home_dir == HOME_CW)
			{
				if (!info->limits.ccw)
				{
                    configure_movement(info, DIR_FORWARD, HOMING_SPEED);
					run_ctl(info);
					info->ctrl.homing_mode = HOME_WAIT_LIMIT;
				}
			}
			else
			{
				if (!info->limits.cw)
				{
                    configure_movement(info, DIR_REVERSE, HOMING_SPEED);
					run_ctl(info);
					info->ctrl.homing_mode = HOME_WAIT_LIMIT;
				}
			}
		}

		else if (info->save.config.params.home.limit_switch == HOME_TO_INDEX)
		{
			info->limits.index = false;
			/* Zero delay counter, set the direction and velocity then go in that direction
			 * for the delay amount.  If at any time the index reset the encoder
			 * counts as it passes the index, the encoder service will take us to HOME_SUCCES
			 * */
			if (info->save.config.params.home.home_dir == HOME_CW_FIRST)
			{
				info->ctrl.homing_mode = HOME_CW_DIR_FIRST;
			}
			else if (info->save.config.params.home.home_dir == HOME_CCW_FIRST)
			{
				info->ctrl.homing_mode = HOME_CCW_DIR_FIRST;
			}
		}
		else
		{
			info->ctrl.homing_mode = HOME_IDLE;
			info->ctrl.mode = STOP;
		}
		break;

	case HOME_CW_DIR_FIRST:
		info->counter.homing_counter = 0;
        // (2025-01-07, sbenish):  This did not apply the reversal flag.
        // This seems like a bug.
        // If this was intended, inject "false" into the enable_reversal param.
        configure_movement(info, DIR_FORWARD, HOMING_SPEED);
		run_ctl(info);
		info->ctrl.homing_mode = HOME_CCW_DIR;
		break;

	case HOME_CCW_DIR:
		if (info->counter.homing_counter > HOMING_TIME)
		{
			info->counter.homing_counter = 0;
            configure_movement(info, DIR_REVERSE, HOMING_SPEED);
			run_ctl(info);
			info->ctrl.homing_mode = HOME_RETURN_CW;
		}
		break;

	case HOME_RETURN_CW:
		// if we haven't passed the index, go the opposite direction twice as long
		if (info->counter.homing_counter > HOMING_TIME * 2)
		{
			info->counter.homing_counter = 0;
            // (2025-01-07, sbenish):  This did not apply the reversal flag.
            // This seems like a bug.
            // If this was intended, inject "false" into the enable_reversal param.
            configure_movement(info, DIR_FORWARD, HOMING_SPEED);
			run_ctl(info);
			info->ctrl.homing_mode = HOME_INDEX_NOT_FOUND;
		}
		break;

	case HOME_CCW_DIR_FIRST:
		info->counter.homing_counter = 0;
        // (2025-01-07, sbenish):  This did not apply the reversal flag.
        // This seems like a bug.
        // If this was intended, inject "false" into the enable_reversal param.
        configure_movement(info, DIR_REVERSE, HOMING_SPEED);
		run_ctl(info);
		info->ctrl.homing_mode = HOME_CW_DIR;
		break;

	case HOME_CW_DIR:
		if (info->counter.homing_counter > HOMING_TIME)
		{
			info->counter.homing_counter = 0;
            // (2025-01-07, sbenish):  This did not apply the reversal flag.
            // This seems like a bug.
            // If this was intended, inject "false" into the enable_reversal param.
            configure_movement(info, DIR_FORWARD, HOMING_SPEED);
			run_ctl(info);
			info->ctrl.homing_mode = HOME_RETURN_CCW;
		}
		break;

	case HOME_RETURN_CCW:
		// if we haven't passed the index, go the opposite direction twice as long
		if (info->counter.homing_counter > HOMING_TIME * 2)
		{
			info->counter.homing_counter = 0;
            // (2025-01-07, sbenish):  This did not apply the reversal flag.
            // This seems like a bug.
            // If this was intended, inject "false" into the enable_reversal param.
            configure_movement(info, DIR_REVERSE, HOMING_SPEED);
			run_ctl(info);
			info->ctrl.homing_mode = HOME_INDEX_NOT_FOUND;
		}
		break;

	case HOME_INDEX_NOT_FOUND:
		// we did not pass the index, goto HOME_CLEAR to reset the encoder counts
		if (info->counter.homing_counter > HOMING_TIME)
		{
			info->ctrl.homing_mode = HOME_CLEAR;
			// set homed flags
			info->limits.homed = NOT_HOMED_STATUS;
			info->enc.homed = NOT_HOMED_STATUS;
		}
		break;

	case HOME_CLEAR:
		soft_stop_stepper(info->slot);
		/*Disable encoder zero on limit mode*/
		set_reg(C_SET_HOMING, info->slot, 0, (uint32_t) C_HOMING_DISABLE);

		info->ctrl.mode = STOP;
		break;

	case HOME_WAIT_LIMIT:
		/* check limit above will kick us out of here*/
		/* TODO need to add hard stop limit*/

		break;
	case HOME_EXIT_LIMIT:
		if ((info->save.config.params.home.home_dir == HOME_CW && !info->limits.cw)
			|| (info->save.config.params.home.home_dir == HOME_CCW && !info->limits.ccw))
		{
			/*Put encoder in zero on limit mode*/
			set_reg(C_SET_HOMING, info->slot, 0, (uint32_t) C_HOMING_ENABLE);
			info->ctrl.homing_mode = HOME_WAIT_LIMIT;
		}
	break;

	case HOME_DELAYED_START:

		// check method of homing
		if (info->save.config.params.home.limit_switch == HOME_TO_LIMIT
			|| info->save.config.params.home.limit_switch == HOME_TO_HARD_STOP)
		{
			sync_stepper_steps_to_encoder_counts(info);
            configure_movement(info,
                               info->save.config.params.home.home_dir == HOME_CW ? DIR_FORWARD : DIR_REVERSE,
                               HOMING_SPEED);
            run_ctl(info);
            info->ctrl.homing_mode = HOME_EXIT_LIMIT;
		}
	break;
	}

	// Run limits has side effects -> apply after short circuit
	if (info->ctrl.homing_mode != HOME_EXIT_LIMIT && check_run_limits(info)) 
	{
		if (Tst_bits(info->save.config.params.flags.flags, HAS_ENCODER))
		{
			/*Disable encoder in zero on limit mode*/
			set_reg(C_SET_HOMING, info->slot, 0, (uint32_t) C_HOMING_DISABLE);
			// set homed flags
			info->limits.homed = IS_HOMED_STATUS;
			info->enc.homed = IS_HOMED_STATUS;

		}
		else
			info->enc.enc_pos = 0;

		sync_stepper_steps_to_encoder_counts(info);

        // (sbenish) If this line is not set, "service_stepper" will execute logic to
        // reset the homing state to not homed if the offset distance executes.
        info->ctrl.homing_mode = HOME_IDLE;

		if (info->save.config.params.home.offset_distance != 0)
		{
			/* if counting is reversed multiple by -1*/
			if (info->save.config.params.flags.flags & ENCODER_REVERSED)
			{
				info->enc.enc_pos =
						(info->save.config.params.home.offset_distance);
			}
			else
			{
				info->enc.enc_pos =
						(info->save.config.params.home.offset_distance * -1);
			}

			set_encoder_position(info->slot,
					info->save.config.params.flags.flags, &info->enc,
					info->enc.enc_pos);
			sync_stepper_steps_to_encoder_counts(info);
			/*reverse the direction we homed to*/
            // (sbenish):  This is cleaner than using configure_movement().
			info->ctrl.cmd_dir = !info->ctrl.cmd_dir;
			info->counter.cmnd_pos = 0;
			info->pid.max_velocity = 1;
			info->ctrl.mode = PID;
			info->ctrl.mode = GOTO;
			return;
		}
	}

	info->counter.homing_counter++;
}

// CONTRACT:  cmd_dir has already been processed with reversals.
// MARK:  SPI Mutex Required
static void run_ctl(Stepper_info *info)
{
	if (check_run_limits(info))
		return;

	info->ctrl.cur_dir = info->ctrl.cmd_dir;
	info->ctrl.cur_vel = info->ctrl.cmd_vel;

	run_stepper(info->slot, info->ctrl.cmd_dir, info->ctrl.cmd_vel);
}

/**
 * @brief The PID controller operates in encoder counts.  It has a deadband of +-1 count
 * @param info
 */
// MARK:  SPI Mutex Required
static void pid_ctrl(Stepper_info *info)
{
	float input = info->enc.enc_pos;

	// Compute error
	float error = info->counter.cmnd_pos - input;

	debug_print("error1 %d",(int32_t) error);

	// for any rotational stage, check to see which way is the shortest except for the inverted
	// NND switcher.
	if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME))
	{
		if (abs(error) > (float) info->save.config.params.config.max_pos / 2)
		{
			if (info->counter.cmnd_pos < input)
			{
				error = (float)info->counter.cmnd_pos + ((float)info->save.config.params.config.max_pos - input);
			}
			else
			{
				error = (float)input + ((float)info->save.config.params.config.max_pos - (float)info->counter.cmnd_pos);
				error *= -1;
			}
		}
	}

#if 0
	/* Take the longest way.
	 * This is for the inverted NDD switcher where we need to make sure it travels almost 360 degrees.
	 * There are conditions where the magnet could be place where it will go the wrong way from position
	 * to the other position */
	else if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW)
	{
		if (abs(error) < (info->save.config.params.config.max_pos - input))
		{
			if (info->counter.cmnd_pos < input)
			{
				error = info->save.config.params.config.max_pos;
			}
			else
			{
				error = info->save.config.params.config.max_pos;
				error *= -1;
			}
		}
	}
#endif

	info->pid.error = (int32_t) error;

	// Compute integral
    // (sbenish) The float cast is critical to this operation.
	info->pid.iterm += (float)info->save.config.params.pid.Ki * error;
	if (info->pid.iterm > info->save.config.params.pid.imax)
		info->pid.iterm = info->save.config.params.pid.imax;
	else if (info->pid.iterm < (-1.0 * (float) info->save.config.params.pid.imax))
		info->pid.iterm = -1.0 * (float) info->save.config.params.pid.imax;

	// Compute differential on input
	float dinput = input - info->pid.lastin;
	// Keep track of some variables for next execution
	info->pid.lastin = input;

	// Compute PID output
    // (sbenish) The float cast is critical to this operation.
    float out = (float)info->save.config.params.pid.Kp * error
        + info->pid.iterm - (float)info->save.config.params.pid.Kd * dinput;

	/*Scale down the output to give higher granularity for higher resolution encoders*/
	out /= 10;

	// deadband
	if (abs(error) <= info->save.config.params.drive.deadband)
	{
		info->pid.kickout_count++;
		if (info->pid.kickout_count >= info->save.config.params.drive.kickout_time)
		{
			hard_stop_stepper(info->slot);
			if(info->ctrl.mode != HOMING)
				info->ctrl.mode = STOP;
			out = 0;
		}
	}
	else //if(abs(error) > info->save.config.params.drive.deadband)
	{
		info->pid.kickout_count = 0;
	}


	debug_print("error2 %d",(int32_t) error);
    const bool DIRECTION = out >= 0 ? DIR_FORWARD : DIR_REVERSE;

    // Out will be used for speed, so it needs to become non-negative.
    out = abs(out);

	// Set the speed
	// Limit the max speed, for synchronized motion max_velocity is computed automatically
	// otherwise it is set to 1
	float maxV = info->pid.max_velocity * stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_UNBOUND);
	float maxV_steps_per_sec = (1 << info->save.config.params.drive.step_mode)
			* 15.258789 * maxV;
	float out_speed_steps_per_sec = out * (1 << info->save.config.params.drive.step_mode) * 0.0149011;

	if (out_speed_steps_per_sec > maxV_steps_per_sec)
	{
		out = maxV_steps_per_sec
				/ (0.0149011 * (1 << info->save.config.params.drive.step_mode));
	}

    const uint16_t SPEED_VALUE = (uint16_t)std::clamp<float>(ceil(out / 1024), 0.0, 1 << (16 - 1));
    configure_movement(info, DIRECTION, SPEED_VALUE);

	// check both hard and soft limits
    // this needs to be done after the movement direction is calculated.
	if (check_run_limits(info))
	{
		return;
	}

	// save the new direction and velocity as current
	info->ctrl.cur_dir = info->ctrl.cmd_dir;
	info->ctrl.cur_vel = info->ctrl.cmd_vel;

	// dispatch commend to stepper drive
	run_stepper(info->slot, info->ctrl.cmd_dir, info->ctrl.cmd_vel);
}

/**
 * @brief If the stage move they set a flag stage_moved.  Here we check if
 * the stage has been in idle for STEPPER_IDLE_TIME.  If this time has elapsed then
 * we save the position.  The 25LC1024-I/SM external EEPROM has 1,000,000
 * write cycles.  If this event occurs  500 times a day, then 500 x 365 = 182500/year
 * Therefore 1,000,000 / 182500 = 5.5years
 */
// MARK:  SPI Mutex Required
static void position_save_check(Stepper_info *info)
{
	if (!info->stage_position_saved
			&& info->stage_moved_elapse_count > STEPPER_SAVE_POSITION_IDLE_TIME)
	{
		info->stage_position_saved = true;
		info->stage_moved_elapse_count = 0;

		get_el_pos_stepper(info);

		/*Copy the encoder position, and zero to the save structure*/
		info->save.store.enc_pos = info->enc.enc_pos;
		info->save.store.enc_zero = info->enc.enc_zero;

		/*Save position to eeprom page index 1 */
		eeprom_25LC1024_write(SLOT_EEPROM_ADDRESS(info->slot, 1),
				sizeof(info->save.store), (uint8_t*) &info->save.store);
	}
}

/**
 * @brief Check global slot flag to stop stepper
 * @param info
 */
static bool check_em_stop(Stepper_info *info)
{
	if (slots[info->slot].em_stop == true)
	{
		info->ctrl.mode = STOP;
		info->ctrl.selected_stage = false;
		slots[info->slot].em_stop = false;
		return 1;
	}
	return 0;
}

/*** @brief Check if we need to save position

 * @param info
 */
// MARK:  SPI Mutex Required
static void check_save_position(Stepper_info *info)
{
	// if encoder is absolute, don't save position
	if ((info->enc.type == ENCODER_TYPE_ABS_INDEX_LINEAR)
			|| (info->enc.type == ENCODER_TYPE_ABS_BISS_LINEAR)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
	{
		return;
	}

	bool moving = (bool) Tst_bits(info->ctrl.status_bits, STATUS_CW_MOVE) |
	(bool) Tst_bits(info->ctrl.status_bits, STATUS_CCW_MOVE);

	if (moving)
	{
		info->stage_moved_elapse_count = 0;
		info->stage_position_saved = false;
	}
	else
	{
		position_save_check(info);
		info->stage_moved_elapse_count++;
	}
}

/**
 * @brief Check for collision if we are moving and a threshold is greater than 0.  To disable
 * collision detection, set threshold to zero.  Counts/unit must
 * @param info
 */
static bool check_for_collision(Stepper_info *info)
{
	float stepper_in_encoder_counts = info->counter.step_pos_32bit
			/ info->save.config.params.config.counts_per_unit;

	if ((info->save.config.params.config.collision_threshold > 0))
	{
		int32_t delta_t = stepper_in_encoder_counts - info->enc.enc_pos;
		delta_t = abs(delta_t);

		// check for wrap around for rotational encoders
		if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
				|| (info->enc.type
						== ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
				|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
		{
			if (delta_t > (int32_t) info->save.config.params.config.max_pos / 2)
				delta_t = info->save.config.params.config.max_pos - delta_t;
		}

		info->collision = false;

		/*If collision in encoder counts*/
		if (delta_t > info->save.config.params.config.collision_threshold)
		{
			if ((info->ctrl.mode != HOMING) && (info->ctrl.mode != STOP)
					&& (info->ctrl.mode != IDLE))
			{
				if ((info->enc.type != ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
						&& (info->enc.type
								!= ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
						&& (info->enc.type
								!= ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
				{
					info->channel_enable = false;
				}

				info->limits.hard_stop = true;
				info->collision = true;
				info->ctrl.mode = STOP;
				return 1;
			}
			else if(info->ctrl.mode == HOMING)
			{
				if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
						|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
						|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
				{
					info->collision = true;
				}
			}
		}
	}
	return 0;
}

/**
 * Check to see is we are at a stored position to update the variable current_stored_position
 * that is sent to the USB out task to update LED's
 * @param info
 */
static void check_if_at_stored_position(Stepper_info *info)
{
	int32_t stored_pos;
	int32_t delta;

	for (int position_num = 0; position_num < STEPPER_NUM_OF_STORE_POS;
			++position_num)
	{
		stored_pos = info->save.store.stored_pos[position_num];

		/*If the current position is with in the deaband of the stored position then
		 * this is the current_stored_position.  If none are then set to
		 * STEPPER_CURRENT_STORE_POS_NONE*/
		delta = (abs(info->enc.enc_pos - stored_pos));
		if (delta <= info->save.store.stored_pos_deadband)
		{
			info->current_stored_position = position_num;
			break;
		}
		else
		{
			info->current_stored_position = CURRENT_STORE_POS_NONE;
		}
	}
}

static void configure_movement(Stepper_info* info, bool direction, uint16_t target_speed, bool enable_reversal) {
    info->ctrl.cmd_dir = direction;
    if (enable_reversal && (info->save.config.params.flags.flags & STEPPER_REVERSED))
    {
        info->ctrl.cmd_dir = !info->ctrl.cmd_dir;
    }

    /*need to multiple by 1024 to get from max_speed to speed sent from drive commands*/
    info->ctrl.cmd_vel = 1024 * std::min<uint32_t>(
        target_speed, stepper_get_speed_channel_value(info,
                                                     info->ctrl.current_speed_channel));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
// used to run the static function position_save_check from the USB parser to save the
// encoder set value from APT command MGMSG_MOT_SET_ENCCOUNTER
// MARK:  SPI Mutex Required
void write_flash(Stepper_info *info)
{
	position_save_check(info);
}

// MARK:  SPI Mutex Required
void magnetic_sensor_auto_homing_check(Stepper_info *info)
{
	if(info->ctrl.mode == HOMING)
		return;

	// 2147483647 is a blank position so just get out
	if (info->save.store.stored_pos[1] >= 2147483647)
	{
		return;
	}

	/*Used for magnetic encoder without motor, manual stage like the Veneto EPI turret*/
	if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL)
	{
		info->enc.homed = IS_HOMED_STATUS;
		info->ctrl.mode = STOP;
		return;
	}

	/*This is only for magnetic sensors with auto homing*/
	if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
	{
		/*EPI auto insertion homing*/
		/*If the EPI wheel was removed, set homed to false and stop the motor*/
		if ((info->enc.mlo) && (info->enc.homed == IS_HOMED_STATUS))
		{
			info->limits.homed = NOT_HOMED_STATUS;

			/*check if the wheel is actually out with the optical sensor, we could get
			 * a false reading if the magnetic sensor is a bit too far away from the magnet*/

			// turn on optical switch
			set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 1);

			/* In case the magnet is too far and mlo is set, to avoid it from not trying to home, we
			 * can use the opto switch to sense the turret.  If we sense the lip of the turret
			 * then the turret must be in, then we should home the stage.  Could be that we are in
			 * the slot so we can check again after the motor moves
			 * */
			if (read_limit(info->slot)) // if in slot or turret is out
			{
				info->enc.homed = NOT_HOMED_STATUS;
				info->enc.home_delay_cnt = 0;
				info->ctrl.mode = STOP;
			}
			else // we sense the turret
			{
				// turn off optical switch
				set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 0);
			}
		}
	}

	if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
			|| (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
	{
		if (info->enc.mag_good) // NOT_HOMED_STATUS
		{
			if ((info->enc.homed == NOT_HOMED_STATUS) || (info->enc.homed == HOMING_FAILED_STATUS))
			{
				if (info->enc.home_delay_cnt >= 200)
				{
					info->ctrl.mode = HOMING;
					info->ctrl.homing_mode = HOME_START;
					info->enc.homed = HOMING_STATUS;
				}
				info->enc.home_delay_cnt++;
			}
		}
	}
}

//else if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW)
//{
//
//}


bool stepper_store_not_default(Stepper_Store * const p_store)
{
    bool rt = p_store->el_pos == 0
           && p_store->enc_pos == 0
           && p_store->enc_zero == 0
           && p_store->stored_pos_deadband == 0;

    for (int i = 0; i < NUM_OF_STORED_POS && rt; ++i)
    {
        rt = rt && p_store->stored_pos[i] == 2147483647;
    }

    return !rt;
}


void stepper_goto_stored_pos(Stepper_info *info, uint8_t position_num)
{
	// 2147483647 is a blank position so just get out
	if (info->save.store.stored_pos[position_num] >= 2147483647)
	{
		info->current_stored_position = CURRENT_STORE_POS_NONE;
		return;
	}
	// get the position to goto from the array of stored values
	info->counter.cmnd_pos = info->save.store.stored_pos[position_num];
    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_ABSOLUTE;

	/*Use PID controller for goto*/
	if (Tst_bits(info->save.config.params.flags.flags, USE_PID))
	{
		info->pid.max_velocity = 1;
		info->ctrl.mode = PID;
	}
	else /*Used normal goto*/
	{
		if (info->ctrl.mode != IDLE)
			info->ctrl.goto_mode = GOTO_CLEAR;
		else
			info->ctrl.goto_mode = GOTO_START;

		info->ctrl.mode = GOTO;
	}
}

// MARK:  SPI Mutex Required
void service_stepper(Stepper_info *info, USB_Slave_Message *slave_message)
{
	stepper_log_ids log_id = STEPPER_NO_LOG;

	/*If the stage is disable, make sure it is stopped, if stopped just get out*/
	if (info->channel_enable == false)
	{
		if (info->ctrl.mode > IDLE)
			info->ctrl.mode = STOP;
		else
			return;
	}

	if(info->ctrl.mode != HOMING)
	{
		if (check_em_stop(info))
		{
			setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
					STEPPER_ERROR_TYPE, STEPPER_EMERGENCY_STOP, info->enc.enc_pos,
					0, 0);
		}
	}

	if (check_for_collision(info))
	{
		setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
				STEPPER_ERROR_TYPE, STEPPER_COLLISION_STOP, info->enc.enc_pos,
				0, 0);
	}

	check_if_at_stored_position(info);

	/*Stages with no motor, like the Veneto EPI encoded with NO motor*/
	if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL)
	{
		if (info->ctrl.mode > IDLE)
			info->ctrl.mode = STOP;
		else
			return;
	}

	// check if we need to save the position
	check_save_position(info);

	// if a limit is hit set this flag to dispatch a log event
	if (info->limits.limit_flag_for_logging == true)
	{
		setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
				STEPPER_LOG_TYPE, STEPPER_LIMIT_HIT, info->enc.enc_pos, 0, 0);
	}

	// Catch any mode changes away from the homing routine.
	if (info->ctrl.mode != HOMING && info->ctrl.homing_mode != HOME_IDLE)
	{
		// Halt the homing routine.
		if (info->save.config.params.encoder.encoder_type == ENCODER_TYPE_QUAD_LINEAR)
		{
			// Disable the homing detection register.
			set_reg(C_SET_HOMING, info->slot, 0, (uint32_t) C_HOMING_DISABLE);
		}
		/* Homing for epi turret on the inverted for the magnetic sensor type encoder*/
		else if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
		{
			// Disable the digital output register.
			set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 0);
		}

		info->enc.homed = NOT_HOMED_STATUS;
		info->limits.homed = NOT_HOMED_STATUS;
		info->ctrl.homing_mode = HOME_IDLE;
	}

	switch (info->ctrl.mode)
	{
	case STOP:
		hard_stop_stepper(info->slot);
		if (info->save.config.params.drive.kval_hold == 0)
			soft_hiz_stepper(info->slot);
		info->ctrl.cur_vel = 0;
		info->ctrl.homing_mode = HOME_IDLE;
		info->limits.hard_stop = false;

		// Only sync step_pos to enc_position if the collision_threshold is greater than
		// zero so that we can still verify the coefficients
		if (info->save.config.params.config.collision_threshold > 0)
			sync_stepper_steps_to_encoder_counts(info);

		// PID always active, no kickout
		if (Tst_bits(info->save.config.params.flags.flags, USE_PID)
				&& !Tst_bits(info->save.config.params.flags.flags, PID_KICKOUT)
				&& !info->collision)
		{
			info->pid.max_velocity = 1;
			info->ctrl.mode = PID;
		}
		else
		{
			/* only send the "stop log" to logger here because it will overrun the USB if we
			 * send if every time the PID jumps in and out of PID and STOP modes*/

			info->ctrl.mode = IDLE;
			log_id = STEPPER_CONTROLLER_STOP;
		}
		info->counter.cmnd_pos = info->enc.enc_pos;
        break;

	case IDLE:		break;

	case GOTO:
//		debug_print("goto %d",info->slot);
        // (sbenish) Since we have not supported non-pid stages for some time,
        // the code is being deprecated.
        pid_ctrl(info);
		log_id = STEPPER_CONTROLLER_PID;
		break;

	case RUN:
//		debug_print("run %d",info->slot);
        // (sbenish) Because ctrl.cmd_dir needs to be kept the same, we must do the reversal at the top-level.
        configure_movement(info, info->ctrl.cmd_dir,
                           stepper_get_speed_channel_value(
                            info, STEPPER_SPEED_CHANNEL_VELOCITY),
                           false);
		run_ctl(info);
		log_id = STEPPER_CONTROLLER_RUN;
		break;

	case JOG:
//		debug_print("jog %d",info->slot);
        // (sbenish) Since we have not supported non-pid stages for some time,
        // the code is being deprecated.
        pid_ctrl(info);
		log_id = STEPPER_CONTROLLER_PID;
		break;

	case PID:
//		debug_print("pid %d",info->slot);
		pid_ctrl(info);
		log_id = STEPPER_CONTROLLER_PID;
		break;

	case HOMING:
//		debug_print("home %d",info->slot);
		if (info->ctrl.homing_mode == HOME_START)
		{
			info->limits.hard_stop = false;
			/*Sync the stepper and encoder so if collision detection is used for hard stops*/
			sync_stepper_steps_to_encoder_counts(info);
		}

		if (info->save.config.params.encoder.encoder_type == ENCODER_TYPE_QUAD_LINEAR)
		{
			homing_ctl(info);
		}
		else if (info->enc.type == ENCODER_TYPE_ABS_BISS_LINEAR)
		{
			info->enc.enc_zero = 0; // zero out the virtual offset
		}
		/* Homing for epi turret on the inverted for the magnetic sensor type encoder*/
		else if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
		{
			homing_ctl_mag_epi_turret(info);
		}
		/* Homing for af switcher on the inverted for the magnetic sensor type encoder*/
		else if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION) ||
				(info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW))
		{
			homing_ctl_mag(info);
		}
		else
		{
			info->ctrl.mode = IDLE;
			info->ctrl.homing_mode = HOME_IDLE;
		}
		log_id = STEPPER_CONTROLLER_HOMING;
		break;

	}
	if (log_id != STEPPER_NO_LOG)
		setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
				STEPPER_LOG_TYPE, log_id, info->enc.enc_pos, 0, 0);
}
