// efs.cc

/**************************************************************************//**
 * \file efs.cc
 * \author Sean Benish
 *****************************************************************************/
#include "efs.hh"
#include "efs.h"
#include "efs-cache.hh"

#include <span>
#include <type_traits>
#include <string.h>

#include "task.h"

#include "25lc1024.h"
#include "lock_guard.hh"

using namespace efs;

/*****************************************************************************
 * Constants
 *****************************************************************************/
constexpr uint8_t EFS_HEADER_PAGES = 1;

constexpr uint16_t EFS_TOTAL_PAGES = (EFS_EEPROM_END - EFS_EEPROM_START) / EFS_PAGE_SIZE;

// The number of allocation pages for the LUT manager.
constexpr uint8_t EFS_IPAGES = 1 + ((EFS_TOTAL_PAGES - 1) / 8 / EFS_PAGE_SIZE);

// The number of pages for LUT data.
constexpr uint16_t EFS_DATA_PAGES = EFS_TOTAL_PAGES - EFS_IPAGES - EFS_HEADER_PAGES;

constexpr std::size_t EFS_FILES_IN_SHARED_HEADER_PAGE = (EFS_PAGE_SIZE - sizeof(efs_header)) / sizeof(file_metadata);
constexpr std::size_t EFS_FILES_IN_HEADER_PAGE = (EFS_PAGE_SIZE) / sizeof(file_metadata);

constexpr std::size_t EFS_MAX_FILES = (EFS_HEADER_PAGES == 0) ? 0 : (
    EFS_FILES_IN_SHARED_HEADER_PAGE + (EFS_HEADER_PAGES - 1)*EFS_FILES_IN_HEADER_PAGE
);

constexpr char EFS_IDENTIFER[3] = {'E','F','S'};
constexpr uint8_t EFS_VERSION = 0;

/*****************************************************************************
 * Macros
 *****************************************************************************/

#define PAGE_TO_IPAGE(page)             ((page) / EFS_PAGE_SIZE)
#define PAGE_TO_IPAGE_BYTE(page)        (((page) % EFS_PAGE_SIZE) / 8)
#define PAGE_TO_IPAGE_BIT(page)         ((page) % 8)

#define IPAGE_TO_PAGE(ipage, byte, bit) (8*(EFS_PAGE_SIZE * (ipage) + (byte)) + (bit))

/// Outputs the allocation mask
/// It creates a mask offset from the start bit with "length" set bits.
#define IPAGE_MASK(start_bit, length)   ((uint8_t)((0xFF << (start_bit)) & ~(0xFF << ((start_bit) + (length)))))



/*****************************************************************************
 * Data Types
 *****************************************************************************/
constexpr efs_header EFS_STD_HEADER =
{
    {'E','F','S'},
    EFS_VERSION,
    EFS_PAGE_SIZE,
    EFS_TOTAL_PAGES,
    EFS_EEPROM_START,
    EFS_HEADER_PAGES,
    0
};


/// \brief Namespace implement ipage algorithmns.
namespace ipage_span
{
    template<typename T>
    concept trait = requires (T t, uint16_t efs_page, uint16_t pages_to_allocate, uint16_t offset, uint16_t size)
    {
        { t.is_allocated(efs_page) } -> std::convertible_to<bool>;
        { t.begin() } -> std::same_as<uint16_t>;
        { t.end() } -> std::same_as<uint16_t>;
        { t.size() } -> std::same_as<std::size_t>;
    };

    template<typename T>
    concept customization_point_subspan = trait<T> && requires (T t, uint16_t offset, uint16_t size)
    {
        { t.subspan(offset) } -> trait;
        { t.subspan(offset, size) } -> trait;
    };


    template<typename T>
    concept customization_point_find_next_unallocated_page = requires(T t, uint16_t efs_page)
    {
        { t.find_next_unallocated_page(efs_page) } -> std::same_as<uint16_t>;
    };

    template<typename T>
    concept customization_point_find_next_allocated_page = requires(T t, uint16_t efs_page)
    {
        { t.find_next_allocated_page(efs_page) } -> std::same_as<uint16_t>;
    };

    template<trait T>
    class subspan_adapter
    {
    public:
        using parent_type = T;

        constexpr uint16_t begin() const
        { return _begin; }

        constexpr uint16_t end() const
        { return _end; }

        constexpr std::size_t size() const
        { return _end - _begin; }

        constexpr bool is_allocated(uint16_t efs_page)
        { return _parent.is_allocated(efs_page); }

        constexpr bool is_allocated(uint16_t efs_page) const
            requires requires (const T& cr, uint16_t efs_page) {{ cr.is_allocated(efs_page) } -> std::convertible_to<bool>;}
        { return _parent.is_allocated(efs_page); }
        
        constexpr subspan_adapter<T> subspan(uint16_t offset) const & {
            return subspan_adapter(_parent, _begin + offset, size());
        }
        constexpr subspan_adapter<T> subspan(uint16_t offset, uint16_t size) const & {
            return subspan_adapter(_parent, _begin + offset, _begin + offset);
        }
        constexpr subspan_adapter<T>&& subspan(uint16_t offset) && {
            _begin += offset;
            return std::move(*this);
        }
        constexpr subspan_adapter<T>&& subspan(uint16_t offset, uint16_t size) && {
            _begin += offset;
            _end = _begin + size;
            return std::move(*this);
        }

        constexpr subspan_adapter(T&& parent, uint16_t offset)
            : _parent(std::forward<T>(parent))
            , _begin(parent.begin() + offset)
            , _end(parent.end())
        {}

        constexpr subspan_adapter(T&& parent, uint16_t offset, uint16_t size)
            : _parent(std::forward<T>(parent))
            , _begin(parent.begin() + offset)
            , _end(_begin + size)
        {}
    private:
        T _parent;
        uint16_t _begin;
        uint16_t _end;
    };

    template<typename T>
    subspan_adapter(T&, uint16_t) -> subspan_adapter<T&>;
    template<typename T>
    subspan_adapter(T&, uint16_t, uint16_t) -> subspan_adapter<T&>;
    template<typename T>
    subspan_adapter(const T&, uint16_t) -> subspan_adapter<const T&>;
    template<typename T>
    subspan_adapter(const T&, uint16_t, uint16_t) -> subspan_adapter<const T&>;

