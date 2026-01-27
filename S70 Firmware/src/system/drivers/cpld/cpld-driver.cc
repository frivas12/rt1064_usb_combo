/**
 * \file cpld-driver.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "cpld.h"
#include "cpld.hh"

#include "spi-transfer-handle.hh"
#include "spi.h"

#include <array>

using namespace drivers;
using namespace drivers::cpld;

void cpld::write_register(spi::handle_factory &factory,
                          const message_header &header,
                          const message_payload &payload) {
    const uint16_t CMD = static_cast<uint16_t>(header.command);
    const uint8_t ADDR = static_cast<uint8_t>(header.address);
    std::array<std::byte, 8> BUFFER{
        std::byte(CMD >> 8), std::byte(CMD),
        std::byte(ADDR),      std::byte(payload.mid_data),
        std::byte(payload.data >> 24),  std::byte(payload.data >> 16),
        std::byte(payload.data >> 8),   std::byte(payload.data),
    };

    factory.create_handle(CPLD_SPI_MODE, false, CS_CPLD)
        .write(std::span(BUFFER));
}

message_payload cpld::read_register(spi::handle_factory &factory,
                                    const message_header &header) {
    std::array<std::byte, 9> BUFFER{
        std::byte(static_cast<uint16_t>(header.command) >> 8),
        std::byte(static_cast<uint16_t>(header.command)),
        std::byte(header.address),
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
    };

    factory.create_handle(CPLD_SPI_MODE, false, CS_CPLD)
        .transfer(std::span(BUFFER));

    return message_payload{
        .data = (static_cast<uint32_t>(BUFFER[5]) << 24) |
                (static_cast<uint32_t>(BUFFER[6]) << 16) |
                (static_cast<uint32_t>(BUFFER[7]) << 8) |
                (static_cast<uint32_t>(BUFFER[8]) << 0),
        .mid_data = std::to_integer<uint8_t>(BUFFER[4]),
    };
}

bool cpld::read_emergency_stop_flag(spi::handle_factory &factory) {
    return (read_register(factory,
                          message_header{
                              .command = commands_e::READ_EM_STOP_FLAG,
                              .address = addresses_e::LED,
                          })
                .data &
            0x01) != 0;
}

// EOF
