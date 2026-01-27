#include "./mcm_shutter_params.hh"

#include <algorithm>
#include <span>
#include <type_traits>
#include <variant>

#include "FreeRTOSConfig.h"
#include "apt-parsing.hh"
#include "integer-serialization.hh"
#include "overload.hh"

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
using ser_ostream = stream_serializer<little_endian_serializer>;
using ser_istream = stream_deserializer<little_endian_serializer>;

constexpr ser_ostream create_ostream(
    const std::span<std::byte>& span) noexcept {
    return ser_ostream(span, little_endian_serializer());
}

constexpr ser_istream create_istream(
    const std::span<const std::byte>& span) noexcept {
    return ser_istream(span, little_endian_serializer());
}

static void deserialize_common(mcm_shutter_params::payload_type& dest,
                               ser_istream& src);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
mcm_shutter_params::request_type mcm_shutter_params::request_type::deserialize(
    uint8_t param1, uint8_t param2) {
    return request_type{
        .channel = param1,
    };
}

std::optional<mcm_shutter_params::payload_type>
mcm_shutter_params::payload_type::deserialize(
    std::span<const std::byte> buffer) {
    std::optional<mcm_shutter_params::payload_type> rt;

    switch (buffer.size()) {
    default:
        break;

    case APT_SIZES[0]:
        rt = deserialize(buffer.template subspan<0, APT_SIZES[0]>());
        break;

    case APT_SIZES[1]:
        rt = deserialize(buffer.template subspan<0, APT_SIZES[1]>());
        break;

    case APT_SIZES[2]:
        rt = deserialize(buffer.template subspan<0, APT_SIZES[2]>());
        break;
    }

    return rt;
}
mcm_shutter_params::payload_type mcm_shutter_params::payload_type::deserialize(
    const std::span<const std::byte, APT_SIZES[0]>& buffer) {
    mcm_shutter_params::payload_type rt;
    ser_istream stream = create_istream(buffer);

    deserialize_common(rt, stream);

    // monostate -> empty
    rt.optional_data = {};

    return rt;
}

mcm_shutter_params::payload_type mcm_shutter_params::payload_type::deserialize(
    const std::span<const std::byte, APT_SIZES[1]>& buffer) {
    mcm_shutter_params::payload_type rt;
    ser_istream stream = create_istream(buffer);

    deserialize_common(rt, stream);

    rt.optional_data =
        variant_len15{.reverse_open_level = stream.read<uint8_t>() != 0};

    return rt;
}

mcm_shutter_params::payload_type mcm_shutter_params::payload_type::deserialize(
    const std::span<const std::byte, APT_SIZES[2]>& buffer) {
    mcm_shutter_params::payload_type rt;
    ser_istream stream = create_istream(buffer);

    deserialize_common(rt, stream);

    variant_len19& data     = rt.optional_data.emplace<variant_len19>();
    data.reverse_open_level = stream.read<uint8_t>() != 0;
    data.actuation_holdoff_time =
        decltype(data.actuation_holdoff_time)(stream.read<uint32_t>());
    return rt;
}

std::size_t mcm_shutter_params::payload_type::serialize(
    std::span<std::byte, usb::apt_response::RESPONSE_BUFFER_SIZE> buffer)
    const {
    ser_ostream stream = create_ostream(buffer);

    static_assert(std::ranges::max(APT_SIZES) <= buffer.size());

    stream.write(channel)
        .write(static_cast<uint8_t>(initial_state))
        .write(static_cast<uint8_t>(type))
        .write(static_cast<uint8_t>(external_trigger_mode))
        .write(on_time.count())
        .write(duty_cycle_pulse)
        .write(duty_cycle_hold);

    return std::visit(overload([](const std::monostate&) {
        return APT_SIZES[0];
        }, [&stream](const variant_len15& value) {
        stream.write(static_cast<uint8_t>(value.reverse_open_level ? 1 : 0));
        return APT_SIZES[1];
        }, [&stream](const variant_len19& value) {
        stream.write(static_cast<uint8_t>(value.reverse_open_level ? 1 : 0))
            .write(value.actuation_holdoff_time.count());
        return APT_SIZES[2];
        }), optional_data);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static void deserialize_common(mcm_shutter_params::payload_type& dest,
                               ser_istream& src) {
    dest.channel = src.read<uint16_t>();
    dest.initial_state =
        static_cast<shutter_types::states_e>(src.read<uint8_t>());
    dest.type = static_cast<shutter_types::types_e>(src.read<uint8_t>());
    dest.external_trigger_mode =
        static_cast<shutter_types::trigger_modes_e>(src.read<uint8_t>());
    dest.on_time = utils::time::tens_of_ms<uint8_t>(src.read<uint8_t>());
    dest.duty_cycle_pulse = src.read<uint32_t>();
    dest.duty_cycle_hold  = src.read<uint32_t>();
}

// EOF
