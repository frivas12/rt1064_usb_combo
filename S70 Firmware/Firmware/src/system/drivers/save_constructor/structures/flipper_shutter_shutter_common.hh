/**
 * \file flipper_shutter_shutter_common.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-21
 */
#pragma once

#include <span>

#include "../structure_id.hh"
#include "shutter-types.hh"

namespace drivers::save_constructor {
struct flipper_shutter_shutter_common {
    static constexpr Struct_ID ID = Struct_ID::FLIPPER_SHUTTER_SHUTTER_COMMON;
    static constexpr std::size_t SIZE = 1;

    apt::shutter_types::states_e power_up_state;
    apt::shutter_types::trigger_modes_e default_trigger_mode;

    static flipper_shutter_shutter_common
    deserialize(std::span<const std::byte, SIZE> src);
    void serialize(std::span<std::byte, SIZE> dest) const;
};
} // namespace drivers::save_constructor

// EOF
