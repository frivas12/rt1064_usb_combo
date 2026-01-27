
/**
 * \file flipper-shutter-persistence.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-24
 *
 * The flipper shutter spreads its saved settings out over
 * 6 of its 10 EEPROM pages.
 * Page 0 is used for top-level settings.
 * Page 1 is used for all the SIO channel settings.
 * Page 2 through 5 are used for the shutter channel settings,
 * on page each.
 *
 * These persistence value are designed to be robust against data
 * errors and integrate into the existing query system.
 * Unlike previous iterations, the saved settings can be queried and
 * configures customly on a per-channel basis for shutters
 * and a channel range basis for flippers mirrors (SIOs).
 *
 * Each page has a basic format:
 * Byte(s)          Description
 * 0                CRC8 Validator
 * 1 - 2            Length Identifier.
 * 3                Payload version enum
 * 4+               Payload
 *
 * For channel pages 2 through 5:
 * 4 - 7            Saved device signature
 * 8 - 15           Saved device serial number
 * 16+              Payload
 *
 * The validator simply hashes the bytes 1, 2, and then the remaining number of
 * bytes specified by the length identifier. If the length identifier goes off
 * the page or the CRC8 is invalid, the page is assumed to be invalid and not
 * applied. For channel pages, if the currently connected device does not match
 * the saved device values AND device detection has NOT been disabled, the page
 * is invalid.
 */
#pragma once

#include <cstdint>
#include <expected>

#include "25lc1024.h"
#include "board.h"
#include "channels.hh"
#include "cpld-shutter-driver.hh"
#include "defs.h"
#include "flipper-shutter-driver.hh"
#include "flipper-shutter-types.hh"
#include "integer-serialization.hh"
#include "shutter-controller.hh"
#include "slot_nums.h"
#include "spi-transfer-handle.hh"
#include "user-eeprom.hh"

namespace cards::flipper_shutter::persistence {

enum class serializer_errors_e {
    SUCCESS = 0,

    LAYOUT_ERROR_TOO_SMALL =
        1,  // Occurs when the layout gives the serializer too few bytes.
            //
    LAYOUT_ERROR_INVALID_CHANNEL =
        2,  // Occurs when layout does not support the given channel.

    SERIAL_ERROR_SIZE_INVALD =
        1000,  // Occurs when the reported size of the correct type is invalid.
    SERIAL_ERROR_UNSUPPORTED_VERSION =
        1001,                         // Occurs when the type is not supported.
    SERIAL_ERROR_CRC_INVALID = 1002,  // Occurs when the CRC is incorrect.
    SERIAL_ERROR_INVALID_VALUE =
        1003,  // Occurs when the struct value is invalid.
};

using page_view = std::span<std::byte, EEPROM_25LC1024_PAGE_SIZE>;

template <typename T>
concept layout_defintion =
    requires(T t, slot_nums slot, driver::channel_id channel) {
        {
            t.card_settings(slot)
        } -> std::same_as<drivers::eeprom::address_span>;
        {
            t.sio_settings(slot, channel)
        } -> std::same_as<drivers::eeprom::address_span>;
        {
            t.shutter_settings(slot, channel)
        } -> std::same_as<drivers::eeprom::address_span>;
    };

struct page_header {
    uint16_t length;
    uint8_t crc;
    uint8_t version;

    constexpr void serialize(const std::span<std::byte, 4>& dest) const {
        dest[0] = std::byte{crc};
        native_endian_serializer().serialize(dest.subspan<1, 2>(), length);
        dest[3] = std::byte{version};
    }

    static constexpr page_header deserialize(
        const std::span<const std::byte, 4>& src) {
        auto ser = native_endian_serializer();
        return page_header{
            .length  = ser.deserialize_uint16(src.subspan<1, 2>()),
            .crc     = ser.deserialize_uint8(src.subspan<0, 1>()),
            .version = ser.deserialize_uint8(src.subspan<3, 1>()),
        };
    }
};

struct card_settings {
    bool enable_interlock_cable;
    bool shutter_ttl_control_disables_usb_control;

