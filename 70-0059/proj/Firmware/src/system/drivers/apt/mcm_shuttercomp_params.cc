#include "./mcm_shuttercomp_params.hh"

#include "integer-serialization.hh"
#include "mcm_shutter_params.hh"
#include "shutter-types.hh"

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
mcm_shuttercomp_params::request_type
mcm_shuttercomp_params::request_type::deserialize(uint8_t param1,
                                                  uint8_t param2) {
    return request_type{
        .channel = static_cast<channel_t>(param1),
    };
}

mcm_shuttercomp_params::payload_type
mcm_shuttercomp_params::payload_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    payload_type rt;

    // * The order matters.
    rt.channel                = static_cast<channel_t>(stream.read<uint16_t>());
    rt.open_actuation_voltage = stream.read<float>();
    rt.open_actuation_time    = stream.read<float>();
    rt.opened_hold_voltage    = stream.read<float>();
    rt.close_actuation_voltage = stream.read<float>();
    rt.close_actuation_time    = stream.read<float>();
    rt.closed_hold_voltage     = stream.read<float>();
    rt.power_up_state =
        static_cast<shutter_types::states_e>(stream.read<uint8_t>());
    rt.trigger_mode =
        static_cast<shutter_types::trigger_modes_e>(stream.read<uint8_t>());
    rt.open_holdoff_time = stream.read<float>();
    rt.close_holdoff_time = stream.read<float>();

    return rt;
}
void mcm_shuttercomp_params::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(static_cast<uint16_t>(channel))
        .write(open_actuation_voltage)
        .write(open_actuation_time)
        .write(opened_hold_voltage)
        .write(close_actuation_voltage)
        .write(close_actuation_time)
        .write(closed_hold_voltage)
        .write(static_cast<uint8_t>(power_up_state))
        .write(static_cast<uint8_t>(trigger_mode))
        .write(open_holdoff_time)
        .write(close_holdoff_time);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
