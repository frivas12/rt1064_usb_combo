#include "./shutter-controller.hh"

#include "cpld-shutter-driver.hh"
#include "portmacro.h"

using namespace cards::shutter;

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
static drivers::cpld::shutter_module::channel_state to_actuation_state(
    const controller::state_waveform& waveform);
static drivers::cpld::shutter_module::channel_state to_holding_state(
    const controller::state_waveform& waveform);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void controller::async_open() { _target_state = states_e::OPEN; }

void controller::async_close() { _target_state = states_e::CLOSED; }

void controller::async_toggle() {
    switch (current_state()) {
    case states_e::OPEN:
    case states_e::OPENING:
        _target_state = states_e::CLOSED;
        break;
    case states_e::CLOSED:
    case states_e::CLOSING:
        _target_state = states_e::OPEN;
        break;

    default:
        break;
    }
}

void controller::async_ground() { _target_state = states_e::GROUNDED; }

controller::states_e controller::current_state() const { return _fsm_state; }

controller::states_e controller::target_state() const { return _target_state; }

void controller::service(
    drivers::cpld::shutter_module::stateful_channel_proxy& proxy,
    TickType_t now) {
    auto service_waveform = [&](const state_waveform& waveform,
                                states_e actuation_state,
                                states_e holding_state) {
        proxy.set_state_voltage_pattern(to_actuation_state(waveform));
        this->_fsm_state =
            waveform.actuation_duration != 0 ? actuation_state : holding_state;
        this->_last_actuation_began = now;
    };

    auto service_state_start = [this, &service_waveform, &proxy]() {
        switch (target_state()) {
        case states_e::CLOSED:
            service_waveform(_closed_waveform, states_e::CLOSING,
                             states_e::CLOSED);
            break;
        case states_e::OPEN:
            service_waveform(_open_waveform, states_e::OPENING, states_e::OPEN);
            break;
        case states_e::GROUNDED:
            proxy.set_state_voltage_pattern(
                drivers::cpld::shutter_module::channel_state{
                    .duty_cycle     = 0,
                    .phase_polarity = proxy.state().phase_polarity,
                });

            _fsm_state = states_e::GROUNDED;
            break;
        default:
            break;
        }
    };

    switch (_fsm_state) {
    case states_e::CLOSED:
    case states_e::OPEN:
    case states_e::UNKNOWN:
    case states_e::GROUNDED:
        if (target_state() != current_state()) {
            service_state_start();
        }
        break;
    case states_e::OPENING:
    case states_e::CLOSING: {
        const bool OPENING = _fsm_state == states_e::OPENING;
        const state_waveform& WAVEFORM =
            OPENING ? _open_waveform : _closed_waveform;
        const bool NEW_MOVEMENT_STATE = (_fsm_state == states_e::OPENING &&
                                         target_state() != states_e::OPEN) ||
                                        (_fsm_state == states_e::CLOSING &&
                                         target_state() != states_e::CLOSED);
        const TickType_t DELTA = now - _last_actuation_began;
        if (NEW_MOVEMENT_STATE && DELTA >= WAVEFORM.actuation_holdoff) {
            service_state_start();
        } else if (now - _last_actuation_began >= WAVEFORM.actuation_duration) {
            proxy.set_state_voltage_pattern(to_holding_state(WAVEFORM));
            _fsm_state = OPENING ? states_e::OPEN : states_e::CLOSED;
        }
    } break;
    }
}

TickType_t controller::get_remaining_actuation_time(TickType_t now) const {
    auto get_time = [&](const state_waveform& waveform) {
        const TickType_t DELTA = now - _last_actuation_began;
        return (DELTA > waveform.actuation_duration)
                   ? 0
                   : (waveform.actuation_duration - DELTA);
    };

    switch (_fsm_state) {
    case states_e::OPENING:
        return get_time(_open_waveform);
    case states_e::CLOSING:
        return get_time(_closed_waveform);

    default:
        return portMAX_DELAY;
    }
}
const controller::state_waveform& controller::open_waveform() const {
    return _open_waveform;
}
const controller::state_waveform& controller::closed_waveform() const {
    return _closed_waveform;
}

controller controller::create(
    drivers::cpld::shutter_module::stateful_channel_proxy& proxy,
    const state_waveform& open_waveform, const state_waveform& closed_waveform,
    const creation_options& options) {
    auto rt = controller(open_waveform, closed_waveform, options);

    // Set 0 volts across the shutter.
    proxy.set_duty_cycle(0);

    return rt;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
controller::controller(const state_waveform& open_waveform,
                       const state_waveform& closed_waveform,
                       const creation_options& options)
    : _open_waveform(open_waveform)
    , _closed_waveform(closed_waveform)
    , _last_actuation_began(0)
    , _fsm_state(states_e::GROUNDED)
    , _target_state(options.starting_position) {
    // Clamp the duties to their max values.
    _open_waveform.actuation_duty =
        std::clamp<drivers::cpld::shutter_module::duty_type>(
            _open_waveform.actuation_duty, 0, options.pwm_period);
    _open_waveform.holding_duty =
        std::clamp<drivers::cpld::shutter_module::duty_type>(
            _open_waveform.holding_duty, 0, options.pwm_period);

    _closed_waveform.actuation_duty =
        std::clamp<drivers::cpld::shutter_module::duty_type>(
            _closed_waveform.actuation_duty, 0, options.pwm_period);
    _closed_waveform.holding_duty =
        std::clamp<drivers::cpld::shutter_module::duty_type>(
            _closed_waveform.holding_duty, 0, options.pwm_period);
}

static drivers::cpld::shutter_module::channel_state to_actuation_state(
    const controller::state_waveform& waveform) {
    return drivers::cpld::shutter_module::channel_state{
        .duty_cycle = waveform.actuation_duration != 0 ? waveform.actuation_duty
                                                       : waveform.holding_duty,
        .phase_polarity = waveform.polarity,
    };
}

static drivers::cpld::shutter_module::channel_state to_holding_state(
    const controller::state_waveform& waveform) {
    return drivers::cpld::shutter_module::channel_state{
        .duty_cycle     = waveform.holding_duty,
        .phase_polarity = waveform.polarity,
    };
}

// EOF
