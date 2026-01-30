/**
 * \file cpld.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include "spi-transfer-handle.hh"

#include "slot_nums.h"

#include <cstdint>

namespace drivers::cpld
{
    enum class module_modes_e : uint32_t
    {
        DISABLED = 0,
        ONE_WIRE = 1,
        ENABLED = 2,
    };

    enum class addresses_e : uint8_t
    {
        SLOT1 = 0,
        SLOT2 = 1,
        SLOT3 = 2,
        SLOT4 = 3,
        SLOT5 = 4,
        SLOT6 = 5,
        SLOT7 = 6,
        SLOT8 = 7,

        REVISION = 8,
        LED = 9,
        UART_CONTROLLER = 10,
    };

    enum class commands_e : uint16_t
    {
        SET_ENABLE_STEPPER_CARD = 1,
        SET_ENABLE_SERVO_CARD = 2,
        SET_ENABLE_OTM_DAC_CARD = 3,
        SET_ENABLE_RS232_CARD = 4,
        SET_ENABLE_IO_CARD = 5,
        SET_ENABLE_SHUTTER_CARD = 6,
        SET_ENABLE_PEIZO_ELLIPTEC_CARD = 7,
        SET_ENABLE_PEIZO_CARD = 8,

// PWM commands
        SET_PWM_FREQ = 48,
        SET_PWM_DUTY = 49,

// Quad encoder commands
        SET_QUAD_COUNTS = 50,
        SET_HOMING = 51,

// Servo card commands
        SET_SERVO_CARD_PHASE = 52,

// OTM Dac card commands
        SET_OTM_DAC_LASER_EN = 53,

// uart commands
        SET_UART_SLOT = 54,

// 4 shutter card commands
        SET_SUTTER_PWM_DUTY = 55,
        SET_SUTTER_PHASE_DUTY = 56,
        SET_SUTTER_PWM_PERIOD = 60,

// IO card commands
        SET_IO_PWM_DUTY = 57,

// Stepper card commands
        SET_STEPPER_DIGITAL_OUTPUT = 59,

// ********** Read commands (16 bit) **********
        READ_CPLD_REV = 0x8001,
        READ_INTERUPTS = 0x8002,
        READ_QUAD_COUNTS = 0x8003,
        READ_EM_STOP_FLAG = 0x8004,
        READ_QUAD_BUFFER = 0x8005,
        READ_BISS_ENC = 0x8006,
        READ_SSI_MAG = 0x8007,
    };

    struct message_header
    {
        commands_e command;
        addresses_e address;
    };
    struct message_payload
    {
        uint32_t data;
        uint8_t mid_data;
    };

    /// \brief Writes the message to the CPLD.
    void write_register(spi::handle_factory& factory, const message_header& header, const message_payload& payload);

    /// @brief Reads a reply message from the CPLD.
    message_payload read_register(spi::handle_factory& factory, const message_header& header);

    /// \brief Checks if the emergency stop flag has been set.
    bool read_emergency_stop_flag(spi::handle_factory& factory);

    /// @brief Converts a slot to a CPLD address.
    inline constexpr addresses_e to_address(slot_nums slot) noexcept {
        static_assert((uint8_t)SLOT_1 == (uint8_t)addresses_e::SLOT1);
        return static_cast<addresses_e>(slot);
    }
}

// EOF
