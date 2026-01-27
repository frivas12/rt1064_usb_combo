/**
 * @file hexapod.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "hexapod.h"
#include <pins.h>
#include "Debugging.h"
#include "synchronized_motion.h"
#include "hex_kins.h"
#include <ref.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/



/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static bool compare_pose(EmcPose a, EmcPose b);
/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool compare_pose(EmcPose a, EmcPose b)
{
	//there are errors when calculating floats.  need to use some error factor when comparing.
	if(fabsf(a.tran.x - b.tran.x) < FLT_MIN && fabsf(a.tran.y - b.tran.y) < FLT_MIN &&
			fabsf(a.tran.z - b.tran.z) < FLT_MIN && fabsf(a.a - b.a) < FLT_MIN &&
			fabsf(a.b - b.b) < FLT_MIN && fabsf(a.c - b.c) < FLT_MIN)
	{
		return true;  //return true if the two poses are identical
	}

	return false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void hexapod_init(void)
{
	for (uint8_t slot = 0; slot < 6; ++slot)
	{
			slots[slot].card_type = ST_HC_Stepper_type;
	}

	init_slots();

}

void hexapod_init_struct(Sm_info *info)
{
	configure_haldata();

	info->sm_data.pose_current.tran.x = 1;//-X_OFFS;
	info->sm_data.pose_current.tran.y = 0;//-Y_OFFS;
	info->sm_data.pose_current.tran.z = 100;//-Z_OFFS + STAGE_HEIGHT;
	info->sm_data.pose_current.a = .2;
	info->sm_data.pose_current.b = .1;
	info->sm_data.pose_current.c = 0;

	info->sm_data.pose_command = info->sm_data.pose_current;


    // (sbenish):  null_positions in sm_data is not aligned -> undefined behavior
    //             this big copy is why i avoid packed structures
    float32_t null_positions[6];
    for (int i = 0; i < 6; ++i)
    { null_positions[i] = info->sm_data.null_positions[i]; }
	kinematicsInverse(info->sm_data.pose_current, null_positions);
    for (int i = 0; i < 6; ++i)
    { info->sm_data.null_positions[i] = null_positions[i]; }

	float32_t temp_hex_struts[6];

	kinematicsInverse(info->sm_data.pose_command, temp_hex_struts);
	EmcPose temp_pose;
	temp_pose.tran.x = 0;
	temp_pose.tran.y = 0;
	temp_pose.tran.z = 100;
	temp_pose.a = 0;
	temp_pose.b = 0;
	temp_pose.c = 0;
	kinematicsForward(temp_hex_struts, &temp_pose);

	for(uint8_t c = 0; c < 6; c++){
		info->sm_data.struts[c] = temp_hex_struts[c] - info->sm_data.null_positions[c];
	}

}

void service_hexapod(Sm_info *info)
{
	float32_t temp_hex_struts[6];
	if(!compare_pose(info->sm_data.pose_current, info->sm_data.pose_command))
	{
		kinematicsInverse(info->sm_data.pose_command, temp_hex_struts);
		for(uint8_t c = 0; c < 6; c++)
		{
			info->sm_data.struts[c] = temp_hex_struts[c] - info->sm_data.null_positions[c];
		}


		info->sm_data.pose_current = info->sm_data.pose_command;

	}
}
