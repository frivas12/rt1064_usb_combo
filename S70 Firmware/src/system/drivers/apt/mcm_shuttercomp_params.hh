/**
 * \file mcm_shuttercomp_params.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-11-21
 *
 * @copyright Copyright (c) 2024
 */
#pragma once

#include <cstdint>
#include <span>

#include "./apt-types.hh"
#include "./shutter-types.hh"
#include "apt.h"

#include "./mcm_shutter_params.hh"

namespace drivers::apt {

struct mcm_shuttercomp_params {
    // static constexpr uint16_t COMMAND_SET;
    // static constexpr uint16_t COMMAND_REQ;
    // static constexpr uint16_t COMMAND_GET;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    /**
    * \note All voltage fields are in units Volts.
    * \note All time fields are in units seconds.
    */
    struct payload_type {
        static constexpr std::size_t APT_SIZE = 36;

        float open_actuation_voltage;
        float open_actuation_time;
        float open_holdoff_time;
        float opened_hold_voltage;
        float close_actuation_voltage;
        float close_actuation_time;
        float closed_hold_voltage;
        float close_holdoff_time;
        channel_t channel;
        shutter_types::states_e power_up_state;
        shutter_types::trigger_modes_e trigger_mode;

        static payload_type deserialize(
            std::span<const std::byte, APT_SIZE> src);
        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}  // namespace drivers::apt

// EOF
