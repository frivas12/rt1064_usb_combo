#include "fs-shutter-module.hh"

#include "mcm_shutter_params.hh"
#include "mcm_shuttercomp_params.hh"
#include "overload.hh"
#include "shutter-controller.hh"
#include "shutter-types.hh"
#include "time.hh"

using namespace cards::flipper_shutter;
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
modules::shutter::state::state(resources& resource, const shutter::config& cfg)
    : driver(controller::create(resource.proxy, cfg.open_waveform,
                                cfg.closed_waveform,
                                controller::creation_options{
                                    .starting_position = cfg.power_up_state,
                                    .pwm_period        = cfg.pwm_period,
                                }))
    , enabled(true) {}

void modules::shutter::activate(resources& resource) noexcept {
    if (_state) {
        deactivate(resource);
    }
    _state.emplace(resource, _config);
}

void modules::shutter::deactivate(resources& resource) noexcept {
    // Ground the shutter before destroying the driver.
    _state->driver.async_ground();
    _state->driver.service(resource.proxy, 0);

    _state.reset();
}

bool cards::flipper_shutter::modules::apt_handler(
    shutter& module,
    const drivers::apt::mod_chanenablestate::payload_type& command) {
    if (!module.is_active()) {
        return false;
    }

    module.get_state()->enabled = command.is_enabled;
    return true;
}
bool cards::flipper_shutter::modules::apt_handler(
    const shutter& module,
    const drivers::apt::mod_chanenablestate::request_type& command,
    drivers::apt::mod_chanenablestate::payload_type& response) {
    response.channel    = command.channel;
    response.is_enabled = module.is_active() && module.get_state()->enabled;
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    shutter& module,
    const drivers::apt::mcm_shutter_params::payload_type& command,
    shutter::resources& resources, bool block_activations) {
    using stypes = drivers::apt::shutter_types::types_e;
    using ctrler = cards::shutter::controller;

    if (command.type == stypes::NONE) {
        module.deactivate(resources);
        module.with_config_if_inactive([](auto& config) {
            config = modules::shutter::config::get_default();
        });
    } else {
        const bool IS_ACTIVE = module.with_config(
            [&command](modules::shutter::config& config) {
                config.trigger_mode = command.external_trigger_mode;
                config.power_up_state =
                    static_cast<ctrler::states_e>(command.initial_state);

                // Default: polarity = false -> voltage is -

                const bool OPEN_POLARITY = std::visit(
                    overload([](std::monostate) { return true; },
                             [](const drivers::apt::mcm_shutter_params::
                                    payload_type::variant_len15& variant) {
                                 // Reverse open level = false -> OPEN_POLARITY
                                 // = false
                                 return !variant.reverse_open_level;
                             },
                             [](const drivers::apt::mcm_shutter_params::
                                    payload_type::variant_len19& variant) {
                                 // Reverse open level = false -> OPEN_POLARITY
                                 // = false
                                 return !variant.reverse_open_level;
                             }),
                    command.optional_data);
                const TickType_t HOLD_TIME = std::visit(
                    overload([](std::monostate) -> TickType_t { return 0; },
                             [](const drivers::apt::mcm_shutter_params::
                                    payload_type::variant_len15&)
                                 -> TickType_t { return 0; },
                             [](const drivers::apt::mcm_shutter_params::
                                    payload_type::variant_len19& variant)
                                 -> TickType_t {
                                 return utils::time::to_ticks(
                                     variant.actuation_holdoff_time);
                             }),
                    command.optional_data);

                switch (command.type) {
                case stypes::PULSED:
                    config.open_waveform =
                        ctrler::state_waveform::create_pulsed(
                            OPEN_POLARITY, command.duty_cycle_pulse,
                            utils::time::to_ticks(command.on_time));
                    config.closed_waveform =
                        ctrler::state_waveform::create_pulsed(
                            !OPEN_POLARITY, command.duty_cycle_pulse,
                            utils::time::to_ticks(command.on_time));

                    config.open_waveform.actuation_holdoff   = HOLD_TIME;
                    config.closed_waveform.actuation_holdoff = HOLD_TIME;
                    return true;

                case stypes::PULSE_HOLD:
                    config.open_waveform =
                        ctrler::state_waveform::create_pulse_hold(
                            OPEN_POLARITY, command.duty_cycle_hold,
                            command.duty_cycle_pulse,
                            utils::time::to_ticks(command.on_time));
                    config.closed_waveform =
                        ctrler::state_waveform::create_pulse_hold(
                            !OPEN_POLARITY, command.duty_cycle_hold,
                            command.duty_cycle_pulse,
                            utils::time::to_ticks(command.on_time));

                    config.open_waveform.actuation_holdoff   = HOLD_TIME;
                    config.closed_waveform.actuation_holdoff = HOLD_TIME;
                    return true;

                case stypes::PULSE_NO_REVERSE:
                    config.open_waveform =
                        ctrler::state_waveform::create_pulse_hold(
                            OPEN_POLARITY, command.duty_cycle_hold,
                            command.duty_cycle_pulse,
                            utils::time::to_ticks(command.on_time));
                    config.closed_waveform =
                        ctrler::state_waveform::create_grounded();

                    config.open_waveform.actuation_holdoff   = HOLD_TIME;
                    config.closed_waveform.actuation_holdoff = HOLD_TIME;
                    return true;

                case stypes::NONE:
                default:
                    return false;
                }
            },
            resources);

        if (!module.is_active() && IS_ACTIVE && !block_activations) {
            module.activate(resources);
        }
    }

    return true;
}
bool cards::flipper_shutter::modules::apt_handler(
    const shutter& module,
    const drivers::apt::mcm_shutter_params::request_type& command,
    drivers::apt::mcm_shutter_params::payload_type& response) {
    namespace st = drivers::apt::shutter_types;

    const auto& CONFIG = module.get_config();
    switch (CONFIG.power_up_state) {
        using pups = cards::shutter::controller::states_e;
        using apts = st::states_e;

    case pups::CLOSED:
        response.initial_state = apts::CLOSED;
        break;

    case pups::OPEN:
    default:
        response.initial_state = apts::OPEN;
        break;
    }

    const bool PULSE_DUTIES_THE_SAME = CONFIG.open_waveform.actuation_duty ==
                                       CONFIG.closed_waveform.actuation_duty;
    const bool HOLD_DUTIES_ARE_THE_SAME = CONFIG.open_waveform.holding_duty ==
                                          CONFIG.closed_waveform.holding_duty;
    const bool HOLD_DUTIES_ARE_ZERO  = CONFIG.open_waveform.holding_duty == 0;
    const bool PULSE_DUTIES_ARE_ZERO = CONFIG.open_waveform.actuation_duty == 0;
    if (PULSE_DUTIES_ARE_ZERO && HOLD_DUTIES_ARE_ZERO) {
        response.type = st::types_e::NONE;
    } else if (PULSE_DUTIES_THE_SAME && HOLD_DUTIES_ARE_THE_SAME) {
        response.type = HOLD_DUTIES_ARE_ZERO ? st::types_e::PULSED
                                             : st::types_e::PULSE_HOLD;
    } else {
        response.type = st::types_e::PULSE_NO_REVERSE;
    }

    response.external_trigger_mode = CONFIG.trigger_mode;
    response.duty_cycle_pulse      = CONFIG.open_waveform.actuation_duty;
    response.duty_cycle_hold       = CONFIG.open_waveform.holding_duty;
    response.channel               = command.channel;
    response.on_time = utils::time::to_duration<decltype(response.on_time)>(
        CONFIG.open_waveform.actuation_duration);

    /**
     * Default Polarity:
     * open=false/grounded
     * closed=true
     */
    const bool IS_OPEN_LEVEL_DEFAULT = CONFIG.open_waveform.polarity;
    if ((CONFIG.closed_waveform.polarity == !CONFIG.open_waveform.polarity) ||
        CONFIG.closed_waveform.is_grounded()) {
        response.optional_data =
            drivers::apt::mcm_shutter_params::payload_type::variant_len19{
                .actuation_holdoff_time = utils::time::to_duration<
                    utils::time::hundreds_of_us<uint32_t>>(
                    CONFIG.open_waveform.actuation_holdoff),
                .reverse_open_level = !IS_OPEN_LEVEL_DEFAULT,
            };
    } else {
        response.optional_data = {};
    }
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    shutter& module,
    const drivers::apt::mot_solenoid_state::payload_type& command) {
    if (!module.is_active()) {
        return false;
    }

    if (module.get_state()->enabled) {
        if (command.solenoid_on) {
            module.get_state()->driver.async_open();
        } else {
            module.get_state()->driver.async_close();
        }
    }

    return true;
}
bool cards::flipper_shutter::modules::apt_handler(
    const shutter& module,
    const drivers::apt::mot_solenoid_state::request_type& command,
    drivers::apt::mot_solenoid_state::payload_type& response) {
    response.channel = command.channel;
    response.solenoid_on =
        module.is_active() && module.get_state()->driver.current_state() !=
                                  cards::shutter::controller::states_e::CLOSED;
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    shutter& module,
    const drivers::apt::mcm_shuttercomp_params::payload_type& command,
    shutter::resources& resources, bool block_activations) {
    const bool OPEN_HAS_SAME_SIGN = (command.open_actuation_voltage >= 0 &&
                                     command.opened_hold_voltage >= 0) ||
                                    (command.open_actuation_voltage <= 0 &&
                                     command.opened_hold_voltage <= 0);
    const bool CLOSE_HAS_SAME_SIGN = (command.close_actuation_voltage >= 0 &&
                                      command.closed_hold_voltage >= 0) ||
                                     (command.close_actuation_voltage <= 0 &&
                                      command.closed_hold_voltage <= 0);

    // The holding and acutation voltages cannot have different signs.
    if (!OPEN_HAS_SAME_SIGN || !CLOSE_HAS_SAME_SIGN ||
        command.open_actuation_time < 0 || command.close_actuation_time < 0) {
        return false;
    }

    module.with_config(
        [&](shutter::config& config) {
            auto voltage_to_duty =
                [MAX  = config.max_voltage(),
                 COEF = config.voltage_coefficient()](
                    float voltage) -> shutter::controller::duty_type {
                if (voltage < 0) {
                    voltage = -voltage;
                }
                if (voltage > MAX) {
                    voltage = MAX;
                }
                return static_cast<shutter::controller::duty_type>(voltage /
                                                                   COEF);
            };
            auto get_polarity = [](float actuation_voltage,
                                   float holding_voltage) {
                if (actuation_voltage == 0) {
                    return holding_voltage < 0;
                }
                return actuation_voltage < 0;
            };
            config.open_waveform.actuation_duty =
                voltage_to_duty(command.open_actuation_voltage);
            config.open_waveform.actuation_duration =
                utils::time::bound_seconds_to_ticks(
                    command.open_actuation_time);
            config.open_waveform.actuation_holdoff =
                utils::time::bound_seconds_to_ticks(command.open_holdoff_time);
            config.open_waveform.holding_duty =
                voltage_to_duty(command.open_actuation_voltage);
            config.open_waveform.polarity = get_polarity(
                command.open_actuation_voltage, command.opened_hold_voltage);
            config.closed_waveform.actuation_duty =
                voltage_to_duty(command.close_actuation_voltage);
            config.closed_waveform.actuation_duration =
                utils::time::bound_seconds_to_ticks(
                    command.close_actuation_time);
            config.closed_waveform.actuation_holdoff =
                utils::time::bound_seconds_to_ticks(command.close_holdoff_time);
            config.closed_waveform.holding_duty =
                voltage_to_duty(command.close_actuation_voltage);
            config.closed_waveform.polarity = get_polarity(
                command.close_actuation_voltage, command.closed_hold_voltage);
            config.trigger_mode = command.trigger_mode;
            config.power_up_state =
                static_cast<cards::shutter::controller::states_e>(
                    command.power_up_state);
        },
        resources);

    const bool GROUNDED_WAVEFORM =
        module.get_config().open_waveform.actuation_duty == 0 &&
        module.get_config().open_waveform.holding_duty == 0 &&
        module.get_config().closed_waveform.actuation_duty == 0 &&
        module.get_config().closed_waveform.holding_duty == 0;

    if (GROUNDED_WAVEFORM) {
        module.deactivate(resources);
    } else if (!module.is_active() && !block_activations) {
        module.activate(resources);
    }
    return true;
}
bool cards::flipper_shutter::modules::apt_handler(
    const shutter& module,
    const drivers::apt::mcm_shuttercomp_params::request_type& command,
    drivers::apt::mcm_shuttercomp_params::payload_type& response) {
    const auto& CONFIG = module.get_config();

    response.open_actuation_voltage = CONFIG.open_waveform.actuation_duty *
                                      CONFIG.voltage_coefficient() *
                                      (CONFIG.open_waveform.polarity ? -1 : 1);
    response.open_actuation_time = CONFIG.open_waveform.actuation_duration /
                                   (float)utils::time::TICKS_PER_SECOND;
    response.open_holdoff_time = CONFIG.open_waveform.actuation_holdoff /
                                 (float)utils::time::TICKS_PER_SECOND;
    response.opened_hold_voltage = CONFIG.open_waveform.holding_duty *
                                   CONFIG.voltage_coefficient() *
                                   (CONFIG.open_waveform.polarity ? -1 : 1);
    response.close_actuation_voltage =
        CONFIG.closed_waveform.actuation_duty * CONFIG.voltage_coefficient();
    response.close_actuation_time = CONFIG.closed_waveform.actuation_duration /
                                    (float)utils::time::TICKS_PER_SECOND;
    response.close_holdoff_time = CONFIG.closed_waveform.actuation_holdoff /
                                  (float)utils::time::TICKS_PER_SECOND;
    response.closed_hold_voltage = CONFIG.closed_waveform.holding_duty *
                                   CONFIG.voltage_coefficient() *
                                   (CONFIG.open_waveform.polarity ? -1 : 1);
    response.channel = command.channel;
    response.power_up_state =
        static_cast<drivers::apt::shutter_types::states_e>(
            CONFIG.power_up_state);
    response.trigger_mode = CONFIG.trigger_mode;

    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    shutter& module,
    const drivers::apt::mcm_shutter_trigger::payload_type& command,
    shutter::resources& resources) {
    module.with_config([MODE = command.mode](
                           shutter::config& cfg) { cfg.trigger_mode = MODE; },
                       resources);
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    const shutter& module,
    const drivers::apt::mcm_shutter_trigger::request_type& command,
    drivers::apt::mcm_shutter_trigger::payload_type& response)
{
    const auto& CFG = module.get_config();

    response.mode = CFG.trigger_mode;
    return true;
}

