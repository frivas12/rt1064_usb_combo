/**
 * \file flipper-shutter-thread.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-08-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include "defs.hh"
#include "slot_types.h"
#include "slot_nums.h"

#include "FreeRTOS.h"
#include "task.h"



namespace cards::flipper_shutter
{

/**
 * Spawns a new thread for the flipper shutter on the given slot.
 * Also updates the handle in sys_task for the slot.
 * \warning Slot value, ownership, and card compatability are tested before the
 *          the task is spawned.
 * \param[in]       slot The hardware slot that the card is installed in.
 * \return TaskHandle_t A handle to the task created or NULL if
 *                      a state-check failed.
 */
TaskHandle_t spawn_thread(slot_nums slot);

}

// EOF
