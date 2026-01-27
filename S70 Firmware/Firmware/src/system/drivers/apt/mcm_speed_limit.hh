/**
 * \file mcm_speed_limit.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2025-01-06
 *
 * @copyright Copyright (c) 2024
 */
#pragma once

#include <bit>
#include <cstdint>

#include "./apt-command.hh"
#include "./apt-types.hh"
#include "apt-parsing.hh"
#include "apt.h"

namespace drivers::apt {

/**
 * A command that allows users to specify their desired maximum speed.
 */
struct mcm_speed_limit {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_SPEED_LIMIT;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_SPEED_LIMIT;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_SPEED_LIMIT;

    enum class movement_type_e : uint8_t {
        ABSOLUTE = 0,
        RELATIVE = 1,
        JOG      = 2,
        VELOCITY = 3,
        HOMING   = 4,

        // The enums below are for future use.
        // Add these to the MOVEMENT_TYPES array below.
        _UNUSED5 = 5,
        _UNUSED6 = 6,
        _UNUSED7 = 7,
    };

    static constexpr std::array<movement_type_e, 5> MOVEMENT_TYPES = {
        movement_type_e::ABSOLUTE,
        movement_type_e::RELATIVE,
        movement_type_e::JOG,
        movement_type_e::VELOCITY,
        movement_type_e::HOMING,
    };

    struct request_type {
        static constexpr std::size_t APT_SIZE = 3;

        channel_t channel;
        uint8_t type_bitset;

        static request_type deserialize(
            const std::span<const std::byte, APT_SIZE>& src);

        constexpr bool is_hardware_channel_selected() const {
            return type_bitset == 0;
        }

        constexpr bool one_channel_selected() const {
            return std::popcount(type_bitset) == 1;
        }

        constexpr bool is_movement_type_set(movement_type_e value) const {
            return type_bitset & (1 << (int)value);
        }

        /**
        * \pre one_channel_selected() is true
        */
        constexpr movement_type_e get_movement_type() const {
            for (movement_type_e type : MOVEMENT_TYPES) {
                if (is_movement_type_set(type)) {
                    return type;
                }
            }

            return movement_type_e::ABSOLUTE;
        }
    };

    struct payload_type {
        static constexpr std::size_t APT_SIZE = 7;

        uint32_t speed_limit;
        channel_t channel;
        uint8_t type_bitset;

        static payload_type deserialize(
            const std::span<const std::byte, APT_SIZE>& src);
        void serialize(const std::span<std::byte, APT_SIZE>& dest) const;

        constexpr bool is_movement_type_set(movement_type_e value) const {
            return type_bitset & (1 << (int)value);
        }

        constexpr payload_type& set_movement_type(movement_type_e type,
                                                  bool value) & {
            const uint8_t MASK = 1 << (int)type;
            if (value) {
                type_bitset |= MASK;
            } else {
                type_bitset &= MASK;
            }
            return *this;
        }

        constexpr payload_type&& set_movement_type(movement_type_e type,
                                                   bool value) && {
            const uint8_t MASK = 1 << (int)type;
            if (value) {
                type_bitset |= MASK;
            } else {
                type_bitset &= MASK;
            }
            return std::move(*this);
        }
    };
};

}  // namespace drivers::apt

// EOF
