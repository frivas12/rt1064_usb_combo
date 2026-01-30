/**
 * \file joystick_map_in.hh
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

namespace drivers::apt {

struct joystick_map_in {
    static constexpr uint16_t COMMAND_SET = MGMSG_MOD_SET_JOYSTICK_MAP_IN;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MOD_REQ_JOYSTICK_MAP_IN;
    static constexpr uint16_t COMMAND_GET = MGMSG_MOD_GET_JOYSTICK_MAP_IN;

    struct request_type {
        uint8_t port_number;
        uint8_t control_number;

        constexpr uint8_t address() const { return port_number + 1; }

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 21;

        uint32_t control_mode;

        uint16_t permitted_vendor_id;
        uint16_t permitted_product_id;
        uint16_t target_control_numbers_bitset;

        uint8_t port_number;
        uint8_t control_number;
        uint8_t target_port_numbers_bitset;
        uint8_t destination_slots_bitset;
        uint8_t destination_channels_bitset;
        uint8_t destination_ports_bitset;
        uint8_t destination_virtual_bitset;
        uint8_t speed_modifier;
        uint8_t dead_band;
        uint8_t reverse_direction;

        bool control_disabled;

        constexpr uint8_t address() const { return port_number + 1; }

        static payload_type deserialize(
            std::span<const std::byte, APT_SIZE> src);
        void serialize(std::span<std::byte, APT_SIZE> dest) const;
    };
};

}  // namespace drivers::apt

// EOF
