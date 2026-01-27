/**
 * \file flipper-shutter-query.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-20
 *
 * The flipper shutter uses a query algorithm to configure its settings.
 * The LUT database in the firmware is used to query for any overloads
 * to the structure defined here for a given device.
 * Otherwise, every shutter cable will be able to be instantiated by
 * data on the cable.
 *
 * To instantiate a sio channel, the query to the channel must be
 * successful.
 * This is a one-byte structure.
 * If a value does not exist on the one-wire, we assume the channel does not
 * exist.
 * If a value does not exist on the configuration LUT, we assume a config
 * struct miss.
 * Sio settings by default exist at pointer id 0xFFF0 - 0xFFF2.
 *
 * To instantiate a shutter, the query must find a shutter settings
 * and both waveforms for the shutter's channel.
 * The settings structure is 2 bytes long.
 * Each waveform structure is 7 bytes long.
 * If all the three structures are missing on the one-wire, we assume the
 * channel does not exist.
 * If a structure is missing in any other configuration, we assume a
 * configuration sturct miss.
 * Shutter channel 1's settings exist at pointer id
 * 0xFFF3, 0xFFF4, and 0xFFF5 (settings, open waveform, closed waveform).
 * Shutter channel 2's settings exist at pointer id
 * 0xFFF6, 0xFFF7, and 0xFFF8 (settings, open waveform, closed waveform).
 * Shutter channel 3's settings exist at pointer id
 * 0xFFF9, 0xFFFA, and 0xFFFB (settings, open waveform, closed waveform).
 * Shutter channel 4's settings exist at pointer id
 * 0xFFFC, 0xFFFD, and 0xFFFE (settings, open waveform, closed waveform).
 *
 * A full load is 3 sios and 3 shutters (b/c the cable does not support
 * the 4th shutter).
 * The interlock setting must be loaded from board RAM.
 * the 4th shutter setting must be loaded from board RAM or a custom
 * configuration in the device and config LUT.
 * */
#pragma once

#include <expected>

#include "channels.hh"
#include "defs.h"
#include "defs.hh"
#include "device_detect.h"
#include "flipper-shutter-driver.hh"
#include "flipper-shutter-persistence.hh"
#include "lut_manager.hh"
#include "optional-like.hh"
#include "pnp_status.h"
#include "pnp_status.hh"
#include "save_size.hh"
#include "slot_nums.h"
#include "spi-transfer-handle.hh"
#include "structure_id.hh"
#include "structures/flipper_shutter_shutter_common.hh"
#include "structures/flipper_shutter_sio.hh"
#include "structures/shutter_controller_waveform.hh"
#include "user-eeprom.hh"
#include "value_type.hh"

namespace cards::flipper_shutter::persistence {

/**
 * Top-level builder-like object for interacting with the configuration
 * of the flipper shutter card.
 */
class setup_query {
   public:
    const card_settings* get_card_settings() const;
    const sio_settings* get_sio_settings(driver::channel_id channel) const;
    const shutter_settings* get_shutter_settings(
        driver::channel_id channel) const;

    void add(const persistence::card_settings& value);
    void add(const persistence::sio_settings& value,
             driver::channel_id channel);
    void add(const persistence::shutter_settings& value,
             driver::channel_id channel);

   private:
    std::array<std::optional<persistence::shutter_settings>,
               std::size(driver::CHANNEL_RANGE_SHUTTER)>
        _shutters;
    std::array<std::optional<persistence::sio_settings>,
               std::size(driver::CHANNEL_RANGE_SIO)>
        _sios;
    std::optional<persistence::card_settings> _card_settings;
};

struct sio_channel_config {
    drivers::save_constructor::flipper_shutter_sio config;
    driver::channel_id channel;
};

struct shutter_channel_config {
    drivers::save_constructor::shutter_controller_waveform positive_waveform;
    drivers::save_constructor::shutter_controller_waveform negative_waveform;
    drivers::save_constructor::flipper_shutter_shutter_common common;
    driver::channel_id channel;
};

enum class query_errors_e {
    SUCCESS = 0,
};

class device_query {
   public:
    struct shutter_bundle {
        const lut_manager::config_lut_key_t* common;
        const lut_manager::config_lut_key_t* open_waveform;
        const lut_manager::config_lut_key_t* closed_waveform;
    };

