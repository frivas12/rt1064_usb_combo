// efs-handle.cc

// clang-format off
/**************************************************************************/ /**
* \file efs-handle.cc
* \author Sean Benish
*****************************************************************************/
// clang-format on
#include "efs-handle.hh"

#include "25lc1024.h"
#include "user-eeprom.hh"

#include <tuple>

using namespace efs;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

handle::handle(user_e source) : _source(source), _open(false) {}

handle::handle(user_e source, const file_metadata &r_metadata)
    : _metadata(r_metadata), _source(source), _open(true) {}

handle::handle(handle &&r_handle)
    : _metadata(r_handle._metadata), _source(r_handle._source),
      _open(r_handle._open) {
    r_handle._open = false;
}
handle &handle::operator=(handle &&r_handle) {
    _metadata = r_handle._metadata;
    _source = r_handle._source;
    _open = r_handle._open;
    r_handle._open = false;

    return *this;
}

handle::~handle() { close(); }

void handle::close() {
    if (is_valid()) {
        release_ownership();
        _open = false;
    }
}

uint32_t handle::read(drivers::spi::handle_factory &factory,
                      const uint32_t address, std::span<std::byte> dest) const {
    if (!can_read()) {
        return 0;
    }

    // Dereferencing the optional b/c we are assuming the condition is alreay
    // met.
    drivers::eeprom::stream_descriptor descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(
            _metadata.to_address_span());

    descriptor.seek(pnp_database::seeking::begin, address);
    auto stream = drivers::eeprom::basic_eeprom_stream(factory, descriptor);

    if constexpr (HANDLE_READ_YIELD_BYTES == 0) {
        return stream.read(dest);
    } else {
        std::size_t rt = 0;

        while (true) {
            const std::size_t AMOUNT =
                std::min(dest.size(), HANDLE_READ_YIELD_BYTES);
            const std::size_t READ = stream.read(dest.subspan(0, AMOUNT));
            rt += READ;

            // If it goes out of bounds or fails.
            if (READ != HANDLE_READ_YIELD_BYTES) {
                break;
            }

            dest = dest.subspan(HANDLE_READ_YIELD_BYTES);

            // Toggle lock to allow other threads to use the lock if needed.
            factory.release_lock();
            factory.acquire_lock();
        }

        return rt;
    }
}

uint32_t handle::write(drivers::spi::handle_factory &factory, uint32_t address,
                       std::span<const std::byte> src) const {
    if (!can_write()) {
        return 0;
    }
    // Dereferencing the optional b/c we are assuming the condition is alreay
    // met.
    drivers::eeprom::stream_descriptor descriptor =
        *drivers::eeprom::stream_descriptor::from_address_range(
            _metadata.to_address_span());

    descriptor.seek(pnp_database::seeking::begin, address);
    auto stream = drivers::eeprom::basic_eeprom_stream(factory, descriptor);

    auto write_to_page = [&src, &stream](std::size_t page_size) {
        const uint32_t SIZE_ON_PAGE = std::min(page_size, src.size());
        const auto rt = stream.write(src.subspan(0, SIZE_ON_PAGE));
        src = src.subspan(SIZE_ON_PAGE,
                          rt == SIZE_ON_PAGE ? std::dynamic_extent : 0);

        return rt;
    };

    // Write potentially page-unaligned address to end of page (or end of data)
    const uint32_t END_EEPROM_ADDRESS_OF_CURRENT_PAGE =
        descriptor.global_address() + EFS_PAGE_SIZE -
        (descriptor.global_address() % EFS_PAGE_SIZE);
    uint32_t rt = write_to_page(END_EEPROM_ADDRESS_OF_CURRENT_PAGE -
                                descriptor.global_address());

    // Write remaining data with page-aligned addresses.
    while (!src.empty()) {
        rt += write_to_page(EEPROM_25LC1024_PAGE_SIZE);
    }

    return rt;
}

// EOF
