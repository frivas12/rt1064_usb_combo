// eeprom_mapper.cc

/**************************************************************************//**
 * \file eeprom_mapper.cc
 * \author Sean Benish
 *****************************************************************************/

#include "eeprom_mapper.hh"

#include "25lc1024.h"

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

EepromMapperSlim::EepromMapperSlim() : linked(false)
{}

// MARK:  SPI Mutex Required
void EepromMapperSlim::load(const std::size_t address, const std::size_t length, void * const p_dest)
{
    // Load data from EEPROM.
    eeprom_25LC1024_read(
        static_cast<const uint32_t>(address),
        static_cast<const uint32_t>(length),
        static_cast<uint8_t*>(p_dest)
    );

    // Update the CRC and mark as linked.
    loaded_crc = calculate_crc(length, p_dest);
    linked = true;
}

// MARK:  SPI Mutex Required
void EepromMapperSlim::save(const std::size_t address, const std::size_t length, const void * const p_source)
{
    // Update the CRC and mark as linked.
    loaded_crc = calculate_crc(length, p_source);
    linked = true;

    // Save data to EEPROM.
    eeprom_25LC1024_write(
        static_cast<const uint32_t>(address),
        static_cast<const uint32_t>(length),
        static_cast<const uint8_t*>(p_source)
    );
}

// MARK:  SPI Mutex Required
void EepromMapperSlim::save_if_dirty(const std::size_t address, const std::size_t length, const void * const p_source)
{
    CRC8_t calced = calculate_crc(length, p_source);
    if (calced != loaded_crc)
    {
        loaded_crc = calced;
        linked = true;

        eeprom_25LC1024_write(
            static_cast<const uint32_t>(address),
            static_cast<const uint32_t>(length),
            static_cast<const uint8_t*>(p_source)
        );
    }
}

bool EepromMapperSlim::is_linked() const
{
    return linked;
}

bool EepromMapperSlim::is_dirty(const std::size_t length, const void * const p_source) const
{
    return calculate_crc(length, p_source) != loaded_crc;
}

CRC8_t EepromMapperSlim::get_eeprom_crc() const
{
    return loaded_crc;
}

CRC8_t EepromMapperSlim::calculate_crc(const std::size_t length, const void * const p_source, const CRC8_t previous_crc) const
{
    const char * const p_mem = static_cast<const char*>(p_source);

    return CRC_split(p_mem, static_cast<uint32_t>(length), previous_crc);
}



EepromMapper::EepromMapper(const std::size_t eeprom_address, const std::size_t memory_length,
    void * const p_memory) : ADDRESS(eeprom_address), SIZE(memory_length), 
    P_MEMORY(p_memory)
{}

void EepromMapper::load()
{
    EepromMapperSlim::load(ADDRESS, SIZE, P_MEMORY);
}

void EepromMapper::save()
{
    EepromMapperSlim::save(ADDRESS, SIZE, P_MEMORY);
}

void EepromMapper::save_if_dirty()
{
    EepromMapperSlim::save_if_dirty(ADDRESS, SIZE, P_MEMORY);
}

bool EepromMapper::is_linked() const
{
    return EepromMapperSlim::is_linked();
}

bool EepromMapper::is_dirty() const
{
    return EepromMapperSlim::is_dirty(SIZE, P_MEMORY);
}

CRC8_t EepromMapper::get_eeprom_crc() const
{
    return EepromMapperSlim::get_eeprom_crc();
}

CRC8_t EepromMapper::calculate_crc(const CRC8_t previous_crc) const
{
    return EepromMapperSlim::calculate_crc(SIZE, P_MEMORY, previous_crc);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
