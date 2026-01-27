/**
 * @file synchronized_motion.c
 *
 * @brief If the slot use_synchronized_moition flag is set then data is passed to the sm task rather
 * than the slot card directly.  Below is a diagram of the stepper_synchronized_motion_task task.
 *
 *@dot
 digraph G {
 "stepper_synchronized_motion_task()"->"create structures and queues"->"task loop";
 "task loop"->"read_slot_queues(sm_rx)";
 "read_slot_queues(sm_rx)"->"send_slot_queue()";
 "send_slot_queue()"->"service_pc_usb"
 "service_pc_usb"->"service_sm_joystick";
 "service_smr_joystick"->"do_work()";
 "do_work()"->"vTaskDelayUntil()";
 "vTaskDelayUntil()"->"task loop";
 }
 * @enddot
 */

#include <asf.h>
#include <pins.h>
#include "Debugging.h"
#include <apt_parse.h>
#include "synchronized_motion.h"
#include "virtual_motion.h"
#include "hid.h"
#include "usb_device.h"
#include "hexapod.h"
#include "hex_kins.h"
#include <arm_math.h>
#include "encoder.h"
#include "apt.h"
#include "25lc1024.h"


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static bool service_sm_joystick(Sm_info *info, USB_Host_Rx_Message *host_message);
static void create_queues(void);
static void read_slot_queues(stepper_sm_Rx_data *sm_rx);
static void send_slot_queue(uint8_t slot, stepper_sm_Tx_data *sm_tx);
static void set_default_params(Sm_info *info);
static void read_eeprom_data(Sm_info *info);
static void write_eeprom_data(Sm_info *info);
static bool parse(Sm_info *info, USB_Slave_Message *slave_message);
static bool service_pc_usb(Sm_info *info, USB_Slave_Message *slave_message);
static bool service_ftdi_usb(Sm_info *info, USB_Slave_Message *slave_message);
static bool check_slot_using_sm(Sm_info *info, uint8_t slot_to_check);
static void calculate_motion(Sm_info *info);
static void get_trans_points(Sm_info *info);
/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool service_sm_joystick(Sm_info *info, USB_Host_Rx_Message *host_message)
{
	bool new_data = false;
	if (xUSB_Host_Rx_Queue[SLOT_SM_QUEUE_INDEX] != NULL)
	{
		while ( xQueueReceive(xUSB_Host_Rx_Queue[SLOT_SM_QUEUE_INDEX], host_message,
				0) == pdPASS)
		{
			switch(host_message->usage_page)
			{
			case HID_USAGE_PAGE_GENERIC_DESKTOP:

				switch (host_message->usage_id)
				{
				case CTL_AXIS:
				case HID_USAGE_X:
				case HID_USAGE_Y:
				case HID_USAGE_Z:
				case HID_USAGE_RX:
				case HID_USAGE_RY:
				case HID_USAGE_RZ:
				case HID_USAGE_WHEEL:
					if (info->save.params.config.sm_type > DISABLED_SM_TYPE)
					{
						if (host_message->destination_virtual == (1 << SLOT_1))
						{
							info->sm_data.pose_command.tran.x += host_message->control_data / 10;
							info->sm_data.latest_joystick_update[SLOT_1] =  host_message->control_data;
						}
						else if (host_message->destination_virtual == (1 << SLOT_2))
						{
							info->sm_data.pose_command.tran.y += host_message->control_data / 10;
							info->sm_data.latest_joystick_update[SLOT_2] =  host_message->control_data;
						}
						else if (host_message->destination_virtual == (1 << SLOT_3))
						{
							info->sm_data.pose_command.tran.z += host_message->control_data / 20;
							info->sm_data.latest_joystick_update[SLOT_3] =  host_message->control_data;
						}
						else if (host_message->destination_virtual == (1 << SLOT_4))
						{
							info->sm_data.pose_command.a -= host_message->control_data / 30;
							info->sm_data.latest_joystick_update[SLOT_4] =  host_message->control_data;
						}
						else if (host_message->destination_virtual == (1 << SLOT_5))
						{
							info->sm_data.pose_command.b -= host_message->control_data / 30;
							info->sm_data.latest_joystick_update[SLOT_5] =  host_message->control_data;
						}
						else if (host_message->destination_virtual == (1 << SLOT_6))
						{
							info->sm_data.pose_command.c -= host_message->control_data / 30;
							info->sm_data.latest_joystick_update[SLOT_6] =  host_message->control_data;
						}
					}
				break;
				}
			break;
			}
			new_data = true;
		}
	}
	return new_data;

}

