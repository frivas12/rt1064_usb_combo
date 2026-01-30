#pragma once

#include <limits>

#include "cpld-shutter-driver.hh"
#include "flipper-shutter-persistence.hh"
#include "flipper-shutter-types.hh"
#include "mcm_shutter_params.hh"
#include "mcm_shutter_trigger.hh"
#include "mcm_shuttercomp_params.hh"
#include "mod_chanenablestate.hh"
#include "mot_solenoid_state.hh"
#include "shutter-controller.hh"

/**
 * Configuration and state module for the flipper-shutter's shutter
 * channels.
 */
namespace cards::flipper_shutter {
namespace modules {
class shutter {
   public:
    using controller = cards::shutter::controller;

    struct config {
        controller::state_waveform open_waveform;
        controller::state_waveform closed_waveform;
        controller::states_e power_up_state;
        float bus_voltage;
        drivers::cpld::shutter_module::period_type pwm_period;
        trigger_modes_e trigger_mode;

        constexpr float voltage_coefficient() const {
            return bus_voltage / pwm_period;
        }

        constexpr float max_voltage() const { return bus_voltage; }

        /**
         * Changes the PWM period and also updates the duties in the open and
         * closed waveforms to their equivalent value.
         */
        constexpr config& propagate_period_change(
            drivers::cpld::shutter_module::period_type new_period) {
            if (new_period == pwm_period) {
                return *this;
            }

            auto adjust_waveform = [OLD = pwm_period, NEW = new_period](
                                       controller::state_waveform& waveform) {
                waveform.actuation_duty =
                    drivers::cpld::shutter_module::get_equivalent_duty(
                        waveform.actuation_duty, OLD, NEW);
                waveform.holding_duty =
                    drivers::cpld::shutter_module::get_equivalent_duty(
                        waveform.holding_duty, OLD, NEW);
            };
            adjust_waveform(open_waveform);
            adjust_waveform(closed_waveform);

            pwm_period = new_period;

            return *this;
        }

        static constexpr config get_default() noexcept {
            return {
                .open_waveform = controller::state_waveform::create_grounded(),
                .closed_waveform =
                    controller::state_waveform::create_grounded(),
                .power_up_state = controller::states_e::GROUNDED,
                .bus_voltage    = 24.0,
                .pwm_period     = drivers::cpld::shutter_module::DEFAULT_PERIOD,
                .trigger_mode   = trigger_modes_e::DISABLED,
            };
        }
    };

    /// \brief Resources needed to setup the state.
    struct resources {
        drivers::cpld::shutter_module::stateful_channel_proxy& proxy;
    };

    struct state {
        cards::shutter::controller driver;
        bool enabled;

        state(resources& resources, const config& cfg);
    };

    inline const config& get_config() const noexcept { return _config; }
    inline state* get_state() noexcept { return _state ? &*_state : nullptr; }
    inline const state* get_state() const noexcept {
        return _state ? &*_state : nullptr;
    };

    /// \brief If the module is active (if it has state).
    inline bool is_active() const noexcept { return _state.has_value(); }

    /// \brief Forces the module to become active.
    /// If the module is already active, it will deactive the module before
    /// re-activating.
    void activate(resources& resource) noexcept;

    /// \brief Forces the module to become inactive.
    void deactivate(resources& resource) noexcept;

    /**
     * Allows visitors to mutate the config and, if active, reloads the state
     * object.
     * \param  visitor The function that mutates the config.
     * \param  proxy   The cpld shutter-module proxy driving this channel.
     * \return The return of the visitor (if any).
     */
    auto with_config(std::invocable<config&> auto&& visitor,
                     resources& resource) noexcept {
        if constexpr (std::is_void_v<std::invoke_result_t<
                          std::decay_t<decltype(visitor)>, config&>>) {
            visitor(_config);
            update_state_from_config(resource);
        } else {
            auto rt = visitor(_config);
            update_state_from_config(resource);
            return rt;
        }
    }

    /**
     * Allows visitors to mutate the config without the proxy resource if and
     * only if the module does is inactive.
     * \param  visitor The function that mutates the config.
     * \return If the visitor callback was invoked.
     */
    bool with_config_if_inactive(
        std::invocable<config&> auto&& visitor) noexcept {
        if (is_active()) {
            return false;
        }
        visitor(_config);
        return true;
    }

   private:
    std::optional<state> _state = {};
    config _config              = config::get_default();

    void update_state_from_config(resources& resource) noexcept;
};

bool apt_handler(
    shutter& module,
    const drivers::apt::mod_chanenablestate::payload_type& command);
bool apt_handler(const shutter& module,
                 const drivers::apt::mod_chanenablestate::request_type& command,
                 drivers::apt::mod_chanenablestate::payload_type& response);

/**
 * \param block_activations Optional (default: false)
 *        If set to true, shutters modules will not transition from the
 *        inactive state to the active state.
 */
bool apt_handler(shutter& module,
                 const drivers::apt::mcm_shutter_params::payload_type& command,
                 shutter::resources& resources, bool block_activations = false);
bool apt_handler(const shutter& module,
                 const drivers::apt::mcm_shutter_params::request_type& command,
                 drivers::apt::mcm_shutter_params::payload_type& response);

bool apt_handler(shutter& module,
                 const drivers::apt::mot_solenoid_state::payload_type& command);
bool apt_handler(const shutter& module,
                 const drivers::apt::mot_solenoid_state::request_type& command,
                 drivers::apt::mot_solenoid_state::payload_type& response);

/**
 * Sets the mirror params to the best ability
 * of the hardware.
 * \note Voltage and duration values will clamp to their proper values.
 */
bool apt_handler(
    shutter& module,
    const drivers::apt::mcm_shuttercomp_params::payload_type& command,
    shutter::resources& resources, bool block_activations = false);
bool apt_handler(
    const shutter& module,
    const drivers::apt::mcm_shuttercomp_params::request_type& command,
    drivers::apt::mcm_shuttercomp_params::payload_type& response);

bool apt_handler(shutter& module,
                 const drivers::apt::mcm_shutter_trigger::payload_type& command,
                 shutter::resources& resources);
bool apt_handler(const shutter& module,
                 const drivers::apt::mcm_shutter_trigger::request_type& command,
                 drivers::apt::mcm_shutter_trigger::payload_type& response);

}  // namespace modules
namespace persistence {

struct envir_context {
    float bus_voltage;
    drivers::cpld::shutter_module::period_type pwm_period;
};

/// \brief Loads the persistence shutter settings into the module's config.
void load_config(modules::shutter::config& dest, const shutter_settings& src,
                 const envir_context& context) noexcept;

/// \brief Stores the config into shutter settings object for serialization.
void store_config(shutter_settings& dest, const modules::shutter::config& src,
                  const envir_context& context) noexcept;
}  // namespace persistence

}  // namespace cards::flipper_shutter
