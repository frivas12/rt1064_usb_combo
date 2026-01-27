/**
 * \file mod_joystick_info.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-10-03
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

struct mod_joystick_info {
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOD_REQ_JOYSTICK_INFO;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOD_GET_JOYSTICK_INFO;

    struct request_type {
        uint8_t port_number;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {

        union {
            struct {
                uint8_t port_count;
            } hub;
            struct {
                uint8_t input_control_count;
                uint8_t output_control_count;
            } joystick;
        } multiplexed;

        uint16_t vendor_id;
        uint16_t product_id;

        uint8_t port_number;
        uint8_t type_flags;

        constexpr bool is_hub() const { return (type_flags & 0x01) != 0; }
        constexpr bool is_low_speed() const { return (type_flags & 0x02) != 0; }
    };

    // Optional Specializations
    static void get(drivers::usb::apt_response_builder& builder, const payload_type& payload);
};

}

// EOF
