/**
 * \file mot_solenoid_state.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-06
 * 
 * @copyright Copyright (c) 2024
 */
#pragma once

#include <cstdint>
#include <span>

#include "./apt-command.hh"
#include "./apt-types.hh"

#include "apt.h"


namespace drivers::apt
{

struct mot_solenoid_state {
    static constexpr uint16_t COMMAND_SET = MGMSG_MOT_SET_SOL_STATE;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOT_REQ_SOL_STATE;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOT_GET_SOL_STATE;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        bool solenoid_on;
        channel_t channel;

        static payload_type deserialize(uint8_t param1, uint8_t param2);
        drivers::usb::apt_response::parameters serialize() const;
    };
};

}

// EOF