void persistence::load_config(
    modules::shutter::config& dest, const persistence::shutter_settings& src,
    const persistence::envir_context& context) noexcept {
    dest.open_waveform = src.open_waveform.to_state_waveform(
        context.bus_voltage, context.pwm_period);
    dest.closed_waveform = src.closed_waveform.to_state_waveform(
        context.bus_voltage, context.pwm_period);
    dest.trigger_mode   = src.trigger_mode;
    dest.power_up_state = src.state_at_powerup;
    dest.bus_voltage    = context.bus_voltage;
    dest.pwm_period     = context.pwm_period;
}

void persistence::store_config(
    persistence::shutter_settings& dest, const modules::shutter::config& src,
    const persistence::envir_context& context) noexcept {
    dest.open_waveform.from_state_waveform(
        src.open_waveform, context.bus_voltage, context.pwm_period);
    dest.closed_waveform.from_state_waveform(
        src.closed_waveform, context.bus_voltage, context.pwm_period);
    dest.trigger_mode     = src.trigger_mode;
    dest.state_at_powerup = src.power_up_state;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
void modules::shutter::update_state_from_config(resources& resource) noexcept {
    if (!_state) {
        return;
    }

    state& obj = *_state;

    const bool RESET =
        (obj.driver.open_waveform() != _config.open_waveform) ||
        (obj.driver.closed_waveform() != _config.closed_waveform);

    if (RESET) {
        activate(resource);
    }
}
