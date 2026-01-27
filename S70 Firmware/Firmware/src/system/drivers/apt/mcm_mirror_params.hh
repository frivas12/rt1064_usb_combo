/**
 * \file mcm_mirror_params.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 06-06-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <cstdint>
#include <span>

#include "./apt-command.hh"
#include "./apt-types.hh"
#include "./mirror-types.hh"

#include "apt.h"


namespace drivers::apt
{

struct mcm_mirror_params {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_MIRROR_PARAMS;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_MIRROR_PARAMS;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_MIRROR_PARAMS;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 6;

        mirror_types::states_e initial_state;
        mirror_types::modes_e mode;
        channel_t channel;
        uint8_t pwm_low_val;
        uint8_t pwm_high_val;

        static payload_type
        deserialize(std::span<const std::byte, APT_SIZE> src);
        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}

// EOF