    /**
     * Find the the EFS page starting from efs_page that is unallocated.
     * \pre obj.begin() <= efs_page < obj.end()
     * \return The leftmost, unallocated EFS page to \ref{efs_page}.
     *         If none was found, end().
     */
    template<trait T>
    constexpr uint16_t find_next_unallocated_page(T&& obj, uint16_t efs_page) {
        if constexpr (customization_point_find_next_unallocated_page<T>) {
            return obj.find_next_unallocated_page(efs_page);
        } else {
            for (; obj.is_allocated(efs_page); ++efs_page);
            return efs_page;
        }
    }

    /**
     * Find the the EFS page starting from efs_page that is allocated.
     * \pre obj.begin() <= efs_page < obj.end()
     * \return The leftmost, allocated EFS page to \ref{efs_page}.
     *         If none was found, end().
     */
    template<trait T>
    constexpr uint16_t find_next_allocated_page(T&& obj, uint16_t efs_page) {
        if constexpr (customization_point_find_next_allocated_page<T>) {
            return obj.find_next_allocated_page(efs_page);
        } else {
            for (; !obj.is_allocated(efs_page); ++efs_page);
            return efs_page;
        }
    }

    template<trait T>
    constexpr auto subspan(T&& obj, uint16_t offset) {
        if constexpr (customization_point_subspan<T>) {
            return obj.subspan(offset);
        } else {
            return subspan_adapter(std::forward<T>(obj), offset);
        }
    }

    template<trait T>
    constexpr auto subspan(T&& obj, uint16_t offset, uint16_t size) {
        if constexpr (customization_point_subspan<T>) {
            return obj.subspan(offset, size);
        } else {
            return subspan_adapter(std::forward<T>(obj), offset, size);
        }
    }

    /**
     * Attempts to allocate the pages specified within the span.
     * \return The EFS page such that for all pages in range [return, return + pages_to_allocate)
     *         are unallocated.
     *         If non such subrange was found, then span.end();
     */
    constexpr uint16_t find_unallocated_region(trait auto&& span, uint16_t pages_to_allocate) {
        uint16_t itr = span.begin();
        const uint16_t END = span.end();
        while (itr != END) {
            // Step 1:  Skip allocated_pages
            itr = find_next_unallocated_page(span, itr);

            // Step 2:  Obtain subrange for allocation.
            const uint16_t SUBRANGE_END = std::min<uint16_t>(itr + pages_to_allocate, END);

            // Step 3:  Check the size of the allocation region.
            const std::size_t ALLOCATION_REGION_SIZE = SUBRANGE_END - itr;
            if (ALLOCATION_REGION_SIZE != pages_to_allocate)
            { return span.end(); }

            // Step 4:  Find the leftmost allocated page in the allocation region.
            uint16_t allocated_page_in_allocation_region_or_end = find_next_allocated_page(subspan(span, itr - span.begin(), ALLOCATION_REGION_SIZE), itr);
            static_assert(std::is_lvalue_reference_v<decltype(span)> == std::is_lvalue_reference_v<typename decltype(subspan(span, 0, 0))::parent_type>, "Subspan may be allocate a copy");
            const std::size_t FREE_PAGES_IN_ALLOCATION_REGION = SUBRANGE_END - allocated_page_in_allocation_region_or_end;

            if (FREE_PAGES_IN_ALLOCATION_REGION == pages_to_allocate) {
                break;
            } else {
                itr = allocated_page_in_allocation_region_or_end;
            }
        }

        return itr;
    };
};

/// \brief IPage span over a single byte value.
class ipage_span_byte
{
public:
    /// \pre begin() <= efs_page < end()
    constexpr bool is_allocated(uint16_t efs_page) const
    { return (_value & (0x01 << (efs_page - _efs_page))) != 0; }

    /// \pre begin() <= efs_page < end()
    constexpr uint16_t find_next_unallocated_page (uint16_t efs_page) const {
        uint8_t local = _value;
        uint16_t rt = begin();
        while ((local & 0x01) == 0) {
            local >>= 1;
            ++rt;
        }
        return rt;
    }

    /// \pre begin() <= efs_page < end()
    constexpr uint16_t find_next_allocated_page (uint16_t efs_page) const {
        uint8_t local = _value;
        uint16_t rt = begin();
        while ((local & 0x01) != 0) {
            local >>= 1;
            ++rt;
        }
        return rt;
    }

    static constexpr uint16_t size()
    { return 8; }

    constexpr uint16_t begin() const
    { return _efs_page; }

    constexpr uint16_t end() const
    { return begin() + size(); }

    constexpr ipage_span_byte(uint16_t efs_page, uint8_t ipage_byte)
        : _efs_page(efs_page)
        , _value(ipage_byte)
    {}
private:
    uint16_t _efs_page;
    uint8_t _value;
};


/**
 * Mixin for ipage_span::trait concepts.
 * Require the following methods to be implemented:
 * - ipage_span::trait::begin()
 * - ipage_span::trait::end()
 * - {this->select(efs_page)} -> ipage_span::trait;
 * \note The select() method is restricted to only allow
 *       one created object to be alive at a given time
 *       within the mixin methods.
 */
template<typename Self>
class subspan_operations_mixins
{
public:
    /// \pre begin() <= efs_page < end()
    constexpr bool is_allocated(uint16_t efs_page) {
        return self().select(efs_page).is_allocated(efs_page);
    }
    constexpr bool is_allocated(uint16_t efs_page) const {
        return self().select(efs_page).is_allocated(efs_page);
    }

    /// \pre begin() <= efs_page < end()
    constexpr uint16_t find_next_unallocated_page (uint16_t efs_page) {
        while (efs_page < self().end()) {
            ipage_span::trait auto start = self().select(efs_page);
            efs_page = ipage_span::find_next_unallocated_page(start, efs_page);

            // If the page was found, break out of the loop.
            if (efs_page != start.end()) {
                break;
            }
        }

        return efs_page;
    }
    constexpr uint16_t find_next_unallocated_page (uint16_t efs_page) const {
        while (efs_page < self().end()) {
            ipage_span::trait auto start = self().select(efs_page);
            efs_page = ipage_span::find_next_unallocated_page(start, efs_page);

            // If the page was found, break out of the loop.
            if (efs_page != start.end()) {
                break;
            }
        }

        return efs_page;
    }

