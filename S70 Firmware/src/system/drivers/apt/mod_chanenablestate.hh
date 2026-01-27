/**
 * \file mod_chanenablestate.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <cstdint>
#include <span>

#include "./apt-command.hh"
#include "./apt-types.hh"

#include "apt.h"


namespace drivers::apt
{

struct mod_chanenablestate {
    static constexpr uint16_t COMMAND_SET = MGMSG_MOD_SET_CHANENABLESTATE;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOD_REQ_CHANENABLESTATE;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOD_GET_CHANENABLESTATE;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        channel_t channel;
        bool is_enabled;

        static payload_type deserialize(uint8_t param1, uint8_t param2);
        drivers::usb::apt_response::parameters serialize() const;
    };
};

}

// EOF