static void create_queues(void)
{
	for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
	{
		/* Create a queue for receiving data from the slot task. */
		xStepper_SM_Rx_Queue[slot] = xQueueCreate(1, sizeof(xStepper_SM_Rx_Queue));
		if (xStepper_SM_Rx_Queue[slot] == NULL)
		{
			debug_print(
					"ERROR: The Slot %d synchronized_moition Rx queue could not be created./r/n",
					slot);
		}

		/* Create a queue for sending data to the slot task. */
		xStepper_SM_Tx_Queue[slot] = xQueueCreate(1, sizeof(stepper_sm_Tx_data));
		if (xStepper_SM_Tx_Queue[slot] == NULL)
		{
			debug_print(
					"ERROR: The Slot %d synchronized_moition Tx queue could not be created./r/n",
					slot);
		}
	}
}

static void read_slot_queues(stepper_sm_Rx_data *sm_rx)
{
	/*Check if new queue data from the stepper slots is available*/
	for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS; slot++)
	{
		if (xStepper_SM_Tx_Queue[slot] != NULL)
		{
			if ( xQueueReceive(xStepper_SM_Rx_Queue[slot],	&sm_rx[slot], 0) == pdPASS)
			{
				//debug_print("S %d = %d /r/n", slot, sm_rx[slot].position);
			}
		}
	}
}

static void send_slot_queue(uint8_t slot, stepper_sm_Tx_data *sm_tx)
{
	/*Send the queue to the stepper task but don't wait*/
	xQueueSend(xStepper_SM_Tx_Queue[slot], (void * ) &sm_tx[slot], 0);

}

static void set_default_params(Sm_info *info)
{
	info->save.eprom_data_type = SM_SAVE_EEPROM_START;
	info->save.params.config.sm_type = DISABLED_SM_TYPE;
	info->save.params.config.x_slot = SLOT_1;
	info->save.params.config.y_slot = SLOT_2;
	info->save.params.config.z_slot = SLOT_3;
	info->save.params.config.VirtualMotionDelta = 100000;

	sm_clear_rotation_matrix(info);
}

// MARK:  SPI Mutex Required
static void read_eeprom_data(Sm_info *info)
{
	uint8_t length_params = sizeof(info->save);

	/* Calculate the address*/
	uint32_t params_address = SM_SAVE_EEPROM_START;

	/* Read all the params data into params structure*/
	eeprom_25LC1024_read(params_address, length_params, (uint8_t*) &info->save);

	/*Validate the data read from eeprom by looking at the first byte which holds data type*/
	if (info->save.eprom_data_type != SM_DATA_TYPE)
	{
		set_default_params(info);
		write_eeprom_data(info);
	}
}

// MARK:  SPI Mutex Required
static void write_eeprom_data(Sm_info *info)
{
	info->save.eprom_data_type = SM_DATA_TYPE;

	/*Save the new values to EEPROM*/
	eeprom_25LC1024_write(SM_SAVE_EEPROM_START, sizeof(info->save),
			(uint8_t*) &info->save);
}

