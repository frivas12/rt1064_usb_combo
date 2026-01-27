/**
 * \file user-eeprom.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-05-18
 *
 * The drivers::eeprom namespace provides application-level
 * abstractions of the EEPROM layout.
 */
#pragma once


#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>

#include "25lc1024.h"
#include "spi-transfer-handle.hh"
#include "stream-io.hh"

namespace drivers::eeprom {

void read(drivers::spi::handle_factory& factory, uint32_t address,
          std::span<std::byte> dest);
void write(drivers::spi::handle_factory& factory, uint32_t address,
           std::span<const std::byte> src);

struct address_span {
    uint32_t head_address;
    uint32_t tail_address;

    constexpr address_span intersect(const address_span& other) const {
        const uint32_t HEAD = std::max(head_address, other.head_address);
        const uint32_t TAIL = std::min(tail_address, other.tail_address);
        return address_span{
            .head_address = HEAD,
            .tail_address = TAIL < HEAD ? HEAD : TAIL,
        };
    }

    /// \brief Splits the address into even segments of count n and takes the
    /// index.
    ///        If the size cannot be made even, the last element will have the
    ///        remainder elements.
    constexpr address_span split_and_index(std::size_t n,
                                           std::size_t index) const {
        const uint32_t SIZE = (tail_address - head_address) / n;
        return address_span{
            .head_address = static_cast<uint32_t>(head_address + index * SIZE),
            .tail_address =
                (index == n - 1)
                    ? tail_address
                    : static_cast<uint32_t>(head_address + (index + 1) * SIZE),
        };
    }

    /// \brief Splits the address into even segments of count n.
    ///        If the size cannot be made even, the last element will have the
    ///        remainder elements.
    constexpr std::ranges::range auto split(std::size_t n) const {
        return std::ranges::iota_view(static_cast<std::size_t>(0), n) |
               std::views::transform([&, n](std::size_t index) {
                   return split_and_index(n, index);
               });
    }

    constexpr address_span split_every_and_index(uint32_t count,
                                                 std::size_t index) const {
        const uint32_t HEAD =
            head_address + static_cast<uint32_t>(index * count);
        return address_span{
            .head_address = HEAD,
            .tail_address = std::min(tail_address, HEAD + count),
        };
    }

    constexpr address_span subspan(uint32_t offset) const {
        return address_span{
            .head_address = head_address + offset,
            .tail_address = tail_address,
        };
    }

    constexpr address_span subspan(uint32_t offset, uint32_t size) const {
        return address_span{
            .head_address = head_address + offset,
            .tail_address = head_address + offset + size,
        };
    }

    constexpr uint32_t size() const { return tail_address - head_address; }
};

namespace regions {

/// \brief Address span for data related to the type of the board.
constexpr address_span BOARD_TYPE =
    address_span{.head_address = 0 * EEPROM_25LC1024_PAGE_SIZE,
                 .tail_address = 1 * EEPROM_25LC1024_PAGE_SIZE};

/// \brief Address span for persisted data for each slot card.
/// Slots 1 - 7 have 10 pages.
/// Invalid slots values will empty address spans.
/// \param[in]    slot The slot to access.
constexpr address_span slot(slot_nums slot) {
    const uint32_t HEAD =
        (1 + static_cast<uint8_t>(slot) * 10) * EEPROM_25LC1024_PAGE_SIZE;
    return address_span{
        .head_address = HEAD,
        .tail_address =
            HEAD + ((slot <= SLOT_7) ? (10 * EEPROM_25LC1024_PAGE_SIZE) : 0),
    };
}

/**
 * Addess span for persisted data for each HID device's IN mappings.
 * \param[in]   index The index of the HID device.
 *                    Range: [0, 7]
 *                    Invalid indices will return empty spans.
 */
constexpr address_span hid_in(std::size_t port) {
    const uint32_t HEAD = (81 + port) * EEPROM_25LC1024_PAGE_SIZE;
    return address_span{
        .head_address = HEAD,
        .tail_address = HEAD + (port < 8 ? EEPROM_25LC1024_PAGE_SIZE : 0),
    };
}

/**
 * Addess span for persisted data for each HID device's OUT mappings.
 * \param[in]   index The index of the HID device.
 *                    Range: [0, 7]
 *                    Invalid indices will return empty spans.
 */
constexpr address_span hid_out(std::size_t port) {
    const uint32_t HEAD = (89 + port) * EEPROM_25LC1024_PAGE_SIZE;
    return address_span{
        .head_address = HEAD,
        .tail_address = HEAD + (port < 8 ? EEPROM_25LC1024_PAGE_SIZE : 0),
    };
}

constexpr address_span bad_power_restart_check = address_span{
    .head_address = 97 * EEPROM_25LC1024_PAGE_SIZE,
    .tail_address = 98 * EEPROM_25LC1024_PAGE_SIZE,
};

constexpr address_span board_application_data = address_span{
    .head_address = 98 * EEPROM_25LC1024_PAGE_SIZE,
    .tail_address = 99 * EEPROM_25LC1024_PAGE_SIZE,
};

constexpr address_span synchronized_motion = address_span{
    .head_address = 99 * EEPROM_25LC1024_PAGE_SIZE,
    .tail_address = 100 * EEPROM_25LC1024_PAGE_SIZE,
};

constexpr address_span embedded_file_system = address_span{
    .head_address = 256 * EEPROM_25LC1024_PAGE_SIZE,
    .tail_address = 512 * EEPROM_25LC1024_PAGE_SIZE,
};

}  // namespace regions

/**
 * Simple utilty container for caching an EEPROM page.
 * \warning The page cache will discard dirty data when
 *          the destructor is called.  If the data should
 *          be persisted, see \ref{flush_guard}.
 */
class page_cache {
   public:
    bool contains_address(uint32_t address) const;
    bool is_dirty() const;

