#include "flipper-shutter-persistence.hh"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <utility>

#include "FreeRTOSConfig.h"
#include "channels.hh"
#include "cpld-shutter-driver.hh"
#include "defs.h"
#include "integer-serialization.hh"
#include "shutter-controller.hh"
#include "spi-transfer-handle.hh"
#include "stream-io.hh"
#include "user-eeprom.hh"

using namespace cards::flipper_shutter;
using namespace cards::flipper_shutter::persistence;

using std::byte;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
template <typename T>
using return_t = std::expected<T, serializer_errors_e>;

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void write_header(drivers::eeprom::cache_backend_eeprom_stream& dest,
                         const page_header& header);
static void write_device_header(
    drivers::eeprom::cache_backend_eeprom_stream& cache,
    const uint64_t& serial_number);
static uint8_t old_calculate_crc(
    drivers::eeprom::cache_backend_eeprom_stream& stream, uint16_t length);
static uint8_t calculate_crc(
    drivers::eeprom::cache_backend_eeprom_stream& stream, uint16_t length);
static serializer_errors_e check_header(uint16_t size, uint8_t version,
                                        uint8_t crc, const page_header& header);
static void deserialize_device_header(
    drivers::eeprom::cache_backend_eeprom_stream& stream, uint64_t& sn_dest);
static serializer_errors_e erase(
    drivers::eeprom::cache_backend_eeprom_stream& cache);

template <
    std::invocable<drivers::eeprom::cache_backend_eeprom_stream&> Serializer>
static std::size_t serialize_with_header(
    page_header& header, drivers::eeprom::cache_backend_eeprom_stream& stream,
    Serializer&& callback) {
    std::array<byte, 4> header_buffer;
    header.serialize(std::span(header_buffer));
    const auto BEGIN_ADDR = stream.address();
    // Skip over the CRC.  Then, write the length, version, and execute the
    // callback.
    stream.seek(pnp_database::seeking::current, 1);
    stream.write(std::span(header_buffer).subspan(1));
    callback(stream);
    const auto END_ADDR = stream.address();
    const std::size_t CALLBACK_WIDTH =
        static_cast<std::size_t>(END_ADDR - BEGIN_ADDR) -
        4;  // -4 for the header.

    // Seek to just after the CRC and calculate the CRC term.
    stream.seek(pnp_database::seeking::begin, BEGIN_ADDR + 1);
    header.crc = calculate_crc(
        stream, header.length + 2);  // +2 for the length term itself.

    // Write the CRC term and go to the end.
    stream.seek(pnp_database::seeking::begin, BEGIN_ADDR);
    stream.write(static_cast<byte>(header.crc));
    stream.seek(pnp_database::seeking::begin, END_ADDR);

    return CALLBACK_WIDTH;
}

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
std::expected<card_settings, serializer_errors_e>
card_settings::serializer_v0::deserialize(drivers::spi::handle_factory& factory,
                                          drivers::eeprom::page_cache& view,
                                          const page_header& header) {
    using return_t = return_t<card_settings>;

    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);
    auto error = check_header(SIZE, 0, old_calculate_crc(stream, SIZE), header);
    if (error == serializer_errors_e::SUCCESS) {
        card_settings rt = card_settings::get_defaults();

        rt.enable_interlock_cable = stream.read() != byte{0};

        return rt;
    } else {
        return return_t{std::unexpect, error};
    }
}
serializer_errors_e card_settings::serializer_v0::serialize(
    const card_settings& value, drivers::spi::handle_factory& factory,
    drivers::eeprom::page_cache& view) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);

    // Order matters.
    stream.write(byte(value.enable_interlock_cable ? 1 : 0));

    page_header header{
        .length  = SIZE,
        .crc     = old_calculate_crc(stream, SIZE),
        .version = 0,
    };
    write_header(stream, header);
    return serializer_errors_e::SUCCESS;
}
serializer_errors_e card_settings::serializer_v0::erase(
    drivers::spi::handle_factory& factory, drivers::eeprom::page_cache& cache) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        cache);
    return ::erase(stream);
}

