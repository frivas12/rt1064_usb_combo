/**
 * \file cpld-shutter-driver.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "cpld-shutter-driver.hh"

#include "cpld.hh"

using namespace drivers::cpld::shutter_module;

void drivers::cpld::shutter_module::disable_module(
    drivers::spi::handle_factory& factory, slot_nums slot) {
    write_register(
        factory,
        message_header{.command = commands_e::SET_ENABLE_SHUTTER_CARD,
                       .address = to_address(slot)},
        message_payload{
            .data     = static_cast<uint32_t>(module_modes_e::DISABLED),
            .mid_data = 0,
        });
}

void drivers::cpld::shutter_module::start_listening_for_devices(
    drivers::spi::handle_factory& factory, slot_nums slot) {
    write_register(
        factory,
        message_header{.command = commands_e::SET_ENABLE_SHUTTER_CARD,
                       .address = to_address(slot)},
        message_payload{
            .data     = static_cast<uint32_t>(module_modes_e::ONE_WIRE),
            .mid_data = 0,
        });
}

void drivers::cpld::shutter_module::enable_module(
    drivers::spi::handle_factory& factory, slot_nums slot) {
    write_register(
        factory,
        message_header{.command = commands_e::SET_ENABLE_SHUTTER_CARD,
                       .address = to_address(slot)},
        message_payload{
            .data     = static_cast<uint32_t>(module_modes_e::ENABLED),
            .mid_data = 0,
        });
}

void drivers::cpld::shutter_module::set_duty_cycle(
    drivers::spi::handle_factory& factory, slot_nums slot, channel_id channel,
    duty_type duty_cycle) {
    write_register(factory,
                   message_header{.command = commands_e::SET_SUTTER_PWM_DUTY,
                                  .address = to_address(slot)},
                   message_payload{
                       .data = static_cast<uint32_t>(duty_cycle) |
                               (static_cast<uint32_t>(channel) << 16),
                       .mid_data = 0,
                   });
}

void drivers::cpld::shutter_module::set_phase_polarity(
    drivers::spi::handle_factory& factory, slot_nums slot, channel_id channel,
    bool phase_polarity) {
    write_register(factory,
                   message_header{.command = commands_e::SET_SUTTER_PHASE_DUTY,
                                  .address = to_address(slot)},
                   message_payload{
                       .data = static_cast<uint32_t>(phase_polarity ? 1 : 0) |
                               (static_cast<uint32_t>(channel) << 1),
                       .mid_data = 0,
                   });
}
void drivers::cpld::shutter_module::set_period(
    drivers::spi::handle_factory& factory, slot_nums slot, period_type value) {
    write_register(factory,
                   message_header{.command = commands_e::SET_SUTTER_PWM_PERIOD,
                                  .address = to_address(slot)},
                   message_payload{
                       .data = static_cast<uint32_t>(
                           drivers::cpld::shutter_module::clamp_period(value)),
                       .mid_data = 0,
                   });
}

void stateful_channel_proxy::set_duty_cycle(duty_type duty_cycle) {
    // Clamping the duty cycle.
    duty_cycle = std::clamp<duty_type>(duty_cycle, 0, DEFAULT_PERIOD);

    drivers::cpld::shutter_module::set_duty_cycle(_factory, _slot, _channel,
                                                  duty_cycle);
    _state.duty_cycle = duty_cycle;
}
void stateful_channel_proxy::set_phase_polarity(bool phase_polarity) {
    drivers::cpld::shutter_module::set_phase_polarity(_factory, _slot, _channel,
                                                      phase_polarity);
    _state.phase_polarity = phase_polarity;
}

void stateful_channel_proxy::set_state_voltage_pattern(channel_state state) {
    const channel_state& CURRENT = _state;
    const channel_state& TARGET  = state;

    // If a polarity needs to be flipped, flip it at the lower voltage.
    // Therefore, a decreasing absolute duty cycle will flip after
    // the new duty cycle is set, but an increase abs.
    // duty cycle will flip before the new duty cycle is set.
    if (CURRENT.phase_polarity == TARGET.phase_polarity) {
        set_duty_cycle(TARGET.duty_cycle);
    } else if (CURRENT.duty_cycle > TARGET.duty_cycle) {
        set_duty_cycle(TARGET.duty_cycle);
        set_phase_polarity(TARGET.phase_polarity);
    } else {
        set_phase_polarity(TARGET.phase_polarity);
        set_duty_cycle(TARGET.duty_cycle);
    }
}

stateful_channel_proxy::stateful_channel_proxy(channel_state& state,
                                               spi::handle_factory& factory,
                                               slot_nums slot,
                                               channel_id channel)
    : _factory(factory), _state(state), _slot(slot), _channel(channel) {}

// EOF