    /// \pre begin() <= efs_page < end()
    constexpr uint16_t find_next_allocated_page (uint16_t efs_page) {
        while (efs_page < self().end()) {
            ipage_span::trait auto start = self().select(efs_page);
            efs_page = ipage_span::find_next_allocated_page(start, efs_page);

            // If the page was found, break out of the loop.
            if (efs_page != start.end()) {
                break;
            }
        }

        return efs_page;
    }
    constexpr uint16_t find_next_allocated_page (uint16_t efs_page) const {
        while (efs_page < self().end()) {
            ipage_span::trait auto start = self().select(efs_page);
            efs_page = ipage_span::find_next_allocated_page(start, efs_page);

            // If the page was found, break out of the loop.
            if (efs_page != start.end()) {
                break;
            }
        }

        return efs_page;
    }

    constexpr std::size_t size()
    { return self().end() - self().begin(); }
    constexpr std::size_t size() const
    { return self().end() - self().begin(); }
private:
    constexpr Self& self()
    { return *static_cast<Self*>(this); }
    constexpr const Self& self() const
    { return *static_cast<const Self*>(this); }
};

/// \brief An ipage span using the std::span of byte values.
class ipage_span_of_byte_spans : public subspan_operations_mixins<ipage_span_of_byte_spans>
{
public:
    /// \pre begin() <= efs_page < end()
    constexpr ipage_span_byte select(uint16_t efs_page) const {
        const uint16_t PAGE_OFFSET = efs_page - _efs_page;
        const uint16_t BYTE_OFFSET = PAGE_OFFSET / 8;

        return ipage_span_byte(begin() + 8 * BYTE_OFFSET, _serialized_ipages[BYTE_OFFSET]);
    }
    constexpr uint16_t begin() const
    { return _efs_page; }

    constexpr uint16_t end() const
    { return _efs_page + _serialized_ipages.size() * ipage_span_byte::size(); }

    constexpr ipage_span_of_byte_spans(uint16_t efs_page, std::span<uint8_t> ipages)
        : _serialized_ipages(ipages)
        , _efs_page(efs_page)
    {}
private:
    std::span<uint8_t> _serialized_ipages;
    uint16_t _efs_page;
};

/// \brief An ipage span of the entirety of EEPROM using a lazy internal page cache.
class ipage_span_full_eeprom : public subspan_operations_mixins<ipage_span_full_eeprom>
{
public:
    /// \pre begin() <= efs_page < end()
    /// \warning The lifetime of the returned span must end before select is called again.
    constexpr ipage_span_of_byte_spans select(uint16_t efs_page) {
        const uint16_t IPAGE = PAGE_TO_IPAGE(efs_page);

        // Recaches the buffer.
        if (IPAGE != _page_in_buffer) {
            if (IPAGE < EFS_IPAGES) {
                const uint16_t EFS_PAGE = IPAGE + EFS_HEADER_PAGES;
                // Read in IPage
                eeprom_25LC1024_read_safe(
                    EFS_EEPROM_START + (EFS_PAGE) * EFS_PAGE_SIZE,
                    _buffer.size(),
                    _buffer.data()
                );
                _page_in_buffer = EFS_PAGE;
            } else {
                _page_in_buffer = EFS_TOTAL_PAGES;
            }
        }
        return ipage_span_of_byte_spans(
            _page_in_buffer,
            (_page_in_buffer == EFS_TOTAL_PAGES)
            ? std::span<uint8_t>()
            : std::span<uint8_t>(_buffer)
        );
    }

    constexpr uint16_t begin() const
    { return 0; }

    constexpr uint16_t end() const
    { return EFS_TOTAL_PAGES; }

private:
    std::array<uint8_t, EFS_PAGE_SIZE> _buffer;
    uint16_t _page_in_buffer = EFS_TOTAL_PAGES;
};

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
/**
 * If space is found, it will set the metadata's page start and allocate ipages.
 * \warning Takes xSPI_Semaphore internally.
 * \param[in]       r_metadata Metadata object that needs its start page assigned.
 * \param[in]       p_buffer Buffer containing the EEPROM page size.
 * \return true Pages were allocated.
 * \return false Pages were not allocated.
 */
static bool allocate_ipages(file_metadata& r_metadata, uint8_t * p_buffer = nullptr);

/**
 * \warning External code needs to ensure mutual exclusion.
 */
static bool take_file_ownership(const file_identifier_t id);

/**
 * \warning External code needs to ensure mutual exclusion.
 */
static void release_file_ownership(const file_identifier_t id);

/**
 * Searches EEPROM for the file data.
 * \warning Internally gets xSPI_Semaphore.
 * \warning External code needs to ensure mutual exclusion.
 * \param id 
 * \return true The file identifer was found.
 * \return false The metadata was not found.
 */
static bool find_metadata(const file_identifier_t id, file_metadata& r_metadata, const TickType_t timeout);

/**
 * Sets all bits on an IPage starting at an EEPROM page.
 * \warning External code needs to ensure mutual exclusion.
 * \param[in]       start_page The staring page on the EEPROM to begin settings.
 * \param[in]       number_of_pages The number of EEPROM pages to change.
 * \param[in]       bit_state True to set the bits, false to reset the bits.
 * \param[inout]    p_existing_page Optional.  Pointer to existing memory for reading the EEPROM page.
 */
static void set_ipage_values(uint16_t start_page, const uint16_t number_of_pages, const bool bit_state,
    uint8_t * const p_existing_page = nullptr);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

static SemaphoreHandle_t s_lock_files;
// BEGIN    Locked Data (s_lock_files)
static uint16_t s_remaining_pages;
static uint16_t s_remaining_files;
static uint16_t s_rightmost_contiguous_space; // Lowest-valued page to empty space.
static uint8_t s_files_owned[32];
static bool s_lockdown = true; ///> When set, no handles can be created.
static metadata_cache_pool<EFS_EXTERNAL_CACHE_SIZE> s_external_caches;
static file_cache * sp_file_caches;

