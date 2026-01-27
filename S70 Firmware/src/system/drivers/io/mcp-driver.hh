/**
 * \file mcp-driver.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 11-06-2023
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "mcp23s09.h"
#include "spi-transfer-handle.hh"

namespace drivers::io {

enum class directions_e {
    IN,
    OUT,
};

enum class interrupt_modes_e {
    DISABLED,
    ON_LOGIC_LOW,
    ON_LOGIC_HIGH,
    ON_CHANGE,
};

/**
 * Software to drive the MCP23S09 8-channel GPIO chip.
 * Channels:
 * [0, 7]   Interruptable GPIO
 */
struct mcp23s09_driver {
    static constexpr uint8_t MCP23S09_IODIR = 0x00;
    static constexpr uint8_t MCP23S09_IPOL = 0x01;
    static constexpr uint8_t MCP23S09_GPINTEN = 0x02;
    static constexpr uint8_t MCP23S09_DEFVAL = 0x03;
    static constexpr uint8_t MCP23S09_INTCON = 0x04;
    static constexpr uint8_t MCP23S09_IOCON = 0x05;
    static constexpr uint8_t MCP23S09_GPPU = 0x06;
    static constexpr uint8_t MCP23S09_INTF = 0x07;
    static constexpr uint8_t MCP23S09_INTCAP = 0x08;
    static constexpr uint8_t MCP23S09_GPIO = 0x09;
    static constexpr uint8_t MCP23S09_OLAT = 0x0A;

    using channel_id = uint8_t;
    using channel_mask = uint8_t;

    /// @brief Sub-object enabling SPI operations.
    struct spi_methods {
        inline mcp23s09_driver &parent() { return _parent; }

        inline const mcp23s09_driver &parent() const { return _parent; }

        /// \pre channel is a valid channel.
        void set_gpio_value(channel_id channel, bool value);

        void set_gpio_group_value(channel_mask mask, bool value);

        /// \pre channel is a valid channel.
        void set_gpio_direction(channel_id channel, directions_e direction);

        void set_gpio_group_direction(channel_mask mask,
                                      directions_e direction);

        /// \pre channel is a valid channel.
        void set_gpio_pullup(channel_id channel, bool enable_pullup);

        void set_gpio_group_pullup(channel_mask mask, bool enable_pullup);

        /// \pre channel is a valid channel.
        void set_gpio_inversion(channel_id channel, bool value);

        void set_gpio_group_inversion(channel_mask mask, bool value);

        /// \pre channel is a valid channel.
        void set_gpio_interrupt_mode(channel_id channel,
                                     interrupt_modes_e value) noexcept;

        void set_gpio_group_interrupt_mode(channel_mask mask,
                                           interrupt_modes_e value) noexcept;

        uint8_t query_register(uint8_t reg);

        void query_interrupt_flags();

        /// @brief Stores the value of the interrupt register.
        ///        May clear interrupt depending on settings.
        void query_interrupt_value();

        /**
         * Stores the value of the GPIO pins.
         * May clear interrupt depending on settings.
         */
        void query_gpio_value();

        /// @brief Clears the interrupt pin if the GPIO/INTCAP register is not
        /// needed or settings is not known.
        void clear_interrupt();

        /**
         * Performs set operations in bulk to change the
         * state of the MCP chip into the state provided.
         *
         * First, the interrupt are disabled if any will be changing.
         * Next, the IO registers are updated.
         * Finally, the interrupt registers are enabled.
         */
        void flush_underlying(const mcp23s09_state &state);

        friend class mcp23s09_driver;

      private:
        drivers::spi::handle_factory &_factory;
        mcp23s09_driver &_parent;

        spi_methods(mcp23s09_driver &parent,
                    drivers::spi::handle_factory &factory);

        void write_register(uint8_t reg, uint8_t state);

        std::byte read_register(uint8_t reg);

        inline bool does_intcap_clear_interrupt() const {
            return (_parent._state.io_config_reg & (1 << 0)) != 0;
        }
    };

    /// \pre channel is a valid channel.
    inline bool get_gpio_value(channel_id channel) const noexcept {
        return get_gpio_values() & channel_id_to_mask(channel);
    }

    /// \pre channel is a valid channel.
    inline bool get_output_latch(channel_id channel) const noexcept {
        return get_output_latches() & channel_id_to_mask(channel);
    }

    /// \brief Returns the bit-mapped values of all the channels.
    channel_mask get_gpio_values() const noexcept;

    /// \brief Returns the bit-mapped value of the output latch for all the
    /// channels.
    channel_mask get_output_latches() const noexcept;

    /// \pre channel is a valid channel.
    directions_e get_gpio_direction(channel_id channel) const noexcept;

    /// \pre channel is a valid channel.
    bool get_gpio_inversion(channel_id channel) const noexcept;

    /// \pre channel is a valid channel.
    interrupt_modes_e
    get_gpio_interrupt_mode(channel_id channel) const noexcept;

    /**
     * Indicates if the channel has an interrupt.
     */
    bool is_channel_interrupted(channel_id channel) const noexcept;

    /**
     * Initializes the mcp23s09 driver.
     * The factory will be used to set the hardware in an initial state.
     * \param[in]       factory The SPI factory.
     * \param[in]       initial_conditions The initial conditions for the chip.
     * \param[in]       chip_select The chip select for the chip.
     * \return The MCP driver without any attached resources.
     */
    static mcp23s09_driver create(drivers::spi::handle_factory &factory,
                                  const mcp23s09_state &initial_conditions,
                                  uint8_t chip_select);

    /**
     * Injects an SPI handle factory to enable the use of SPI methods.
     * \warning The lifetime of the returned object may not exceed the
     *          lifetime of the factory or the parent object.
     * \note Unless needed, prefer the \ref{with_resource} visitor method.
     * \param[in]       factory The factory to inject.
     * \return A sub-object that provides SPI methods for this driver.
     */
    spi_methods inject_resource(drivers::spi::handle_factory &factory);

    /**
     * Invokes the visitor callback with the SPI spi methods.
     * This is preferred over \ref{inject_resource} as it guarantees that
     * lifetimes are not violated. \param[in]       visitor A function that uses
     * the SPI methods. \param[in]       factory The SPI handle factory to
     * inject. \return The return of the visitor.
     */
    auto with_resource(std::invocable<spi_methods &> auto &&visitor,
                       drivers::spi::handle_factory &factory) {
        spi_methods methods = inject_resource(factory);

        return visitor(methods);
    }

    /// \pre channel in [0, 7]
    static constexpr channel_mask
    channel_id_to_mask(channel_id channel) noexcept {
        return 0x01 << channel;
    }

    inline mcp23s09_state get_state() { return _state; }

    friend class spi_methods;

  protected:
    mcp23s09_state _state;
    uint8_t _chip_select;

    mcp23s09_driver(const mcp23s09_state &state, uint8_t chip_select);
};

} // namespace drivers::io

// EOF
