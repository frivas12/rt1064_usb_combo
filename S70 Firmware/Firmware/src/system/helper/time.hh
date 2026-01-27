/**
 * \file time.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-05-30
 */
#pragma once

#include <chrono>
#include <concepts>
#include <limits>

#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"

namespace utils::time {

template <typename T>
concept duration_like = requires() {
    typename T::rep;
    typename T::period;
};

/// \brief A value containing 10s of milliseconds of time.
///        Used primarily for serialization.
template <std::integral Rep>
using tens_of_ms = std::chrono::duration<Rep, std::centi>;

/// \brief A value containing 100s of microseconds of time.
///        Used primarily for serialization.
template <std::integral Rep>
using hundreds_of_us = std::chrono::duration<Rep, std::ratio<1, 10000>>;

static constexpr uint32_t TICKS_PER_SECOND = 1000 * pdMS_TO_TICKS(1);

using ticktype_duration =
    std::chrono::duration<TickType_t, std::ratio<1, TICKS_PER_SECOND>>;

/// The maximum amount of time in seconds that TickType can be used to
/// represent a duration.
static constexpr float MAX_SECONDS_IN_DURATION_TICKTYPE = static_cast<float>(
    static_cast<double>(std::numeric_limits<TickType_t>::max()) / TICKS_PER_SECOND);

template <std::integral Rep, typename Period>
constexpr ticktype_duration to_tick_duration(
    const std::chrono::duration<Rep, Period>& time) {
    return std::chrono::duration_cast<ticktype_duration>(time);
}

template <std::integral Rep, typename Period>
constexpr TickType_t to_ticks(const std::chrono::duration<Rep, Period>& time) {
    return to_tick_duration(time).count();
}

/**
* Converts a floating-point value in seconds into TickType_t.
* If the floating-point value represents a duration longer that
* TickType_t can represent, it will be bounded to its upper value.
*/
constexpr TickType_t bound_seconds_to_ticks(float seconds) {
    if (seconds > MAX_SECONDS_IN_DURATION_TICKTYPE) {
        seconds = MAX_SECONDS_IN_DURATION_TICKTYPE;
    }
    return static_cast<TickType_t>(seconds * TICKS_PER_SECOND);
}

template <std::integral Rep, typename Period>
constexpr std::chrono::duration<Rep, Period> to_duration(
    ticktype_duration ticks) {
    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
        ticks);
}
template <duration_like Duration>
constexpr Duration to_duration(ticktype_duration ticks) {
    return std::chrono::duration_cast<Duration>(ticks);
}

template <std::integral Rep, typename Period>
constexpr std::chrono::duration<Rep, Period> to_duration(TickType_t ticks) {
    return to_duration<Rep, Period>(ticktype_duration{ticks});
}
template <duration_like Duration>
constexpr Duration to_duration(TickType_t ticks) {
    return to_duration<Duration>(ticktype_duration{ticks});
}

}  // namespace utils::time

// EOF
