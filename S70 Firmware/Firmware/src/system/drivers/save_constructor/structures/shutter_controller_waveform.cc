#include "shutter_controller_waveform.hh"
#include "cpld-shutter-driver.hh"
#include "n-bit-integers.hh"
#include "time.hh"
#include <cstddef>
#include <variant>

using namespace drivers::save_constructor;

static uint8_t
create_exponent_enum(const shutter_controller_waveform::serialized_time &time) {
    if (std::holds_alternative<
            std::chrono::duration<uint16_t, std::ratio<1, 10000>>>(time)) {
        return 0;
    }

    if (std::holds_alternative<std::chrono::duration<uint16_t, std::milli>>(
            time)) {
        return 1;
    }

    if (std::holds_alternative<std::chrono::duration<uint16_t, std::centi>>(
            time)) {
        return 2;
    }

    return 3;
}

static shutter_controller_waveform::serialized_time
from_serialied(uint16_t counts, uint8_t exponent) {
    switch (exponent) {
    case 0:
        return std::chrono::duration<uint16_t, std::ratio<1, 10000>>{counts};

    case 1:
        return std::chrono::duration<uint16_t, std::milli>{counts};

    case 2:
        return std::chrono::duration<uint16_t, std::centi>{counts};

    case 3:
    default:
        return std::chrono::duration<uint16_t, std::deci>{counts};
    }
}

template<std::size_t BitWidth, std::size_t Offset>
static constexpr int16_t
deserialize_integer(std::span<const std::byte, 2> src)
{
    static_assert(BitWidth + Offset <= 16);
    auto set = ((uint16_t)src[0]
             | (uint16_t)(src[1] << 8)) >> Offset;
    return utils::bit::intb<BitWidth>{static_cast<decltype(utils::bit::intb<BitWidth>::val)>(set)}.val;
}

/**
 * Binary layout
 * 17 16 15 14 13 12 11 10 0F 0E 0D 0C 0B 0A 09 08 07 06 05 04 03 02 01 00
 * P--P--P--P--P--P--P--P--P--P--P--P--H--H--H--H--H--H--H--H--H--H--H--H-
 * 27 26 25 24 23 22 21 20 1F 1E 1D 1C 1B 1A 19 18
 * E--E--C--C--C--C--C--C--C--C--C--C--C--C--C--C-
 * P = 12-bit signed integer for pulse width
 * H = 12-bit signed integer for hold width
 * E = 2-bit enum for serialized time exponent
 *   00 - std::ratio<1, 10000>
 *   01 - std::milli
 *   10 - std::centi
 *   11 - std::deci
 * C = 14-bit unsigned integer for the count of the duration
 *
 * 2 Bytes at the end are unused.
 */

shutter_controller_waveform
shutter_controller_waveform::deserialize(std::span<const std::byte, SIZE> src) {
    const uint16_t COUNTS =
        std::to_integer<uint16_t>(src[3]) |
        (std::to_integer<uint16_t>(src[4] & std::byte{0x3F}) << 8);
    const uint8_t ENUM = (std::to_integer<uint8_t>(src[4]) >> 6) & 0x03;

    return shutter_controller_waveform {
        .pulse_duty = deserialize_integer<12, 4>(src.subspan<1, 2>()),
        .holding_duty = deserialize_integer<12, 0>(src.subspan<0, 2>()),
        .pulse_width = from_serialied(COUNTS, ENUM),
    };
}

void shutter_controller_waveform::serialize(
    std::span<std::byte, SIZE> dest) const {
    const uint8_t EXPONENT = create_exponent_enum(pulse_width);
    const uint16_t COUNT = std::visit(
        [](const auto &value) { return value.count(); }, pulse_width);

    dest[0] = std::byte(holding_duty & 0x0FF);
    dest[1] =
        std::byte(((pulse_duty & 0x00F) << 4) | ((holding_duty & 0xF00) >> 16));
    dest[2] = std::byte((pulse_duty & 0xFF0) >> 4);
    dest[3] = std::byte(COUNT & 0x00FF);
    dest[4] = std::byte(((EXPONENT & 0x3) << 6) | ((COUNT & 0x3F00) >> 8));
}

shutter_controller_waveform shutter_controller_waveform::from(
    const cards::shutter::controller::state_waveform &waveform) {
    return shutter_controller_waveform{
        .pulse_duty = (int16_t)(cpld::shutter_module::add_sign(waveform.actuation_duty) *
                      (waveform.polarity ? 1 : -1)),
        .holding_duty = (int16_t)(cpld::shutter_module::add_sign(waveform.holding_duty) *
                        (waveform.polarity ? 1 : -1)),
        .pulse_width = to_serialized_time(
            utils::time::ticktype_duration{waveform.actuation_duration}),
    };
}

std::optional<cards::shutter::controller::state_waveform>
shutter_controller_waveform::to_state_waveform() const {
    if ((pulse_duty > 0) != (holding_duty > 0)) {
        return {};
    }

    return cards::shutter::controller::state_waveform{
        .actuation_duration =
            unpack_serialized_time<utils::time::ticktype_duration>(pulse_width)
                .count(),
        .actuation_duty = cpld::shutter_module::abs(pulse_duty),
        .holding_duty = cpld::shutter_module::abs(holding_duty),
        .polarity = pulse_duty > 0,
    };
}

// EOF
