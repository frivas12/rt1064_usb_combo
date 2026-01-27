#include "./mcm_shutter_trigger.hh"

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
mcm_shutter_trigger::request_type
mcm_shutter_trigger::request_type::deserialize(uint8_t param1,
                                                  uint8_t param2) {
    return request_type{
        .channel = static_cast<channel_t>(param1),
    };
}

mcm_shutter_trigger::payload_type
mcm_shutter_trigger::payload_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    payload_type rt;

    // * The order matters.
    rt.channel                = static_cast<channel_t>(stream.read<uint16_t>());
    rt.mode =
        static_cast<shutter_types::trigger_modes_e>(stream.read<uint8_t>());

    return rt;
}
void mcm_shutter_trigger::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(static_cast<uint16_t>(channel))
        .write(static_cast<uint8_t>(mode));
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
