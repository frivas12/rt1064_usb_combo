#include "./joystick_map_out.hh"

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
joystick_map_out::request_type joystick_map_out::request_type::deserialize(
    uint8_t param1, uint8_t param2) {
    return request_type{
        .port_number    = param1,
        .control_number = param2,
    };
}

joystick_map_out::payload_type joystick_map_out::payload_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer{});

    payload_type rt;

    // * The order matters.
    rt.port_number           = stream.read<uint8_t>();
    rt.control_number        = stream.read<uint8_t>();
    rt.usage_type            = stream.read<uint8_t>();
    rt.permitted_vendor_id   = stream.read<uint16_t>();
    rt.permitted_product_id  = stream.read<uint16_t>();
    rt.led_mode              = stream.read<uint8_t>();
    rt.color_ids[0]          = stream.read<uint8_t>();
    rt.color_ids[1]          = stream.read<uint8_t>();
    rt.color_ids[2]          = stream.read<uint8_t>();
    rt.source_slot_bitset    = stream.read<uint8_t>();
    rt.source_channel_bitset = stream.read<uint8_t>();
    rt.source_port_bitset    = stream.read<uint8_t>();
    rt.source_virtual_bitset = stream.read<uint8_t>();

    return rt;
}
void joystick_map_out::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer{});

    stream.write(port_number)
        .write(control_number)
        .write(usage_type)
        .write(permitted_vendor_id)
        .write(permitted_product_id)
        .write(led_mode)
        .write(color_ids[0])
        .write(color_ids[1])
        .write(color_ids[2])
        .write(source_slot_bitset)
        .write(source_channel_bitset)
        .write(source_port_bitset)
        .write(source_virtual_bitset);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