std::expected<sio_settings, serializer_errors_e>
sio_settings::serializer_v0::deserialize(drivers::spi::handle_factory& factory,
                                         drivers::eeprom::page_cache& view,
                                         const page_header& header) {
    using return_t = return_t<sio_settings>;
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);

    auto error = check_header(SIZE, 0, old_calculate_crc(stream, SIZE), header);
    if (error == serializer_errors_e::SUCCESS) {
        sio_settings settings = sio_settings::get_defaults();

        // Order matters.
        deserialize_device_header(stream, settings.saved_serial_number);
        // There are 3 channels--we are skipping over the ones we don't want.
        auto idx = utils::channel_to_index(_channel, driver::CHANNEL_RANGE_SIO);
        stream.seek(pnp_database::seeking::current, idx);
        const uint8_t BYTE1 = std::to_integer<uint8_t>(*stream.read());
        stream.seek(pnp_database::seeking::current, 2 - idx);

        settings.drives_high               = (BYTE1 & (1 << 0)) != 0;
        settings.is_high_level_in_position = (BYTE1 & (1 << 1)) != 0;

        return return_t{settings};
    } else {
        return return_t{std::unexpect, error};
    }
}
serializer_errors_e sio_settings::serializer_v0::serialize(
    const sio_settings& value, drivers::spi::handle_factory& factory,
    drivers::eeprom::page_cache& view) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);

    write_device_header(stream, value.saved_serial_number);

    byte byte1 = byte(((value.drives_high ? 1 : 0) << 0) |
                      ((value.is_high_level_in_position ? 1 : 0) << 1));

    // B/c we declare that the stream is cache-based, the page cache is
    // guaranteed to load the previous values.
    auto idx = utils::channel_to_index(_channel, driver::CHANNEL_RANGE_SIO);
    stream.seek(pnp_database::seeking::current, idx);
    stream.write(byte1);
    stream.seek(pnp_database::seeking::current, 2 - idx);

    page_header header{
        .length  = SIZE,
        .crc     = old_calculate_crc(stream, SIZE),
        .version = 0,
    };
    write_header(stream, header);
    return serializer_errors_e::SUCCESS;
}
serializer_errors_e sio_settings::serializer_v0::erase(
    drivers::spi::handle_factory& factory, drivers::eeprom::page_cache& view) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);
    return ::erase(stream);
}

serializer_errors_e shutter_settings::serializer_base::erase(
    drivers::spi::handle_factory& factory, drivers::eeprom::page_cache& view) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(_span);
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);
    return ::erase(stream);
}

std::expected<shutter_settings, serializer_errors_e>
shutter_settings::serializer_v0::deserialize(
    drivers::spi::handle_factory& factory, drivers::eeprom::page_cache& view,
    const page_header& header) {
    using return_t = return_t<shutter_settings>;
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(span());
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);

    const auto BEGIN_ADDR = stream.address();
    stream.seek(pnp_database::seeking::current, 1);
    const uint8_t CALCULATED_CRC = calculate_crc(stream, 2 + SIZE);
    stream.seek(pnp_database::seeking::begin, BEGIN_ADDR + 4);

    auto error = check_header(SIZE, 0, CALCULATED_CRC, header);
    if (error == serializer_errors_e::SUCCESS) {
        shutter_settings settings = shutter_settings::get_defaults();

        auto ser = little_endian_serializer();
        std::array<byte, 8> buffer;

        // Order matters
        stream.read(std::span<byte, 8>(buffer));
        settings.saved_serial_number =
            ser.deserialize_uint64(std::span<byte, 8>(buffer));

        stream.read(std::span<byte, 8>(buffer));
        auto has_open_waveform = this->deserialize(settings.open_waveform,
                                                   std::span<byte, 8>(buffer));

        stream.read(std::span<byte, 8>(buffer));
        auto has_closed_waveform = this->deserialize(
            settings.closed_waveform, std::span<byte, 8>(buffer));

        settings.trigger_mode = static_cast<trigger_modes_e>(*stream.read());
        settings.state_at_powerup =
            static_cast<shutter::controller::states_e>(*stream.read());

        if (has_open_waveform && has_closed_waveform) {
            return return_t{settings};
        } else {
            return return_t(std::unexpect,
                            serializer_errors_e::SERIAL_ERROR_INVALID_VALUE);
        }
    } else {
        return return_t{std::unexpect, error};
    }
}

