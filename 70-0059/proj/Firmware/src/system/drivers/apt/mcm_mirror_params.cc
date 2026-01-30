#include "mcm_mirror_params.hh"

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
mcm_mirror_params::request_type
mcm_mirror_params::request_type::deserialize(uint8_t param1, uint8_t param2) {
    return request_type{
        .channel = param1,
    };
}

mcm_mirror_params::payload_type mcm_mirror_params::payload_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer{});

    mcm_mirror_params::payload_type rt;

    rt.channel = stream.read<uint16_t>();
    rt.initial_state = static_cast<mirror_types::states_e>(stream.read<uint8_t>());
    rt.mode = static_cast<mirror_types::modes_e>(stream.read<uint8_t>());
    rt.pwm_low_val = stream.read<uint8_t>();
    rt.pwm_high_val = stream.read<uint8_t>();

    return rt;
}

void mcm_mirror_params::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(channel)
        .write(static_cast<uint8_t>(initial_state))
        .write(static_cast<uint8_t>(mode))
        .write(pwm_low_val)
        .write(pwm_high_val);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF