/**
 * \file flipper-shutter-driver.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "flipper-shutter-driver.hh"

#include "channels.hh"
#include "cpld-shutter-driver.hh"
#include "mcp-builder.hh"
#include "mcp-driver.hh"
#include "mcp23s09.h"
#include "pin-object.hh"
#include "pins.h"

using namespace cards::flipper_shutter;
using namespace drivers::io;
using pin = drivers::io::pin;

/*****************************************************************************
 * Constants
 *****************************************************************************/
// MCP Channels
static constexpr mcp23s09_driver::channel_id CHANNEL_SIO_2    = 0;
static constexpr mcp23s09_driver::channel_id CHANNEL_SIO_1    = 1;
static constexpr mcp23s09_driver::channel_id CHANNEL_SIO_0    = 2;
static constexpr mcp23s09_driver::channel_id CHANNEL_POWER_EN = 3;
static constexpr mcp23s09_driver::channel_id CHANNEL_TRIG_1   = 4;
static constexpr mcp23s09_driver::channel_id CHANNEL_TRIG_2   = 5;
static constexpr mcp23s09_driver::channel_id CHANNEL_TRIG_3   = 6;
static constexpr mcp23s09_driver::channel_id CHANNEL_TRIG_4   = 7;

// constexpr magic to make the driver's initial conditions easy to read w/o
// decoding.
static constexpr mcp23s09_state DRIVER_INITIAL_CONDITIONS =
    drivers::io::mcp23s09_builder()
        .interrupt_as_open_drain()
        .interrupt_signals_high()
        .interrupt_clears_reading_gpio()

        // The "as_input()" ensures that only a week pull-up is used.
        .attach_gpio(CHANNEL_SIO_0,
                     drivers::io::mcp23s09_builder::gpio_builder().as_input())
        .attach_gpio(CHANNEL_SIO_1,
                     drivers::io::mcp23s09_builder::gpio_builder().as_input())
        .attach_gpio(CHANNEL_SIO_2,
                     drivers::io::mcp23s09_builder::gpio_builder().as_input())

        /// Power enable has no pull-up resistor execpt in the chip.
        .attach_gpio(CHANNEL_POWER_EN,
                     drivers::io::mcp23s09_builder::gpio_builder()
                         .as_low_output()
                         .pullup())

        .attach_gpio(CHANNEL_TRIG_1,
                     drivers::io::mcp23s09_builder::gpio_builder()
                         .as_input()
                         .interrupt_on_change())
        .attach_gpio(CHANNEL_TRIG_2,
                     drivers::io::mcp23s09_builder::gpio_builder()
                         .as_input()
                         .interrupt_on_change())
        .attach_gpio(CHANNEL_TRIG_3,
                     drivers::io::mcp23s09_builder::gpio_builder()
                         .as_input()
                         .interrupt_on_change())
        .attach_gpio(CHANNEL_TRIG_4,
                     drivers::io::mcp23s09_builder::gpio_builder()
                         .as_input()
                         .interrupt_on_change())

        .build();

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static pin slot_to_adc_pin(slot_nums slot);
static void interlock_pin_force_bridges_to_sleep(pin& pin_object);
static void interlock_pin_force_bridges_to_wake(pin& pin_object);
static void interlock_pin_configure_as_input(pin& pin_object);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
const drivers::cpld::shutter_module::channel_state&
driver::get_shutter_cpld_state(channel_id channel) const {
    return _cpld_shutter_state[channel - std::get<0>(CHANNEL_RANGE_SHUTTER)];
}

drivers::cpld::shutter_module::stateful_channel_proxy
driver::get_shutter_cpld_driver(channel_id channel,
                                drivers::spi::handle_factory& factory) {
    return drivers::cpld::shutter_module::stateful_channel_proxy(
        _cpld_shutter_state[channel - std::get<0>(CHANNEL_RANGE_SHUTTER)],
        factory, _slot, channel);
}

