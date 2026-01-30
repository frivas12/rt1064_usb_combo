#pragma once

#include <stdint.h>

#define VERBOSE                         0   //Set to 0 if you want only error tests to output
#define USING_SECTOR_0                  0   //Set to 1 if sector 0 is being used, otherwise set the 0 to use sector 2 and 3.

#define GET_DEVICE_SAVE_SIZE(slot)      (slot + 4 + sizeof(LUT_EEPROM_CRC_t) + sizeof(device_config_t))
#define GET_DEVICE_COUNT(slot)          (slot + 2)

void cycler(char* p_data, uint32_t count);