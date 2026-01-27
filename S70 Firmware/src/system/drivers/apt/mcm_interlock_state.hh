/**
 * \file mcm_interlock_state.hh
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

#include "apt.h"

namespace drivers::apt {

struct mcm_interlock_state {
    static constexpr uint16_t COMMAND_REQ = MGMSG_MCM_REQ_INTERLOCK_STATE;
    static constexpr uint16_t COMMAND_GET = MGMSG_MCM_GET_INTERLOCK_STATE;

    using payload_type = bool;

    static drivers::usb::apt_response_builder& get(drivers::usb::apt_response_builder& builder, bool interlock_engaged);
};

} // namespace drivers::apt

// EOF
