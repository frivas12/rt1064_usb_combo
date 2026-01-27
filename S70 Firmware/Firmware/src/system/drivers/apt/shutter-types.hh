/**
 * \file shutter-types.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 06-06-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <cstdint>

namespace drivers::apt::shutter_types
{
enum class states_e
{
    OPEN = 0,
    CLOSED = 1,
    UNKNOWN = 2,
    OPENING,
    CLOSING,
};

enum class types_e
{
    /// \brief No shutter driver.
    NONE = 0,

    /// \brief A pulse-only shutter with position given by polarity.
    PULSED = 1,

    /// \brief A pulse-hold shutter with equal state_waveforms but with different polarities (open is postive).
    PULSE_HOLD = 2,

    /// \brief A pulse-hold shutter with a postive open state_waveform and a grounded closed polarity.
    PULSE_NO_REVERSE = 3,
};

enum class trigger_modes_e
{
    /// @brief The trigger line is disabled and won't be used.
    DISABLED = 0,

    /// @brief The trigger line is enabled and follows high -> open.
    ENABLED = 1,

    /// @brief The trigger line is enabled and follows low -> open.
    ENABLED_INVERTED = 2,
};
}

// EOF
