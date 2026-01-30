/**
 * \file cpld-shutter-driver.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 *
 * The shutter module of the CPLD provides 4 pairs of
 * coupled PWM signals, a one-wire pin, and a chip select
 * for a GPIO multiplexer.
 *
 * The 4 coupled PWM signals have two output pins.
 * Channel #:   Left        Right
 * 0:           IN_1_1      IN_1_2
 * 1:           IN_1_3      IN_1_4
 * 2:           IN_2_1      IN_2_2
 * 3:           IN_2_3      IN_2_4
 * Each channel has one PWM signal with a variable duty cycle.
 * While the phase is FALSE, the Left pin is grounded and the Right
 * pin passes the PWM signal.
 * While the phase is TRUE, the Left pin passes the PWM signal
 * and the Right pin is grounded.
 * If a load is connected between the left and right pins, then
 * the voltage across the load can be reversed by setting the phase.
 */
#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <type_traits>

#include "slot_nums.h"
#include "spi-transfer-handle.hh"
#include "time.hh"

namespace drivers::cpld::shutter_module {
/// @brief Integer frequency divider of the cpld's PWM clock.
/// It is a 12-bit integer.
using period_type = uint16_t;

/// @brief Counter in the range [0, value) where "value" is a frequency_type
///        equal to the value of the PWM channel's frequency module.
using duty_type = uint16_t;

/// @brief Type of the duty that is signed.
///        This is typed such that [-MAXIMUM_PERIOD, MAXIMUM_PERIOD] can be
///        stored.
using signed_duty_type = int16_t;

/// @brief The type of the channel id for the CPLD's shutter module.
using channel_id = uint8_t;

/// @brief Power-up frequency of the shutter driver.
static constexpr period_type DEFAULT_PERIOD = 1900;

/// @brief Minimum period (maximum frequency) of the shutter.
static constexpr period_type MINIMUM_PERIOD = 38;  // 1 MHz

/// @brief Maximum period (minimum frequency) of the shutter.
static constexpr period_type MAXIMUM_PERIOD = 4095;  // 9.280 kHz

/// @brief The maximum number of channels of the CPLD shutter module.
static constexpr std::size_t CHANNEL_COUNT = 4;

/// @brief Converts a duty percentage to the integer duty_type.
constexpr duty_type duty_percentage_to_integer(
    double percentage, duty_type period = DEFAULT_PERIOD) {
    return static_cast<duty_type>(percentage * period / 100);
}

/// @brief Converts a duty integer (duty_type) to a percentage.
constexpr double duty_integer_to_percentage(duty_type integer,
                                            duty_type period = DEFAULT_PERIOD) {
    return static_cast<double>(integer) / period * 100;
}

/// @brief Converts the period type to Hz.
constexpr double to_hz(period_type period) {
    static constexpr double COEFFICIENT = 20E3 * 1900;
    return COEFFICIENT / period;
}

/// @brief Converts the Hz to the period type (without clamping).
constexpr period_type from_hz(double frequency) {
    static constexpr double COEFFICIENT = 20E3 * 1900;
    return COEFFICIENT / frequency;
}

/// \brief Converts a duty type value to a signed duty type value.
constexpr signed_duty_type add_sign(duty_type duty_cycle) noexcept {
    return static_cast<signed_duty_type>(duty_cycle);
}

/// \brief Converts a signed duty type value to its unsigned counterpart
///        via absolute value.
constexpr duty_type abs(signed_duty_type signed_duty_cycle) noexcept {
    return static_cast<duty_type>(std::abs(signed_duty_cycle));
}

/// \brief Applies std::clamp with the min and max values for the period.
constexpr period_type clamp_period(uint16_t value) noexcept {
    return std::clamp<uint16_t>(value, MINIMUM_PERIOD, MAXIMUM_PERIOD);
}

/// \brief Applies std::clamp with the min and max values for the period.
/// This overload participates when the size of the value type is larger
/// than the period type.
template <std::unsigned_integral T>
    requires(sizeof(T) > sizeof(uint16_t))
constexpr period_type clamp_period(T value) noexcept {
    return std::clamp<T>(value, MINIMUM_PERIOD, MAXIMUM_PERIOD);
}

/// Calculates a duty value such that duty/orignal_period ≈ return/new_period.
constexpr duty_type get_equivalent_duty(duty_type duty,
                                        period_type original_period,
                                        period_type new_period) {
    return duty * new_period / original_period;
}

/// Calculates a duty value such that duty/orignal_period ≈ return/new_period.
constexpr signed_duty_type get_equivalent_duty(signed_duty_type duty,
                                               period_type original_period,
                                               period_type new_period) {
    return duty * new_period / original_period;
}

/**
 * Disables the module.
 * \pre slot is valid
 */
void disable_module(drivers::spi::handle_factory& factory, slot_nums slot);
/**
 * Enables the one-wire mode (only one-wire pin enabled).
 * \pre slot is valid
 */
void start_listening_for_devices(drivers::spi::handle_factory& factory,
                                 slot_nums slot);
/**
 * Enables the module.
 * \warning If another module is enabled on the given slot,
 *          the CPLD can be damaged!
 * \pre slot is valid
 */
void enable_module(drivers::spi::handle_factory& factory, slot_nums slot);

/**
 * Sets the duty cycle of the PWM signal.
 * \pre slot is valid
 * \pre 0 <= channel < CHANNEL_COUNT
 */
void set_duty_cycle(drivers::spi::handle_factory& factory, slot_nums slot,
                    channel_id channel, duty_type duty_cycle);

/**
 * Set the phase polarity of the PWM signal.
 * \pre slot is valid
 * \pre 0 <= channel < CHANNEL_COUNT
 */
void set_phase_polarity(drivers::spi::handle_factory& factory, slot_nums slot,
                        channel_id channel, bool phase_polarity);

/**
 * Sets the period of the PWM signal.
 * The PWM signal will be immediately reset when this occurs.
 * \pre slot is valid.
 */
void set_period(drivers::spi::handle_factory& factory, slot_nums slot,
                period_type value);

/// @brief The state of the channel.
struct channel_state {
    duty_type duty_cycle;
    bool phase_polarity;