// END      Locked Data (s_lock_files)

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

// MARK:  SPI Mutex Required
void efs::init()
{
    s_lock_files = xSemaphoreCreateMutex();

    if (s_lock_files == nullptr)
    {
        // TODO:  Throw an error
        return;
    }
    memset(s_files_owned, 0, sizeof(s_files_owned));
    sp_file_caches = nullptr;

    efs_header header;

    eeprom_25LC1024_read(
        EFS_EEPROM_START,
        sizeof(header),
        reinterpret_cast<uint8_t*>(&header)
    );

    // ! Because this erases header data, make sure to back-up the filesystem before a firmware update.
    if (
        memcmp(header.identifier, EFS_IDENTIFER, sizeof(EFS_IDENTIFER)) != 0 ||
        header.version != EFS_VERSION ||
        header.page_size != EFS_PAGE_SIZE ||
        header.page_count != EFS_TOTAL_PAGES ||
        header.eeprom_start_address != EFS_EEPROM_START ||
        header.header_pages != EFS_HEADER_PAGES
    ) {
        erase();

        memcpy(&header.identifier, EFS_IDENTIFER, sizeof(EFS_IDENTIFER));
        header.version = EFS_VERSION;
        header.page_size = EFS_PAGE_SIZE;
        header.page_count = EFS_TOTAL_PAGES;
        header.eeprom_start_address = EFS_EEPROM_START;
        header.header_pages = EFS_HEADER_PAGES;

        eeprom_25LC1024_write(
            EFS_EEPROM_START,
            sizeof(header),
            reinterpret_cast<uint8_t*>(&header)
        );
    }

    // Calculate remaining files
    s_remaining_files = EFS_MAX_FILES;
    s_remaining_pages = EFS_DATA_PAGES;

    uint8_t * const p_buffer = new uint8_t[EFS_PAGE_SIZE];
    if (p_buffer == nullptr)
    {
        // TODO error
    }

    for (uint16_t header_page = 0; header_page < EFS_HEADER_PAGES; ++header_page)
    {
        const std::size_t FILES_IN_PAGE = (header_page == 0) ? EFS_FILES_IN_SHARED_HEADER_PAGE : EFS_FILES_IN_HEADER_PAGE;
        const std::size_t OFFSET = (header_page == 0) ? sizeof(efs_header) : 0;

        // Read in EEPROM page.
        eeprom_25LC1024_read(
            EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
            EFS_PAGE_SIZE,
            p_buffer
        );

        for (std::size_t file = 0; file < FILES_IN_PAGE; ++file)
        {
            file_metadata metadata;
            memcpy(&metadata, &p_buffer[OFFSET + file*sizeof(file_metadata)], sizeof(metadata));

            if ((metadata.attr & FILE_ATTRIBUTES_MASK_NOT_ALLOCATED) == 0)
            {
                --s_remaining_files;
                s_remaining_pages -= metadata.length;
            }
        }
    }

    // Calculate the rightmost contiguous space.
    const uint16_t START_IPAGE_PAGE = PAGE_TO_IPAGE(EFS_TOTAL_PAGES - 1);
    const uint16_t START_IPAGE_BYTE = PAGE_TO_IPAGE_BYTE(EFS_TOTAL_PAGES - 1);
    const uint8_t START_IPAGE_BIT = PAGE_TO_IPAGE_BIT(EFS_TOTAL_PAGES - 1);

    uint16_t ipage = START_IPAGE_PAGE;
    while (s_rightmost_contiguous_space == 0)
    {
        // Read in page
        eeprom_25LC1024_read(
            EFS_EEPROM_START + (ipage + EFS_HEADER_PAGES) * EFS_PAGE_SIZE,
            EFS_PAGE_SIZE,
            reinterpret_cast<uint8_t *>(p_buffer)
        );

        uint16_t byte = (ipage == START_IPAGE_PAGE) ? START_IPAGE_BYTE : (EFS_PAGE_SIZE - 1);

        if (ipage == START_IPAGE_BYTE)
        {
            // Mask out any page after the start page (so allocated pages after start page does not
            // trigger the detection).
            p_buffer[START_IPAGE_BYTE] |= (0xFF >> START_IPAGE_BIT);
        }

        while (s_rightmost_contiguous_space == 0)
        {
            // Seach each byte value to find a page with a reserved page.
            const uint8_t VALUE = p_buffer[byte];
            if (VALUE != 0xFF)
            {
                // Find the rightmost zero (allocated page)
                for(int8_t i = 7; i >= 0 && s_rightmost_contiguous_space == 0; --i)
                {
                    if (((VALUE >> i) & 0x01) == 0)
                    {
                        s_rightmost_contiguous_space = IPAGE_TO_PAGE(ipage, byte, static_cast<uint8_t>(i)) + 1;
                    }
                }
            }

            if (byte == 0) {
                break;
            } else {
                --byte;
            }
        }

        if (ipage == 0) {
            break;
        } else {
            --ipage;
        }
    }

    delete[] p_buffer;

    // Initialize any lazy caches so far (statically created).
    file_cache * p_cache = sp_file_caches;
    while (p_cache != nullptr)
    {
        p_cache->init_from_isr(p_cache->get_file_id());
    }

    // Invalidate all of the local caches.
    s_external_caches.clear();

    // Remove the lockdown.
    s_lockdown = false;
}

void efs_init(void)
{
    efs::init();
}


