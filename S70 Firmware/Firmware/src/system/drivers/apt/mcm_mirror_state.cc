#include "./mcm_mirror_state.hh"

#include "integer-serialization.hh"

using namespace drivers::apt;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
mcm_mirror_state::request_type
mcm_mirror_state::request_type::deserialize(uint8_t param1, uint8_t param2) {
    return request_type{
        .channel = param1,
    };
}

mcm_mirror_state::payload_type
mcm_mirror_state::payload_type::deserialize(uint8_t param1, uint8_t param2) {
    return payload_type{
        .channel = param1,
        .state = static_cast<mirror_types::states_e>(param2),
    };
}
drivers::usb::apt_response::parameters
mcm_mirror_state::payload_type::serialize() const {
    return drivers::usb::apt_response::parameters {
        .param1 = static_cast<uint8_t>(channel),
        .param2 = static_cast<uint8_t>(state),
    };
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF