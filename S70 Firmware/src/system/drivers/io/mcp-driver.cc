/**
 * \file mcp-driver.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 11-06-2023
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "mcp-driver.hh"

#include "mcp23s09.h"
#include "spi-transfer-handle.hh"
#include "user_spi.h"

using namespace drivers::io;

/*****************************************************************************
 * Constants
 *****************************************************************************/
static constexpr uint8_t DISABLE_INTERRUPTS = 0x00;

/*****************************************************************************
 * Macros
 *****************************************************************************/
// Debug option that will raise and assert if the
// read and written values are not equal.
#define enableLOCK_IF_WRITE_FAILED            0



#ifdef enableLOCK_IF_WRITE_FAILED
#include "FreeRTOS.h"
#include "task.h"
#endif

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
mcp23s09_driver::channel_mask
mcp23s09_driver::get_gpio_values() const noexcept {
    return _state.io_pin_state;
}
mcp23s09_driver::channel_mask
mcp23s09_driver::get_output_latches() const noexcept {
    return _state.io_latch;
}

directions_e
mcp23s09_driver::get_gpio_direction(channel_id channel) const noexcept {
    return (_state.io_dir & channel_id_to_mask(channel)) != 0
               ? directions_e::IN
               : directions_e::OUT;
}

bool mcp23s09_driver::get_gpio_inversion(channel_id channel) const noexcept {
    return (_state.io_polarity & channel_id_to_mask(channel)) != 0;
}

interrupt_modes_e
mcp23s09_driver::get_gpio_interrupt_mode(channel_id channel) const noexcept {
    const uint8_t MASK = channel_id_to_mask(channel);
    const bool interrupt_enabled = (_state.io_interrupt_on_change & MASK) != 0;
    if (!interrupt_enabled)
        return interrupt_modes_e::DISABLED;

    const bool interrupts_on_change = (_state.io_int_comp_reg & MASK) == 0;
    if (interrupts_on_change)
        return interrupt_modes_e::ON_CHANGE;

    const bool logic_high = (_state.io_compare & MASK) != 0;
    if (logic_high)
        return interrupt_modes_e::ON_LOGIC_HIGH;

    return interrupt_modes_e::ON_LOGIC_LOW;
}

bool mcp23s09_driver::is_channel_interrupted(
    channel_id channel) const noexcept {
    return _state.io_interrupt_flags & channel_id_to_mask(channel);
}

mcp23s09_driver
mcp23s09_driver::create(drivers::spi::handle_factory &factory,
                        const mcp23s09_state &initial_conditions,
                        uint8_t chip_select) {
    mcp23s09_driver rt = mcp23s09_driver(initial_conditions, chip_select);
    spi_methods injected = spi_methods(rt, factory);

    // Set the gpio driver
    injected.write_register(MCP23S09_IOCON, rt._state.io_config_reg);

    // Disable any interrupts
    injected.write_register(MCP23S09_GPINTEN, DISABLE_INTERRUPTS);

    // Set the output latch and change the direction
    injected.write_register(MCP23S09_OLAT, rt._state.io_latch);
    injected.write_register(MCP23S09_IODIR, rt._state.io_dir);

    // Setup pullups
    injected.write_register(MCP23S09_GPPU, rt._state.io_pullup);

    // Change polarity
    injected.write_register(MCP23S09_IPOL, rt._state.io_polarity);

    // Read to clear the chip interrupts, then clear memory interrupts.
    injected.read_register(MCP23S09_INTCAP);
    rt._state.interrupt_capture_reg = 0;

    // Setup interrupt fields
    injected.write_register(MCP23S09_DEFVAL, rt._state.io_compare);
    injected.write_register(MCP23S09_INTCON, rt._state.io_int_comp_reg);

    // Enable selected interrupts
    injected.write_register(MCP23S09_GPINTEN, rt._state.io_interrupt_on_change);

    return rt;
}

mcp23s09_driver::spi_methods
mcp23s09_driver::inject_resource(drivers::spi::handle_factory &factory) {
    return spi_methods(*this, factory);
}

void mcp23s09_driver::spi_methods::set_gpio_value(channel_id channel,
                                                  bool value) {
    set_gpio_group_value(mcp23s09_driver::channel_id_to_mask(channel), value);
}

void mcp23s09_driver::spi_methods::set_gpio_group_value(channel_mask mask,
                                                        bool value) {
    if (value) {
        _parent._state.io_latch |= mask;
    } else {
        _parent._state.io_latch &= ~mask;
    }

    write_register(MCP23S09_OLAT, _parent._state.io_latch);
}