bool efs::erase()
{
    // TODO:  Find out why this won't compile.
    //static_assert(std::is_standard_layout_v<file_metadata>);

    // Begin the lockdown
    s_lockdown = true;

    // Invalidate all caches.
    file_cache * p_cleanup = sp_file_caches;
    while (p_cleanup != nullptr)
    {
        p_cleanup->_cache.invalidate();
        p_cleanup = p_cleanup->_p_next;
    }

    s_external_caches.clear();

    // Wait until all files are free.
    for (std::size_t i = 0; i < sizeof(s_files_owned); ++i)
    {
        // Optimization for files already free.
        if (s_files_owned[i] != 0xFF)
        {
            for (int j = 0; j < 8; ++j)
            {
                bool in_use = (s_files_owned[i] & (1 << j)) > 0;
                while (in_use)
                {
                    // Yield
                    vTaskDelay(0);
                    in_use = (s_files_owned[i] & (1 << j)) > 0;
                }
            }
        }
    }

    // This is 256 bytes of 0xFF.
    uint8_t * page_buffer = new uint8_t[EFS_PAGE_SIZE];
    if (page_buffer == nullptr)
    {
        // TODO: Error
        return false;
    }
    memset(page_buffer, 0xFF, EFS_PAGE_SIZE);

    
    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
    // Mark all files as unallocated
    for (uint16_t header_page = 0; header_page < EFS_HEADER_PAGES; ++header_page)
    {
        // Unallocate all files

        if (header_page == 0)
        {
            efs_header header = EFS_STD_HEADER;

            memcpy(&page_buffer[0], &header, sizeof(header));
        }

        // Writeback
        eeprom_25LC1024_write(
            EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
            EFS_PAGE_SIZE,
            reinterpret_cast<uint8_t*>(page_buffer)
        );

        if (header_page == 0)
        {
            memset(page_buffer, 0xFF, sizeof(efs_header));
        }
    }
    
    constexpr std::size_t END_OF_FORCE_ZERO = EFS_HEADER_PAGES + EFS_IPAGES;
    for(std::size_t page = 0; page < EFS_IPAGES; ++page)
    {
        // Clear all bits that represent the IPage and header pages.
        const std::size_t FIRST_PAGE = 8*EFS_PAGE_SIZE*page;
        const bool AFFECTED = FIRST_PAGE < END_OF_FORCE_ZERO;
        const std::size_t MIN_VAL = (EFS_PAGE_SIZE < END_OF_FORCE_ZERO - FIRST_PAGE) ? (EFS_PAGE_SIZE) : (END_OF_FORCE_ZERO - FIRST_PAGE);
        const std::size_t PAGES_AFFECTED = (AFFECTED) ? (MIN_VAL) : 1;
        const std::size_t FULL_BYTES_AFFECTED = PAGES_AFFECTED / 8;
        const std::size_t PARTIALLY_AFFECTED = PAGES_AFFECTED % 8;
        if (AFFECTED)
        {
            memset(page_buffer, 0, FULL_BYTES_AFFECTED);
            page_buffer[FULL_BYTES_AFFECTED] &= ~IPAGE_MASK(0, PARTIALLY_AFFECTED);
        }

        // Writeback
        eeprom_25LC1024_write(
            EFS_EEPROM_START + EFS_PAGE_SIZE*(page + EFS_HEADER_PAGES),
            EFS_PAGE_SIZE,
            reinterpret_cast<uint8_t*>(page_buffer)
        );

        // Reset the bits.
        memset(page_buffer, 0xFF, FULL_BYTES_AFFECTED + (PARTIALLY_AFFECTED > 0) ? 1 : 0);
    }
    xSemaphoreGive(xSPI_Semaphore);

    delete[] page_buffer;

    s_remaining_files = EFS_MAX_FILES;
    s_remaining_pages = EFS_DATA_PAGES;

    return true;
}

bool efs::create_file(
    const file_identifier_t id,
    const uint16_t pages_to_occupy,
    const file_attributes_t attributes,
    const TickType_t timeout
) {
    bool rt = false;
    uint16_t alloc_header_page = 0;
    uint32_t alloc_header_offset = 0;

    file_metadata metadata = {id, attributes, 0, pages_to_occupy};

    uint8_t * const p_buffer = new uint8_t[EEPROM_25LC1024_PAGE_SIZE];
    if (p_buffer == nullptr)
    {
        // TODO:  ERROR
        return false;
    }

    lock_guard lg(s_lock_files);
    // Search for an existing file as well as an empty header space.
    bool prevent_allocation = s_lockdown;
    for (uint16_t header_page = 0; !prevent_allocation && header_page < EFS_HEADER_PAGES; ++header_page)
    {
        const std::size_t FILES_IN_PAGE = (header_page == 0) ? EFS_FILES_IN_SHARED_HEADER_PAGE : EFS_FILES_IN_HEADER_PAGE;
        const std::size_t OFFSET = (header_page == 0) ? sizeof(efs_header) : 0;

        // Read in EEPROM page.
        eeprom_25LC1024_read_safe(
            EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
            EFS_PAGE_SIZE,
            p_buffer
        );

        for (std::size_t file = 0; file < FILES_IN_PAGE; ++file)
        {
            file_metadata metadata;
            memcpy(&metadata, &p_buffer[OFFSET + file*sizeof(file_metadata)], sizeof(metadata));
            const bool METADATA_IS_VALID = (metadata.attr & FILE_ATTRIBUTES_MASK_NOT_ALLOCATED) == 0;

            if (metadata.id == id && METADATA_IS_VALID)
            {
                prevent_allocation = true;
                rt = false;
                break;
            }


            if (!rt && !prevent_allocation && !METADATA_IS_VALID)
            {
                alloc_header_page = header_page;
                alloc_header_offset = OFFSET + file*sizeof(file_metadata);
                rt = true;
            }
        }
    }

    if (!prevent_allocation)
    {
        // If true, s_remaining_pages, IPages, and s_rightmost_contiguous_space will be updated.
        rt = allocate_ipages(metadata, p_buffer);
    }
    delete[] p_buffer;

    if (rt)
    {
        // Write to header.
        eeprom_25LC1024_write_safe(
            EFS_EEPROM_START + EFS_PAGE_SIZE*alloc_header_page + alloc_header_offset,
            sizeof(file_metadata),
            reinterpret_cast<uint8_t*>(&metadata)
        );

        file_cache * p_cache = sp_file_caches;
        while (p_cache != nullptr)
        {
            if (p_cache->_cache.get_metadata().id == id)
            {
                p_cache->_cache.cache(metadata);
            }

            p_cache = p_cache->_p_next;
        }

        --s_remaining_files;
    }


    return rt;
}

