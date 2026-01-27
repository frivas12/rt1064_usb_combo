#include "fs-mirror-module.hh"

#include "flipper-shutter-persistence.hh"
#include "mirror-types.hh"

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
modules::mirror::state::state(const mirror::config& cfg)
    : target_level(cfg.default_level), enabled(true) {}

void modules::mirror::activate() noexcept {
    if (_state) {
        deactivate();
    }
    _state.emplace(_config);
}

void modules::mirror::deactivate() noexcept {
    // Ground the shutter before destroying the driver.
    _state.reset();
}

bool cards::flipper_shutter::modules::apt_handler(
    mirror& module,
    const drivers::apt::mod_chanenablestate::payload_type& command) {
    if (!module.is_active()) {
        return false;
    }

    module.get_state()->enabled = command.is_enabled;
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    const mirror& module,
    const drivers::apt::mod_chanenablestate::request_type& command,
    drivers::apt::mod_chanenablestate::payload_type& response) {
    response.channel    = command.channel;
    response.is_enabled = module.is_active() && module.get_state()->enabled;
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    mirror& module,
    const drivers::apt::mcm_mirror_state::payload_type& command) {
    if (!module.is_active()) {
        return false;
    }

    if (module.get_state()->enabled) {
        switch (command.state) {
            using s = drivers::apt::mirror_types::states_e;
        case s::OUT:
        case s::IN:
            module.get_state()->set_position(module.get_config(),
                                             command.state);
            break;
        default:
            break;
        }
    }

    return true;
}
bool cards::flipper_shutter::modules::apt_handler(
    const mirror& module,
    const drivers::apt::mcm_mirror_state::request_type& command,
    drivers::apt::mcm_mirror_state::payload_type& response) {
    response.channel = command.channel;
    response.state =
        !module.is_active()
            ? drivers::apt::mirror_types::states_e::UNKNOWN
            : module.get_state()->get_position(module.get_config());
    return true;
}

bool cards::flipper_shutter::modules::apt_handler(
    const mirror& module,
    const drivers::apt::mcm_mirror_params::request_type& command,
    drivers::apt::mcm_mirror_params::payload_type& response) {
    response.channel = command.channel;
    response.initial_state =
        drivers::apt::mirror_types::states_e::IN;  // Due to pull-up resistor.
    response.mode         = drivers::apt::mirror_types::modes_e::SIGNAL,
    response.pwm_low_val  = 0;
    response.pwm_high_val = 0;

    return true;
}

void persistence::load_config(modules::mirror::config& dest,
                              const persistence::sio_settings& src) noexcept {
    dest.default_level    = src.drives_high;
    dest.is_logic_high_in = src.is_high_level_in_position;
}

void persistence::store_config(persistence::sio_settings& dest,
                               const modules::mirror::config& src) noexcept {
    dest.drives_high               = src.default_level;
    dest.is_high_level_in_position = src.is_logic_high_in;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
void modules::mirror::update_state_from_config() noexcept {
    if (!_state) {
        return;
    }

    // state& obj = *_state;

    // If anything needs to be done in the future to
    // trigger reset logic, do it here.
    // const bool RESET = false;
    //
    // if (RESET) {
    //     activate();
    // }
}
