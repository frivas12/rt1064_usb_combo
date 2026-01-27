/**
 * \file shutter-controller.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-07
 *
 * Container for logic to control a shutter.
 * A shutter is defined as a bi-positional device
 * that starts in some state (default:  CLOSED).
 *
 * When openning the shutter, a "actuation" voltage is applied
 * over the shutter for some duration of time.
 * Then, a "holding" voltage is applied.
 * When no voltage is applied, the shutter moves to a "closed" state.
 *
 * The shutter is attached to the CPLD shutter outputs of the same channel.
 * When its phase is forward (default: phase == false), then it sees a postive
 * voltage.
 * When its phase is backwards (default:  phase == true), then it sees
 * a negative voltage.
 */
#pragma once

#include "cpld-shutter-driver.hh"
#include "shutter-types.hh"
#include "spi-transfer-handle.hh"

namespace cards::shutter {

class controller {
   public:
    using duty_type        = drivers::cpld::shutter_module::duty_type;
    using signed_duty_type = drivers::cpld::shutter_module::signed_duty_type;

    enum class states_e {
        UNKNOWN =
            static_cast<int>(drivers::apt::shutter_types::states_e::UNKNOWN),
        CLOSED =
            static_cast<int>(drivers::apt::shutter_types::states_e::CLOSED),
        OPEN = static_cast<int>(drivers::apt::shutter_types::states_e::OPEN),
        OPENING =
            static_cast<int>(drivers::apt::shutter_types::states_e::OPENING),
        CLOSING =
            static_cast<int>(drivers::apt::shutter_types::states_e::CLOSING),

        GROUNDED,
    };

    static constexpr drivers::apt::shutter_types::states_e to_apt_state(
        states_e state) {
        using apt_states = drivers::apt::shutter_types::states_e;
        if (state == states_e::GROUNDED) {
            return apt_states::UNKNOWN;
        } else {
            return static_cast<apt_states>(state);
        }
    }

    static constexpr states_e from_apt_state(
        drivers::apt::shutter_types::states_e state) {
        return static_cast<states_e>(state);
    }

    /// \brief Schedules the shutter to open.
    void async_open();

    /// \brief Schedules the shutter to close.
    void async_close();

    /// \brief Schedules the shutter to toggle its state.
    void async_toggle();

    /// \brief Schedules the shutter to enter its ground state.
    void async_ground();

    /// \brief Indicates the current state.
    states_e current_state() const;

    /// \brief Indicates the target state.
    states_e target_state() const;

    /// \brief Services the shutter.
    void service(drivers::cpld::shutter_module::stateful_channel_proxy& proxy,
                 TickType_t now);

    /// \brief Returns the remaining amount of time of the actuation.
    ///        If the device is not actuating, the sentinel portMAX_DELAY
    ///        will be sent instead.
    TickType_t get_remaining_actuation_time(TickType_t now) const;

    /**
     * The open and closed state can be actuated using a state waveform.
     */
    struct state_waveform {
        TickType_t actuation_duration = 0;

        /// The amount of time that can pass until the acutation can be
        /// prematurely aborted.
        TickType_t actuation_holdoff = 0;
        duty_type actuation_duty     = 0;
        duty_type holding_duty       = 0;
        bool polarity                = 0;

        bool operator==(const state_waveform&) const = default;

        constexpr signed_duty_type signed_actuation_duty() const {
            const auto RT =
                drivers::cpld::shutter_module::add_sign(actuation_duty);
            return polarity ? -RT : RT;
        }

        constexpr signed_duty_type signed_holding_duty() const {
            const auto RT =
                drivers::cpld::shutter_module::add_sign(holding_duty);
            return polarity ? -RT : RT;
        }

        /// @brief Creates a state waveform that grounds the shutter.
        static constexpr state_waveform create_grounded() {
            return state_waveform{
                .actuation_duration = 0,
                .actuation_holdoff  = 0,
                .actuation_duty     = 0,
                .holding_duty       = 0,
                .polarity           = false,
            };
        }
        constexpr bool is_grounded() const {
            return actuation_duty == 0 && holding_duty == 0;
        }

        /// @brief Creates a state waveform that only holds at a constant level.
        static constexpr state_waveform create_holding(bool polarity,
                                                       duty_type holding_duty) {
            return state_waveform{
                .actuation_duration = 0,
                .actuation_holdoff  = 0,
                .actuation_duty     = 0,
                .holding_duty       = holding_duty,
                .polarity           = polarity,
            };
        }
        static constexpr state_waveform create_holding(
            const drivers::cpld::shutter_module::channel_state& state) {
            return create_holding(state.phase_polarity, state.duty_cycle);
        }
        static constexpr state_waveform create_holding(
            signed_duty_type signed_duty) {
            return create_holding(
                drivers::cpld::shutter_module::channel_state::from(
                    signed_duty));
        }

        /// @brief Creates a state waveform that only sends a pulse for a period
        /// of time and returns grounded.
        static constexpr state_waveform create_pulsed(bool polarity,
                                                      duty_type actuation_duty,
                                                      TickType_t pulse_width) {
            return state_waveform{
                .actuation_duration = pulse_width,
                .actuation_holdoff  = 0,
                .actuation_duty     = actuation_duty,
                .holding_duty       = 0,
                .polarity           = polarity,
            };
        }
        static constexpr state_waveform create_pulsed(
            const drivers::cpld::shutter_module::channel_state& state,
            TickType_t pulse_width) {
            return create_pulsed(state.phase_polarity, state.duty_cycle,
                                 pulse_width);
        }
        static constexpr state_waveform create_pulsed(
            signed_duty_type signed_duty, TickType_t pulse_width) {
            return create_pulsed(
                drivers::cpld::shutter_module::channel_state::from(signed_duty),
                pulse_width);
        }

        /// @brief Creates a state waveform that first sends a pulse for a
        /// period of time, then holds at constant level.
        static constexpr state_waveform create_pulse_hold(
            bool polarity, duty_type holding_duty, duty_type actuation_duty,
            TickType_t pulse_width) {
            return state_waveform{
                .actuation_duration = pulse_width,
                .actuation_holdoff  = 0,
                .actuation_duty     = actuation_duty,
                .holding_duty       = holding_duty,
                .polarity           = polarity,
            };
        }
    };

    const state_waveform& open_waveform() const;
    const state_waveform& closed_waveform() const;

    struct creation_options {
        states_e starting_position = states_e::UNKNOWN;
        drivers::cpld::shutter_module::period_type pwm_period =
            drivers::cpld::shutter_module::DEFAULT_PERIOD;
    };

    static controller create(
        drivers::cpld::shutter_module::stateful_channel_proxy& proxy,
        const state_waveform& open_waveform,
        const state_waveform& closed_waveform, const creation_options& options);

    static controller create(
        drivers::cpld::shutter_module::stateful_channel_proxy& proxy,
        const state_waveform& open_waveform,
        const state_waveform& closed_waveform) {
        return create(proxy, open_waveform, closed_waveform, {});
    }

   protected:
    controller(const state_waveform& open_waveform,
               const state_waveform& closed_waveform,
               const creation_options& options);

   private:
    state_waveform _open_waveform;
    state_waveform _closed_waveform;
    TickType_t _last_actuation_began;
    states_e _fsm_state;
    states_e _target_state;
};

}  // namespace cards::shutter

// EOF