void mcp23s09_driver::spi_methods::set_gpio_direction(channel_id channel,
                                                      directions_e direction) {
    set_gpio_group_direction(channel_id_to_mask(channel), direction);
}

void mcp23s09_driver::spi_methods::set_gpio_group_direction(
    channel_mask mask, directions_e direction) {
    if (direction == directions_e::IN) {
        _parent._state.io_dir |= mask;
    } else {
        _parent._state.io_dir &= ~mask;
    }

    write_register(MCP23S09_IODIR, _parent._state.io_dir);
}

void mcp23s09_driver::spi_methods::set_gpio_pullup(channel_id channel,
                                                   bool enable_pullup) {
    set_gpio_group_pullup(channel_id_to_mask(channel), enable_pullup);
}

void mcp23s09_driver::spi_methods::set_gpio_group_pullup(channel_mask mask,
                                                         bool enable_pullup) {
    if (enable_pullup) {
        _parent._state.io_pullup |= mask;
    } else {
        _parent._state.io_pullup &= ~mask;
    }

    write_register(MCP23S09_GPPU, _parent._state.io_pullup);
}

void mcp23s09_driver::spi_methods::set_gpio_inversion(channel_id channel,
                                                      bool value) {
    set_gpio_group_inversion(channel_id_to_mask(channel), value);
}

void mcp23s09_driver::spi_methods::set_gpio_group_inversion(channel_mask mask,
                                                            bool value) {
    if (value) {
        _parent._state.io_polarity |= mask;
    } else {
        _parent._state.io_polarity &= ~mask;
    }

    write_register(MCP23S09_IPOL, _parent._state.io_polarity);
}

void mcp23s09_driver::spi_methods::set_gpio_interrupt_mode(
    channel_id channel, interrupt_modes_e value) noexcept {
    set_gpio_group_interrupt_mode(channel_id_to_mask(channel), value);
}

void mcp23s09_driver::spi_methods::set_gpio_group_interrupt_mode(
    channel_mask mask, interrupt_modes_e value) noexcept {
    // Disable interrupts
    _parent._state.io_interrupt_on_change &= ~mask;
    write_register(MCP23S09_GPINTEN, _parent._state.io_interrupt_on_change);

    // Change interrupt settings as seems fit
    switch (value) {
    case interrupt_modes_e::DISABLED:
        break;

    case interrupt_modes_e::ON_LOGIC_LOW:
        _parent._state.io_interrupt_on_change |= mask;
        _parent._state.io_compare &= ~mask;
        _parent._state.io_int_comp_reg |= mask;
        break;

    case interrupt_modes_e::ON_LOGIC_HIGH:
        _parent._state.io_interrupt_on_change |= mask;
        _parent._state.io_compare |= mask;
        _parent._state.io_int_comp_reg |= mask;
        break;

    case interrupt_modes_e::ON_CHANGE:
        _parent._state.io_interrupt_on_change |= mask;
        _parent._state.io_int_comp_reg &= ~mask;
        break;
    }

    // Write changes and enable interrupts (if applicable)
    switch (value) {
    case interrupt_modes_e::DISABLED:
        break;
    case interrupt_modes_e::ON_LOGIC_LOW:
    case interrupt_modes_e::ON_LOGIC_HIGH:
    case interrupt_modes_e::ON_CHANGE:
        write_register(MCP23S09_DEFVAL, _parent._state.io_compare);
        write_register(MCP23S09_INTCON, _parent._state.io_int_comp_reg);
        write_register(MCP23S09_GPINTEN, _parent._state.io_interrupt_on_change);
        break;
    }
}

uint8_t mcp23s09_driver::spi_methods::query_register(uint8_t reg) {
    uint8_t * dest = nullptr;
    switch(reg) {
        case MCP23S09_IODIR:
            dest = &_parent._state.io_dir;
            break;

        case MCP23S09_IPOL:
            dest = &_parent._state.io_polarity;
            break;

        case MCP23S09_GPINTEN:
            dest = &_parent._state.io_interrupt_on_change;
            break;
        case MCP23S09_DEFVAL:
            dest = &_parent._state.io_compare;
            break;
        case MCP23S09_INTCON:
            dest = &_parent._state.io_int_comp_reg;
            break;
        case MCP23S09_IOCON:
            dest = &_parent._state.io_config_reg;
            break;
        case MCP23S09_GPPU:
            dest = &_parent._state.io_pullup;
            break;
        case MCP23S09_INTF:
            dest = &_parent._state.io_interrupt_flags;
            break;
        case MCP23S09_INTCAP:
            dest = &_parent._state.interrupt_capture_reg;
            break;
        case MCP23S09_GPIO:
            dest = &_parent._state.io_pin_state;
            break;
        case MCP23S09_OLAT:
            dest = &_parent._state.io_latch;
            break;
    }

    if (dest != nullptr) {
        *dest = std::to_integer<uint8_t>(read_register(reg));
        return *dest;
    } else {
        return 0;
    }
}

