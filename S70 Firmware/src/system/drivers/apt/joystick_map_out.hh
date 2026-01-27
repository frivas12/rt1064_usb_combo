/**
 * \file joystick_map_out.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-10-01
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

struct joystick_map_out {
    static constexpr uint16_t COMMAND_SET = MGMSG_MOD_SET_JOYSTICK_MAP_OUT;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOD_REQ_JOYSTICK_MAP_OUT;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOD_GET_JOYSTICK_MAP_OUT;

    struct request_type {
        uint8_t port_number;
        uint8_t control_number;

        constexpr uint8_t address() const { return port_number + 1; }

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 15;

        uint16_t permitted_vendor_id;
        uint16_t permitted_product_id;

        uint8_t color_ids[3];

        uint8_t port_number;
        uint8_t control_number;
        uint8_t usage_type;
        uint8_t led_mode;
        uint8_t source_slot_bitset;
        uint8_t source_channel_bitset;
        uint8_t source_port_bitset;
        uint8_t source_virtual_bitset;

        constexpr uint8_t address() const { return port_number + 1; }


        static payload_type
        deserialize(std::span<const std::byte, APT_SIZE> src);
        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}

// EOF
