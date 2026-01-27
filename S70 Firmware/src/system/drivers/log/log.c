/**
 * @file log.c
 *
 * @brief Functions for ???
 *
 */

#include "log.h"

#include "apt.h"
#include <asf.h>
#include <string.h>
#include "Debugging.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

uint8_t slot_prev_type[NUMBER_OF_BOARD_SLOTS + 5];
uint8_t slot_prev_id[NUMBER_OF_BOARD_SLOTS + 5];

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
// static void send_log(Log log, uint8_t slot, USB_Slave_Message *slave_message);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// static void send_log(Log log, uint8_t slot, USB_Slave_Message *slave_message)
// {
// 	uint8_t response_buffer[6 + sizeof(Log)] =
// 	{ 0 };

// 	/* Header */
// 	response_buffer[0] = MGMSG_MCM_POST_LOG;
// 	response_buffer[1] = MGMSG_MCM_POST_LOG >> 8;
// 	response_buffer[2] = sizeof(Log);
// 	response_buffer[3] = 0x00;
// 	response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
// 	response_buffer[5] = SLOT_1_ID + slot; /* source*/

// 	/*error type*/
// //	response_buffer[6] = log.type;
// //	response_buffer[7] = log.id;
// 	memcpy(&response_buffer[6], &log, sizeof(Log));

// 	xSemaphoreTake(slave_message->xUSB_Slave_TxSemaphore, portMAX_DELAY);
// 	slave_message->write((uint8_t*) response_buffer, 6 + sizeof(Log));
// 	xSemaphoreGive(slave_message->xUSB_Slave_TxSemaphore);
// }

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void setup_and_send_log(USB_Slave_Message *slave_message, uint8_t slot,
bool no_repeat, uint8_t type, uint8_t id, uint32_t var1, uint32_t var2,
		uint32_t var3)
{
#if ENABLE_SYSTEM_LOGGING
	if (board.save.enable_log)
	{
		Log log;

		// wait for the PC to start the logger
		if (board.send_log_ready == false)
			return;

//		log.slot = slot;
		log.type = type;
		log.id = id;
		log.var1 = var1;
		log.var2 = var2;
		log.var3 = var3;

		// if this command is the same as the previous, don't send it
		// We need to change the slot number so that i

		if (slot == SYNC_MOTION_ID)
			slot = NUMBER_OF_BOARD_SLOTS + 1;
		else if (slot == MOTHERBOARD_ID)
			slot = NUMBER_OF_BOARD_SLOTS + 2;
		else if (slot == MOTHERBOARD_ID_STANDALONE)
			slot = NUMBER_OF_BOARD_SLOTS + 3;

		if (no_repeat)
		{
			if (type == slot_prev_type[slot])
				if (id == slot_prev_id[slot])
					return;
		}
		error_print("no_repeat %d type %d id %d\n", no_repeat, type, id);

		// save the sent command to compare next time
		slot_prev_type[slot] = type;
		slot_prev_id[slot] = id;

		send_log(log, slot, slave_message);
	}
#endif
}