    inline const lut_manager::config_lut_key_t* sio_config(
        driver::channel_id channel) const {
        static constexpr std::size_t OFFSET = 0;
        static constexpr const auto& RANGE  = driver::CHANNEL_RANGE_SIO;
        return utils::is_channel_in_range(channel, RANGE)
                   ? &_keys[OFFSET + utils::channel_to_index(channel, RANGE)]
                   : nullptr;
    }
    inline shutter_bundle shutter_config(driver::channel_id channel) const {
        static constexpr std::size_t OFFSET =
            std::size(driver::CHANNEL_RANGE_SIO);
        static constexpr const auto& RANGE = driver::CHANNEL_RANGE_SHUTTER;
        shutter_bundle rt;
        if (utils::is_channel_in_range(channel, RANGE)) {
            const lut_manager::config_lut_key_t* BASE =
                &_keys[OFFSET + 3 * utils::channel_to_index(channel, RANGE)];
            rt.common          = BASE + 0;
            rt.open_waveform   = BASE + 1;
            rt.closed_waveform = BASE + 2;
        } else {
            rt.common          = nullptr;
            rt.open_waveform   = nullptr;
            rt.closed_waveform = nullptr;
        }
        return rt;
    }

    static std::expected<device_query, query_errors_e> query(
        drivers::spi::handle_factory& factory,
        const lut_manager::device_lut_key_t& key);

    static std::expected<device_query, query_errors_e> from_array(
        const std::span<lut_manager::config_lut_key_t, 15>& keys);

    static constexpr device_query get_defaults() {
        device_query rt;

        for (std::size_t i = 0; i < rt._keys.size(); ++i) {
            if (i < 3) {
                rt._keys[i].struct_id =
                    static_cast<struct_id_t>(Struct_ID::FLIPPER_SHUTTER_SIO);
            } else {
                const std::size_t PART = (i - 3) % 3;
                switch (PART) {
                case 0:
                    rt._keys[i].struct_id = static_cast<struct_id_t>(
                        Struct_ID::FLIPPER_SHUTTER_SHUTTER_COMMON);
                    break;
                case 1:
                case 2:
                    rt._keys[i].struct_id = static_cast<struct_id_t>(
                        Struct_ID::SHUTTER_CONTROLLER_WAVEFORM);
                    break;
                }
            }
            rt._keys[i].config_id = static_cast<uint16_t>(0xFFF0 + i);
        }

        return rt;
    }

   private:
    std::array<lut_manager::config_lut_key_t, 15> _keys;
};

// class onewire_database {
//   public:
//     struct config_query_result {
//         std::span<const std::byte> payload;
//         struct_id_t type;
//         config_id_t id;
//     };
//
//     std::optional<device_query> query_device(slot_types attached_card);
//
//     config_query_result query_config(config_id_t id);
//
//     inline config_query_result query_config(std::size_t index) {
//         if (index > 15) {
//             return {};
//         }
//
//         return query_config((config_id_t)(0xFFF0 + index));
//     }
// };
//
// class lut_query_handle {
//   public:
//     static std::expected<device_query, query_errors_e>
//     query_device(const lut_manager::device_lut_key_t &key);
//
//     static std::expected<drivers::save_constructor::flipper_shutter_sio,
//                          query_errors_e>
//     query_sio(config_id_t id);
//
//     static std::expected<
//         drivers::save_constructor::flipper_shutter_shutter_common,
//         query_errors_e>
//     query_shutter_common(config_id_t id);
//
//     static
//     std::expected<drivers::save_constructor::shutter_controller_waveform,
//                          query_errors_e>
//     query_waveform(config_id_t id);
//
//   private:
//     drivers::spi::handle_factory &_factory;
//     drivers::eeprom::page_cache &_cache;
// };

struct query_params {
    uint64_t connected_sn;
    slot_types connected_card;
};

/**
* Loads data from the persistence_controller to the setup query.
* If ignore_signature_matching is false, then the saved configuration
* and the connected device must have the same signature and serial number.
* If ignore_signature_matching is true, then this matching check is ignored.
*/
template <layout_defintion Layout>
pnp_status_t apply_query_to_setup(const query_params& params, setup_query& dest,
                                  persistence_controller<Layout>& src,
                                  bool ignore_signature_matching = false) {
    if (auto settings = src.get_card_settings(); settings) {
        dest.add(*settings);
    }

    auto can_run = [&params, ignore_signature_matching](
                       const uint64_t& serial_num) -> bool {
        return ignore_signature_matching || serial_num == OW_SERIAL_NUMBER_WILDCARD
            || serial_num == params.connected_sn;
    };

    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SIO)) {
        if (dest.get_sio_settings(channel) != nullptr) {
            continue;
        }

        if (auto settings = src.get_sio_settings(channel);
            settings &&
            can_run(settings->saved_serial_number)) {
            dest.add(*settings, channel);
        }
    }

    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SHUTTER)) {
        if (dest.get_shutter_settings(channel) != nullptr) {
            continue;
        }

        if (auto settings = src.get_shutter_settings(channel);
            settings &&
            can_run(settings->saved_serial_number)) {
            dest.add(*settings, channel);
        }
    }

    return pnp_status::NO_ERRORS;
}

}  // namespace cards::flipper_shutter::persistence
