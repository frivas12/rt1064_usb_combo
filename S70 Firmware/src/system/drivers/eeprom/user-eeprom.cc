#include "user-eeprom.hh"

#include "25lc1024.h"

using namespace drivers::eeprom;

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

void drivers::eeprom::read(drivers::spi::handle_factory &factory,
                           uint32_t address, std::span<std::byte> dest) {
    factory.acquire_lock();
    eeprom_25LC1024_read(address, dest.size(),
                         reinterpret_cast<uint8_t *>(dest.data()));
}

void drivers::eeprom::write(drivers::spi::handle_factory &factory,
                            uint32_t address, std::span<const std::byte> src) {
    factory.acquire_lock();
    eeprom_25LC1024_write(address, src.size(),
                          reinterpret_cast<const uint8_t *>(src.data()));
}

bool page_cache::contains_address(uint32_t address) const {
    return address >= _head_address && (address - _head_address) < _size;
}

bool page_cache::is_dirty() const { return _dirty; }

uint32_t page_cache::page_address() const { return _head_address; }

std::size_t page_cache::size() const { return _size; }

address_span page_cache::as_address_span() const {
    return address_span{
        .head_address = _head_address,
        .tail_address = _head_address + _size,
    };
}

std::span<const std::byte> page_cache::subspan(uint32_t address,
                                               std::size_t size) const {
    std::span<const std::byte> span{_cache};
    if (address < _head_address) {
        return span.subspan(0, 0);
    }

    const uint32_t OFFSET = address - _head_address;
    const std::size_t MOD_SIZE = _size - OFFSET;
    return span.subspan(OFFSET, std::min(MOD_SIZE, size));
}

std::span<std::byte> page_cache::mutable_subspan(uint32_t address,
                                                 std::size_t size) {
    std::span<std::byte> span{_cache};
    if (address < _head_address) {
        return span.subspan(0, 0);
    }

    const uint32_t OFFSET = address - _head_address;
    const std::size_t MOD_SIZE = _size - OFFSET;
    const std::size_t SIZE = std::min(MOD_SIZE, size);
    _dirty = _dirty || SIZE > 0;
    return span.subspan(OFFSET, SIZE);
}

void page_cache::flush(drivers::spi::handle_factory &factory) {
    if (!_dirty) {
        return;
    }

    drivers::eeprom::write(factory, _head_address,
                           std::span(_cache).subspan(0, _size));
    _dirty = false;
}

void page_cache::cache(drivers::spi::handle_factory &factory,
                       uint32_t address_in_page) {
    _head_address =
        address_in_page - address_in_page % EEPROM_25LC1024_PAGE_SIZE;
    recache(factory);
}

void page_cache::recache(drivers::spi::handle_factory &factory) {
    drivers::eeprom::read(factory, _head_address, std::span{_cache});
    // _size = EEPROM_25LC1024_PAGE_SIZE;
    _dirty = false;
}

page_cache::page_cache(const page_cache &other)
    : _cache(other._cache), _head_address(other._head_address),
      _dirty(other._dirty) {}

flush_guard::flush_guard(drivers::spi::handle_factory &factory,
                         page_cache &cache)
    : _factory(factory), _cache(cache) {}

flush_guard::~flush_guard() { _cache.flush(_factory); }

stream_descriptor::local_address_type
stream_descriptor::seek(seek_begin_t, std::size_t from_head) {
    if (from_head > size()) {
        _current_address = _tail_address;
    } else {
        _current_address = _head_address + from_head;
    }

    return address();
}
stream_descriptor::local_address_type
stream_descriptor::seek(seek_end_t, std::size_t from_tail) {
    if (from_tail > size()) {
        _current_address = _head_address;
    } else {
        _current_address = _tail_address - from_tail;
    }
    return address();
}

stream_descriptor::local_address_type
stream_descriptor::seek(seek_current_t, difference_type offset) {
    const local_address_type ADDR = address();
    if (offset < 0 && static_cast<global_address_type>(-offset) >= ADDR) {
        _current_address = _head_address;
    } else if (offset > 0 &&
               static_cast<global_address_type>(offset) > (size() - ADDR)) {
        _current_address = _tail_address;
    } else {
        _current_address += offset;
    }

    return address();
}
std::optional<stream_descriptor>
stream_descriptor::from_address_range(global_address_type head_address,
                                      global_address_type tail_address) {
    using return_t = std::optional<stream_descriptor>;
    return tail_address >= head_address
               ? return_t{stream_descriptor(head_address, tail_address)}
               : return_t{};
}

stream_descriptor
stream_descriptor::from_address_count(global_address_type head_address,
                                      std::size_t size) {
    return stream_descriptor(head_address, head_address + size);
}