    static constexpr card_settings get_defaults() noexcept {
        return card_settings{
            .enable_interlock_cable                   = false,
            .shutter_ttl_control_disables_usb_control = false,
        };
    }

    class serializer_v0 {
       public:
        static constexpr std::size_t SIZE = 1 + 1;

        std::expected<card_settings, serializer_errors_e> deserialize(
            drivers::spi::handle_factory& factory,
            drivers::eeprom::page_cache& view, const page_header& header);
        serializer_errors_e serialize(const card_settings& value,
                                      drivers::spi::handle_factory& factory,
                                      drivers::eeprom::page_cache& view);
        serializer_errors_e erase(drivers::spi::handle_factory& factory,
                                  drivers::eeprom::page_cache& view);

        static std::expected<serializer_v0, serializer_errors_e> create(
            layout_defintion auto&& layout, slot_nums slot) {
            auto addrs = layout.card_settings(slot);

            if (addrs.size() < SIZE - 1) {
                return std::expected<serializer_v0, serializer_errors_e>{
                    std::unexpect, serializer_errors_e::LAYOUT_ERROR_TOO_SMALL};
            }

            return serializer_v0(addrs);
        };

       private:
        drivers::eeprom::address_span _span;

        serializer_v0(drivers::eeprom::address_span span);
    };
};

struct sio_settings {
    uint64_t saved_serial_number;
    bool drives_high;
    bool is_high_level_in_position;

    static constexpr sio_settings get_defaults() noexcept {
        return sio_settings{
            .saved_serial_number       = 0xFFFFFFFFFFFFFFFF,
            .drives_high               = true,
            .is_high_level_in_position = true,
        };
    };

    class serializer_v0 {
       public:
        static constexpr std::size_t SIZE = 1 + 8 + 4 + 3 * (1);

        std::expected<sio_settings, serializer_errors_e> deserialize(
            drivers::spi::handle_factory& factory,
            drivers::eeprom::page_cache& view, const page_header& header);
        serializer_errors_e serialize(const sio_settings& value,
                                      drivers::spi::handle_factory& factory,
                                      drivers::eeprom::page_cache& view);
        serializer_errors_e erase(drivers::spi::handle_factory& factory,
                                  drivers::eeprom::page_cache& view);

        static std::expected<serializer_v0, serializer_errors_e> create(
            layout_defintion auto&& layout, slot_nums slot,
            driver::channel_id channel) {
            auto addrs = layout.sio_settings(slot, channel);

            if (addrs.size() < SIZE - 1) {
                return std::expected<serializer_v0, serializer_errors_e>{
                    std::unexpect, serializer_errors_e::LAYOUT_ERROR_TOO_SMALL};
            }
            if (!utils::is_channel_in_range(channel,
                                            driver::CHANNEL_RANGE_SIO)) {
                return std::expected<serializer_v0, serializer_errors_e>{
                    std::unexpect,
                    serializer_errors_e::LAYOUT_ERROR_INVALID_CHANNEL};
            }

            return serializer_v0(addrs, channel);
        };

       private:
        drivers::eeprom::address_span _span;
        driver::channel_id _channel;

        serializer_v0(drivers::eeprom::address_span span,
                      driver::channel_id channel);
    };
};

struct shutter_settings {
    struct serialized_waveform {
        TickType_t actuation_duration = 0;
        TickType_t actuation_holdoff  = 0;
        float actuation_voltage       = 0;
        float holding_voltage         = 0;

