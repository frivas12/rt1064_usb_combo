// lut.hh

#ifndef _SRC_SYSTEM_DRIVERS_LUT_LUT_HH_
#define _SRC_SYSTEM_DRIVERS_LUT_LUT_HH_

/**************************************************************************//**
 * \file lut.hh
 * \author Sean Benish
 * \brief Driver for a Look-Up Table.
 *****************************************************************************/
#include "efs-handle.hh"
#include "25lc1024.h"

#include "defs.hh"
#include "slot_types.h"
#include "lut_types.hh"

#include <cstring>


/*****************************************************************************
 * Defines
 *****************************************************************************/

#define INVALID_KEY                 -1
#define INVALID_PAYLOAD_SIZE        0

/**
 * The latest version of LUT files that the firmware supports.
 */
constexpr uint8_t LUT_LATEST_FW_VERSION = 1;

/*****************************************************************************
 * Data Types
 *****************************************************************************/
template<typename STRUCTURE_KEY>
struct __attribute__((packed)) LUTHeaderEntry
{
    STRUCTURE_KEY key;          ///> Key that identies what the series of pages are.
    uint16_t starting_offset;   ///> Page offset from header page of LUT that holds the page with the first entry.
};

template<typename DISCRIMINATOR_KEY, typename PAYLOAD>
struct __attribute__((packed)) LUTEntry
{
    DISCRIMINATOR_KEY key;      ///> Key that discriminates an individual entry from the structure key.
    PAYLOAD payload;            ///> Payload of the structure key.
    CRC8_t crc;                 ///> CRC8 for error detection.
};

template<std::size_t INDIRECTION_COUNT>
struct __attribute__((packed)) LUT_Header
{
    uint8_t lut_version;                    ///> The version on the LUT file that is used.
    uint8_t indirection_count;              ///> The level of indirection of the LUT file (basic lut -> 0).
    uint16_t lut_page_size;                 ///> The number of bytes in a LUT page.
    uint8_t keys[2 + INDIRECTION_COUNT];
    /**
     * 8-bits are given to each level of indirection.
     * Then, a structure key is appended, indicating the amount of structure keys stored.
     */
};

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
class LUT
{
private:
    uint32_t (* const _get_payload_size) (const STRUCTURE_KEY);

    template<typename, typename, typename> friend class IndirectLUT;

public:

    LUT(const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY));

    /**
     * Looks for an entry with the provided keys and loads it into the provided
     * data structure.
     * \param[in]       r_handle The handle to the EFS file that stores the LUT.
     * \param[in]       str_key Structure key for the LUT.
     * \param[in]       disc_key Discriminator key for the LUT.
     * \param[out]      p_payload Pointer to the structure to configure from the LUT.
     * \param[in]       crc_init Initial crc to load.  Used with indirect luts.
     * \param[in]       start_address Starting address on the file.  Defaults to 0.
     * \param[in]       r_header Reference to the header object for the file.
     * \return OKAY on success.  Otherwise, a specific error.
     */
    LUT_ERROR load_data (const efs::handle& r_handle,
        const STRUCTURE_KEY str_key, const DISCRIMINATOR_KEY disc_key,
        char * const p_payload);

    LUT_ERROR load_data (const efs::handle& r_handle,
        const STRUCTURE_KEY str_key, const DISCRIMINATOR_KEY disc_key,
        char * const p_payload, const uint8_t crc_init,
        const uint32_t start_address, const LUT_Header<0>& r_header);

    const LUT_ID ID;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

constexpr int16_t SEARCH_INVALID = 0xFFFF;
template<typename KEY>
uint16_t search_page(const char * const p_data, const uint16_t entires_on_page, const uint32_t entry_size,
    const KEY key)
{
    int32_t start = 0;
    int32_t end = entires_on_page;

    while (start <= end)
    {
        const uint16_t current = (start + end) / 2;
        KEY current_key;
        memcpy(&current_key, p_data + current * entry_size, sizeof(KEY));

        if (current_key == key)
        {
            return current;
        } else if (current_key < key) {
            start = current + 1;
        } else {
            end = current - 1;
        }
    }

    return static_cast<uint16_t>(SEARCH_INVALID);
}

/**
 * Duplicates the header for a LUT with one less level of indirection.
 * \tparam INDIRECTION_COUNT The level of indirection for the starting object.
 * \param[in]       r_header Header at the current indirection level.
 * \return A header at an indirection level one less than in put header.
 */
template<std::size_t INDIRECTION_COUNT>
static LUT_Header<INDIRECTION_COUNT - 1> step_down_lut_header(const LUT_Header<INDIRECTION_COUNT>& r_header)
{
    LUT_Header<INDIRECTION_COUNT - 1> r_new;
    r_new.indirection_count = r_header.indirection_count - 1;
    r_new.lut_page_size = r_header.lut_page_size;
    r_new.lut_version = r_header.lut_version;
    memcpy(r_new.keys, &r_header.keys[1], INDIRECTION_COUNT + 1);
    return r_new;
}