std::expected<shutter_settings, serializer_errors_e>
shutter_settings::serializer_v1::deserialize(
    drivers::spi::handle_factory& factory, drivers::eeprom::page_cache& view,
    const page_header& header) {
    using return_t = return_t<shutter_settings>;
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(span());
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);

    const auto BEGIN_ADDR = stream.address();
    stream.seek(pnp_database::seeking::current, 1);
    const uint8_t CALCULATED_CRC = calculate_crc(stream, 2 + SIZE);
    stream.seek(pnp_database::seeking::begin, BEGIN_ADDR + 4);

    auto error = check_header(SIZE, 1, CALCULATED_CRC, header);
    if (error == serializer_errors_e::SUCCESS) {
        shutter_settings settings = shutter_settings::get_defaults();

        auto ser = little_endian_serializer();
        std::array<byte, 16> buffer;
        const std::span<byte, 8> SPAN8 =
            std::span(buffer).template subspan<0, 8>();
        const std::span<byte, 16> SPAN16 =
            std::span(buffer).template subspan<0, 16>();

        // Order matters
        stream.read(SPAN8);
        settings.saved_serial_number = ser.deserialize_uint64(SPAN8);

        stream.read(SPAN16);
        auto has_open_waveform =
            this->deserialize(settings.open_waveform, SPAN16);

        stream.read(SPAN16);
        auto has_closed_waveform =
            this->deserialize(settings.closed_waveform, SPAN16);

        settings.trigger_mode = static_cast<trigger_modes_e>(*stream.read());
        settings.state_at_powerup =
            static_cast<shutter::controller::states_e>(*stream.read());

        if (has_open_waveform && has_closed_waveform) {
            return return_t{settings};
        } else {
            return return_t(std::unexpect,
                            serializer_errors_e::SERIAL_ERROR_INVALID_VALUE);
        }
    } else {
        return return_t{std::unexpect, error};
    }
}

serializer_errors_e shutter_settings::serializer_v1::serialize(
    const shutter_settings& value, drivers::spi::handle_factory& factory,
    drivers::eeprom::page_cache& view) {
    auto descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(span());
    drivers::eeprom::cache_backend_eeprom_stream stream(factory, descriptor,
                                                        view);
    auto serialize = [&](drivers::eeprom::cache_backend_eeprom_stream& stream) {
        std::array<byte, 16> buffer;
        const std::span<byte, 8> SPAN8 =
            std::span(buffer).template subspan<0, 8>();
        const std::span<byte, 16> SPAN16 =
            std::span(buffer).template subspan<0, 16>();
        auto ser = little_endian_serializer();

        ser.serialize(SPAN8, value.saved_serial_number);
        stream.write(SPAN8);
        this->serialize(SPAN16, value.open_waveform);
        stream.write(SPAN16);
        this->serialize(SPAN16, value.closed_waveform);
        stream.write(SPAN16);
        stream.write(static_cast<std::byte>(value.trigger_mode));
        stream.write(static_cast<std::byte>(value.state_at_powerup));
    };

    page_header header;
    header.version = 1;
    header.length  = SIZE;

    const std::size_t CALLBACK_WIDTH =
        serialize_with_header(header, stream, serialize);

    // Debug check for seralization width.
    configASSERT(CALLBACK_WIDTH + 1 == SIZE);

    return serializer_errors_e::SUCCESS;
}

/*****************************************************************************
 * Private Header Functions
 *****************************************************************************/
card_settings::serializer_v0::serializer_v0(drivers::eeprom::address_span span)
    : _span(span) {}
sio_settings::serializer_v0::serializer_v0(drivers::eeprom::address_span span,
                                           driver::channel_id channel)
    : _span(span), _channel(channel) {}

bool shutter_settings::serializer_v0::deserialize(
    serialized_waveform& dest, const std::span<const std::byte, 8>& src) {
    auto ser = little_endian_serializer();

    auto pulse_duty = ser.deserialize_int16(src.subspan<4, 2>());
    auto hold_duty  = ser.deserialize_int16(src.subspan<6, 2>());

    if ((pulse_duty > 0 && hold_duty < 0) ||
        (pulse_duty < 0 && hold_duty > 0)) {
        return false;
    }

    // Using assignment to inherit any defaults.
    // Version 0 stored the duty directly without adjustments
    // for bus voltage or pwm period.
    // Specifically, it assumed 24V bus voltage with 1900 period (20 kHz).
    static constexpr drivers::cpld::shutter_module::duty_type PERIOD = 1900;
    dest = serialized_waveform{
        .actuation_duration = utils::time::to_ticks(
            time_format{ser.deserialize_uint32(src.subspan<0, 4>())}),
        .actuation_voltage = drivers::cpld::shutter_module::to_voltage<float>(
            pulse_duty, 24, PERIOD),
        .holding_voltage = drivers::cpld::shutter_module::to_voltage<float>(
            hold_duty, 24, PERIOD),
    };
    return true;
}

