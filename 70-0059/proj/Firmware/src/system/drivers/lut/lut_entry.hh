// lut_entry.hh

/**************************************************************************//**
 * \file lut_entry.hh
 * \author Sean Benish
 * \brief Data type for LUT entires.
 *****************************************************************************/
#pragma once

#include <stdint.h>
#include <cstring>

template<typename DISCRIMINATOR_KEY, typename PAYLOAD_TYPE, typename CRC_TYPE>
struct __attribute__((packed)) LUTEntry
{
    DISCRIMINATOR_KEY identifier;
    PAYLOAD_TYPE payload;
    CRC_TYPE crc;

    LUTEntry(const uint16_t page, const char * const p_data);
};

template<typename DISCRIMINATOR_KEY, typename PAYLOAD_TYPE, typename CRC_TYPE>
LUTEntry<DISCRIMINATOR_KEY, PAYLOAD_TYPE, CRC_TYPE>::LUTEntry(const uint16_t page, const char * const p_data)
{
    void(page); // Needed to allow for abstraction with LUT.
    memcpy(this, p_data, sizeof(LUTEntry<DISCRIMINATOR_KEY, PAYLOAD_TYPE, CRC_TYPE>));
}

//EOF
