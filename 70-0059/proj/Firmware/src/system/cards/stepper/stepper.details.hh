#pragma once

// For symbol collection due to the macros.
#include <concepts>

#include "apt-command.hh"
#include "mcm_speed_limit.hh"
#include "slot_nums.h"
#include "stepper.h"

namespace cards::stepper {

/**
 * \brief Adapter legacy ptr buffers into a
 * drivers::usb::apt_response_builder reference for interop.
 *
 * Converts a legacy buffer of size
 * drivers::usb::apt_response::RESPONSE_BUFFER_SIZE to an l-value reference
 * to drivers::usb::apt_response_builder.
 * This then gets passed to a response_encoder to encode the response onto
 * the builder.
 * \param slot                The slot creating the response.
 * \param legacy_buffer       A pointer to the byte-buffer of length
 *        drivers::usb::apt_response::RESPONSE_BUFFER_SIZE.
 * \param legacy_length_used  A reference to a u8 to store the number of bytes
 *        written to the legacy buffer.
 * \param response_encoder    A function of type (apt_response_builder -> void)
 *        that encodes the response onto the builder.
 */
constexpr void with_response_builder(
    slot_nums slot, uint8_t* legacy_buffer, uint8_t& legacy_length_used,
    std::invocable<drivers::usb::apt_response_builder&> auto&&
        response_encoder) {
    auto new_api_adapter = drivers::usb::apt_response_builder(
        std::span<std::byte>(reinterpret_cast<std::byte*>(legacy_buffer) + 6,
                             drivers::usb::apt_response::RESPONSE_BUFFER_SIZE)
            .template subspan<
                0, drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>());
    response_encoder(new_api_adapter);

    legacy_buffer[0] = (uint8_t)new_api_adapter.command();
    legacy_buffer[1] = (uint8_t)(new_api_adapter.command() >> 8);
    legacy_buffer[4] = HOST_ID;
    legacy_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(slot);

    std::visit(
        [legacy_buffer, &legacy_length_used](const auto& value) {
            using type = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<
                              type, drivers::usb::apt_response::parameters>) {
                legacy_buffer[2] = value.param1;
                legacy_buffer[3] = value.param2;

                legacy_length_used = 6;
            } else {
                legacy_buffer[2] = (uint8_t)(value.data.size());
                legacy_buffer[3] = (uint8_t)(value.data.size() >> 8);
                legacy_buffer[4] |= 0x80;

                for (std::size_t i = 0; i < value.data.size(); ++i) {
                    legacy_buffer[6 + i] =
                        std::to_integer<uint8_t>(value.data[i]);
                }

                legacy_length_used = 6 + value.data.size();
            }
        },
        new_api_adapter.payload());
}

drivers::apt::mcm_speed_limit::payload_type apt_handler(
    const drivers::apt::mcm_speed_limit::request_type& request,
    const Stepper_info& stepper);

void apt_handler(const drivers::apt::mcm_speed_limit::payload_type& set,
                 Stepper_info& stepper);

}  // namespace cards::stepper