bool shutter_settings::serializer_v1::deserialize(
    serialized_waveform& dest, const std::span<const std::byte, 16>& src) {
    auto ser = little_endian_serializer();

    auto pulse_duty = ser.deserialize_float(src.subspan<8, 4>());
    auto hold_duty  = ser.deserialize_float(src.subspan<12, 4>());

    if ((pulse_duty > 0 && hold_duty < 0) ||
        (pulse_duty < 0 && hold_duty > 0)) {
        return false;
    }

    // Using assignment to inherit any defaults.
    dest = serialized_waveform{
        .actuation_duration = utils::time::to_ticks(
            time_format{ser.deserialize_uint32(src.subspan<0, 4>())}),
        .actuation_holdoff = utils::time::to_ticks(
            time_format{ser.deserialize_uint32(src.subspan<4, 4>())}),
        .actuation_voltage = pulse_duty,
        .holding_voltage   = hold_duty,
    };
    return true;
}

void shutter_settings::serializer_v1::serialize(
    const std::span<std::byte, 16>& dest, const serialized_waveform& value) {
    auto ser                         = little_endian_serializer();
    const time_format ACTUATION_TIME = std::chrono::duration_cast<time_format>(
        utils::time::ticktype_duration(value.actuation_duration));
    const time_format HOLDOFF_TIME = std::chrono::duration_cast<time_format>(
        utils::time::ticktype_duration(value.actuation_holdoff));

    ser.serialize(dest.subspan<0, 4>(), ACTUATION_TIME.count());
    ser.serialize(dest.subspan<4, 4>(), HOLDOFF_TIME.count());
    ser.serialize(dest.subspan<8, 4>(), value.actuation_voltage);
    ser.serialize(dest.subspan<12, 4>(), value.holding_voltage);
}

/*****************************************************************************
 * Private State Functions
 *****************************************************************************/
static void write_header(drivers::eeprom::cache_backend_eeprom_stream& dest,
                         const page_header& header) {
    std::array<byte, 4> buff;
    header.serialize(std::span(buff));

    dest.seek(pnp_database::seeking::begin, 0);
    dest.write(std::span(buff));
}

static void write_device_header(
    drivers::eeprom::cache_backend_eeprom_stream& stream,
    const uint64_t& serial_number) {
    auto ser = little_endian_serializer();
    std::array<byte, 8> buff;

    ser.serialize(std::span(buff).subspan<0, 8>(), serial_number);

    stream.seek(pnp_database::seeking::begin, 4);
    stream.write(std::span(buff));
}

static uint8_t calculate_crc(
    drivers::eeprom::cache_backend_eeprom_stream& stream, uint16_t length) {
    uint8_t crc           = 0;
    std::size_t remaining = length;
    while (remaining > 0) {
        std::span<const std::byte> src = stream.read_to_cache_end();
        src = src.subspan(0, std::min(src.size(), remaining));

        crc = CRC_split(reinterpret_cast<const char*>(src.data()), src.size(),
                        crc);
        remaining -= src.size();
    }

    return crc;
}

static uint8_t old_calculate_crc(
    drivers::eeprom::cache_backend_eeprom_stream& stream, uint16_t length) {
    stream.seek(pnp_database::seeking::begin, 1);

    uint8_t crc           = 0;
    std::size_t remaining = length + 2;
    while (remaining > 0) {
        std::span<const std::byte> src = stream.read_to_cache_end();
        src = src.subspan(0, std::min(src.size(), remaining));

        crc = CRC_split(reinterpret_cast<const char*>(src.data()), src.size(),
                        crc);
        remaining -= src.size();
    }

    return crc;
}

static serializer_errors_e check_header(uint16_t size, uint8_t version,
                                        uint8_t calculated_crc,
                                        const page_header& header) {
    if (version != header.version)
        return serializer_errors_e::SERIAL_ERROR_UNSUPPORTED_VERSION;

    if (size != header.length)
        return serializer_errors_e::SERIAL_ERROR_SIZE_INVALD;

    if (calculated_crc != header.crc)
        return serializer_errors_e::SERIAL_ERROR_CRC_INVALID;

    return serializer_errors_e::SUCCESS;
}

static void deserialize_device_header(
    drivers::eeprom::cache_backend_eeprom_stream& cache, uint64_t& sn_dest) {
    auto serializer = little_endian_serializer();

    std::array<byte, 8> buffer;

    cache.seek(pnp_database::seeking::begin, 4);
    cache.read(std::span(buffer));

    sn_dest = serializer.deserialize_uint64(std::span(buffer).subspan<0, 8>());
}

static serializer_errors_e erase(
    drivers::eeprom::cache_backend_eeprom_stream& cache) {
    std::array<std::byte, 2> buff = {byte{0}, byte{0}};

    cache.seek(pnp_database::seeking::begin, 1);
    cache.write(std::span(buff));
    return serializer_errors_e::SUCCESS;
}

// EOF
