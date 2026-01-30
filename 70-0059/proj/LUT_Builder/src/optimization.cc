// optimization.cc

/**************************************************************************//**
 * \file optimization.cc
 * \author Sean Benish
 *****************************************************************************/
#include "lut-builder/optimization.hh"

#include "lut-builder/intermediate-formats.hh"

using namespace optimization;

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

lut_header::lut_header(const std::size_t key_size, const std::size_t key_count)
    : _key_size(key_size), _key_count(key_count)
{}

std::size_t lut_header::min_size() const
{
    return 3*_key_size + 2*sizeof(uint16_t);
}

std::size_t lut_header::max_size() const
{
    return _key_size + (sizeof(uint16_t) + _key_size)*(_key_count + 1);
}

uint16_t lut_header::get_page_count(const uint16_t lut_page_size) const
{
    const std::size_t STATIC_OFFSET = _key_size;
    const std::size_t HEADER_ENTRY_SIZE = sizeof(uint16_t) + _key_size;
    const std::size_t ENTRIES_ON_FIRST_HEADER_PAGE = (lut_page_size - STATIC_OFFSET) / HEADER_ENTRY_SIZE;
    const std::size_t ENTIRES_ON_OTHER_HEADER_PAGE = lut_page_size / HEADER_ENTRY_SIZE;
    return static_cast<uint16_t>(((_key_count + 1) <= ENTRIES_ON_FIRST_HEADER_PAGE) ? 1 : (
        (_key_count  - ENTRIES_ON_FIRST_HEADER_PAGE + ENTIRES_ON_OTHER_HEADER_PAGE) / ENTIRES_ON_OTHER_HEADER_PAGE));
}


lut_entries::lut_entries(const std::size_t entry_size, const std::size_t entry_count)
    : _entry_size(entry_size), _count(entry_count)
{}

std::size_t lut_entries::min_size() const
{
    return _entry_size;
}

std::size_t lut_entries::max_size() const
{
    return _entry_size * _count;
}

uint16_t lut_entries::get_page_count(const uint16_t lut_page_size) const
{
    const std::size_t ENTIRES_PER_PAGE = lut_page_size / _entry_size;
    return static_cast<uint16_t>((_count + ENTIRES_PER_PAGE - 1) / ENTIRES_PER_PAGE);
}


global_lut_header::global_lut_header(const std::size_t key_count)
    : _keys(key_count)
{}

std::size_t global_lut_header::min_size() const
{
    return sizeof(LUT_Header::lut_version) + sizeof(LUT_Header::lut_page_size)
        + sizeof(LUT_Header::indirection_count) + sizeof(uint8_t)*_keys;
}

std::size_t global_lut_header::max_size() const
{
    return min_size();
}

uint16_t global_lut_header::get_page_count(const uint16_t lut_page_size) const
{
    // ! \warning This must be true.
    return 1;
}


/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
