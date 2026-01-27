/**
 * \file flipper-shutter-driver.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-09-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <bitset>

#include "channels.hh"
#include "cpld-shutter-driver.hh"
#include "mcp-driver.hh"
#include "mcp23s09.h"
#include "pin-object.hh"
#include "spi-transfer-handle.hh"

namespace cards::flipper_shutter {

/**
 * Implements the hardware communication for the flipper-shutter card.
 *
 * Channel enumeration goes as follows...
 * [0,  3]: Shutters (1, 2, 3, 4)
 * [4,  6]: Output GPIO (SIO) (SIO0, 1, 2)
 *  7     : Power Enable Output GPIO
 */
class driver {
   public:
    using channel_id = uint8_t;

    // The default PWM period of the flipper-shutter driver.
    static constexpr drivers::cpld::shutter_module::period_type
        DEFAULT_PWM_PERIOD = drivers::cpld::shutter_module::from_hz(80000);

    static_assert(
        DEFAULT_PWM_PERIOD ==
            drivers::cpld::shutter_module::clamp_period(DEFAULT_PWM_PERIOD),
        "The default PWM period is out-of-bounds");

    static constexpr std::size_t CHANNELS = 8;

    // Double-ended inclusive range a.k.a. [x, y]
    static constexpr utils::channel_range<channel_id> CHANNEL_RANGE_SHUTTER =
        utils::channel_range<channel_id>(0, 3);
    static constexpr utils::channel_range<channel_id> CHANNEL_RANGE_SIO =
        utils::channel_range<channel_id>(4, 6);

    static constexpr channel_id CHANNEL_SHUTTER_1 =
        utils::index_to_channel(0, CHANNEL_RANGE_SHUTTER);
    static constexpr channel_id CHANNEL_SHUTTER_2 =
        utils::index_to_channel(1, CHANNEL_RANGE_SHUTTER);
    static constexpr channel_id CHANNEL_SHUTTER_3 =
        utils::index_to_channel(2, CHANNEL_RANGE_SHUTTER);
    static constexpr channel_id CHANNEL_SHUTTER_4 =
        utils::index_to_channel(3, CHANNEL_RANGE_SHUTTER);
    static constexpr channel_id CHANNEL_SIO_0 =
        utils::index_to_channel(0, CHANNEL_RANGE_SIO);
    static constexpr channel_id CHANNEL_SIO_1 =
        utils::index_to_channel(1, CHANNEL_RANGE_SIO);
    static constexpr channel_id CHANNEL_SIO_2 =
        utils::index_to_channel(2, CHANNEL_RANGE_SIO);
    static constexpr channel_id CHANNEL_POWER_ENABLE = 7;

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    const drivers::cpld::shutter_module::channel_state& get_shutter_cpld_state(
        channel_id channel) const;

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    drivers::cpld::shutter_module::stateful_channel_proxy
    get_shutter_cpld_driver(channel_id channel,
                            drivers::spi::handle_factory& factory);

    enum class sio_modes_e {
        OFF = 0,
        ON  = 1,
        DISABLED,
    };

    /// \pre channel in CHANNEL_RANGE_SIO
    sio_modes_e get_sio_mode(channel_id channel) const;

    /// \pre channel in CHANNEL_RANGE_SIO
    void set_sio_mode(drivers::spi::handle_factory& factory, channel_id channel,
                      sio_modes_e mode);

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    drivers::io::interrupt_modes_e get_shutter_interrupt_mode(
        channel_id channel) const;

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    void set_shutter_interrupt_mode(drivers::spi::handle_factory& factory,
                                    channel_id channel,
                                    drivers::io::interrupt_modes_e mode);

    /// \pre channel in CHANNGE_RANGE_SHUTTER
    bool get_shutter_trigger_level(channel_id channel);

    bool is_power_enabled() const;

    void set_power_enabled(drivers::spi::handle_factory& factory,
                           bool enable_power);

    /// @brief Checks if any of the channels raised an interrupt and clears the
    /// hardware interrupt.
    /// \return If any of the channels had an interrupt.
    bool query_interrupt_channels(drivers::spi::handle_factory& factory);

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    bool channel_has_interrupt(channel_id channel) const;

    /// \pre channel in CHANNEL_RANGE_SHUTTER
    void clear_channel_interrupt(channel_id channel);

    void interlock_pin_force_bridges_to_sleep();

    void interlock_pin_force_bridges_to_wake();

    void interlock_pin_configure_as_input();

    /// \pre interlock_pin was configured as an input
    bool is_interlock_pin_engaged();

    constexpr slot_nums get_slot() const { return _slot; }

    constexpr drivers::cpld::shutter_module::period_type get_pwm_period()
        const {
        return _pwm_period;
    }

    void set_pwm_period(drivers::spi::handle_factory& factory,
                        drivers::cpld::shutter_module::period_type period);

    struct creation_options {
        std::bitset<std::size(CHANNEL_RANGE_SIO)> sio_power_up_state;
        bool invert_power_pin;

        static constexpr creation_options get_default() noexcept {
            return creation_options{

                // Defaults to high since we have a pull-up
                .sio_power_up_state =
                    std::bitset<std::size(CHANNEL_RANGE_SIO)>().set(),
                .invert_power_pin = false,
            };
        }
    };

    /**
     * Creates a new driver sitting at the slot.
     * \param[in]       factory A factory that is used during initialization.
     * \param[in]       slot The slot of the driver.
     */
    static driver create(drivers::spi::handle_factory& factory, slot_nums slot,
                         const creation_options& options);

    inline static driver create(drivers::spi::handle_factory& factory,
                                slot_nums slot) {
        const creation_options options = creation_options::get_default();
        return create(factory, slot, options);
    }

   private:
    std::array<drivers::cpld::shutter_module::channel_state, 4>
        _cpld_shutter_state;
    drivers::io::mcp23s09_driver _gpio;
    drivers::io::pin _adc_pin;
    std::bitset<4> _channel_has_interrupt;
    drivers::cpld::shutter_module::period_type _pwm_period;
    slot_nums _slot;
    bool _inverted_power;

    driver(drivers::spi::handle_factory& factory, slot_nums slot,
           drivers::io::pin&& adc_pin, const mcp23s09_state& initial_state,
           bool inverted_power);

    /// \pre channel in [4, 6]
    static constexpr drivers::io::mcp23s09_driver::channel_id to_sio_channel(
        channel_id channel) {
        return 6 - channel;
    }

    /// \pre channel in [0, 3]
    static constexpr drivers::io::mcp23s09_driver::channel_id
    to_trigger_channel(channel_id channel) {
        return channel + 4;
    }
};

}  // namespace cards::flipper_shutter

// EOF