driver::sio_modes_e driver::get_sio_mode(channel_id channel) const {
    if (_gpio.get_gpio_direction(to_sio_channel(channel)) == directions_e::IN) {
        return sio_modes_e::DISABLED;
    } else {
        return _gpio.get_output_latch(to_sio_channel(channel))
                   ? sio_modes_e::ON
                   : sio_modes_e::OFF;
    }
}

void driver::set_sio_mode(drivers::spi::handle_factory& factory,
                          channel_id channel, driver::sio_modes_e level) {
    const bool IS_INPUT =
        _gpio.get_gpio_direction(to_sio_channel(channel)) == directions_e::IN;
    _gpio.with_resource(
        [channel = to_sio_channel(channel), level, IS_INPUT](auto& resource) {
            if (level == sio_modes_e::DISABLED) {
                resource.set_gpio_direction(channel, directions_e::IN);
            } else {
                resource.set_gpio_value(channel, level == sio_modes_e::ON);

                if (IS_INPUT) {
                    resource.set_gpio_direction(channel, directions_e::OUT);
                }
            }
        },
        factory);
}
drivers::io::interrupt_modes_e driver::get_shutter_interrupt_mode(
    channel_id channel) const {
    return _gpio.get_gpio_interrupt_mode(to_trigger_channel(channel));
}

void driver::set_shutter_interrupt_mode(drivers::spi::handle_factory& factory,
                                        channel_id channel,
                                        drivers::io::interrupt_modes_e mode) {
    _gpio.with_resource(
        [CHAN = to_trigger_channel(channel), mode](auto& resource) {
            resource.set_gpio_interrupt_mode(CHAN, mode);
        },
        factory);
}

bool driver::get_shutter_trigger_level(channel_id channel) {
    return _gpio.get_gpio_value(to_trigger_channel(channel));
}

bool driver::is_power_enabled() const {
    return _gpio.get_gpio_value(CHANNEL_POWER_EN) != _inverted_power;
}

void driver::set_power_enabled(drivers::spi::handle_factory& factory,
                               bool enable_power) {
    // _gpio.inject_resource(factory).set_gpio_value(CHANNEL_POWER_EN,
    //                                               enable_power);
    _gpio.with_resource(
        [enable_power = enable_power != _inverted_power](auto& spi_methods) {
            spi_methods.set_gpio_value(CHANNEL_POWER_EN, enable_power);
        },
        factory);
}

bool driver::query_interrupt_channels(drivers::spi::handle_factory& factory) {
    _gpio.with_resource(
        [&](decltype(_gpio)::spi_methods& spi) {
            spi.query_interrupt_flags();
            _channel_has_interrupt[0] =
                _gpio.is_channel_interrupted(CHANNEL_TRIG_1);
            _channel_has_interrupt[1] =
                _gpio.is_channel_interrupted(CHANNEL_TRIG_2);
            _channel_has_interrupt[2] =
                _gpio.is_channel_interrupted(CHANNEL_TRIG_3);
            _channel_has_interrupt[3] =
                _gpio.is_channel_interrupted(CHANNEL_TRIG_4);
            spi.clear_interrupt();
        },
        factory);

    return _channel_has_interrupt.any();
}

bool driver::channel_has_interrupt(channel_id channel) const {
    return _channel_has_interrupt[utils::channel_to_index(
        channel, CHANNEL_RANGE_SHUTTER)];
}

void driver::clear_channel_interrupt(channel_id channel) {
    _channel_has_interrupt[utils::channel_to_index(
        channel, CHANNEL_RANGE_SHUTTER)] = false;
}

void driver::interlock_pin_force_bridges_to_sleep() {
    ::interlock_pin_force_bridges_to_sleep(_adc_pin);
}

void driver::interlock_pin_force_bridges_to_wake() {
    ::interlock_pin_force_bridges_to_wake(_adc_pin);
}

void driver::interlock_pin_configure_as_input() {
    ::interlock_pin_configure_as_input(_adc_pin);
}