    uint32_t page_address() const;
    std::size_t size() const;

    address_span as_address_span() const;

    /// \brief Obtains a subspan of the data in the cache.
    ///        The subspan cannot be mutated.
    std::span<const std::byte> subspan(uint32_t address,
                                       std::size_t size) const;

    /// \brief Obtains a subspan of the data in the cache that can be modified.
    ///        Flags the cache as dirty, invoking a write-back on flush.
    std::span<std::byte> mutable_subspan(uint32_t address, std::size_t size);

    /// \brief If the cache is dirty, it will be written to EEPROM.
    void flush(drivers::spi::handle_factory& factory);

    /// \brief Changes the page stored in the cache.
    /// \warning If the data is dirty, the changes will be disposed.
    void cache(drivers::spi::handle_factory& factory, uint32_t address_in_page);

    /// \brief Re-reads the data of the cache from the disk.
    void recache(drivers::spi::handle_factory& factory);

    page_cache(const page_cache&);
    ~page_cache() = default;

    page_cache(drivers::spi::handle_factory& factory, uint32_t address_in_page);

   protected:
    constexpr page_cache() = default;

   private:
    std::array<std::byte, EEPROM_25LC1024_PAGE_SIZE> _cache;
    static constexpr std::size_t _size = EEPROM_25LC1024_PAGE_SIZE;
    uint32_t _head_address;
    bool _dirty = false;
};

/// RAII guard that guarantees that a flush occurs when this object goes
/// out of scope.
class flush_guard {
   public:
    flush_guard(drivers::spi::handle_factory& factory, page_cache& cache);

    flush_guard(flush_guard&&) = delete;

    ~flush_guard();

    inline page_cache& cache() { return _cache; }

    inline const page_cache& cache() const { return _cache; }

    inline page_cache* operator->() { return &_cache; }

    inline const page_cache* operator->() const { return &_cache; }

   private:
    drivers::spi::handle_factory& _factory;
    page_cache& _cache;
};

/**
 * A container for an EEPROM stream's state without requiring any runtime
 * resources for IO operations.
 *
 * This container can be used for stream operations w/o the IO resource
 * requirments of a stream object.
 * In addition, this can be used to save the state of a stream.
 */
class stream_descriptor {
   public:
    using global_address_type = uint32_t;
    using local_address_type  = uint32_t;
    using difference_type     = int32_t;

    using seek_begin_t   = pnp_database::seeking::begin_t;
    using seek_end_t     = pnp_database::seeking::end_t;
    using seek_current_t = pnp_database::seeking::current_t;

    local_address_type seek(seek_begin_t, std::size_t from_head);
    local_address_type seek(seek_end_t, std::size_t from_tail);
    local_address_type seek(seek_current_t, difference_type offset);

    inline global_address_type head_global_address() const {
        return _head_address;
    }

    inline global_address_type tail_global_address() const {
        return _tail_address;
    }

    inline global_address_type global_address() const {
        return _current_address;
    }

    inline local_address_type address() const {
        return _current_address - _head_address;
    }