void mcp23s09_driver::spi_methods::query_interrupt_flags() {
    _parent._state.io_interrupt_flags =
        std::to_integer<uint8_t>(read_register(MCP23S09_INTF));
}

void mcp23s09_driver::spi_methods::query_interrupt_value() {
    _parent._state.interrupt_capture_reg =
        std::to_integer<uint8_t>(read_register(MCP23S09_INTCAP));
}

void mcp23s09_driver::spi_methods::query_gpio_value() {
    _parent._state.io_pin_state =
        std::to_integer<uint8_t>(read_register(MCP23S09_GPIO));
}

void mcp23s09_driver::spi_methods::clear_interrupt() {
    // Resets interrupt flags.
    _parent._state.io_interrupt_flags = 0;

    if (does_intcap_clear_interrupt()) {
        query_interrupt_value();
    } else {
        query_gpio_value();
    }
}

void mcp23s09_driver::spi_methods::flush_underlying(
    const mcp23s09_state &info) {
    // Check if interrupts need to be disabled
    mcp23s09_state &base = _parent._state;
    const bool CHANGING_INTERRUPTS =
        info.io_interrupt_on_change != base.io_interrupt_on_change;
    if (CHANGING_INTERRUPTS) {
        // Disable interrupts
        write_register(MCP23S09_GPINTEN, DISABLE_INTERRUPTS);
    }

    if (base.io_latch != info.io_latch)
        write_register(MCP23S09_OLAT, info.io_latch);

    if (base.io_dir != info.io_dir)
        write_register(MCP23S09_IODIR, info.io_dir);

    if (base.io_polarity != info.io_polarity)
        write_register(MCP23S09_IPOL, info.io_polarity);

    if (base.io_pullup != info.io_pullup)
        write_register(MCP23S09_GPPU, info.io_pullup);

    if (CHANGING_INTERRUPTS) {
        // Set fields, then enabled selected interrupts
        if (info.io_compare != base.io_compare) {
            write_register(MCP23S09_DEFVAL, info.io_compare);
        }
        if (info.io_int_comp_reg != base.io_int_comp_reg) {
            write_register(MCP23S09_INTCON, info.io_int_comp_reg);
        }
        write_register(MCP23S09_GPINTEN, info.io_interrupt_on_change);
    }

    base.io_dir = info.io_dir;
    base.io_polarity = info.io_polarity;
    base.io_interrupt_on_change = info.io_interrupt_on_change;
    base.io_compare = info.io_compare;
    base.io_int_comp_reg = info.io_int_comp_reg;
    base.io_pullup = info.io_pullup;
    base.io_latch = info.io_latch;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
mcp23s09_driver::mcp23s09_driver(const mcp23s09_state &state, uint8_t chip_select)
    : _state(state), _chip_select(chip_select)

{}

mcp23s09_driver::spi_methods::spi_methods(mcp23s09_driver &parent,
                                          drivers::spi::handle_factory &factory)
    : _factory(factory), _parent(parent) {}

void mcp23s09_driver::spi_methods::write_register(uint8_t reg, uint8_t state) {
    std::array<std::byte, 3> data{std::byte{0x40}, std::byte{reg},
                                  std::byte{state}};

    auto handle =
        _factory.create_handle(_SPI_MODE_3, false, _parent._chip_select);
    handle.transfer(std::span(data));

#if enableLOCK_IF_WRITE_FAILED
#warning "MCP Lock if Write Failed is enabled.  Please, disable when done testing."
    // Delay
    vTaskDelay(2);

    const std::byte READBACK = read_register(reg);

    // Loop forever
    if (READBACK != std::byte{state}) {
        for(;;);
    }
#endif
}

std::byte mcp23s09_driver::spi_methods::read_register(uint8_t reg) {
    std::array<std::byte, 3> data{std::byte{0x41}, std::byte{reg},
                                  std::byte{0x00}};
    auto handle =
        _factory.create_handle(_SPI_MODE_3, false, _parent._chip_select);
    handle.transfer(std::span(data));

    return data[2];
}

// EOF