        constexpr cards::shutter::controller::state_waveform to_state_waveform(
            float bus_voltage,
            drivers::cpld::shutter_module::period_type pwm_period) const {
            using namespace drivers::cpld::shutter_module;

            const duty_type ACTUATION_DUTY =
                abs(to_duty_type(actuation_voltage, bus_voltage, pwm_period));
            const duty_type HOLDING_DUTY =
                abs(to_duty_type(holding_voltage, bus_voltage, pwm_period));

            return cards::shutter::controller::state_waveform{
                .actuation_duration = actuation_duration,
                .actuation_holdoff  = actuation_holdoff,
                .actuation_duty     = ACTUATION_DUTY,
                .holding_duty       = HOLDING_DUTY,
                .polarity           = actuation_voltage > 0};
        }

        constexpr serialized_waveform& from_state_waveform(
            const cards::shutter::controller::state_waveform& src,
            float bus_voltage,
            drivers::cpld::shutter_module::period_type pwm_period) {
            using namespace drivers::cpld::shutter_module;

            const float COEFF = src.polarity ? 1 : -1;

            actuation_duration = src.actuation_duration;
            actuation_holdoff  = src.actuation_holdoff;
            actuation_voltage =
                COEFF * to_voltage(src.actuation_duty, bus_voltage, pwm_period);
            holding_voltage =
                COEFF * to_voltage(src.holding_duty, bus_voltage, pwm_period);

            return *this;
        }
    };
    uint64_t saved_serial_number;
    serialized_waveform open_waveform;
    serialized_waveform closed_waveform;
    trigger_modes_e trigger_mode;
    cards::shutter::controller::states_e state_at_powerup;

    static constexpr shutter_settings get_defaults() noexcept {
        return shutter_settings{
            .saved_serial_number = 0xFFFFFFFFFFFFFFFF,
            .open_waveform       = {},
            .closed_waveform     = {},
            .trigger_mode        = trigger_modes_e::DISABLED,
            .state_at_powerup = cards::shutter::controller::states_e::UNKNOWN,
        };
    }

    class serializer_base {
        drivers::eeprom::address_span _span;

       protected:
        constexpr serializer_base(const drivers::eeprom::address_span& span)
            : _span(span) {}
        constexpr const drivers::eeprom::address_span& span() const {
            return _span;
        }

       public:
        serializer_errors_e erase(drivers::spi::handle_factory& factory,
                                  drivers::eeprom::page_cache& view);
    };

    class serializer_v0 : public serializer_base {
       public:
        static constexpr std::size_t SIZE = 1          // version
                                            + 8        // serial number
                                            + 2 * (8)  // waveforms
                                            + 1        // trigger mode
                                            + 1        // state at powerup
            ;

        std::expected<shutter_settings, serializer_errors_e> deserialize(
            drivers::spi::handle_factory& factory,
            drivers::eeprom::page_cache& view, const page_header& header);

        static std::expected<serializer_v0, serializer_errors_e> create(
            layout_defintion auto&& layout, slot_nums slot,
            driver::channel_id channel) {
            auto addrs = layout.shutter_settings(slot, channel);

            if (addrs.size() < SIZE - 1) {
                return std::expected<serializer_v0, serializer_errors_e>{
                    std::unexpect, serializer_errors_e::LAYOUT_ERROR_TOO_SMALL};
            }
            if (!utils::is_channel_in_range(channel,
                                            driver::CHANNEL_RANGE_SHUTTER)) {
                return std::expected<serializer_v0, serializer_errors_e>{
                    std::unexpect,
                    serializer_errors_e::LAYOUT_ERROR_INVALID_CHANNEL};
            }

            return serializer_v0(addrs);
        };

       private:
        using time_format =
            std::chrono::duration<uint32_t, std::ratio<1, 10000>>;

        constexpr serializer_v0(drivers::eeprom::address_span span)
            : serializer_base(span) {}

        static bool deserialize(serialized_waveform& dest,
                                const std::span<const std::byte, 8>& src);
    };

    // Changelog: Appended 4 bytes at the end of each waveform to store
    //            the actuation holdoff time.
    class serializer_v1 : public serializer_base {
       public:
        static constexpr std::size_t SIZE = 1           // version
                                            + 8         // serial number
                                            + 2 * (16)  // waveforms
                                            + 1         // trigger mode
                                            + 1         // state at powerup
            ;

