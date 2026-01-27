#include "./mcm_pwm_period.hh"

#include <expected>

#include "FreeRTOSConfig.h"
#include "apt-command.hh"
#include "apt-parsing.hh"
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
parser_result<mcm_pwm_period::payload_type> mcm_pwm_period::set(
    const drivers::usb::apt_basic_command& command) {
    if (command.command() != COMMAND_SET) {
        return std::unexpected(parser_errors_e::INVALID_COMMAND);
    } else if (!command.has_extended_data()) {
        return std::unexpected(parser_errors_e::INVALID_PAYLOAD_MODE);
    } else if (command.extended_data().size() < 4) {
        return std::unexpected(parser_errors_e::INVALID_PAYLOAD_SIZE);
    }
    return mcm_pwm_period::payload_type{
        .current = little_endian_serializer().deserialize_uint32(
            command.extended_data().template subspan<0, 4>()),
    };
}

drivers::usb::apt_response_builder& mcm_pwm_period::get(
    drivers::usb::apt_response_builder& builder,
    const mcm_pwm_period::payload_type& value) {
    return builder.command(COMMAND_GET)
        .extended_data(
            [&value](
                const std::span<
                    std::byte, usb::apt_response::RESPONSE_BUFFER_SIZE>& dest) {
                auto rt = dest.subspan(0, 12);

                auto stream =
                    stream_serializer(dest, little_endian_serializer());
                stream.write(value.current)
                    .write(value.get_only.min)
                    .write(value.get_only.max);

                return rt;
            });
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
