#include "./joystick_map_in.hh"

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
joystick_map_in::request_type joystick_map_in::request_type::deserialize(
    uint8_t param1, uint8_t param2) {
    return request_type{
        .port_number    = param1,
        .control_number = param2,
    };
}

joystick_map_in::payload_type joystick_map_in::payload_type::deserialize(
    std::span<const std::byte, APT_SIZE> src) {
    auto stream = stream_deserializer(src, little_endian_serializer());

    payload_type rt;

    // * The order matters.
    rt.port_number                   = stream.read<uint8_t>();
    rt.control_number                = stream.read<uint8_t>();
    rt.permitted_vendor_id           = stream.read<uint16_t>();
    rt.permitted_product_id          = stream.read<uint16_t>();
    rt.target_port_numbers_bitset    = stream.read<uint8_t>();
    rt.target_control_numbers_bitset = stream.read<uint16_t>();
    rt.destination_slots_bitset      = stream.read<uint8_t>();
    rt.destination_channels_bitset   = stream.read<uint8_t>();
    rt.destination_ports_bitset      = stream.read<uint8_t>();
    rt.destination_virtual_bitset    = stream.read<uint8_t>();
    rt.speed_modifier                = stream.read<uint8_t>();
    rt.reverse_direction             = stream.read<uint8_t>();
    rt.dead_band                     = stream.read<uint8_t>();
    rt.control_mode                  = stream.read<uint32_t>();
    rt.control_disabled              = stream.read<uint8_t>() != 0;

    return rt;
}
void joystick_map_in::payload_type::serialize(
    std::span<std::byte, APT_SIZE> dest) const {
    auto stream = stream_serializer(dest, little_endian_serializer());

    stream.write(port_number)
        .write(control_number)
        .write(permitted_vendor_id)
        .write(permitted_product_id)
        .write(target_port_numbers_bitset)
        .write(target_control_numbers_bitset)
        .write(destination_slots_bitset)
        .write(destination_channels_bitset)
        .write(destination_ports_bitset)
        .write(destination_virtual_bitset)
        .write(speed_modifier)
        .write(reverse_direction)
        .write(dead_band)
        .write(control_mode)
        .write(static_cast<uint8_t>(control_disabled ? 0x01 : 0x00));
}

// Optional Specializations
// static ? joystick_map_in::set(const drivers::usb::apt_command&,...);
// static ? joystick_map_in::req(const drivers::usb::apt_command&,...);
// static drivers::usb::apt_response joystick_map_in::get(?);

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
