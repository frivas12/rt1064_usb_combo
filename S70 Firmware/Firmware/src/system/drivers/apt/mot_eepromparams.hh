/**
 * \file mot_eepromparams.hh
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
#include "integer-serialization.hh"

namespace drivers::apt {

struct mot_eepromparams {
    static constexpr uint16_t COMMAND_SET = MGMSG_MOT_SET_EEPROMPARAMS;

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 4;

        std::array<std::byte, 2> parameters;
        uint16_t msg_id;

        constexpr uint16_t params_as_channel() const {
            auto ser = little_endian_serializer();

            return ser.deserialize_uint16(std::span(parameters));
        }

        static payload_type
        deserialize(std::span<const std::byte, APT_SIZE> src);
    };
};

} // namespace drivers::apt

// EOF