        std::expected<shutter_settings, serializer_errors_e> deserialize(
            drivers::spi::handle_factory& factory,
            drivers::eeprom::page_cache& view, const page_header& header);
        serializer_errors_e serialize(const shutter_settings& value,
                                      drivers::spi::handle_factory& factory,
                                      drivers::eeprom::page_cache& view);

        static std::expected<serializer_v1, serializer_errors_e> create(
            layout_defintion auto&& layout, slot_nums slot,
            driver::channel_id channel) {
            auto addrs = layout.shutter_settings(slot, channel);

            if (addrs.size() < SIZE - 1) {
                return std::expected<serializer_v1, serializer_errors_e>{
                    std::unexpect, serializer_errors_e::LAYOUT_ERROR_TOO_SMALL};
            }
            if (!utils::is_channel_in_range(channel,
                                            driver::CHANNEL_RANGE_SHUTTER)) {
                return std::expected<serializer_v1, serializer_errors_e>{
                    std::unexpect,
                    serializer_errors_e::LAYOUT_ERROR_INVALID_CHANNEL};
            }

            return serializer_v1(addrs);
        };

       private:
        using time_format =
            std::chrono::duration<uint32_t, std::ratio<1, 10000>>;

        constexpr serializer_v1(drivers::eeprom::address_span span)
            : serializer_base(span) {}

        static void serialize(const std::span<std::byte, 16>& dest,
                              const serialized_waveform& value);
        static bool deserialize(serialized_waveform& dest,
                                const std::span<const std::byte, 16>& src);
    };
};

template <layout_defintion Layout>
class persistence_controller {
   public:
    using layout = Layout;

    /// \brief Attempts to deserialize the card settings.
    std::expected<card_settings, serializer_errors_e> get_card_settings() {
        page_header header = read_header(_layout.card_settings(_slot));

        switch (header.version) {
        case 0:
            if (auto ser = card_settings::serializer_v0::create(_layout, _slot);
                ser) {
                return ser->deserialize(_spi, _cache, header);
            } else {
                return std::expected<card_settings, serializer_errors_e>(
                    std::unexpect, ser.error());
            }
        default:
            return std::expected<card_settings, serializer_errors_e>(
                std::unexpect,
                serializer_errors_e::SERIAL_ERROR_UNSUPPORTED_VERSION);
        }
    }

    /// \brief Attempts to deserialize the sio settings for the given channel.
    std::expected<sio_settings, serializer_errors_e> get_sio_settings(
        driver::channel_id channel) {
        page_header header = read_header(_layout.sio_settings(_slot, channel));

        switch (header.version) {
        case 0:
            if (auto ser = sio_settings::serializer_v0::create(_layout, _slot,
                                                               channel);
                ser) {
                return ser->deserialize(_spi, _cache, header);
            } else {
                return std::expected<sio_settings, serializer_errors_e>(
                    std::unexpect, ser.error());
            }
        default:
            return std::expected<sio_settings, serializer_errors_e>(
                std::unexpect,
                serializer_errors_e::SERIAL_ERROR_UNSUPPORTED_VERSION);
        }
    }

    /// \brief Attempts to deserialize the shutter settings for the given
    /// channel.
    std::expected<shutter_settings, serializer_errors_e> get_shutter_settings(
        driver::channel_id channel) {
        page_header header =
            read_header(_layout.shutter_settings(_slot, channel));

        switch (header.version) {
        case 0:
            if (auto ser = shutter_settings::serializer_v0::create(
                    _layout, _slot, channel);
                ser) {
                return ser->deserialize(_spi, _cache, header);
            } else {
                return std::expected<shutter_settings, serializer_errors_e>(
                    std::unexpect, ser.error());
            }
        case 1:
            if (auto ser = shutter_settings::serializer_v1::create(
                    _layout, _slot, channel);
                ser) {
                return ser->deserialize(_spi, _cache, header);
            } else {
                return std::expected<shutter_settings, serializer_errors_e>(
                    std::unexpect, ser.error());
            }
        default:
            return std::expected<shutter_settings, serializer_errors_e>(
                std::unexpect,
                serializer_errors_e::SERIAL_ERROR_UNSUPPORTED_VERSION);
        }
    }

