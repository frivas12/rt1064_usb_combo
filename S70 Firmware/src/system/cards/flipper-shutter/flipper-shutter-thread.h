/**
 * \file flipper-shutter-thread.h
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-08-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef SRC_CARDS_FLIPPER_SHUTTER_FLIPPER_SHUTTER_THREAD_H_
#define SRC_CARDS_FLIPPER_SHUTTER_FLIPPER_SHUTTER_THREAD_H_

#include "slot_nums.h"
#include "slot_types.h"
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void flipper_shutter_init(slot_nums slot, slot_types card_type);

#ifdef __cplusplus
}
#endif

#endif

// EOF
