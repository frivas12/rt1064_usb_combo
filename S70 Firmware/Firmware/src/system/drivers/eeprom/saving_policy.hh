#pragma once

#include <cstdint>

#include "defs.h"
#include "defs.hh"
#include "device_detect.h"
#include "slot_types.h"

namespace drivers::eeprom {

enum class saving_policy_e : uint8_t {
    // The device's serial number and signature will be associated
    // with the save.
    ASSOCIATE_WITH_CONNECTED_DEVICE = 0,

    // A specific device type will be specified when saving occurs.
    ASSOCIATE_WITH_SPECIFIED_DEVICE = 1,

    // The save will be applicable to any connected device.
    ASSOCIATE_WITH_ANY_DEVICE = 2,
};

/**
 * Utility class representing contextual information
 * saved to EEPROM.
 */
struct saving_context {
    uint64_t serial_number;
    device_signature_t device_signature;

    static constexpr saving_context get_default() noexcept {
        return saving_context{
            .serial_number    = OW_SERIAL_NUMBER_EMPTY,
            .device_signature = device_signature_t{(slot_types)0xFFFF, 0xFFFF},
        };
    }

    /// Sets the serial number to be ignored.
    constexpr saving_context& ignore_serial_number() noexcept {
        serial_number = OW_SERIAL_NUMBER_WILDCARD;
        return *this;
    }

    /// Sets the device signature to be ignored.
    constexpr saving_context& ignore_signature() noexcept {
        device_signature.slot_type = CARD_IN_SLOT_NOT_PROGRAMMED;
        device_signature.device_id = 0xFFFF;
        return *this;
    }

    constexpr bool is_supported(device_signature_t connected_device,
                                const uint64_t& connect_sn) const noexcept {
        const bool SN_VALID = serial_number == OW_SERIAL_NUMBER_WILDCARD ||
                              serial_number == connect_sn;
        const bool DEV_VALID =
            (device_signature.slot_type == CARD_IN_SLOT_NOT_PROGRAMMED &&
             device_signature.device_id == 0xFFFF) ||
            device_signature == connected_device;

        return SN_VALID && DEV_VALID;
    }

    static constexpr saving_context from_policy(
        saving_policy_e policy, 
        const uint64_t& connected_sn,
        device_signature_t connected_signature,
        const saving_context* manual_context) noexcept {
        saving_context rt;
        switch (policy) {
        case saving_policy_e::ASSOCIATE_WITH_CONNECTED_DEVICE:
            rt.device_signature = connected_signature;
            rt.serial_number    = connected_sn;
            break;
        case saving_policy_e::ASSOCIATE_WITH_SPECIFIED_DEVICE:
            if (manual_context) {
                rt = *manual_context;
            } else {
                rt = saving_context::get_default();
            }
            break;
        case saving_policy_e::ASSOCIATE_WITH_ANY_DEVICE:
            rt.ignore_serial_number();
            rt.ignore_signature();
            break;
        default:
            rt = saving_context::get_default();
            break;
        }

        return rt;
    }
};

}  // namespace drivers::eeprom