bool efs::does_file_exist(
    const file_identifier_t id,
    const TickType_t timeout
)
{
    lock_guard lg(s_lock_files);
    file_metadata _;
    return find_metadata(id, _, timeout);
}


uint16_t efs::get_maximum_files()
{
    return EFS_MAX_FILES;
}

uint16_t efs::get_free_files()
{
    lock_guard lock (s_lock_files);
    return s_remaining_files;
}

uint16_t efs::get_free_pages()
{
    lock_guard lock (s_lock_files);
    return s_remaining_pages;
}

efs_header efs::get_header_info()
{
    return EFS_STD_HEADER;
}

void efs::add_to_external_cache(const file_identifier_t id_to_cache, const TickType_t timeout)
{
    lock_guard lg(s_lock_files);
    file_metadata metadata;
    if (find_metadata(id_to_cache, metadata, timeout))
    {
        s_external_caches.add(metadata);
    }
}



#ifdef _TESTING

file_metadata efs::get_file_metadata(const file_identifier_t id)
{
    file_metadata rt;
    find_metadata(id, rt, portMAX_DELAY);
    return rt;
}

#endif




// efs-handle.hh
handle handle::create_handle(
    const file_identifier_t id,
    const TickType_t timeout,
    const user_e user
) {
    file_metadata metadata;
    bool got_metadata = false;

    lock_guard lg(s_lock_files);

    if (s_lockdown)
    {
        // In lockdown, so return invalid handle.
        return handle(user);
    }

    if (user == EXTERNAL)
    {
        got_metadata = s_external_caches.get_metadata(id, metadata);
    }

    if (!got_metadata)
    {
        got_metadata = find_metadata(id, metadata, timeout);
    }


    if (got_metadata && take_file_ownership(id)) {
        // Owned handle
        return handle(user, metadata);
    } else {
        // Invalid handle
        return handle(user);
    }
}

bool handle::delete_file()
{
    if (!can_delete())
    {
        return false;
    }

    uint8_t * const p_buffer = new uint8_t[EEPROM_25LC1024_PAGE_SIZE];
    if (p_buffer == nullptr)
    {
        // TODO: ERROR
        return false;
    }

    close();
    lock_guard lg (s_lock_files);

    // Invalidate any caches that had the file.
    file_cache * p_cache = sp_file_caches;
    while (p_cache != nullptr)
    {
        if (p_cache->_cache.get_metadata().id == _metadata.id)
        {
            p_cache->_cache.invalidate();
            if (p_cache->_owns_file)
            {
                p_cache->release_ownership();
            }
        }
        p_cache = p_cache->_p_next;
    }

    // Invalidate the external caches
    s_external_caches.remove(_metadata.id);

    // Free Ipages
    set_ipage_values(_metadata.start, _metadata.length, true, p_buffer);
    // Mark as unallocated
    bool finished = false;
    for (uint16_t header_page = 0; !finished && header_page < EFS_HEADER_PAGES; ++header_page)
    {
        const std::size_t FILES_IN_PAGE = (header_page == 0) ? EFS_FILES_IN_SHARED_HEADER_PAGE : EFS_FILES_IN_HEADER_PAGE;
        const std::size_t OFFSET = (header_page == 0) ? sizeof(efs_header) : 0;

        // Read in EEPROM page.
        eeprom_25LC1024_read_safe(
            EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
            EFS_PAGE_SIZE,
            p_buffer
        );

        for (std::size_t file = 0; file < FILES_IN_PAGE; ++file)
        {
            file_metadata metadata;
            memcpy(&metadata, &p_buffer[OFFSET + file*sizeof(file_metadata)], sizeof(metadata));

            if (metadata.id == _metadata.id)
            {
                finished = true;

                // Mark file as unallocated and write to buffer.
                metadata.attr |= FILE_ATTRIBUTES_MASK_NOT_ALLOCATED;
                memcpy(&p_buffer[OFFSET + file*sizeof(file_metadata)], &metadata, sizeof(metadata));

                break;
            }
        }

        if (finished)
        {
            // Writeback
            eeprom_25LC1024_write_safe(
                EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
                EFS_PAGE_SIZE,
                p_buffer
            );
        }
    }

    // Update contiguous space if this was the rightmost file.
    if (_metadata.start + _metadata.length == s_rightmost_contiguous_space)
    {
        s_rightmost_contiguous_space -= _metadata.length;
    }
    ++s_remaining_files;
    s_remaining_pages += _metadata.length;

    delete[] p_buffer;
    return true;
}




// efs-cache.hh
file_cache::~file_cache()
{
    lock_guard lg (s_lock_files);
    file_cache * p_cache = sp_file_caches;
    if (this == sp_file_caches || sp_file_caches == nullptr)
    {
        sp_file_caches = nullptr;
        return;
    }
    while (p_cache->_p_next != nullptr)
    {
        if (p_cache->_p_next == this)
        {
            p_cache->_p_next = this->_p_next;
            break;
        }
        p_cache = p_cache->_p_next;
    }
}

cache_handle file_cache::get_handle(const bool wait_until_valid, const user_e user)
{
    if (!_cache_updated)
    {
        file_cache::init(_cache.get_metadata().id);
    }

    xSemaphoreTake(s_lock_files, portMAX_DELAY);
    // Spin while the cache has a handle.
    while (_has_handle)
    {
        xSemaphoreGive(s_lock_files);
        vTaskDelay(0);
        xSemaphoreTake(s_lock_files, portMAX_DELAY);
    }

    // Spin while the cache is not valid (if set).
    while (wait_until_valid && !_cache.is_valid())
    {
        xSemaphoreGive(s_lock_files);
        vTaskDelay(0);
        xSemaphoreTake(s_lock_files, portMAX_DELAY);
    }
    // A valid handle will only be made if the file exists.
    const bool VALID = _cache.is_valid() && (_owns_file || take_file_ownership(_cache.get_metadata().id));
    _has_handle = VALID && _owns_file;
    xSemaphoreGive(s_lock_files);
    if (VALID) {
        return cache_handle(user, this);
    } else {
        return cache_handle(user);
    }
}

bool file_cache::take_ownership()
{
    if (_owns_file)
    {
        return true;
    }

    xSemaphoreTake(s_lock_files, portMAX_DELAY);
    _owns_file = take_file_ownership(_cache.get_metadata().id);
    xSemaphoreGive(s_lock_files);
    return _owns_file;
}

void file_cache::release_ownership()
{
    if (!_owns_file)
    {
        return;
    }

    // Spin while the cache has a handle.
    xSemaphoreTake(s_lock_files, portMAX_DELAY);
    while (_has_handle)
    {
        xSemaphoreGive(s_lock_files);
        vTaskDelay(0);
        xSemaphoreTake(s_lock_files, portMAX_DELAY);
    }

    release_file_ownership(_cache.get_metadata().id);
    _owns_file = false;

    xSemaphoreGive(s_lock_files);
    
}

bool init_file_cache::take_ownership_init()
{
    if (_owns_file)
    {
        return true;
    }

    _owns_file = take_file_ownership(_cache.get_metadata().id);
    return _owns_file;
}

void init_file_cache::release_ownership_init()
{
    if (!_owns_file)
    {
        return;
    }

    // Spin while the cache has a handle.
    release_file_ownership(_cache.get_metadata().id);
    _owns_file = false;
    
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static bool allocate_ipages(file_metadata& r_metadata, uint8_t * p_buffer)
{
    bool rt = false;
    bool allocated = false;

    if (s_rightmost_contiguous_space + r_metadata.length <= EFS_TOTAL_PAGES)
    {
        // Fast solver, using the next page pointer.
        r_metadata.start = s_rightmost_contiguous_space;
        s_rightmost_contiguous_space += r_metadata.length;
        rt = true;
    } else if (s_remaining_pages >= r_metadata.length) {
        // The subspan operates on data pages that are not in the rightmost contiguous space.
        // This is because any pages at the rightmost contiguous space or later will be guaranteed to be unallocated.
        // Since the fast solver did not use this region, however, it will be ignored for the slow solver.
        constexpr uint16_t OFFSET = EFS_HEADER_PAGES + EFS_IPAGES;
        ipage_span_full_eeprom page_cache;
        ipage_span::trait auto subspan = ipage_span::subspan(page_cache, OFFSET, static_cast<uint16_t>(s_rightmost_contiguous_space - OFFSET));

        r_metadata.start = ipage_span::find_unallocated_region(subspan, r_metadata.length);
        rt = r_metadata.start != subspan.end();
    }

    if (rt)
    {
        // Allocate the memory buffer if it has yet to be allocated.
        allocated = p_buffer == nullptr;
        p_buffer = (p_buffer == nullptr) ? new uint8_t[EEPROM_25LC1024_PAGE_SIZE] : p_buffer;
        if (p_buffer == nullptr)
        {
            ///\todo Crash
        }

        set_ipage_values(r_metadata.start, r_metadata.length, false, p_buffer);
    }

    if (allocated)
    {
        delete[] p_buffer;
    }

    return rt;
}


static bool take_file_ownership(const file_identifier_t id)
{
    const std::size_t byte = static_cast<std::size_t>(static_cast<uint8_t>(id) / 8);
    const std::size_t bit = static_cast<std::size_t>(static_cast<uint8_t>(id) % 8);
    const uint8_t MASK = (1 << bit);
    // Mark the file as free.

    const bool RT = (s_files_owned[byte] & MASK) == 0;
    s_files_owned[byte] |= MASK; // Performance opt. (already set is okay)

    return RT;
}

static void release_file_ownership(const file_identifier_t id)
{
    const std::size_t byte = static_cast<std::size_t>(static_cast<uint8_t>(id) / 8);
    const std::size_t bit = static_cast<std::size_t>(static_cast<uint8_t>(id) % 8);

    // Mark the file as free.
    s_files_owned[byte] &= ~(1 << bit);
}


static bool find_metadata(const file_identifier_t id, file_metadata& r_metadata, const TickType_t timeout)
{
    bool rt = false;

    uint8_t * const p_buffer = new uint8_t[EFS_PAGE_SIZE];
    if (p_buffer == nullptr)
    {
        // TODO:  ERROR
        return false;
    }

    // Search for an existing file as well as an empty header space.
    for (uint16_t header_page = 0; !rt && header_page < EFS_HEADER_PAGES; ++header_page)
    {
        const std::size_t FILES_IN_PAGE = (header_page == 0) ? EFS_FILES_IN_SHARED_HEADER_PAGE : EFS_FILES_IN_HEADER_PAGE;
        const std::size_t OFFSET = (header_page == 0) ? sizeof(efs_header) : 0;

        // Read in EEPROM page.
        eeprom_25LC1024_read_safe(
            EFS_EEPROM_START + EFS_PAGE_SIZE*header_page,
            EFS_PAGE_SIZE,
            p_buffer
        );

        for (std::size_t file = 0; file < FILES_IN_PAGE; ++file)
        {
            memcpy(&r_metadata, &p_buffer[OFFSET + file*sizeof(file_metadata)], sizeof(r_metadata));

            if (r_metadata.id == id && (r_metadata.attr & FILE_ATTRIBUTES_MASK_NOT_ALLOCATED) == 0)
            {
                rt = true;
                break;
            }
        }
    }

    delete[] p_buffer;
    return rt;
}

static void set_ipage_values(uint16_t start_page, const uint16_t number_of_pages, const bool bit_state,
    uint8_t * const p_existing_page)
{
    const bool ALLOCATED_MEMORY = p_existing_page == nullptr;

    // Allocate memory if it did not already exist.
    uint8_t * p_buffer = (ALLOCATED_MEMORY) ? new uint8_t[EEPROM_25LC1024_PAGE_SIZE] : p_existing_page;
    if (p_buffer == nullptr)
    {
        // TODO: Crash
    }

    const uint16_t START_IPAGE_PAGE = PAGE_TO_IPAGE(start_page);
    const uint16_t START_IPAGE_BYTE = PAGE_TO_IPAGE_BYTE(start_page);
    const uint8_t START_IPAGE_BIT = PAGE_TO_IPAGE_BIT(start_page);

    const uint16_t END_PAGE = start_page + number_of_pages;
    const uint16_t END_IPAGE_PAGE = PAGE_TO_IPAGE(END_PAGE);
    const uint16_t END_IPAGE_BYTE = PAGE_TO_IPAGE_BYTE(END_PAGE);
    const uint8_t END_IPAGE_BIT = PAGE_TO_IPAGE_BIT(END_PAGE);

    const uint16_t END_IPAGE_PAGE_INC = PAGE_TO_IPAGE(END_PAGE - 1);
    const uint16_t END_IPAGE_BYTE_INC = PAGE_TO_IPAGE_BYTE(END_PAGE - 1);
    // const uint8_t END_IPAGE_BIT_INC = PAGE_TO_IPAGE_BIT(END_PAGE - 1);

    const uint16_t BYTE_DELTA = EEPROM_25LC1024_PAGE_SIZE*(END_IPAGE_PAGE - START_IPAGE_PAGE) + END_IPAGE_BYTE - START_IPAGE_BYTE;

    const bool DATA_ON_END_IPAGE = END_IPAGE_BYTE > 0 || END_IPAGE_BIT > 0;
    const bool END_BYTE_ENABLED = BYTE_DELTA > 0;
    const bool INTERMEDIATE_BYTES_ENABLED = BYTE_DELTA > 1;
    const bool INTERMEDIATE_BYTES_ON_END_PAGE = END_IPAGE_BYTE > 0;

    for (uint16_t ipage = START_IPAGE_PAGE; ipage < END_IPAGE_PAGE || (ipage == END_IPAGE_PAGE && DATA_ON_END_IPAGE); ++ipage)
    {
        // Read in IPAGE
        eeprom_25LC1024_read_safe(
            EFS_EEPROM_START + (ipage + EFS_HEADER_PAGES) * EEPROM_25LC1024_PAGE_SIZE,
            EEPROM_25LC1024_PAGE_SIZE,
            reinterpret_cast<uint8_t*>(p_buffer)
        );

        // Set first byte.
        if (ipage == START_IPAGE_PAGE)
        {
            const uint16_t PAGES_TO_ALLOCATE = ((START_IPAGE_PAGE == END_IPAGE_PAGE && START_IPAGE_BYTE == END_IPAGE_BYTE)
                ? END_IPAGE_BIT : 8) - START_IPAGE_BIT;
            const uint8_t MASK = IPAGE_MASK(START_IPAGE_BIT, PAGES_TO_ALLOCATE);
            p_buffer[START_IPAGE_BYTE] = (bit_state) ? (p_buffer[START_IPAGE_BYTE] | MASK) : (p_buffer[START_IPAGE_BYTE] & ~MASK);
        }

        const bool INTERMEDIATE_BYTES_ON_THIS_PAGE = INTERMEDIATE_BYTES_ENABLED && (ipage < END_IPAGE_PAGE || INTERMEDIATE_BYTES_ON_END_PAGE);

        // Set intermediate bytes.
        if (INTERMEDIATE_BYTES_ON_THIS_PAGE)
        {
            const uint16_t BYTE_START = (ipage == START_IPAGE_PAGE) ? (START_IPAGE_BYTE + 1) : 0;
            const uint16_t BYTE_END = (ipage == END_IPAGE_PAGE) ? END_IPAGE_BYTE : EEPROM_25LC1024_PAGE_SIZE;
            memset(p_buffer + BYTE_START, bit_state ? 0xFF : 0x00, BYTE_END - BYTE_START);
        }

        // Set last byte
        if (END_BYTE_ENABLED && ipage == END_IPAGE_PAGE_INC)
        {
            const uint8_t MASK = IPAGE_MASK(0, END_IPAGE_BIT);
            p_buffer[END_IPAGE_BYTE_INC] = (bit_state) ? (p_buffer[END_IPAGE_BYTE_INC] | MASK) : (p_buffer[END_IPAGE_BYTE_INC] & ~MASK);
        }

        // Write IPage
        eeprom_25LC1024_write_safe(
            EFS_EEPROM_START + (ipage + EFS_HEADER_PAGES) * EEPROM_25LC1024_PAGE_SIZE,
            EEPROM_25LC1024_PAGE_SIZE,
            reinterpret_cast<uint8_t*>(p_buffer)
        );
    }

    // Remove the memory if it was allocated.
    if (ALLOCATED_MEMORY)
    {
        delete[] p_buffer;
    }
}



// efs-handle.hh
void handle::release_ownership()
{
    lock_guard lg(s_lock_files);
    release_file_ownership(_metadata.id);
}

// efs-cache.hh
void file_cache::init(const file_identifier_t id_to_cache)
{
    file_metadata metadata;
    xSemaphoreTake(s_lock_files, portMAX_DELAY);
    const bool valid = find_metadata(id_to_cache, metadata, portMAX_DELAY);
    _p_next = sp_file_caches;
    sp_file_caches = this;
    _cache_updated = true;
    xSemaphoreGive(s_lock_files);
    if (valid) {
        _cache.cache(metadata);
    } else {
        _cache.cache(file_metadata{id_to_cache, 0, 0, 0});
        _cache.invalidate();
    }
}
void file_cache::init_from_isr(const file_identifier_t id_to_cache)
{
    file_metadata metadata;
    bool valid = false;
    _cache_updated = false;
    BaseType_t _;
    if (xSemaphoreTakeFromISR(s_lock_files, &_) == pdPASS)
    {
        valid = find_metadata(id_to_cache, metadata, portMAX_DELAY);
        _p_next = sp_file_caches;
        sp_file_caches = this;
        xSemaphoreGiveFromISR(s_lock_files, &_);
        _cache_updated = true;
    }
    if (valid) {
        _cache.cache(metadata);
    } else {
        _cache.cache(file_metadata{id_to_cache, 0, 0, 0});
        _cache.invalidate();
    }
}

void cache_handle::release_ownership ()
{
    if (_p_parent->_owns_file) {
        // Transfer ownership back to parent.
        lock_guard lg(s_lock_files);
        _p_parent->_has_handle = false;
    } else {
        // Call back class's release method.
        handle::release_ownership();
    }
}

// EOF
