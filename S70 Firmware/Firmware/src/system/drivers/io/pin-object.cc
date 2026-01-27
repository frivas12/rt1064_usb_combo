/**
 * \file pin-object.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "pin-object.hh"
#include "pio.h"

using namespace drivers::io;

void pin::configure_as_input(attribute_type attributes) {
    pio_set_input(_pio, _pin_mask, attributes);
}
void pin::configure_as_output(bool set_to_high, bool enable_multidrive,
                              bool enable_pullup) {
    pio_set_output(_pio, _pin_mask, static_cast<uint32_t>(set_to_high),
                   static_cast<uint32_t>(enable_multidrive),
                   static_cast<uint32_t>(enable_pullup));
}

bool pin::read() {
    return static_cast<bool>(pio_get(_pio, PIO_INPUT, _pin_mask));
}

bool pin::get_latch() {
    return static_cast<bool>(pio_get(_pio, PIO_OUTPUT_0, _pin_mask));
}

void pin::write(bool value) {
    if (value) {
        pio_set(_pio, _pin_mask);
    } else {
        pio_clear(_pio, _pin_mask);
    }
}

pin pin::from_id(uint32_t pin_id) {
    return pin(pio_get_pin_group(pin_id), pio_get_pin_group_mask(pin_id));
}

pin::pin(Pio *const pio, uint32_t pin_index) : _pio(pio), _pin_mask(pio_get_pin_group_mask(pin_index)) {}

// EOF