    static constexpr channel_state from(signed_duty_type duty_cycle) noexcept {
        return channel_state{
            .duty_cycle     = static_cast<duty_type>(std::abs(duty_cycle)),
            .phase_polarity = duty_cycle >= 0,
        };
    };

    constexpr signed_duty_type to_signed_duty() const {
        return static_cast<signed_duty_type>(duty_cycle) *
               (phase_polarity ? -1 : 1);
    }
};

/// \brief Simple proxy class for operations with a state object.
class stateful_channel_proxy {
   public:
    void set_duty_cycle(duty_type duty_cycle);
    void set_phase_polarity(bool phase_polarity);

    /// \brief Sets the duty cycle and polarity using the voltage pattern.
    ///        The voltage pattern ensures that any transitional voltage
    ///        is closer to 0.
    void set_state_voltage_pattern(channel_state state);

    inline const channel_state& state() const { return _state; }

    stateful_channel_proxy(channel_state& state, spi::handle_factory& factory,
                           slot_nums slot, channel_id channel);

   private:
    drivers::spi::handle_factory& _factory;
    channel_state& _state;
    slot_nums _slot;
    channel_id _channel;
};

/**
 * Converts the target voltage in some representation
 * to the PWM duty cycle.
 * \tparam Rep  A signed integral, unsigned integral, or floating point.
 * \param[in]   target_voltage    The voltage to target.
 * \param[in]   pwm_high_voltage  The voltage of the pwm signal
 *                                when on the ON state.
 * \pre pwm_high_voltage > 0
 * \return The signed duty cycle to produce a value close to the target voltage.
 */
template <std::integral Rep>
constexpr signed_duty_type to_duty_type(Rep target_voltage,
                                        Rep pwm_high_voltage,
                                        period_type period = DEFAULT_PERIOD) {
    using cast_type = std::conditional_t<std::unsigned_integral<Rep>,
                                         std::size_t, std::ptrdiff_t>;
    return static_cast<signed_duty_type>(
        static_cast<cast_type>(target_voltage * period) / pwm_high_voltage);
}
template <std::floating_point Rep>
constexpr signed_duty_type to_duty_type(Rep target_voltage,
                                        Rep pwm_high_voltage,
                                        period_type period = DEFAULT_PERIOD) {
    return static_cast<signed_duty_type>(target_voltage * period /
                                         pwm_high_voltage);
}

/**
 * Converts the target voltage in some representation
 * to the PWM duty cycle.
 * \tparam Rep  A signed integral, unsigned integral, or floating point.
 * \param[in]   duty_cycle        The duty cycle to convert.
 * \param[in]   pwm_high_voltage  The voltage of the pwm signal
 *                                when on the ON state.
 * \pre pwm_high_voltage > 0
 * \pre duty cycle is unsigned if the representation is unsigned.
 * \return The representation close to the duty cycle.
 */
template <std::unsigned_integral Rep>
constexpr Rep to_voltage(duty_type duty_cycle, Rep pwm_high_voltage,
                         period_type pwm_period = DEFAULT_PERIOD) {
    return static_cast<Rep>(
        static_cast<std::size_t>(duty_cycle * pwm_high_voltage) / pwm_period);
}
template <std::signed_integral Rep>
constexpr Rep to_voltage(signed_duty_type duty_cycle, Rep pwm_high_voltage,
                         period_type pwm_period = DEFAULT_PERIOD) {
    return static_cast<Rep>(
        static_cast<std::ptrdiff_t>(duty_cycle * pwm_high_voltage) /
        pwm_period);
}
template <std::floating_point Rep>
constexpr Rep to_voltage(signed_duty_type duty_cycle, Rep pwm_high_voltage,
                         period_type pwm_period = DEFAULT_PERIOD) {
    return static_cast<Rep>(duty_cycle * pwm_high_voltage / pwm_period);
}

template <typename Rep>
    requires std::signed_integral<Rep> || std::floating_point<Rep>
class voltage_controller {
   public:
    struct state_t {
        Rep pwm_high_voltage;
        signed_duty_type min_pwm;
        signed_duty_type max_pwm;