std::optional<std::byte> basic_eeprom_stream::read() {
    std::array<std::byte, 1> buffer;
    using return_t = std::optional<std::byte>;
    return read(std::span(buffer)) == 1 ? return_t{buffer[0]} : return_t{};
}

std::size_t basic_eeprom_stream::read(std::span<std::byte> dest) {
    const address_span OPERATION_SPAN = operation_seek(dest.size());

    const std::size_t SIZE =
        OPERATION_SPAN.tail_address - OPERATION_SPAN.head_address;
    if (SIZE != 0) {
        dest = dest.subspan(0, SIZE);

        drivers::eeprom::read(_factory, OPERATION_SPAN.head_address, dest);
    }

    return SIZE;
}

bool basic_eeprom_stream::write(std::byte value) {
    std::array<std::byte, 1> buffer;
    buffer[0] = value;
    return write(std::span(buffer)) != 0;
}

std::size_t basic_eeprom_stream::write(std::span<const std::byte> src) {
    const address_span OPERATION_SPAN = operation_seek(src.size());

    const std::size_t SIZE =
        OPERATION_SPAN.tail_address - OPERATION_SPAN.head_address;
    if (SIZE != 0) {
        src = src.subspan(0, SIZE);

        drivers::eeprom::write(_factory, OPERATION_SPAN.head_address, src);
    }

    return SIZE;
}
basic_eeprom_stream::basic_eeprom_stream(drivers::spi::handle_factory &factory,
                                         stream_descriptor &stream_descriptor)
    : _state(stream_descriptor), _factory(factory) {}

std::optional<std::byte> cache_backend_eeprom_stream::read() {
    std::array<std::byte, 1> buffer;
    using return_t = std::optional<std::byte>;
    return read(std::span(buffer)) == 1 ? return_t{buffer[0]} : return_t{};
}

std::size_t cache_backend_eeprom_stream::read(std::span<std::byte> dest) {
    address_span operation_span = operation_seek(dest.size());
    const std::size_t SIZE = operation_span.size();

    // operation_span.size() <= dest.size()
    while (operation_span.size() != 0) {
        if (!_cache.contains_address(operation_span.head_address)) {
            _cache.flush(_factory);
            _cache.cache(_factory, operation_span.head_address);
        }

        std::span<const std::byte> read_span = _cache.subspan(
            operation_span.head_address,
            std::min(operation_span.size(), (uint32_t)dest.size()));
        std::ranges::copy(read_span, dest.begin());
        dest = dest.subspan(read_span.size());
        operation_span.head_address += read_span.size();
    }

    return SIZE;
}

std::span<const std::byte> cache_backend_eeprom_stream::read_to_cache_end() {
    if (!_cache.contains_address(global_address())) {
        _cache.flush(_factory);
        _cache.cache(_factory, global_address());
    }
    return _cache.subspan(global_address(), EEPROM_25LC1024_PAGE_SIZE);
}

bool cache_backend_eeprom_stream::write(std::byte value) {
    std::array<std::byte, 1> buffer;
    buffer[0] = value;
    return write(std::span(buffer)) != 0;
}

std::size_t cache_backend_eeprom_stream::write(std::span<const std::byte> src) {
    address_span operation_span = operation_seek(src.size());
    const std::size_t SIZE = operation_span.size();

    // operation_span.size() <= src.size()
    while (operation_span.size() != 0) {
        if (!_cache.contains_address(operation_span.head_address)) {
            _cache.flush(_factory);
            _cache.cache(_factory, operation_span.head_address);
        }

        std::span<std::byte> write_span = _cache.mutable_subspan(
            operation_span.head_address, operation_span.size());
        std::span<const std::byte> processed_src =
            src.subspan(0, std::min(src.size(), write_span.size()));

        std::ranges::copy(processed_src, write_span.begin());
        src = src.subspan(processed_src.size());
        operation_span.head_address += processed_src.size();
    }

    return SIZE;
}

cache_backend_eeprom_stream::cache_backend_eeprom_stream(
    drivers::spi::handle_factory &factory, stream_descriptor &stream_descriptor,
    page_cache &cache)
    : _state(stream_descriptor), _factory(factory), _cache(cache) {}

cache_backend_eeprom_stream::~cache_backend_eeprom_stream() {
    _cache.flush(_factory);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

stream_descriptor::stream_descriptor(global_address_type head,
                                     global_address_type tail)
    : _head_address(head), _tail_address(tail), _current_address(head) {}
stream_descriptor::stream_descriptor(global_address_type head,
                                     global_address_type tail,
                                     global_address_type current)
    : _head_address(head), _tail_address(tail), _current_address(current) {}

page_cache::page_cache(drivers::spi::handle_factory &factory,
                       uint32_t address_in_page) {
    cache(factory, address_in_page);
}

// EOF
