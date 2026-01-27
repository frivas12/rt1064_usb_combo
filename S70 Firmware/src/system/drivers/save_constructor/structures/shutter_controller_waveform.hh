/**
 * \file shutter_controller_waveform.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-21
 */
#pragma once

#include <chrono>
#include <ratio>
#include <span>
#include <variant>

#include "../structure_id.hh"
#include "cpld-shutter-driver.hh"
#include "shutter-controller.hh"
#include "time.hh"

namespace drivers::save_constructor {
struct shutter_controller_waveform {
    static constexpr Struct_ID ID = Struct_ID::SHUTTER_CONTROLLER_WAVEFORM;
    static constexpr std::size_t SIZE = 7;

    using serialized_time =
        std::variant<std::chrono::duration<uint16_t, std::ratio<1, 10000>>,
                     std::chrono::duration<uint16_t, std::milli>,
                     std::chrono::duration<uint16_t, std::centi>,
                     std::chrono::duration<uint16_t, std::deci>>;

    cpld::shutter_module::signed_duty_type pulse_duty;
    cpld::shutter_module::signed_duty_type holding_duty;
    serialized_time pulse_width;

    /**
     * Gets the most fine resolution of the duration that can be serialized.
     * Rounds down to the closest representation.
     */
    template <typename Representation, typename Ratio>
    static constexpr serialized_time to_serialized_time(
        const std::chrono::duration<Representation, Ratio> &duration) {
        static constexpr uint16_t COUNT_MAX = (0x1 << 14) - 1;

        uint16_t large_cast_ticks =
            duration_cast<
                std::chrono::duration<uint64_t, std::ratio<1, 10000>>>(duration)
                .count();

        if (large_cast_ticks <= COUNT_MAX) {
            return serialized_time{
                std::chrono::duration<uint16_t, std::ratio<1, 10000>>(
                    static_cast<uint16_t>(large_cast_ticks))};
        }

        large_cast_ticks /= 10;
        if (large_cast_ticks <= COUNT_MAX) {
            return serialized_time{std::chrono::duration<uint16_t, std::milli>(
                static_cast<uint16_t>(large_cast_ticks))};
        }

        large_cast_ticks /= 10;
        if (large_cast_ticks <= COUNT_MAX) {
            return serialized_time{std::chrono::duration<uint16_t, std::centi>(
                static_cast<uint16_t>(large_cast_ticks))};
        }
        large_cast_ticks /= 10;
        if (large_cast_ticks <= COUNT_MAX) {
            return serialized_time{std::chrono::duration<uint16_t, std::deci>(
                static_cast<uint16_t>(large_cast_ticks))};
        }
        return serialized_time{std::chrono::duration<uint16_t, std::deci>(0)};
    }

    template <utils::time::duration_like Duration>
    static constexpr Duration
    unpack_serialized_time(const serialized_time &time) {
        return std::visit(
            [](const auto &time) {
                return std::chrono::duration_cast<Duration>(time);
            },
            time);
    }

    template <typename Representation, typename Ratio>
    static constexpr std::chrono::duration<Representation, Ratio>
    unpack_serialized_time(const serialized_time &time) {
        return unpack_serialized_time<
            std::chrono::duration<Representation, Ratio>>(time);
    }

    static shutter_controller_waveform
    deserialize(std::span<const std::byte, SIZE> src);

    void serialize(std::span<std::byte, SIZE> dest) const;

    /**
     * Constructs the serialized struct form a state wavefrom.
     */
    static shutter_controller_waveform
    from(const cards::shutter::controller::state_waveform &waveform);


    /**
     * Attempts to deserialize the structure into a state waveform object.
     */
    std::optional<cards::shutter::controller::state_waveform>
    to_state_waveform() const;
};
} // namespace drivers::save_constructor

// EOF
