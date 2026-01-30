// indirect_lut.hh

/**************************************************************************//**
 * \file .hh
 * \author Sean Benish
 * \brief Driver for an indirect LUT.
 *
 * An indirect LUT is a LUT that has smaller "recursive" LUTs inside of it.
 * This allows for creating an extra indirection layer above an existing LUT.
 * It uses 3 keys instead of the 2 keys.
 *****************************************************************************/
#pragma once

#include "lut.tcc"
#include "cppmem.hh"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
template<typename KEY>
struct LUT_LL;

template<typename KEY>
struct LUT_LL
{
    KEY entries_in_header;
    uint16_t pages_needed;
    LUT_LL<KEY> * p_next;
};

template<typename INDIRECTION_KEY, typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
class IndirectLUT
{
    SemaphoreHandle_t _lock;

    uint32_t (* const _get_payload_size) (const STRUCTURE_KEY);     ///> Pointer to size translation function.

public:
    IndirectLUT(const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY));

    /**
     * Looks for an entry with the provided keys and loads it into the provided
     * data structure.
     * \param[in]       r_handle Handle to the EFS file that contains this LUT's data.
     * \param[in]       indr_key Indirection key for the LUT.
     * \param[in]       str_key Structure key for the LUT.
     * \param[in]       disc_key Discriminator key for the LUT.
     * \param[out]      p_payload Pointer to the structure to configure from the LUT.
     * \return OKAY on success.  Otherwise, a specific error.
     */
    LUT_ERROR load_data (const efs::handle& r_handle, INDIRECTION_KEY indr_key, const STRUCTURE_KEY str_key,
        const DISCRIMINATOR_KEY disc_key, char * const p_payload);

    const LUT_ID ID;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

template<typename INDIRECTION_KEY, typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
IndirectLUT<INDIRECTION_KEY, STRUCTURE_KEY, DISCRIMINATOR_KEY>::IndirectLUT(
    const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY))
    :
    _get_payload_size(payload_size_func),
    ID(id)
{
    _lock = xSemaphoreCreateMutex();
    if (_lock == NULL)
    {
        /// \todo Crash
    }
}

template<typename INDIRECTION_KEY, typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
LUT_ERROR IndirectLUT<INDIRECTION_KEY, STRUCTURE_KEY, DISCRIMINATOR_KEY>::load_data(
    const efs::handle& r_handle,
    const INDIRECTION_KEY indr_key, const STRUCTURE_KEY str_key,
    const DISCRIMINATOR_KEY disc_key, char * const p_payload)
{
    LUT_ERROR rt = LUT_ERROR::OKAY;
    
    // * Assumption:  The handle is valid.

    xSemaphoreTake(_lock, portMAX_DELAY);
    
    // Create enough memory to store the page.
    char * const p_page_buffer = static_cast<char*>(pvPortMalloc(EFS_PAGE_SIZE));

    if (p_page_buffer != NULL) {
        // Read in header data.
        LUT_Header<1> header;
        r_handle.read(0, &header, sizeof(header));

        const uint16_t PAGE_SIZE = header.lut_page_size;

        // Get the number of indirect LUTs.

        if (header.lut_version != LUT_LATEST_FW_VERSION)
        {
            return LUT_ERROR::UNSUPPORTED_VERSION;
        }
        if (header.indirection_count != 1)
        {
            return LUT_ERROR::INDIRECTION_MISMATCH;
        }
        if (header.keys[0] != sizeof(INDIRECTION_KEY))
        {
            return LUT_ERROR::KEY_SIZE_MISMATCH;
        }

        r_handle.read(PAGE_SIZE*1, p_page_buffer, PAGE_SIZE);
        INDIRECTION_KEY in_header = static_cast<INDIRECTION_KEY>(p_page_buffer[0] | (static_cast<uint16_t>(p_page_buffer[1]) << 8));
        

        const uint32_t HEADER_SIZE = (static_cast<uint32_t>(in_header) * sizeof(LUTHeaderEntry<STRUCTURE_KEY>)) + sizeof(header);
        const uint16_t HEADER_PAGES = (HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        const uint16_t ENTIRES_IN_FIRST_HEADER_PAGE = (PAGE_SIZE - sizeof(header)) / sizeof(LUTHeaderEntry<STRUCTURE_KEY>);
        const uint16_t ENTRIES_IN_OTHER_HEADER_PAGE = PAGE_SIZE / sizeof(LUTHeaderEntry<STRUCTURE_KEY>);

        uint16_t key_offset = static_cast<uint16_t>(SEARCH_INVALID);


        // Search each header page for the indirection key.
        for (uint16_t header_page = 0; header_page < HEADER_PAGES && key_offset == static_cast<uint16_t>(SEARCH_INVALID); ++header_page)
        {
            const uint16_t ENTRIES_ON_THIS_PAGE = (header_page == HEADER_PAGES - 1) ? (in_header % ENTRIES_IN_OTHER_HEADER_PAGE) :
                ((header_page == 0) ? ENTIRES_IN_FIRST_HEADER_PAGE : ENTRIES_IN_OTHER_HEADER_PAGE);

            char * const p_start = p_page_buffer + ((header_page == 0) ? sizeof(STRUCTURE_KEY) : 0);

            if (header_page != 0)
            {
                r_handle.read(PAGE_SIZE*header_page, p_page_buffer, PAGE_SIZE);
            }

            // Search for entry between the two offsets.
            uint16_t key_index = search_page<INDIRECTION_KEY>(p_start, ENTRIES_ON_THIS_PAGE, sizeof(LUTHeaderEntry<INDIRECTION_KEY>), indr_key);
            if (key_index != static_cast<uint16_t>(SEARCH_INVALID))
            {
                // Save the starting page for the key.
                LUTHeaderEntry<INDIRECTION_KEY> entry;
                memcpy(&entry, p_start + key_index * sizeof(LUTHeaderEntry<INDIRECTION_KEY>), sizeof(entry));
                key_offset = entry.starting_offset;

                if (key_index == ENTRIES_ON_THIS_PAGE - 1 && header_page != HEADER_PAGES - 1) {
                    // End of this page.  Read first entry on the next page.
                    r_handle.read(PAGE_SIZE*(header_page + 1), p_start, sizeof(LUTHeaderEntry<INDIRECTION_KEY>));

                    key_index = 0;
                } else {
                    ++key_index;
                }

                // Save the ending page for the key.
                memcpy(&entry, p_start + key_index * sizeof(LUTHeaderEntry<INDIRECTION_KEY>), sizeof(entry));

                break;
            }

            in_header -= static_cast<INDIRECTION_KEY>(ENTRIES_ON_THIS_PAGE);
        }

        if (key_offset == static_cast<uint16_t>(SEARCH_INVALID)) {
            rt = LUT_ERROR::MISSING_KEY;
        } else {
            // Instantiate a virtual LUT.
            LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY> lut(
                LUT_ID::RECURSIVE,
                _get_payload_size
            );

            const CRC8_t CRC_PART = CRC(reinterpret_cast<const char*>(&indr_key), sizeof(indr_key));

            // Load a smaller LUT starting at the current LUT's offset plus the page pointer to by the key.
            rt = lut.load_data(r_handle, str_key, disc_key, p_payload, CRC_PART, PAGE_SIZE*(1 + key_offset), step_down_lut_header(header));
        }

        vPortFree(p_page_buffer);
    } else {
        rt = LUT_ERROR::OUT_OF_MEMORY;
    }

    xSemaphoreGive(_lock);

    return rt;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

//EOF
