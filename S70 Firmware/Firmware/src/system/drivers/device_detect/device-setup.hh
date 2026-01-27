/**
 * \file device-setup.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-05-30
 *
 * Utilities for setting up device detection
 */
#pragma once

#include <concepts>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>

#include "lock_guard.hh"
#include "lut_manager.hh"
#include "lut_types.hh"
#include "pnp_status.hh"
#include "save_constructor.hh"
#include "save_size.hh"
#include "stream-io.hh"

#include "defs.h"
#include "device_detect.h"
#include "pnp_status.h"
#include "slot_nums.h"
#include "slots.h"

namespace drivers::device_detection {

template <typename T>
concept device_setup_builder =
    requires(T t, device_signature_t device_signature, uint64_t serial_number,
             struct_id_t struct_id,
             std::span<const std::byte> serialized_struct) {
        /// \brief Yields the slot for the builder.
        { t.slot() } -> std::same_as<slot_nums>;

        /// \brief Loads the device setup parameters from non-volatile storage.
        /// \return If the parameters were loaded from non-volatile storage.
        { t.load_from_nv() } -> std::convertible_to<bool>;

        /// \brief Sets up the default values for the setup.
        t.setup_default_values();

        /// \brief If the device information provided can run on the currently
        /// loaded parameters.
        {
            t.can_device_run_on_current_params(device_signature, serial_number)
        } -> std::convertible_to<bool>;

        /// \brief Returns a view to the configuration signature buffer used
        /// with the LUT.
        {
            t.get_configuration_span()
        } -> std::convertible_to<std::span<config_signature_t>>;

        /// \brief Returns a binary buffer where config structures will be
        /// loaded
        /// into for deserialization.
        { t.get_struct_buffer() } -> std::convertible_to<std::span<std::byte>>;

        /// \brief Returns the next structure that is expected to be loaded.
        { t.get_next_struct_to_load() } -> std::same_as<struct_id_t>;

        /// \brief Loads the next structure.
        /// \pre get_next_struct_to_load() == struct_id
        /// \pre serialized_struct.size() ==
        /// save_constructor::get_config_size(struct_id)
        /// \return If the struct was deserialized and loaded.
        {
            t.load_serialized_struct(struct_id, serialized_struct)
        } -> std::convertible_to<bool>;

        /// \brief Indicates that loaded has finished.
        /// \return If the setup is finished.
        { t.finish_loading() } -> std::convertible_to<bool>;
    };

/**
 * Attempts to set up a device associated with the builder.
 * \note Updates the pnp_status of the builder's slot upon exit.
 * \return The pnp_status that was updated.
 */
pnp_status_t build_config(device_setup_builder auto &&builder) {
    // Mask for status bits that will be used.
    static constexpr pnp_status_t PNP_STATUS_MASK = ~(
        pnp_status::GENERAL_CONFIG_ERROR | pnp_status::CONFIGURATION_SET_MISS |
        pnp_status::CONFIGURATION_STRUCT_MISS);

    Slots &info = slots[builder.slot()];
    const bool nv_available = builder.load_from_nv();

    pnp_status_t rt = pnp_status::NO_ERRORS;

    if (!info.save.allow_device_detection) {
        if (!nv_available) {
            builder.setup_default_values();
        }
    } else {
        lut_manager::device_lut_key_t key;
        uint64_t serial_number;
        std::unique_ptr<uint8_t> slot_config;
        std::unique_ptr<uint8_t> config_entries;
        uint16_t slot_config_size = 0;
        uint16_t config_entries_size = 0;
        if (rt == pnp_status::NO_ERRORS) {
            lock_guard lg(info.xSlot_Mutex);
            key.running_slot_card = info.card_type;
            key.connected_device = info.device.info.signature;
            serial_number = info.device.info.serial_num;
            if (info.device.p_slot_config != nullptr) {
                slot_config =
                    std::unique_ptr<uint8_t>(info.device.p_slot_config);
                info.device.p_slot_config = nullptr;
                slot_config_size = info.device.p_slot_config_size;
            }
            if (info.device.p_config_entries != nullptr) {
                config_entries =
                    std::unique_ptr<uint8_t>(info.device.p_config_entries);
                info.device.p_config_entries = nullptr;
                config_entries_size = info.device.p_config_entries_size;
            }
        }
        if (rt == pnp_status::NO_ERRORS && nv_available &&
            builder.can_device_run_on_current_params(
                info.device.info.signature, info.device.info.serial_num)) {
        } else {

            // The size requirement check for the lut_manager::load_data
            // function.
            std::span<config_signature_t> config_signatures =
                builder.get_configuration_span();

            if (rt == pnp_status::NO_ERRORS) {
                if (slot_config_size == 0) {
                    if (::save_constructor::get_slot_save_size(info.card_type)) {
                        rt = pnp_status::GENERAL_CONFIG_ERROR;
                    } else {
                        LUT_ERROR err = lut_manager::load_data(
                            LUT_ID::DEVICE_LUT, &key, config_signatures.data());

                        if (err != LUT_ERROR::OKAY) {
                            rt = pnp_status::GENERAL_CONFIG_ERROR |
                                 pnp_status::CONFIGURATION_SET_MISS;
                        }
                    }
                } else if (slot_config_size != config_signatures.size_bytes()) {
                    rt = pnp_status::GENERAL_CONFIG_ERROR;
                } else {
                    // Copies the slot config into memory.
                    std::memcpy(
                        std::as_writable_bytes(config_signatures).data(),
                        slot_config.get(), slot_config_size);
                }
            }

            if (rt == pnp_status::NO_ERRORS) {
                std::span<std::byte> buffer = builder.get_struct_buffer();
                auto load_to_buffer =
                    [&](const config_signature_t &signature) -> bool {
                        return ::save_constructor::construct_single(
                               signature, buffer.data(), config_entries.get(),
                               config_entries_size) != 0;
                };
                for (config_signature_t &signature : config_signatures) {
                    const uint32_t SIZE =
                        ::save_constructor::get_config_size(signature.struct_id);
                    if (SIZE == 0) {
                        // Invalid structure
                        rt = pnp_status::GENERAL_CONFIG_ERROR |
                             pnp_status::CONFIGURATION_SET_MISS;
                    } else if (SIZE > buffer.size()) {
                        // Buffer was too small.
                        rt = pnp_status::GENERAL_CONFIG_ERROR;
                    } else if (builder.get_next_struct_to_load() !=
                               signature.struct_id) {
                        // Order violation
                        rt = pnp_status::GENERAL_CONFIG_ERROR;
                    } else if (!load_to_buffer(signature)) {
                        rt = pnp_status::GENERAL_CONFIG_ERROR |
                             pnp_status::CONFIGURATION_STRUCT_MISS;
                    } else if (!builder.load_serialized_struct(
                                   signature.struct_id,
                                   buffer.subspan(0, SIZE))) {
                        // Load failed
                        rt = pnp_status::GENERAL_CONFIG_ERROR;
                    }

                    if (rt != pnp_status::NO_ERRORS) {
                        break;
                    }
                }
            }

            // Check that the parmameters were finalized.
            if (rt == pnp_status::NO_ERRORS && !builder.finish_loading()) {
                rt = pnp_status::GENERAL_CONFIG_ERROR;
            }
        }
    }

    // Update the slot's pnp status.
    {
        lock_guard lg(info.xSlot_Mutex);
        info.pnp_status = (info.pnp_status & PNP_STATUS_MASK) | rt;
    }
    return rt;
}

} // namespace drivers::device_detection

// EOF
