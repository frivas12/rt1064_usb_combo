// intermediate-formats.hh

/**************************************************************************//**
 * \file intermediate-formats.hh
 * \author Sean Benish
 * \brief Defines the binary layout of intermediate formats.
 *
 * 
 *****************************************************************************/
#pragma once

#include <cstdint>
#include <vector>

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

struct LUT_Header
{
    uint8_t lut_version;                    ///> The version on the LUT file that is used.
    uint16_t lut_page_size;                 ///> The number of bytes in a LUT page.
    uint8_t indirection_count;              ///> The level of indirection of the LUT file (basic lut -> 0).
    std::vector<uint8_t> key_sizes;
    uint64_t keys_in_header;
};

/**
 * 0:       Lut Version
 * 1:       Indirection Count
 * 2-5:     Entry Size (keys array, signature array, struct, and crc)
 * Bytes 6 onward, 1 byte for key sizes (same as LUT_header)
 * After that, keys in byte form
 * After that, signature (keys)
 * After that, structure
 * After that, CRC
 */
struct compiled_entry
{
    uint8_t lut_version;
    uint8_t key_count;
    uint32_t struct_size;
    uint8_t * p_keys;
    uint8_t * p_signature;
    uint8_t * p_struct;
    uint8_t crc;
};

struct compiled_entry_slim
{
    // Version auto implied
    // Indirection count = keys.size() - 2
    // Entry size implied
    std::vector<std::vector<uint8_t>> keys;
    std::vector<uint8_t> structure;
    uint8_t crc;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