bool driver::is_interlock_pin_engaged() { return !_adc_pin.read(); }


void driver::set_pwm_period(drivers::spi::handle_factory& factory,
                    drivers::cpld::shutter_module::period_type period) {
    _pwm_period = drivers::cpld::shutter_module::clamp_period(period);
    drivers::cpld::shutter_module::set_period(factory, _slot, _pwm_period);
}

driver driver::create(drivers::spi::handle_factory& factory, slot_nums slot,
                      const creation_options& options) {
    // Drive the CPLD PWM modules to 0.
    for (drivers::cpld::shutter_module::channel_id channel = 0;
         channel < drivers::cpld::shutter_module::CHANNEL_COUNT; ++channel) {
        drivers::cpld::shutter_module::set_duty_cycle(factory, slot, channel,
                                                      0);
        drivers::cpld::shutter_module::set_phase_polarity(factory, slot,
                                                          channel, false);
    }

    // Set the interlock pin to force bridges to sleep.
    pin adc_pin = slot_to_adc_pin(slot);
    ::interlock_pin_force_bridges_to_sleep(adc_pin);

    // Enable the CPLD module to allow the CS line
    // to communicate to the GPIO card.
    drivers::cpld::shutter_module::enable_module(factory, slot);

    mcp23s09_state state = DRIVER_INITIAL_CONDITIONS;

    // Set output pin to high.
    if (options.invert_power_pin) {
        state.io_latch |= (0x1 << CHANNEL_POWER_EN);
    }

    // Set the initial state options.
    for (std::size_t i = 0; i < options.sio_power_up_state.size(); ++i) {
        const bool LEVEL = options.sio_power_up_state[i];
        const channel_id CHANNEL =
            utils::index_to_channel(i, CHANNEL_RANGE_SIO);
        const auto DRIVER_CHANNEL = to_sio_channel(CHANNEL);

        const drivers::io::mcp23s09_driver::channel_mask MASK =
            1 << DRIVER_CHANNEL;
        if (LEVEL) {
            state.io_latch |= MASK;
        } else {
            state.io_latch &= ~MASK;
        }
    }

    // Set the period of the PWM channel.
    drivers::cpld::shutter_module::set_period(factory, slot,
                                              DEFAULT_PWM_PERIOD);

    return driver(factory, slot, std::move(adc_pin), state,
                  options.invert_power_pin);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
driver::driver(drivers::spi::handle_factory& factory, slot_nums slot,
               drivers::io::pin&& pin, const mcp23s09_state& initial_state,
               bool invert_power_pin)
    : _gpio(drivers::io::mcp23s09_driver::create(
          factory, initial_state, drivers::spi::slot_to_chip_select(slot)))
    , _adc_pin(pin)
    , _channel_has_interrupt(0)
    , _pwm_period(DEFAULT_PWM_PERIOD)
    , _slot(slot)
    , _inverted_power(invert_power_pin) {}

static pin slot_to_adc_pin(slot_nums slot) {
    switch (slot) {
    case SLOT_1:
        return {PIN_ADC_1};
    case SLOT_2:
        return {PIN_ADC_2};
    case SLOT_3:
        return {PIN_ADC_3};
    case SLOT_4:
        return {PIN_ADC_4};
    case SLOT_5:
        return {PIN_ADC_5};
    case SLOT_6:
        return {PIN_ADC_6};
    case SLOT_7:
        return {PIN_ADC_7};
    default:
        return {0, 0};
    };
}

static void interlock_pin_force_bridges_to_sleep(pin& pin_object) {
    pin_object.configure_as_output(false);
}

static void interlock_pin_force_bridges_to_wake(pin& pin_object) {
    pin_object.configure_as_output(true);
}

static void interlock_pin_configure_as_input(pin& pin_object) {
    static constexpr auto ATTRIBUTES =
        drivers::io::pin::attribute_builder().build();
    pin_object.configure_as_input(ATTRIBUTES);
}

// EOF
