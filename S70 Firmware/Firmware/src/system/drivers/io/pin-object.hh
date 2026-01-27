/**
 * \file pin-object.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "asf.h"
#include "pio.h"

namespace drivers::io {

struct pin {
    /// \brief See PIO_ macros for attributes (pio.h).
    using attribute_type = uint32_t;

    void configure_as_input(attribute_type attributes);
    void configure_as_output(bool set_to_high, bool enable_multidrive = 0,
                             bool enable_pullup = 1);

    /// @brief Reads an input pin.
    bool read();

    /// @brief Gets the output latch on an output pin.
    bool get_latch();

    void write(bool value);

    /// \brief Creates a pin from the pid id.
    static pin from_id(uint32_t pin_id);

    pin(Pio * const pio, uint32_t pin_index);

  private:
    Pio *_pio;
    uint32_t _pin_mask;

  public:
    /// \brief Alternative to PIO_ macro attributes.
    class attribute_builder {
      public:
        static constexpr attribute_type get_default()
        { return attribute_builder().build(); }

        constexpr attribute_builder &pullup() {
            _value |= PIO_PULLUP;
            return *this;
        }

        constexpr attribute_builder &deglitch() {
            _value |= PIO_DEGLITCH;
            return *this;
        }

        constexpr attribute_builder &open_drain() {
            _value |= PIO_OPENDRAIN;
            return *this;
        }

        constexpr attribute_builder &debounce() {
            _value |= PIO_DEBOUNCE;
            return *this;
        }

        constexpr attribute_builder &no_interrupt() {
            _value &= (PIO_IT_AIME | PIO_IT_RE_OR_HL | PIO_IT_EDGE);
            return *this;
        }

        constexpr attribute_builder &interrupt_low_level() {
            no_interrupt();
            _value |= PIO_IT_LOW_LEVEL;
            return *this;
        }

        constexpr attribute_builder &interrupt_high_level() {
            no_interrupt();
            _value |= PIO_IT_HIGH_LEVEL;
            return *this;
        }

        constexpr attribute_builder &interrupt_falling_edge() {
            no_interrupt();
            _value |= PIO_IT_FALL_EDGE;
            return *this;
        }

        constexpr attribute_builder &interrupt_rising_edge() {
            no_interrupt();
            _value |= PIO_IT_RISE_EDGE;
            return *this;
        }

        constexpr attribute_type build() const { return _value; }

      private:
        attribute_type _value = PIO_DEFAULT;
    };
};

} // namespace drivers::io
// EOF