    serializer_errors_e set_card_settings(
        const std::optional<card_settings>& value) {
        using serializer = card_settings::serializer_v0;

        if (auto ser = serializer::create(_layout, _slot); ser) {
            if (value) {
                return ser->serialize(*value, _spi, _cache);
            } else {
                return ser->erase(_spi, _cache);
            }
        } else {
            return ser.error();
        }
    }
    serializer_errors_e set_card_settings(const card_settings& value) {
        return set_card_settings(std::optional(value));
    }

    serializer_errors_e set_sio_settings(
        const std::optional<sio_settings>& value, driver::channel_id channel) {
        using serializer = sio_settings::serializer_v0;

        if (auto ser = serializer::create(_layout, _slot, channel); ser) {
            if (value) {
                return ser->serialize(*value, _spi, _cache);
            } else {
                return ser->erase(_spi, _cache);
            }
        } else {
            return ser.error();
        }
    }
    serializer_errors_e set_sio_settings(const sio_settings& value,
                                         driver::channel_id channel) {
        return set_sio_settings(std::optional(value), channel);
    }

    serializer_errors_e set_shutter_settings(
        const std::optional<shutter_settings>& value,
        driver::channel_id channel) {
        using serializer = shutter_settings::serializer_v1;

        if (auto ser = serializer::create(_layout, _slot, channel); ser) {
            if (value) {
                return ser->serialize(*value, _spi, _cache);
            } else {
                return ser->erase(_spi, _cache);
            }
        } else {
            return ser.error();
        }
    }
    serializer_errors_e set_shutter_settings(const shutter_settings& value,
                                             driver::channel_id channel) {
        return set_shutter_settings(std::optional(value), channel);
    }

    persistence_controller(slot_nums slot, Layout& layout,
                           drivers::spi::handle_factory& spi,
                           drivers::eeprom::page_cache& cache)
        : _layout(layout), _spi(spi), _cache(cache), _slot(slot) {}

    persistence_controller(slot_nums slot, Layout&& layout,
                           drivers::spi::handle_factory& spi,
                           drivers::eeprom::page_cache& cache)
        : _layout(std::move(layout)), _spi(spi), _cache(cache), _slot(slot) {}

   private:
    Layout _layout;
    drivers::spi::handle_factory& _spi;
    drivers::eeprom::page_cache& _cache;
    slot_nums _slot;

    page_header read_header(const drivers::eeprom::address_span& addrs) const {
        _cache.flush(_spi);
        _cache.cache(_spi, addrs.head_address);

        // This will always work, so we can turn it into static.
        return page_header::deserialize(
            _cache.subspan(addrs.head_address, 4).subspan<0, 4>());
    }
};

/// \brief Layout for the 25lc1024 EEPROM.
class layout_25lc1024 {
   public:
    static constexpr bool is_board_supported(board_types type) {
        switch (type) {
        case MCM_41_0117_001:
        case MCM_41_0134_001:
            return true;

        default:
            return false;
        };
    }

    constexpr drivers::eeprom::address_span card_settings(slot_nums slot) {
        return drivers::eeprom::regions::slot(slot).split_and_index(10, 0);
    }
    constexpr drivers::eeprom::address_span sio_settings(
        slot_nums slot, driver::channel_id channel) {
        return drivers::eeprom::regions::slot(slot).split_and_index(10, 1);
    }
    constexpr drivers::eeprom::address_span shutter_settings(
        slot_nums slot, driver::channel_id channel) {
        return drivers::eeprom::regions::slot(slot).split_and_index(
            10, 2 + channel);
    }
};

}  // namespace cards::flipper_shutter::persistence

// EOF
