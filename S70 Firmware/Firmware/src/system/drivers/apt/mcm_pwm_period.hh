#pragma once

#include <cstdint>

#include "apt-command.hh"
#include "apt-parsing.hh"
#include "apt.h"

namespace drivers::apt {

// The GET command contains the tuple (current, min, max).
// The SET command contains the tuple (new_current).
struct mcm_pwm_period {
    static constexpr uint16_t COMMAND_SET = MGMSG_MCM_SET_PWM_PERIOD;
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_PWM_PERIOD;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_PWM_PERIOD;

    struct payload_type {
        uint32_t current;

        /// Undefined for SET payloads.
        struct {
            uint32_t min;
            uint32_t max;
        } get_only;
    };

    static parser_result<payload_type> set(
        const drivers::usb::apt_basic_command& command);
    static drivers::usb::apt_response_builder& get(
        drivers::usb::apt_response_builder& builder, const payload_type& value);
};

}  // namespace drivers::apt

// EOF