static bool parse(Sm_info *info, USB_Slave_Message *slave_message)
{
	uint8_t response_buffer[100] =	{ 0 };
	uint8_t length = 0;
	bool need_to_reply = false;
	EmcPose temp_pose;
	float32_t temp_hex_struts[6];

	switch (slave_message->ucMessageID)
	{

	case MGMSG_MCM_SET_HEX_POSE: /*0x40A0*/
		memcpy(&info->sm_data.pose_command.tran.x,
				&slave_message->extended_data_buf[2], 4);
		memcpy(&info->sm_data.pose_command.tran.y,
				&slave_message->extended_data_buf[6], 4);
		memcpy(&info->sm_data.pose_command.tran.z,
				&slave_message->extended_data_buf[10], 4);
		memcpy(&info->sm_data.pose_command.a, &slave_message->extended_data_buf[14],
				4);
		//		memcpy(&info->sm_data.pose_command.b, &slave_message->buf[18], 4);
//			memcpy(&info->sm_data.pose_command.c, &slave_message->buf[22], 4);
		return true;
		break;

	case MGMSG_MCM_REQ_HEX_POSE: /*0x40A1*/


		temp_hex_struts[0] = (float32_t)(info->sm_rx[0].position * info->sm_rx[0].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[1] = (float32_t)(info->sm_rx[1].position * info->sm_rx[1].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[2] = (float32_t)(info->sm_rx[2].position * info->sm_rx[2].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[3] = (float32_t)(info->sm_rx[3].position * info->sm_rx[3].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[4] = (float32_t)(info->sm_rx[4].position * info->sm_rx[4].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[5] = (float32_t)(info->sm_rx[5].position * info->sm_rx[5].nm_per_count) / (1000.0 * 1000.0);
		temp_hex_struts[0] += info->sm_data.null_positions[0];
		temp_hex_struts[1] += info->sm_data.null_positions[1];
		temp_hex_struts[2] += info->sm_data.null_positions[2];
		temp_hex_struts[3] += info->sm_data.null_positions[3];
		temp_hex_struts[4] += info->sm_data.null_positions[4];
		temp_hex_struts[5] += info->sm_data.null_positions[5];
		temp_pose.tran.x = info->sm_data.pose_current.tran.x;
		temp_pose.tran.y = info->sm_data.pose_current.tran.y;
		temp_pose.tran.z = info->sm_data.pose_current.tran.z;
		temp_pose.a = info->sm_data.pose_current.a;
		temp_pose.b = info->sm_data.pose_current.b;
		temp_pose.c = info->sm_data.pose_current.c;
		kinematicsForward(temp_hex_struts, &temp_pose);

		// Header
		response_buffer[0] = MGMSG_MCM_GET_HEX_POSE;
		response_buffer[1] = MGMSG_MCM_GET_HEX_POSE >> 8;
		response_buffer[2] = 26;
		response_buffer[3] = 0x00;
		response_buffer[4] = HOST_ID | 0x80; 	// destination
		response_buffer[5] = MOTHERBOARD_ID;	// source
		response_buffer[6] = 0x00;  //channel ID is 0x0000 for hexapod
		response_buffer[7] = 0x00;
		memcpy(&response_buffer[8],
				(unsigned char*) (&temp_pose.tran.x), 4);
		memcpy(&response_buffer[12],
				(unsigned char*) (&temp_pose.tran.y), 4);
		memcpy(&response_buffer[16],
				(unsigned char*) (&temp_pose.tran.z), 4);
		memcpy(&response_buffer[20],
				(unsigned char*) (&temp_pose.a), 4);
		memcpy(&response_buffer[24],
				(unsigned char*) (&temp_pose.b), 4);
		memcpy(&response_buffer[28],
				(unsigned char*) (&temp_pose.c), 4);
//			fill_buff(&response_buffer[32], 4, slot[0].stepper->status_bits);
//			fill_buff(&response_buffer[36], 4, slot[1].stepper->status_bits);
//			fill_buff(&response_buffer[40], 4, slot[2].stepper->status_bits);
//			fill_buff(&response_buffer[44], 4, slot[3].stepper->status_bits);
//			fill_buff(&response_buffer[48], 4, slot[4].stepper->status_bits);
//			fill_buff(&response_buffer[52], 4, slot[5].stepper->status_bits);
		need_to_reply = true;
		length = 32;
		break;

	case MGMSG_MCM_SET_SYNC_MOTION_PARAM:
		memcpy(&info->save.params, &slave_message->extended_data_buf[2],
				sizeof(Sm_config_params));
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		write_eeprom_data(info);
		xSemaphoreGive(xSPI_Semaphore);
		break;

	case MGMSG_MCM_REQ_SYNC_MOTION_PARAM:
		// Header
		response_buffer[0] = MGMSG_MCM_GET_SYNC_MOTION_PARAM;
		response_buffer[1] = MGMSG_MCM_GET_SYNC_MOTION_PARAM >> 8;
		response_buffer[2] = 18;
		response_buffer[3] = 0x00;
		response_buffer[4] = HOST_ID | 0x80; 	// destination
		response_buffer[5] = MOTHERBOARD_ID;	// source
		response_buffer[6] = 0x00;  //channel ID is 0x0000 for sm motion
		response_buffer[7] = 0x00;

		memcpy(&response_buffer[8], (unsigned char*) (&info->save.params.config),
				sizeof(Sm_config_params));

		need_to_reply = true;
		length = 6 + 18;
		break;

	case MGMSG_MCM_SET_SYNC_MOTION_POINT:

		if ((info->save.params.config.sm_type == VIRTUAL_AXIS_SM_TYPE) ||
				(info->save.params.config.sm_type == VIRTUAL_PLANE_SM_TYPE))
		{
			/*For a axis we only need 2 points and for a plane translation we need 3 points.
			 * These points can be input in any order.  They must be in units of nm.  The points
			 * come in as encoder counts so we must convert to nm.  nm_per_count * enc_counts*/

			if(slave_message->param1 == 0)	/*Clear points*/
			{
				/* P1*/
				info->PointsMatrix.x.x = 1;
				info->PointsMatrix.x.y = 1;
				info->PointsMatrix.x.z = 1;

				/* P2*/
				info->PointsMatrix.y.x = 1;
				info->PointsMatrix.y.y = 1;
				info->PointsMatrix.y.z = 1;

				/* P1*/
				info->PointsMatrix.z.x = 1;
				info->PointsMatrix.z.y = 1;
				info->PointsMatrix.z.z = 1;
			}
			else if(slave_message->param1 == 1)	/*First point*/
			{
				/*x*/
				info->PointsMatrix.x.x = info->sm_rx[info->save.params.config.x_slot].nm_per_count
						* info->sm_rx[info->save.params.config.x_slot].position;
				/*y*/
				info->PointsMatrix.x.y = info->sm_rx[info->save.params.config.y_slot].nm_per_count
						* info->sm_rx[info->save.params.config.y_slot].position;
				/*z*/
				info->PointsMatrix.x.z = info->sm_rx[info->save.params.config.z_slot].nm_per_count
						* info->sm_rx[info->save.params.config.z_slot].position;
			}
			else if(slave_message->param1 == 2)	/*Second point*/
			{
				/*x*/
				info->PointsMatrix.y.x = info->sm_rx[info->save.params.config.x_slot].nm_per_count
						* info->sm_rx[info->save.params.config.x_slot].position;
				/*y*/
				info->PointsMatrix.y.y = info->sm_rx[info->save.params.config.y_slot].nm_per_count
						* info->sm_rx[info->save.params.config.y_slot].position;
				/*z*/
				info->PointsMatrix.y.z = info->sm_rx[info->save.params.config.z_slot].nm_per_count
						* info->sm_rx[info->save.params.config.z_slot].position;
			}
			else if(slave_message->param1 == 3)	/*Third point*/
			{
				/*x*/
				info->PointsMatrix.z.x = info->sm_rx[info->save.params.config.x_slot].nm_per_count
						* info->sm_rx[info->save.params.config.x_slot].position;
				/*y*/
				info->PointsMatrix.z.y = info->sm_rx[info->save.params.config.y_slot].nm_per_count
						* info->sm_rx[info->save.params.config.y_slot].position;
				/*z*/
				info->PointsMatrix.z.z = info->sm_rx[info->save.params.config.z_slot].nm_per_count
						* info->sm_rx[info->save.params.config.z_slot].position;
			}
			sm_get_rotation_matrix(info);

			/*We need to update the translation points from the actual stepper positions, this adjust the
			 * struts for the rotation matrix*/
			get_trans_points(info);

			xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
			write_eeprom_data(info);
			xSemaphoreGive(xSPI_Semaphore);
		}
		break;

	default:
		usb_slave_command_not_supported(slave_message);
		break;
	}

	if (need_to_reply)
	{
		xSemaphoreTake(xUSB_Slave_TxSemaphore, portMAX_DELAY);
		udi_cdc_write_buf((uint8_t *) response_buffer, length);
		xSemaphoreGive(xUSB_Slave_TxSemaphore);
	}
	return false;
}

static bool service_pc_usb(Sm_info *info, USB_Slave_Message *slave_message)
{
	/*Check if new USB (slave) data to service from PC*/
	if (xUSB_Slave_Rx_Queue[SLOT_SM_QUEUE_INDEX] != NULL)
	{
		/*If we receive data from the PC USB, parse it*/
		if ( xQueueReceive(xUSB_Slave_Rx_Queue[SLOT_SM_QUEUE_INDEX],
				slave_message, 0) == pdPASS)
		{
			if (parse(info, slave_message))
				return true;
		}
	}
	return false;
}

static bool service_ftdi_usb(Sm_info *info, USB_Slave_Message *slave_message)
{
	/*Check if new USB (slave) data to service from PC*/
	if (xUSB_Slave_Rx_Queue[SLOT_SM_QUEUE_INDEX] != NULL)
	{
		/*If we receive data from the PC USB, parse it*/
		if ( xQueueReceive(xUSB_Slave_Rx_Queue[SLOT_SM_QUEUE_INDEX],
				slave_message, 0) == pdPASS)
		{
			if (parse(info, slave_message))
				return true;
		}
	}
	return false;
}

static bool check_slot_using_sm(Sm_info *info, uint8_t slot_to_check)
{
	if (info->save.params.config.sm_type == HEXAPOD_SM_TYPE)
	{
		if (slot_to_check <= SLOT_6)
			return true;
	}
	else if (info->save.params.config.sm_type == VIRTUAL_AXIS_SM_TYPE)
	{
		if ((slot_to_check == info->save.params.config.x_slot)
				|| (slot_to_check == info->save.params.config.y_slot))
		{
			return true;
		}
	}
	else if (info->save.params.config.sm_type == VIRTUAL_PLANE_SM_TYPE)
	{
		if ((slot_to_check == info->save.params.config.x_slot)
				|| (slot_to_check == info->save.params.config.y_slot)
				|| (slot_to_check == info->save.params.config.z_slot))
		{
			return true;
		}
	}
	return false;
}

static void calculate_motion(Sm_info *info)
{
	int32_t delta_counts;
	float delta_steps;
	int32_t slot_position_counts[6];
	float velocity_steps_per_second;
	float time_to_move[6];
	uint8_t max_time_to_move = 0;
	float max_velocity_pct[6];

	//calculate times to complete motion on the axis that have synchronized motion enabled
	for (uint8_t axis = 0; axis < MAX_NUM_SM_AXIS; axis++)
	{
		if (check_slot_using_sm(info, axis))
		{
			/*Calculate the new position from the rotation matrix*/
			slot_position_counts[axis] = (int32_t) (/*info->save.params.config.VirtualMotionDelta*/ 1000 * 1000
					* info->sm_data.struts[axis] / info->sm_rx[axis].nm_per_count);

			/*Calculate the distance to travel in encoder counts*/
			delta_counts = abs(info->sm_rx[axis].position - slot_position_counts[axis]);
			//encoder counts * counts per unit = steps

			/*Calculate the distance  to travel in steps*/
			delta_steps = (float) delta_counts * info->sm_rx[axis].counts_per_unit;

			/*Calculate the velocity*/
			velocity_steps_per_second = 128 * 15.258789 * (float) (info->sm_rx[axis].max_speed);

			/*Calculate the time need for the move*/
			time_to_move[axis] = delta_steps / velocity_steps_per_second;

			/*Find the axis which is going to take the longest time so we can use it below*/
			if (time_to_move[axis] > time_to_move[max_time_to_move])
			{
#if DEBUG_PID_OZONE
//				sm_axis_longest_time = axis;
#endif
				max_time_to_move = axis;
			}
		}
	}

	for (uint8_t axis = 0; axis < MAX_NUM_SM_AXIS; axis++)
	{
		if (check_slot_using_sm(info, axis))
		{
			//calculate ratio of max V for each axis, the fast axis moves at 90% of it's max total V
			max_velocity_pct[axis] = 0.9 * time_to_move[axis]	/ time_to_move[max_time_to_move];
		}
	}
#if 1
	//if stages are not already in correct location, send updated target position
	for (uint8_t c = 0; c < MAX_NUM_SM_AXIS; c++)
	{
		if (check_slot_using_sm(info, c))
		{
			if (info->sm_rx[c].position != slot_position_counts[c])
			{
				info->sm_tx[c].target = slot_position_counts[c];
				info->sm_tx[c].max_velocity = max_velocity_pct[c];
				send_slot_queue(c, info->sm_tx);
			}
		}
	}
#endif
}

static void get_trans_points(Sm_info *info)
{
	PmCartesian move_delta;
	PmCartesian out;

	/*Get the inverse rotation matrix*/
	pmRpyMatInvert(&info->save.params.RMatrix, &info->R_inverseMatrix);

	move_delta.x = ((float) info->sm_rx[info->save.params.config.x_slot].position
			* info->sm_rx[info->save.params.config.x_slot].nm_per_count)
			/ info->save.params.config.VirtualMotionDelta;

	move_delta.y = ((float) info->sm_rx[info->save.params.config.y_slot].position
			* info->sm_rx[info->save.params.config.y_slot].nm_per_count)
			/ info->save.params.config.VirtualMotionDelta;

	move_delta.z = ((float) info->sm_rx[info->save.params.config.z_slot].position
			* info->sm_rx[info->save.params.config.z_slot].nm_per_count)
			/ info->save.params.config.VirtualMotionDelta;

	pmMatCartMult(info->R_inverseMatrix, move_delta, &out);


	info->sm_data.pose_command.tran.x = out.x;
	info->sm_data.pose_command.tran.y = out.y;
	info->sm_data.pose_command.tran.z = out.z;
}

/****************************************************************************
 * Task
 ****************************************************************************/
/**
 * @brief This is the stepper synchronized motion task.
 *
 * @param sm_type	: Use this type to direct where data gets routed to
 * for different application types
 */
static void stepper_synchronized_motion_task(void *pvParameters)
{
	/* Create the structure for holding the synchronized motion data*/
	Sm_info info;

	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
	read_eeprom_data(&info);
	xSemaphoreGive(xSPI_Semaphore);

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = STEPPER_SM_TASK_RATE;

	/* Initialize the xLastWakeTime variable with the current time.*/
	xLastWakeTime = xTaskGetTickCount();

	create_queues();

	/* Create the structure for holding the USB data received from the USB slave task*/
	USB_Slave_Message slave_message;

	/* Create the structure for holding the USB data received from the USB host task*/
	USB_Host_Rx_Message host_message;

	/* Create the structure for holding each slot synchronized motion info*/
//	stepper_sm_Rx_data sm_rx[NUMBER_OF_BOARD_SLOTS];
//	stepper_sm_Tx_data sm_tx[NUMBER_OF_BOARD_SLOTS];

	if(board_type == HEXAPOD_BOARD_REV_001)
	{
		info.save.params.config.sm_type = HEXAPOD_SM_TYPE;
	}

	if (info.save.params.config.sm_type == HEXAPOD_SM_TYPE)
	{
		hexapod_init_struct(&info);
	}
	else if ((info.save.params.config.sm_type == VIRTUAL_AXIS_SM_TYPE)
			|| (info.save.params.config.sm_type == VIRTUAL_PLANE_SM_TYPE))
	{
		/* Wait a bit then get the position for each stepper.*/
		vTaskDelayUntil(&xLastWakeTime, xFrequency*10);
		read_slot_queues(info.sm_rx);
		get_trans_points(&info);
	}

	bool update = false;
	static bool first = true;

	for (;;)
	{
		read_slot_queues(info.sm_rx);

		// TODO need to handle zero a stage
		if (service_pc_usb(&info, &slave_message))
		{
			update = true;
		}

		if (service_ftdi_usb(&info, &slave_message))
		{
			update = true;
		}

		if (service_sm_joystick(&info, &host_message))
		{
			update = true;
		}

		if (update)
		{
			if (info.save.params.config.sm_type == DISABLED_SM_TYPE)
			{
				goto skip;
			}
			else if(info.save.params.config.sm_type == HEXAPOD_SM_TYPE)
			{
				service_hexapod(&info);
			}
			else //VIRTUAL_AXIS_SM_TYPE or VIRTUAL_PLANE_SM_TYPE
			{
				if(first)
				{
					first = false;
					get_trans_points(&info);
				}
				service_virtual_motion(&info);
			}

			calculate_motion(&info);
			update = false;
		}
		else
		{
			first = true;
		}

		skip:

		/* Wait for the next cycle.*/
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stepper_synchronized_motion_init(void)
{
	/* Create task to for the synchronized motion */
	if (xTaskCreate(stepper_synchronized_motion_task, "step_sm",
	TASK_STEPPER_SM_STACK_SIZE,
	NULL,
	TASK_STEPPER_SM_STACK_PRIORITY, &xStepperSMHandle) != pdPASS)
	{
		error_print("Failed to create stepper synchronized motion task\r\n");
	}
}

