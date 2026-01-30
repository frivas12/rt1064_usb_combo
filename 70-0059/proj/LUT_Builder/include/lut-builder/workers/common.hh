#pragma once

#define DONT_USE_ARM_MATH

#include <vector>
#include "defs.h"
#include "tinyxml2.h"
#include "lut_manager.hh"

namespace builders
{
    CRC8_t CRC(const char * p_data, const uint32_t length);

    std::vector<char> serialize_device(const lut_manager::device_lut_key_t signature, const char * p_config_list, const int config_list_size);

    std::vector<char> serialize_config(const lut_manager::config_lut_key_t signature, const char * p_config, const int config_size);

    lut_manager::device_lut_key_t parse_device_signature(tinyxml2::XMLElement * const p_signature);

    lut_manager::config_lut_key_t parse_config_signature(tinyxml2::XMLElement * const p_signature);
};