    inline local_address_type size() const {
        return _tail_address - _head_address;
    }

    /**
     * Creates a new stream memento describing an EEPROM stream
     * operating on the address range [head, tail).
     * \return A memento object unless tail_address < head_address.
     */
    static std::optional<stream_descriptor> from_address_range(
        global_address_type head_address, global_address_type tail_address);

    inline static std::optional<stream_descriptor> from_address_range(
        const address_span& span) {
        return from_address_range(span.head_address, span.tail_address);
    }

    /**
     * Creates a new stream memento describing an EEPROM stream
     * operating on the address range [head, head + size).
     */
    static stream_descriptor from_address_count(
        global_address_type head_address, std::size_t size);

    stream_descriptor(const stream_descriptor&)            = default;
    stream_descriptor& operator=(const stream_descriptor&) = default;

   protected:
    /**
     * \pre head <= tail
     */
    stream_descriptor(global_address_type head, global_address_type tail);

    /**
     * \pre head <= tail
     * \pre head <= current <= tail
     */
    stream_descriptor(global_address_type head, global_address_type tail,
                      global_address_type current);

   private:
    global_address_type _head_address;
    global_address_type _tail_address;

    global_address_type _current_address;
};

class basic_eeprom_stream {
   public:
    using value_type          = std::byte;
    using global_address_type = stream_descriptor::global_address_type;
    using local_address_type  = stream_descriptor::local_address_type;
    using difference_type     = stream_descriptor::difference_type;

    using seek_begin_t   = stream_descriptor::seek_begin_t;
    using seek_end_t     = stream_descriptor::seek_end_t;
    using seek_current_t = stream_descriptor::seek_current_t;

    std::optional<std::byte> read();

    std::size_t read(std::span<std::byte> dest);

    bool write(std::byte value);

    std::size_t write(std::span<const std::byte> src);

    /// \brief Saves the state of the stream into a new descriptor.
    inline stream_descriptor save() const { return _state; }

    /// \brief Loads overwrites the state descriptor with the descriptor
    /// provided.
    inline basic_eeprom_stream& load(const stream_descriptor& state) {
        _state = state;
        return *this;
    }

    inline global_address_type head_global_address() const {
        return _state.head_global_address();
    }

    inline global_address_type tail_global_address() const {
        return _state.tail_global_address();
    }

    inline global_address_type global_address() const {
        return _state.global_address();
    }

    inline local_address_type address() const { return _state.address(); }

    inline local_address_type size() const { return _state.size(); }

    inline local_address_type seek(seek_begin_t _, std::size_t from_head) {
        return _state.seek(_, from_head);
    }

    inline local_address_type seek(seek_end_t _, std::size_t from_tail) {
        return _state.seek(_, from_tail);
    }

    inline local_address_type seek(seek_current_t _, difference_type offset) {
        return _state.seek(_, offset);
    }

    basic_eeprom_stream(drivers::spi::handle_factory& factory,
                        stream_descriptor& stream_descriptor);

    /// \brief Proactive preventing of temporary lifetime extension bugs.
    basic_eeprom_stream(drivers::spi::handle_factory& factory,
                        stream_descriptor&& stream_descriptor) = delete;
    explicit basic_eeprom_stream(const basic_eeprom_stream&)   = delete;
    explicit basic_eeprom_stream(basic_eeprom_stream&&)        = default;
    ~basic_eeprom_stream()                                     = default;

   private:
    stream_descriptor& _state;
    drivers::spi::handle_factory& _factory;

    /// \pre amount >= 0
    /// \return The address span of the operation permitted
    address_span operation_seek(difference_type amount) {
        address_span rt;

        rt.head_address = _state.global_address();
        _state.seek(pnp_database::seeking::current, amount);
        rt.tail_address = _state.global_address();

        return rt;
    }
};

/**
 * An EEPROM stream that uses a reference to
 * a page_cache as an intermediary.
 * No EEPROM operations occur while stream operations
 * occur on a cached page.
 * When the page becomes uncached, the cache is flushed
 * and reloaded.
 * When this object is destroyed, the cache is flushed.
 */
class cache_backend_eeprom_stream {
   public:
    using value_type          = std::byte;
    using global_address_type = stream_descriptor::global_address_type;
    using local_address_type  = stream_descriptor::local_address_type;
    using difference_type     = stream_descriptor::difference_type;

    using seek_begin_t   = stream_descriptor::seek_begin_t;
    using seek_end_t     = stream_descriptor::seek_end_t;
    using seek_current_t = stream_descriptor::seek_current_t;