        Rep max_voltage() const {
            return to_voltage(max_pwm, pwm_high_voltage);
        }

        Rep min_voltage() const {
            return to_voltage(min_pwm, pwm_high_voltage);
        }

        static state_t from_voltage_range(Rep pwm_high_voltage,
                                          Rep controller_min_voltage,
                                          Rep controller_max_voltage) {
            return state_t{
                .pwm_high_voltage = pwm_high_voltage,
                .min_pwm =
                    to_duty_type(controller_min_voltage, pwm_high_voltage),
                .max_pwm =
                    to_duty_type(controller_max_voltage, pwm_high_voltage),
            };
        }
    };

    /// \brief Obtains the voltage of the channel.
    Rep get_voltage() const {
        return to_voltage(_proxy.state().to_signed_duty(),
                          _state.pwm_high_voltage);
    }

    /**
     * Sets the voltage of the channel.
     * The input voltage will be clamped to the state object's
     * min and max voltages.
     * \param[in]   voltage The voltage to set.
     */
    void set_voltage(Rep voltage) const {
        _proxy.set_state_voltage_pattern(channel_state::from(std::clamp(
            _state.min_pwm, to_duty_type(voltage, _state.pwm_high_voltage),
            _state.max_pwm)));
    }

    const state_t& state() const { return _state; }
    const struct channel_state channel_state() const { return _proxy.state(); }

    voltage_controller(state_t& state, stateful_channel_proxy& proxy)
        : _state(state), _proxy(proxy) {}

   private:
    state_t& _state;
    stateful_channel_proxy& _proxy;
};

// (sbenish):  Some code for a voltage playback module that may be useful in the
// future.
/// \brief The state for the \ref{voltage_playback} structure.
// struct voltage_playback_state {
//     struct sample {
//         TickType_t duration;
//         signed_duty_type duty;
//     };
//
//     std::span<sample> samples;
//     std::span<sample>::const_iterator current_sample;
//     TickType_t current_sample_created;
//     signed_duty_type duty_when_stopped;
// };
//
// class voltage_playback {
// public:
//     /// \brief Starts the playback.
//     void start();
//
//     /// \brief Ends the playback.
//     void stop();
//
//     /// \brief Restarts the playback.
//     void restart();
//
//     void service(TickType_t now);
//
//     voltage_playback(stateful_channel_proxy proxy, voltage_playback_state&
//     state);
//
//   private:
//     stateful_channel_proxy _proxy;
//     voltage_playback_state& _state;
// };

}  // namespace drivers::cpld::shutter_module

// EOF
