/**
 * \file mcm_shutter_params.hh
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
#include "./shutter-types.hh"
#include "apt.h"
#include "time.hh"

namespace drivers::apt {

struct mcm_shutter_params {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_SHUTTERPARAMS;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_SHUTTERPARAMS;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_SHUTTERPARAMS;

    struct request_type {
        channel_t channel;

        static request_type deserialize(uint8_t param1, uint8_t param2);
    };

    struct payload_type {
        static constexpr std::array<std::size_t, 3> APT_SIZES = {
            14,  // no variant
            15,  // variant_len15
            19,  // variant_len19
        };

        struct variant_len15 {
            /// If defined, it reverses the open level in the controller.
            bool reverse_open_level;
        };  

        struct variant_len19 {
            /// If defined, this defines the holdoff time for actuation in hundreds of microseconds.
            utils::time::hundreds_of_us<uint32_t> actuation_holdoff_time;


            /// If defined, it reverses the open level in the controller.
            bool reverse_open_level;
        };

        using optional_data_t = std::variant<std::monostate, variant_len15, variant_len19>;

        shutter_types::states_e initial_state;
        shutter_types::types_e type;
        shutter_types::trigger_modes_e external_trigger_mode;
        uint32_t duty_cycle_pulse;
        uint32_t duty_cycle_hold;
        channel_t channel;
        utils::time::tens_of_ms<uint8_t> on_time;

        optional_data_t optional_data;

        static std::optional<payload_type> deserialize(
            std::span<const std::byte> src);
        std::size_t serialize(
            std::span<std::byte, usb::apt_response::RESPONSE_BUFFER_SIZE> dest)
            const;

       private:
        static payload_type deserialize(
            const std::span<const std::byte, APT_SIZES[0]>& src);
        static payload_type deserialize(
            const std::span<const std::byte, APT_SIZES[1]>& src);
        static payload_type deserialize(
            const std::span<const std::byte, APT_SIZES[2]>& src);
    };
};

}  // namespace drivers::apt

// EOF
