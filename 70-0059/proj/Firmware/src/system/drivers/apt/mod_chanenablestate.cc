#include "./mod_chanenablestate.hh"

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
// Header-Only
mod_chanenablestate::request_type
mod_chanenablestate::request_type::deserialize(uint8_t param1, uint8_t param2) {
    return request_type{
        .channel = param1,
    };
}

mod_chanenablestate::payload_type
mod_chanenablestate::payload_type::deserialize(uint8_t param1, uint8_t param2) {
    return payload_type{
        .channel = param1,
        .is_enabled = param2 == 0x01,
    };
}
drivers::usb::apt_response::parameters
mod_chanenablestate::payload_type::serialize() const {
    return drivers::usb::apt_response::parameters {
        .param1 = static_cast<uint8_t>(channel),
        .param2 = static_cast<uint8_t>(is_enabled ? 1 : 0),
    };
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF