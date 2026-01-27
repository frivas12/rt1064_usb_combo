#include "./mod_joystick_info.hh"

#include "apt-command.hh"
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
mod_joystick_info::request_type mod_joystick_info::request_type::deserialize(
    uint8_t param1, uint8_t param2) {
    return request_type{
        .port_number = param1,
    };
}

void mod_joystick_info::get(drivers::usb::apt_response_builder& builder,
                            const payload_type& payload) {
    builder.command(mod_joystick_info::COMMAND_GET)
        .extended_data(
            [&](const std::span<
                std::byte, drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>&
                    dest) -> std::span<std::byte> {
                auto stream =
                    stream_serializer(dest, little_endian_serializer());
                stream.write(payload.port_number)
                    .write(payload.vendor_id)
                    .write(payload.product_id)
                    .write(payload.type_flags);

                if (payload.is_hub()) {
                    stream.write(payload.multiplexed.hub.port_count);
                } else {
                    stream
                        .write(payload.multiplexed.joystick.input_control_count)
                        .write(
                            payload.multiplexed.joystick.output_control_count);
                }

                return dest.subspan(0, dest.size() - stream.bytes_remaining());
            });
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
