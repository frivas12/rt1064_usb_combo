#include "./mcm_speed_limit.hh"

#include <algorithm>

#include "FreeRTOSConfig.h"
#include "apt-parsing.hh"
#include "integer-serialization.hh"
#include "task.h"

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

mcm_speed_limit::request_type mcm_speed_limit::request_type::deserialize(
    const std::span<const std::byte, APT_SIZE>& src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    request_type rt;

    rt.channel     = static_cast<channel_t>(stream.read<uint16_t>());
    rt.type_bitset = stream.read<uint8_t>();

    return rt;
}

mcm_speed_limit::payload_type mcm_speed_limit::payload_type::deserialize(
    const std::span<const std::byte, payload_type::APT_SIZE>& src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    payload_type rt;

    // * The order matters.
    rt.channel     = stream.read<uint16_t>();
    rt.type_bitset = stream.read<uint8_t>();
    rt.speed_limit = stream.read<uint32_t>();

    return rt;
}

void mcm_speed_limit::payload_type::serialize(
    const std::span<std::byte, APT_SIZE>& dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(channel).write(type_bitset).write(speed_limit);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