    std::optional<std::byte> read();

    std::size_t read(std::span<std::byte> dest);

    /**
     * Reads from the current address to the end of the page.
     * \warning The lifetime of the data in the returned span is valid
     *          until the next span or cache operation.
     */
    std::span<const std::byte> read_to_cache_end();

    bool write(std::byte value);

    std::size_t write(std::span<const std::byte> src);

    /// \brief Saves the state of the stream into a new descriptor.
    inline stream_descriptor save() const { return _state; }

    /// \brief Loads overwrites the state descriptor with the descriptor
    /// provided.
    inline cache_backend_eeprom_stream& load(const stream_descriptor& state) {
        _state = state;
        return *this;
    }

    inline global_address_type head_global_address() const {
        return _state.head_global_address();
    }

    inline global_address_type tail_global_address() const {
        return _state.tail_global_address();
    }

    inline global_address_type global_address() const {
        return _state.global_address();
    }

    inline local_address_type address() const { return _state.address(); }

    inline local_address_type size() const { return _state.size(); }

    inline local_address_type seek(seek_begin_t _, std::size_t from_head) {
        return _state.seek(_, from_head);
    }

    inline local_address_type seek(seek_end_t _, std::size_t from_tail) {
        return _state.seek(_, from_tail);
    }

    inline local_address_type seek(seek_current_t _, difference_type offset) {
        return _state.seek(_, offset);
    }

    cache_backend_eeprom_stream(drivers::spi::handle_factory& factory,
                                stream_descriptor& stream_descriptor,
                                page_cache& cache);

    explicit cache_backend_eeprom_stream(const cache_backend_eeprom_stream&) =
        delete;
    explicit cache_backend_eeprom_stream(cache_backend_eeprom_stream&&) =
        default;
    ~cache_backend_eeprom_stream();

   private:
    stream_descriptor& _state;
    drivers::spi::handle_factory& _factory;
    page_cache& _cache;

