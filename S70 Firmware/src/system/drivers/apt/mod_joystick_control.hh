/**
 * \file mod_joystick_control.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date  2024-10-03
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

struct mod_joystick_control {
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOD_REQ_JOYSTICK_CONTROL;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOD_GET_JOYSTICK_CONTROL;

    struct request_type {
        static constexpr std::size_t APT_SIZE = 2;

        /// \brief 0/false -> in, 1/true -> out
        bool in_out_control_type;
        uint8_t port_number;
        uint8_t control_number;


        static request_type
        deserialize(std::span<const std::byte, APT_SIZE> src);
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 7;

        uint16_t hid_usage_page;
        uint16_t hid_usage_id;

        /// \brief 0/false -> in, 1/true -> out
        bool in_out_control_type;
        uint8_t port_number;
        uint8_t control_number;


        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}

// EOF
