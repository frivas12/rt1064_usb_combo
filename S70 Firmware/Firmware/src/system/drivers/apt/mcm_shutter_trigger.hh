#pragma once

#include <cstdint>

#include "apt-command.hh"
#include "apt-parsing.hh"
#include "apt-types.hh"
#include "apt.h"
#include "shutter-types.hh"

namespace drivers::apt {

struct mcm_shutter_trigger {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_SHUTTERTRIG;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_SHUTTERTRIG;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_SHUTTERTRIG;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 3;

        drivers::apt::shutter_types::trigger_modes_e mode;
        channel_t channel;


        static payload_type deserialize(
            std::span<const std::byte, APT_SIZE> src);
        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}  // namespace drivers::apt

// EOF
