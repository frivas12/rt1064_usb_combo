#include "./mod_joystick_control.hh"

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
mod_joystick_control::request_type
mod_joystick_control::request_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    request_type rt;

    // * The order matters.
    rt.port_number         = stream.read<uint8_t>();
    rt.in_out_control_type = stream.read<uint8_t>() != 0;
    rt.control_number      = stream.read<uint8_t>();

    return rt;
}

void mod_joystick_control::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(port_number)
        .write(static_cast<uint8_t>(in_out_control_type ? 0x01 : 0x00))
        .write(control_number)
        .write(hid_usage_page)
        .write(hid_usage_id);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