template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY>::LUT(const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY))
    :
    _get_payload_size(payload_size_func),
    ID(id)
{}

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
LUT_ERROR LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY>::load_data(const efs::handle& r_handle,
    const STRUCTURE_KEY str_key, const DISCRIMINATOR_KEY disc_key,
        char * const p_payload)
{
    LUT_ERROR rt = LUT_ERROR::HEADER_MISSING;

    // * Assumption:  The handle is valid.

    // Read in the header.
    LUT_Header<0> header;
    if (r_handle.read(0, &header, sizeof(header)) == sizeof(header))
    {
        rt = load_data(r_handle, str_key, disc_key, p_payload, 0x00, header.lut_page_size, header);
    }


    return rt;
}

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
LUT_ERROR LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY>::load_data(const efs::handle& r_handle,
    const STRUCTURE_KEY str_key, const DISCRIMINATOR_KEY disc_key,
        char * const p_payload, const uint8_t init_crc, const uint32_t start_address,
        const LUT_Header<0>& r_header)
{
    LUT_ERROR rt = LUT_ERROR::OKAY;

    // * Assumption:  The handle is valid.

    // Check if the structure has no saved data, and allocate the page buffer

    // Size of the structure (in bytes).
    const uint32_t PAYLOAD_SIZE = _get_payload_size(str_key);
    char * const p_page_buffer = static_cast<char*>(pvPortMalloc(EFS_PAGE_SIZE));

    if (PAYLOAD_SIZE == INVALID_PAYLOAD_SIZE)
    {
        rt = LUT_ERROR::BAD_STRUCTURE_KEY;
        if (p_page_buffer != nullptr)
        {
            vPortFree(p_page_buffer);
        }
    } else if (p_page_buffer == nullptr) {
        rt = LUT_ERROR::OUT_OF_MEMORY;
    }

    if (rt == LUT_ERROR::OKAY)
    {
        // Read in header data.
        const uint16_t PAGE_SIZE = r_header.lut_page_size;


        // Verify supported LUT version
        if (r_header.lut_version != LUT_LATEST_FW_VERSION)
        {
            return LUT_ERROR::UNSUPPORTED_VERSION;
        }

        // Verify correct indirection
        if (r_header.indirection_count != 0)
        {
            return LUT_ERROR::INDIRECTION_MISMATCH;
        }

        // Verify correct size
        if (r_header.keys[1] != sizeof(DISCRIMINATOR_KEY) || r_header.keys[0] != sizeof(STRUCTURE_KEY))
        {
            return LUT_ERROR::KEY_SIZE_MISMATCH;
        }
        // Read in how many structure ids are used.
        r_handle.read(start_address, p_page_buffer, PAGE_SIZE);
        STRUCTURE_KEY in_header = static_cast<STRUCTURE_KEY>(p_page_buffer[0] | (static_cast<uint16_t>(p_page_buffer[1]) << 8));
        

        const uint32_t HEADER_SIZE = (static_cast<uint32_t>(in_header + 1) * sizeof(LUTHeaderEntry<STRUCTURE_KEY>)) + sizeof(STRUCTURE_KEY);
        const uint16_t HEADER_PAGES = (HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        const uint16_t ENTIRES_IN_FIRST_HEADER_PAGE = (PAGE_SIZE - sizeof(STRUCTURE_KEY)) / sizeof(LUTHeaderEntry<STRUCTURE_KEY>);
        const uint16_t ENTRIES_IN_OTHER_HEADER_PAGE = PAGE_SIZE / sizeof(LUTHeaderEntry<STRUCTURE_KEY>);

        uint16_t key_offset = static_cast<uint16_t>(SEARCH_INVALID);
        uint16_t next_offset = static_cast<uint16_t>(SEARCH_INVALID);


        // Search in each header page for the structure key.
        for (uint16_t header_page = 0; header_page < HEADER_PAGES && key_offset == static_cast<uint16_t>(SEARCH_INVALID); ++header_page)
        {
            const uint16_t ENTRIES_ON_THIS_PAGE = (header_page == HEADER_PAGES - 1) ? (in_header % ENTRIES_IN_OTHER_HEADER_PAGE) :
                ((header_page == 0) ? ENTIRES_IN_FIRST_HEADER_PAGE : ENTRIES_IN_OTHER_HEADER_PAGE);

            char * const p_start = p_page_buffer + ((header_page == 0) ? sizeof(STRUCTURE_KEY) : 0);

            if (header_page != 0)
            {
                r_handle.read(start_address + PAGE_SIZE*header_page, p_page_buffer, PAGE_SIZE);
            }

            // Binary search for entry between the two offsets.
            uint16_t key_index = search_page<STRUCTURE_KEY>(p_start, ENTRIES_ON_THIS_PAGE, sizeof(LUTHeaderEntry<STRUCTURE_KEY>), str_key);
            if (key_index != static_cast<uint16_t>(SEARCH_INVALID))
            {
                // Save the starting page for the key.
                LUTHeaderEntry<STRUCTURE_KEY> entry;
                memcpy(&entry, p_start + key_index * sizeof(LUTHeaderEntry<STRUCTURE_KEY>), sizeof(LUTHeaderEntry<STRUCTURE_KEY>));
                key_offset = entry.starting_offset;

                if (key_index == ENTRIES_ON_THIS_PAGE - 1 && header_page != HEADER_PAGES - 1)
                {
                    // End of this page.  Read first entry on the next page.
                    r_handle.read(start_address + PAGE_SIZE*(header_page + 1), p_start, sizeof(LUTHeaderEntry<STRUCTURE_KEY>));

                    key_index = 0;
                } else {
                    ++key_index;
                }

                // Save the ending page for the key.
                memcpy(&entry, p_start + key_index * sizeof(LUTHeaderEntry<STRUCTURE_KEY>), sizeof(LUTHeaderEntry<STRUCTURE_KEY>));
                next_offset = entry.starting_offset;
            }

            in_header -= static_cast<STRUCTURE_KEY>(ENTRIES_ON_THIS_PAGE);
        }

        if (key_offset == static_cast<uint16_t>(SEARCH_INVALID))
        {
            rt = LUT_ERROR::MISSING_KEY;
        } else {
            // Get the page number needed.
            const uint32_t ENTRY_SIZE = PAYLOAD_SIZE + sizeof(DISCRIMINATOR_KEY) + sizeof(CRC8_t);
            const uint32_t ENTIRES_PER_PAGE = PAGE_SIZE / ENTRY_SIZE;

            uint16_t start_page = key_offset;
            uint16_t end_page = next_offset - 2;
            uint16_t located_page = static_cast<uint16_t>(SEARCH_INVALID);

            uint16_t entry = static_cast<uint16_t>(SEARCH_INVALID);
            
            // For all but the last page (searching completely filled pages), do a binary search.
            while (start_page <= end_page)
            {
                // Read in the current page.
                const uint16_t current = (start_page + end_page) / 2;
                r_handle.read(start_address + PAGE_SIZE*current, p_page_buffer, PAGE_SIZE);

                // Get the lowest and highest keys from the page.
                DISCRIMINATOR_KEY lowest_key, highest_key;

                memcpy(&lowest_key, p_page_buffer, sizeof(DISCRIMINATOR_KEY));
                memcpy(&highest_key, p_page_buffer + ENTRY_SIZE*(ENTIRES_PER_PAGE - 1), sizeof(DISCRIMINATOR_KEY));

                if (disc_key < lowest_key) {
                    end_page = current - 1;
                } else if (disc_key > highest_key) {
                    start_page = current + 1;
                } else {
                    located_page = current;
                    entry = search_page<DISCRIMINATOR_KEY>(p_page_buffer, ENTIRES_PER_PAGE, ENTRY_SIZE, disc_key);
                    break;
                }
            }
            
            if (located_page == static_cast<uint16_t>(SEARCH_INVALID))
            {
                // Need to check the last page before giving up.
                r_handle.read(start_address + PAGE_SIZE*(next_offset - 1), p_page_buffer, PAGE_SIZE);
                for (uint16_t i = 0; i < ENTIRES_PER_PAGE; ++i)
                {
                    DISCRIMINATOR_KEY key;
                    memcpy(&key, p_page_buffer + i * ENTRY_SIZE, sizeof(DISCRIMINATOR_KEY));
                    if (disc_key == key) {
                        entry = i;
                        break;
                    } else if (key == static_cast<DISCRIMINATOR_KEY>(INVALID_KEY)) {
                        break;
                    }
                }
            }

            if (entry == static_cast<uint16_t>(SEARCH_INVALID)) {
                // The discrimator key could not be found.
                rt = LUT_ERROR::BAD_DISCRIMINATOR_KEY;
            } else {
                const CRC8_t CRC = CRC_split(
                    p_page_buffer + entry*ENTRY_SIZE,
                    ENTRY_SIZE - sizeof(CRC8_t),
                    CRC_split(
                        reinterpret_cast<const char*>(&str_key),
                        sizeof(STRUCTURE_KEY),
                        init_crc
                    )
                );

                // Cross reference computed CRC with the one saved.
                CRC8_t SAVED_CRC;
                memcpy(&SAVED_CRC, p_page_buffer + (entry + 1)*ENTRY_SIZE - sizeof(CRC8_t), sizeof(CRC8_t));

                if (CRC != SAVED_CRC) {
                    rt = LUT_ERROR::INVALID_ENTRY;
                } else {
                    // CRC was valid, so load configuration into memory.

                    memcpy(p_payload, p_page_buffer + entry*ENTRY_SIZE + sizeof(DISCRIMINATOR_KEY),
                        PAYLOAD_SIZE);
                }
            }
        }
    }

    vPortFree(p_page_buffer);

    return rt;
}
#endif

//EOF
