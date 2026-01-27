/**
 * \file thorlabs-shutters.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 06-12-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <tuple>

#include "shutter-controller.hh"

namespace cards::shutter::presets {
using preset = std::tuple<controller, controller::creation_options>;
/**
 * Creates the controller for the SH1 shutter.
 * The SH1 shutter has a pulse-hold open state and a grounded closed state.
 * The interal spring guarantees that grounded (and unpowered) state closes the
 * shutter. Therefore, startup moves the shutter to the closed state.
 */
inline preset
create_model_SH1(drivers::cpld::shutter_module::stateful_channel_proxy &proxy) {
    namespace sm = drivers::cpld::shutter_module;

    static constexpr int PWM_VOLTAGE_MV = 24000;
    static constexpr int HOLD_VOLTAGE_MV = 10000;
    static constexpr int PULSE_VOLTAGE_MV = 24000;
    static constexpr int PULSE_DURATION_MS = 10;

    static constexpr sm::signed_duty_type SH1_PULSE_DUTY =
        sm::to_duty_type(PULSE_VOLTAGE_MV, PWM_VOLTAGE_MV);
    static constexpr sm::signed_duty_type SH1_HOLD_DUTY =
        sm::to_duty_type(HOLD_VOLTAGE_MV, PWM_VOLTAGE_MV);

    static constexpr controller::state_waveform OPEN_WAVEFORM =
        controller::state_waveform::create_pulse_hold(
            true, SH1_HOLD_DUTY, SH1_PULSE_DUTY,
            pdMS_TO_TICKS(PULSE_DURATION_MS));
    static constexpr controller::state_waveform CLOSED_WAVEFORM =
        controller::state_waveform::create_grounded();
    static constexpr controller::creation_options OPTIONS =
        controller::creation_options{
            .starting_position = controller::states_e::CLOSED,
        };

    return std::tuple(
        controller::create(proxy, OPEN_WAVEFORM, CLOSED_WAVEFORM, OPTIONS),
        OPTIONS);
}

/**
 * Creates the controller for the SH05 shutter.
 * The SH05 shutter has a pulse-hold open state and a grounded closed state.
 * The interal spring guarantees that grounded (and unpowered) state closes the
 * shutter. Therefore, startup moves the shutter to the closed state.
 */
inline preset create_model_SH05(
    drivers::cpld::shutter_module::stateful_channel_proxy &proxy) {
    namespace sm = drivers::cpld::shutter_module;

    static constexpr int PWM_VOLTAGE_MV = 24000;
    static constexpr int HOLD_VOLTAGE_MV = 10000;
    static constexpr int PULSE_VOLTAGE_MV = 24000;
    static constexpr int PULSE_DURATION_MS = 5;

    static constexpr sm::signed_duty_type SH1_PULSE_DUTY =
        sm::to_duty_type(PULSE_VOLTAGE_MV, PWM_VOLTAGE_MV);
    static constexpr sm::signed_duty_type SH1_HOLD_DUTY =
        sm::to_duty_type(HOLD_VOLTAGE_MV, PWM_VOLTAGE_MV);

    static constexpr controller::state_waveform OPEN_WAVEFORM =
        controller::state_waveform::create_pulse_hold(
            true, SH1_HOLD_DUTY, SH1_PULSE_DUTY,
            pdMS_TO_TICKS(PULSE_DURATION_MS));
    static constexpr controller::state_waveform CLOSED_WAVEFORM =
        controller::state_waveform::create_grounded();
    static constexpr controller::creation_options OPTIONS =
        controller::creation_options{
            .starting_position = controller::states_e::CLOSED,
        };

    return std::tuple(
        controller::create(proxy, OPEN_WAVEFORM, CLOSED_WAVEFORM, OPTIONS),
        OPTIONS);
}
} // namespace cards::shutter::presets

// EOF