    /// \pre amount >= 0
    /// \return The address span of the operation permitted
    address_span operation_seek(difference_type amount) {
        address_span rt;

        rt.head_address = _state.global_address();
        _state.seek(pnp_database::seeking::current, amount);
        rt.tail_address = _state.global_address();

        return rt;
    }
};
// static_assert(pnp_database::io_stream<basic_eeprom_stream>);
// static_assert(pnp_database::addressable_stream<basic_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_begin_seeking<basic_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_current_seeking<basic_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_end_seeking<basic_eeprom_stream>);

// // constexpr std::tuple<basic_eeprom_stream::global_address_type,
// std::size_t>
// // allocate_cache_to_full_forward_aligned_pages(
// //     basic_eeprom_stream::global_address_type address_to_cache,
// //     std::size_t size_of_cache) {
// //     using return_t =
// //         std::tuple<basic_eeprom_stream::global_address_type, std::size_t>;
// //     if (size_of_cache < EEPROM_25LC1024_PAGE_SIZE) {
// //         return return_t{address_to_cache, size_of_cache};
// //     }
// //     const basic_eeprom_stream::global_address_type PAGE_HEAD =
// //         address_to_cache - address_to_cache % EEPROM_25LC1024_PAGE_SIZE;
// //     const std::size_t BYTES_TO_CACHE =
// //         (size_of_cache / EEPROM_25LC1024_PAGE_SIZE) *
// //         EEPROM_25LC1024_PAGE_SIZE;
// //     return return_t{PAGE_HEAD, BYTES_TO_CACHE};
// // }
//
// template <std::size_t PagesToCache> class page_caching_eeprom_stream {
//   public:
//     using value_type = basic_eeprom_stream::value_type;
//     using global_address_type = basic_eeprom_stream::global_address_type;
//     using local_address_type = basic_eeprom_stream::local_address_type;
//     using difference_type = basic_eeprom_stream::difference_type;
//
//     using seek_begin_t = basic_eeprom_stream::seek_begin_t;
//     using seek_end_t = basic_eeprom_stream::seek_end_t;
//     using seek_current_t = basic_eeprom_stream::seek_current_t;
//
//     std::optional<std::byte> read() {
//         page_cache &cache = get_or_allocate_at_current_address();
//
//         const auto OFFSET = _state.global_address() - cache.addr;
//         const std::optional<std::byte> rt = OFFSET > cache.size
//                                                 ? std::optional<std::byte>{}
//                                                 : cache.cache[OFFSET];
//         _state.seek(pnp_database::seeking::current, 1);
//         return rt;
//     }
//     std::size_t read(std::span<std::byte> dest) {
//         std::size_t rt = 0;
//         while (dest.size() > 0) {
//             page_cache &cache = get_or_allocate_at_current_address();
//
//             std::span<const std::byte> cached_subspan =
//                 cache.address_subspan(_state.global_address(), dest.size());
//
//             if (cached_subspan.size() != 0) {
//                 std::ranges::copy(cached_subspan, dest.begin());
//                 _state.seek(pnp_database::seeking::current,
//                             cached_subspan.size());
//                 rt += cached_subspan.size();
//
//                 dest = dest.subspan(0, cached_subspan.size());
//             } else {
//                 _state.seek(pnp_database::seeking::current, dest.size());
//                 break;
//             }
//         }
//
//         return rt;
//     }
//
//     bool write(std::byte value) {
//         page_cache &cache = get_or_allocate_at_current_address();
//
//         const bool RT = cache.contains_address(_state.address());
//         if (RT) {
//             cache.cache[_state.global_address() - cache.addr] = value;
//             cache.dirty = true;
//         }
//         return RT;
//     }
//
//     std::size_t write(std::span<const std::byte> src) {
//         std::size_t rt = 0;
//         while (src.size() > 0) {
//             page_cache &cache = get_or_allocate_at_current_address();
//
//             std::span<std::byte> cached_subspan =
//                 cache.address_subspan(_state.global_address(), src.size());
//
//             if (cached_subspan.size() != 0) {
//                 std::ranges::copy(src.subspan(0, cached_subspan.size()),
//                                   cached_subspan.begin());
//                 _state.seek(pnp_database::seeking::current,
//                             cached_subspan.size());
//                 rt += cached_subspan.size();
//
//                 src = src.subspan(0, cached_subspan.size());
//             } else {
//                 _state.seek(pnp_database::seeking::current, src.size());
//                 break;
//             }
//         }
//
//         return rt;
//     }
//
//     /// \brief Saves the state of the stream into a memento.
//     basic_eeprom_stream::memento save() const { return _state; }
//
//     /// \brief Loads a memento as the state of the stream.
//     cached_eeprom_stream<PagesToCache> &
//     load(const basic_eeprom_stream::memento &memento) {
//         _state = memento;
//     }
//
//     global_address_type head_global_address() const {
//         return _state.head_global_address();
//     }
//
//     global_address_type tail_global_address() const {
//         return _state.tail_global_address();
//     }
//
//     global_address_type global_address() const {
//         return _state.global_address();
//     }
//
//     local_address_type address() const { return _state.address(); }
//
//     local_address_type size() const { return _state.size(); }
//
//     local_address_type seek(seek_begin_t _, std::size_t from_head) {
//         return _state.seek(_, from_head);
//     }
//
//     local_address_type seek(seek_end_t _, std::size_t from_tail) {
//         return _state.seek(_, from_tail);
//     }
//
//     local_address_type seek(seek_current_t _, difference_type offset) {
//         return _state.seek(_, offset);
//     }
//
//     void flush();
//
//     /// \brief Extracts the basic EEPROM stream while ending the lifetime of
//     the
//     /// cached EEPROM stream.
//     static basic_eeprom_stream
//     uncache(page_caching_eeprom_stream<PagesToCache> &&cached_stream) {
//         return cached_stream._stream;
//     }
//
//   private:
//     struct page_cache {
//         std::array<std::byte, EEPROM_25LC1024_PAGE_SIZE> cache;
//
//         global_address_type addr;
//         std::size_t size;
//
//         uint16_t usage_counter;
//
//         bool dirty;
//
//         constexpr bool allocated() const { return size != 0; }
//
//         /// \pre size > 0
//         void allocate(global_address_type address) {
//             this->addr = address;
//             this->size = EEPROM_25LC1024_PAGE_SIZE;
//             this->dirty = false;
//         }
//
//         void deallocate() { this->size = 0; }
//
//         constexpr bool contains_address(global_address_type address) {
//             return address >= addr && (address - addr) < size;
//         }
//
//         /// \pre contains_address(address) == true
//         std::span<std::byte> address_subspan(global_address_type address,
//                                              std::size_t size) {
//             const auto OFFSET = address - addr;
//             return std::span(cache).subspan(
//                 OFFSET, std::min(this->size - OFFSET, size));
//         }
//     };
//
//     basic_eeprom_stream::memento _state;
//
//     std::array<page_cache, PagesToCache> _caches;
//
//     /// Tells the cache lifetime manager to keep a cache alive.
//     void keep_alive(page_cache &cache) {
//         const bool RENORMALIZE = ++cache.usage_counter == 0;
//
//         // Reduce the usage counter
//         if (RENORMALIZE) {
//             --cache.usage_counter;
//
//             const uint16_t DELTA = std::ranges::min(
//                 _caches | std::views::transform([](const page_cache &value) {
//                     return value.usage_counter;
//                 }));
//
//             for (auto &cache : _caches) {
//                 // If a cache is not being used, remove it b/c of the
//                 // performance penalty of keep alive operations.
//                 if (cache.usage_counter == 0) {
//                     cache.deallocate();
//                 } else {
//                     cache.usage_counter -= DELTA;
//                 }
//             }
//         }
//     }
//
//     /// Allocates an empty cache or reallocates the last used cache.
//     /// \pre page is not cached
//     page_cache &allocate_cache(global_address_type address) {
//         page_cache *lowest_cache = nullptr;
//         uint16_t lowest_usage_counter = 0xFFFF;
//         for (auto &cache : _caches) {
//             if (!cache.allocated()) {
//                 lowest_cache = &cache;
//                 break;
//             } else if (cache.usage_counter < lowest_usage_counter) {
//                 lowest_cache = &cache;
//                 lowest_usage_counter = lowest_cache->usage_counter;
//             }
//         }
//
//         lowest_cache->allocate(address);
//         return *lowest_cache;
//     }
//
//     page_cache *find_cache(global_address_type addr) {
//         for (auto &cache : _caches) {
//             if (cache.allocated() && addr >= cache.addr &&
//                 (addr - cache.addr) < EEPROM_25LC1024_PAGE_SIZE) {
//                 return &cache;
//             }
//         }
//         return nullptr;
//     }
//
//     const page_cache *find_cache(global_address_type addr) const {
//         for (auto &cache : _caches) {
//             if (addr >= cache.addr &&
//                 (addr - cache.addr) < EEPROM_25LC1024_PAGE_SIZE) {
//                 return &cache;
//             }
//         }
//         return nullptr;
//     }
//
//     page_cache &get_or_allocate_at_current_address() {
//         page_cache *cache = find_cache(_state.global_address());
//
//         if (cache == nullptr) {
//             cache = &allocate_cache(_state.global_address());
//             // TODO (sbenish): Read data into cache
//             //                 Set the size of the cache.
//         }
//
//         return *cache;
//     }
// };

// /**
//  * A wrapper for a basic eeprom stream that uses a caching container to
//  minimize
//  * IO operations. The template provides the ability for a user to inject the
//  * type of container as well as an addressing policy of the cache. By
//  default,
//  * the cache's addressing policy will attempt to cache full EEPROM pages
//  unless
//  * the cache is smaller than an EEPROM page.
//  *
//  * In addition, each cache stream provides the ability to set the cache's
//  write
//  * policy. By default, the cache is set to write_back--minimizing the number
//  of
//  * write operations. This works best with the default cache addressing policy
//  to
//  * ensure the minimum number of page writes.
//  */
// template <template <typename> typename CacheContainer,
//           typename CacheAddressingPolicy =
//               decltype(allocate_cache_to_full_forward_aligned_pages)>
//     requires std::invocable<CacheAddressingPolicy,
//                             basic_eeprom_stream::global_address_type,
//                             std::size_t>
// class old_cached_eeprom_stream {
//   public:
//     using value_type = basic_eeprom_stream::value_type;
//     using global_address_type = basic_eeprom_stream::global_address_type;
//     using local_address_type = basic_eeprom_stream::local_address_type;
//     using difference_type = basic_eeprom_stream::difference_type;
//
//     using seek_begin_t = basic_eeprom_stream::seek_begin_t;
//     using seek_end_t = basic_eeprom_stream::seek_end_t;
//     using seek_current_t = basic_eeprom_stream::seek_current_t;
//
//     /**
//      * Write operations always occur on the cache.
//      * Write operations occur simultaneously on the EEPROM and cache.
//      */
//     struct caching_policy_write_through {};
//
//     /**
//      * Write operations may ignore the cache if a cache miss occurs.
//      * Write operations always occurs on the EEPROM.
//      */
//     struct caching_policy_write_around {};
//
//     /**
//      * Write operations always occur on the cache.
//      * Write operations to EEPROM are delayed until the cached data
//      * is evicted or the flush() method is called.
//      */
//     struct caching_policy_write_back {};
//
//     using caching_policies =
//         std::variant<caching_policy_write_through,
//         caching_policy_write_around,
//                      caching_policy_write_back>;
//
//     caching_policies get_caching_policy() const { return _policy; }
//
//     void set_caching_policy(const caching_policies &policy) {
//         _policy = policy;
//     }
//
//     std::optional<std::byte> read() {
//         if (auto ADDRESS = _state.global_address(); in_cache(ADDRESS)) {
//             std::byte rt = _cache[index_in_cache(ADDRESS)];
//             _state.seek(pnp_database::seeking::current, 1);
//             return rt;
//         } else {
//
//         }
//
//         return { };
//         // TODO (sbenish)
//         // if next address is not in cache
//         //  free cache
//         //  allocate cache at next address using the stream
//         // output from the cache
//         // increment the poiner
//     }
//     std::size_t read(std::span<std::byte> dest) {
//         // TODO (sbenish)
//         //
//     }
//
//     bool write(std::byte value);
//     std::size_t write(std::span<const std::byte> src);
//
//     /// \brief Saves the state of the stream into a memento.
//     basic_eeprom_stream::memento save() const { return _stream.save(); }
//
//     /// \brief Loads a memento as the state of the stream.
//     cached_eeprom_stream<CacheContainer, CacheAddressingPolicy> &
//     load(const basic_eeprom_stream::memento &memento);
//
//     global_address_type head_global_address() const {
//         return _stream.head_global_address();
//     }
//
//     global_address_type tail_global_address() const {
//         return _stream.tail_global_address();
//     }
//
//     global_address_type global_address() const {
//         return _stream.global_address();
//     }
//
//     local_address_type address() const { return _stream.address(); }
//
//     local_address_type size() const { return _stream.size(); }
//
//     local_address_type seek(seek_begin_t _, std::size_t from_head) {
//         return _stream.seek(_, from_head);
//     }
//
//     local_address_type seek(seek_end_t _, std::size_t from_tail) {
//         return _stream.seek(_, from_tail);
//     }
//
//     local_address_type seek(seek_current_t _, difference_type offset) {
//         return _stream.seek(_, offset);
//     }
//
//     void flush();
//
//     /// \brief Extracts the basic EEPROM stream while ending the lifetime of
//     the
//     /// cached EEPROM stream.
//     static basic_eeprom_stream
//     uncache(cached_eeprom_stream<CacheContainer, CacheAddressingPolicy>
//                 &&cached_stream) {
//         return cached_stream._stream;
//     }
//
//   private:
//     basic_eeprom_stream::memento _state;
//
//     CacheAddressingPolicy _addressing_policy;
//     CacheContainer<std::byte> _cache;
//     caching_policies _policy;
//
//     std::optional<
//         std::tuple<basic_eeprom_stream::global_address_type, std::size_t>>
//         _cached_address_and_size;
//
//     std::optional<
//         std::tuple<basic_eeprom_stream::global_address_type, std::size_t>>
//         _cached_dirty_address_and_size;
//     ///
//     /// \brief Indicates if the current stream head is in the cache.
//     bool in_cache(global_address_type address) const {
//         return _cached_address_and_size &&
//                address >= std::get<0>(*_cached_address_and_size) &&
//                (address - std::get<0>(*_cached_address_and_size)) <
//                    std::get<1>(*_cached_address_and_size);
//     }
//
//     /// \pre in_cache(address) == true
//     std::size_t index_in_cache(global_address_type address) const {
//         return address - std::get<0>(*_cached_address_and_size);
//     }
//
//     template <typename T> struct addressed_span {
//         std::span<T> span;
//         global_address_type head;
//
//         constexpr global_address_type tail() const {
//             return head + span.size();
//         }
//     };
//
//     std::span<std::byte> get_cache_span(global_address_type address,
//                                         std::size_t size) {
//         if (!_cached_address_and_size ||
//             address < std::get<0>(*_cached_address_and_size)) {
//             return std::span<std::byte>();
//         }
//
//         const std::size_t MODIFIED_SIZE =
//             std::get<1>(*_cached_address_and_size) +
//             std::get<0>(*_cached_address_and_size) - address;
//         const std::size_t SIZE = std::min(MODIFIED_SIZE, size);
//
//         return std::span<std::byte>(_cache).subspan(
//             address - std::get<0>(*_cached_address_and_size), SIZE);
//     }
//
//     std::span<const std::byte> get_cache_span(global_address_type address,
//                                               std::size_t size) const {
//         if (!_cached_address_and_size ||
//             address < std::get<0>(*_cached_address_and_size)) {
//             return std::span<const std::byte>();
//         }
//
//         const std::size_t MODIFIED_SIZE =
//             std::get<1>(*_cached_address_and_size) +
//             std::get<0>(*_cached_address_and_size) - address;
//         const std::size_t SIZE = std::min(MODIFIED_SIZE, size);
//
//         return std::span<const std::byte>(_cache).subspan(
//             address - std::get<0>(*_cached_address_and_size), SIZE);
//     }
//
//     /// \brief Takes the intersection of the passed address span with the
//     cache. addressed_span<std::byte> intersect_with_cache(const
//     addressed_span<std::byte> &span) const {
//         const global_address_type NEW_HEAD =
//             std::max(span.head, std::get<0>(*_cached_address_and_size));
//         const global_address_type NEW_TAIL =
//             std::min(span.tail(), std::get<0>(*_cached_address_and_size) +
//                                       std::get<1>(*_cached_address_and_size));
//
//         return addressed_span{
//             .span = span.span.subspan(NEW_HEAD - span.head,
//                                       NEW_TAIL >= NEW_HEAD ? NEW_TAIL -
//                                       NEW_HEAD
//                                                            : 0),
//             .head = NEW_HEAD,
//         };
//     }
//
//     /**
//      * Splits writes to the cache into the span and split the span into two
//      * sections-- a before section and after section.
//      *
//      * The returned tuple returns { before span, after span }.
//      * If no operation was performed, after span == dest span.
//      * The dest span was fully consumed if the before and after span have 0
//      * size.
//      */
//     std::tuple<addressed_span<std::byte>, addressed_span<std::byte>>
//     write_to_cache(addressed_span<std::byte> &src) {
//         using return_t =
//             std::tuple<addressed_span<std::byte>, addressed_span<std::byte>>;
//
//         addressed_span<std::byte> within_cache = intersect_with_cache(src);
//         std::span<std::byte> cache_span =
//             get_cache_span(within_cache.head, within_cache.span.size());
//         // assert: cache_span.size() == within_cache.span.size()
//
//         std::ranges::copy(within_cache.span, cache_span);
//
//         return return_t{
//             addressed_span<std::byte>{
//                 .head = src.head,
//                 .span = src.span.subspan(0, within_cache.head - src.head),
//             },
//             addressed_span<std::byte>{
//                 .head = within_cache.tail(),
//                 .span = src.span.subspan(within_cache.tail() - src.head,
//                                          src.tail() - within_cache.tail()),
//             }};
//     }
//
//     /**
//      * Splits reads from the cache into the span and split the span into two
//      * sections-- a before section and after section.
//      *
//      * The returned tuple returns { before span, after span }.
//      * If no operation was performed, after span == dest span.
//      * The dest span was fully filled if the before and after span have 0
//      size.
//      */
//     std::tuple<addressed_span<std::byte>, addressed_span<std::byte>>
//     read_from_cache(addressed_span<std::byte> &dest) const {
//         using return_t =
//             std::tuple<addressed_span<std::byte>, addressed_span<std::byte>>;
//
//         addressed_span<std::byte> within_cache = intersect_with_cache(dest);
//         std::span<const std::byte> cache_span =
//             get_cache_span(within_cache.head, within_cache.span.size());
//         // assert: cache_span.size() == within_cache.span.size()
//
//         std::ranges::copy(cache_span, within_cache.span.begin());
//
//         return return_t{
//             addressed_span<std::byte>{
//                 .head = dest.head,
//                 .span = dest.span.subspan(0, within_cache.head - dest.head),
//             },
//             addressed_span<std::byte>{
//                 .head = within_cache.tail(),
//                 .span = dest.span.subspan(within_cache.tail() - dest.head,
//                                           dest.tail() - within_cache.tail()),
//             }};
//     }
// };
// static_assert(pnp_database::io_stream<cached_eeprom_stream>);
// static_assert(pnp_database::addressable_stream<cached_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_begin_seeking<cached_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_current_seeking<cached_eeprom_stream>);
// static_assert(pnp_database::seeking::supports_end_seeking<cached_eeprom_stream>);

}  // namespace drivers::eeprom

// EOF
