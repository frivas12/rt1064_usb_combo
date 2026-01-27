/**
 * \file mcm_mirror_state.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <cstdint>
#include <span>

#include "./apt-command.hh"
#include "./apt-types.hh"
#include "mirror-types.hh"

#include "apt.h"


namespace drivers::apt
{

struct mcm_mirror_state {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_MIRROR_STATE;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_MIRROR_STATE;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_MIRROR_STATE;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        channel_t channel;
        mirror_types::states_e state;
        
        static payload_type deserialize(uint8_t param1, uint8_t param2);
        drivers::usb::apt_response::parameters serialize() const;
    };
};

}

// EOF
