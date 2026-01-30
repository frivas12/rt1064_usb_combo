/**
 * \file mcp-builder.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 11-21-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <array>

#include "mcp-driver.hh"

namespace drivers::io
{


struct mcp23s09_builder
{
    struct gpio_builder
    {
        constexpr inline gpio_builder& as_input()
        { direction = directions_e::IN; return *this; }

        constexpr inline gpio_builder& as_low_output() {
            direction = directions_e::OUT;
            default_is_high = false;
            return *this;
        }

        constexpr inline gpio_builder& as_high_output() {
            direction = directions_e::OUT;
            default_is_high = true;
            return *this;
        }

        constexpr inline gpio_builder& invert()
        { inverted = true; return *this; }

        constexpr inline gpio_builder& interrupt_on_low()
        { mode = interrupt_modes_e::ON_LOGIC_LOW; return *this; }

        constexpr inline gpio_builder& interrupt_on_high()
        { mode = interrupt_modes_e::ON_LOGIC_HIGH; return *this; }

        constexpr inline gpio_builder& interrupt_on_change()
        { mode = interrupt_modes_e::ON_CHANGE; return *this; }

        constexpr inline gpio_builder& pullup()
        { use_pullup = true; return *this; }

        constexpr gpio_builder() = default;
        

        directions_e direction = directions_e::IN;
        bool inverted = false;
        interrupt_modes_e mode = interrupt_modes_e::DISABLED;
        bool use_pullup = false;
        bool default_is_high = false;
    };

    constexpr inline mcp23s09_builder& enable_sequential_operations() noexcept
    { _sequential_ops_enabled = true; return *this; }

    constexpr inline mcp23s09_builder& interrupt_as_open_drain() noexcept
    { _open_drain_enabled = true; return *this; }

    constexpr inline mcp23s09_builder& interrupt_as_active_driver() noexcept
    { _open_drain_enabled = false; return *this; }

    constexpr inline mcp23s09_builder& interrupt_signals_high() noexcept
    { _interrupt_is_active_high = true; return *this; }

    constexpr inline mcp23s09_builder& interrupt_signals_log() noexcept
    { _interrupt_is_active_high = false; return *this; }

    constexpr inline mcp23s09_builder& interrupt_clears_reading_intcap() noexcept
    { _clear_interupt_with_intcap = true; return *this; }

    constexpr inline mcp23s09_builder& interrupt_clears_reading_gpio() noexcept
    { _clear_interupt_with_intcap = false; return *this; }

    constexpr inline mcp23s09_builder& attach_gpio(std::size_t index, gpio_builder builder)
    { _pins[index] = builder; return *this; }

    constexpr inline mcp23s09_state build() const noexcept {
        mcp23s09_state rt = {0};

        for (std::size_t i = 0; i < 8; ++i) {
            rt.io_dir |= (_pins[i].direction == directions_e::IN ? 1 : 0) << i;
            rt.io_polarity |= (_pins[i].inverted ? 1 : 0) << i;
            rt.io_interrupt_on_change |=
                (_pins[i].mode != interrupt_modes_e::DISABLED ? 1 : 0) << i;
            rt.io_compare |= (_pins[i].mode == interrupt_modes_e::ON_LOGIC_HIGH ? 1 : 0) << i;
            rt.io_int_comp_reg |= (_pins[i].mode != interrupt_modes_e::ON_CHANGE ? 1 : 0) << i;
            rt.io_pullup |= (_pins[i].use_pullup ? 1 : 0) << i;
            rt.io_latch |= (_pins[i].default_is_high ? 1 : 0) << i;
        }

        rt.io_config_reg =
            ((_sequential_ops_enabled ? 1 : 0) << 5) |
            ((_open_drain_enabled ? 1 : 0) << 2) |
            ((_interrupt_is_active_high ? 1 : 0) << 1) |
            ((_clear_interupt_with_intcap ? 1 : 0) << 0);

        return rt;
    }

    constexpr mcp23s09_builder() = default;

private:
    std::array<gpio_builder, 8> _pins;
    bool _sequential_ops_enabled = false;
    bool _open_drain_enabled = false;
    bool _interrupt_is_active_high = false;
    bool _clear_interupt_with_intcap = false;
};
    
} 

// EOF
