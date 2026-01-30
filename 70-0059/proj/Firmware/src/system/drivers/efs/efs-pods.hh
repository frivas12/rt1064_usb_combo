// efs-pods.hh

/**************************************************************************//**
 * \file efs-pods.hh
 * \author Sean Benish
 * \brief Plain Old Data (POD) data type for the embedded file system (EFS).
 *
 * 
 *****************************************************************************/
#pragma once

#include <cstdint>

#include "user-eeprom.hh"

namespace efs
{

/*****************************************************************************
 * Defines
 *****************************************************************************/
enum file_types_e : uint8_t
{
    LUT = 0b011,
    // Undefined
    USER = 0b110
};

enum user_e : uint8_t
{
    SUPER = 0x00,
    FIRMWARE = 0x01,
    EXTERNAL = 0x80
};

/*****************************************************************************
 * Data Types
 *****************************************************************************/
typedef uint8_t file_attributes_t;
constexpr uint8_t FILE_ATTRIBUTES_MASK_EXTERNAL_READ    = 0b00000001;
constexpr uint8_t FILE_ATTRIBUTES_MASK_EXTERNAL_WRITE   = 0b00000010;
constexpr uint8_t FILE_ATTRIBUTES_MASK_EXTERNAL_DELETE  = 0b00000100;
constexpr uint8_t FILE_ATTRIBUTES_MASK_FIRMWARE_READ    = 0b00001000;
constexpr uint8_t FILE_ATTRIBUTES_MASK_FIRMWARE_WRITE   = 0b00010000;
constexpr uint8_t FILE_ATTRIBUTES_MASK_FIRMWARE_DELETE  = 0b00100000;
// Unused bit
constexpr uint8_t FILE_ATTRIBUTES_MASK_NOT_ALLOCATED    = 0b10000000;

constexpr file_attributes_t FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS =
    FILE_ATTRIBUTES_MASK_EXTERNAL_READ |
    FILE_ATTRIBUTES_MASK_EXTERNAL_WRITE |
    FILE_ATTRIBUTES_MASK_EXTERNAL_DELETE;
constexpr file_attributes_t FILE_ATTRIBUTES_FIRMWARE_FULL_ACCESS =
    FILE_ATTRIBUTES_MASK_FIRMWARE_READ |
    FILE_ATTRIBUTES_MASK_FIRMWARE_WRITE |
    FILE_ATTRIBUTES_MASK_FIRMWARE_DELETE;
constexpr file_attributes_t DEFAULT_ATTRIBUTES = 
    FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS |
    FILE_ATTRIBUTES_FIRMWARE_FULL_ACCESS;

typedef uint8_t file_identifier_t;
constexpr uint8_t FILE_IDENTIFIER_MASK_TYPE             = 0b11100000;
constexpr uint8_t FILE_IDENTIFIER_MASK_ID               = 0b00011111;

// 32-bit aligned
struct file_metadata
{
    file_identifier_t id;
    file_attributes_t attr;

    /// @brief The starting page of the file in the EFS.
    uint16_t        start;

    /// @brief The number of pages that the file occupies.
    uint16_t        length;

    constexpr drivers::eeprom::address_span to_address_span() const
    {
        const uint32_t HEAD =  EFS_EEPROM_START +
            start * EFS_PAGE_SIZE;
        return drivers::eeprom::address_span
        {
            .head_address = HEAD,
            .tail_address = HEAD + length * EFS_PAGE_SIZE,
        };
    }
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
constexpr file_identifier_t create_id(const uint8_t id, const file_types_e type)
{
    return static_cast<file_identifier_t>((type << 5) | id);
}

}

//EOF
