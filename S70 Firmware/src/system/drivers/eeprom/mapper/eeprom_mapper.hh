// eeprom_mapper.hh

/**************************************************************************//**
 * \file eeprom_mapper.hh
 * \author Sean Benish
 *
 * An object class that maps a section of memory onto a location in EEPROM.
 *****************************************************************************/
#pragma once

#include "defs.hh"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/


/**
 * A shrunk form of the mapper that only contains the necessary information.
 * Allows external classes to define the memory location, address, and size.
 */
class EepromMapperSlim
{
private:
    bool linked;                    ///> If data has been loaded into memory.
    CRC8_t loaded_crc;              ///> The CRC of the data in EEPROM.
public:
    /**
     * Creates a new slim EEPROM mapper.
     */
    EepromMapperSlim ();

    
    void load (const std::size_t address, const std::size_t length, void * const p_dest);

    void save (const std::size_t address, const std::size_t length, const void * const p_source);

    void save_if_dirty (const std::size_t address, const std::size_t length, const void * const p_source);

    /**
     * Indicates if the data in EEPROM and memory have been linked (loaded from or saved to).
     * \return true The data has been loaded/saved at least once.  Data in memory is a modified version
     *              of data in EEPROM.
     * \return false The data has not been loaded into EEPROM.  Data in EEPROM and memory may not be related.
     */
    bool is_linked() const;

    /**
     * Indicates if the data in memory is different from the memory in EEPROM.
     * \return true 
     * \return false 
     */
    bool is_dirty(const std::size_t length, const void * const p_source) const;

    /**
     * Returns the CRC of the data in EEPROM.
     * \note The CRC is NOT saved in EEPROM.
     * \return CRC8_t The CRC of the data in EEPROM.
     */
    CRC8_t get_eeprom_crc() const;

    /**
     * Calculates the CRC of the data in memory.
     * \return CRC8_t The CRC of the data in memory.
     */
    CRC8_t calculate_crc(const std::size_t length, const void * const p_source, const CRC8_t previous_crc = 0) const;
};

/**
 * A class containing metadata for mapping memory into EEPROM.
 */
class EepromMapper : private EepromMapperSlim
{
private:
    const std::size_t ADDRESS;      ///> Address on the EEPROM that the memory should map to.
    const std::size_t SIZE;         ///> Size of the object in EEPROM (and in memory).
    void * const P_MEMORY;          ///> Pointer to the memory mapped to EEPROM.

public:

    /**
     * Constructs an object at a given location in memory and EEPROM.
     * Use the templated factory constuctor if possible.
     * \param[in]       eeprom_address The starting address of the data in EEPROM.
     * \param[in]       memory_length The length (in bytes) of the data in EEPROM AND in memory.
     * \param[in]       p_memory A pointer to the memory that should be mapped.
     */
    EepromMapper(const std::size_t eeprom_address, const std::size_t memory_length, void * const p_memory);

    /**
     * Templated factory constructor to EEPROM map an structure.
     * \tparam T The structure to EEPROM map.  The object should not be a POD object.
     * \param[in]       address 
     * \param[in]       p_memory 
     * \return EepromMapper A mapper.
     */
    template<typename T>
    inline static EepromMapper Create(const std::size_t address, T * const p_memory)
    {
        return EepromMapper(address, sizeof(T), p_memory);
    }

    /**
     * Loads data from EEPROM into memory.
     */
    void load();

    /**
     * Saves data from memory into EEPROM.
     */
    void save();

    /**
     * Saves data from memory into EEPROM only if it was dirty.
     */
    void save_if_dirty();

    /**
     * Indicates if the data in EEPROM and memory have been linked (loaded from or saved to).
     * \return true The data has been loaded/saved at least once.  Data in memory is a modified version
     *              of data in EEPROM.
     * \return false The data has not been loaded into EEPROM.  Data in EEPROM and memory may not be related.
     */
    bool is_linked() const;

    /**
     * Indicates if the data in memory is different from the memory in EEPROM.
     * \return true 
     * \return false 
     */
    bool is_dirty() const;

    /**
     * Returns the CRC of the data in EEPROM.
     * \note The CRC is NOT saved in EEPROM.
     * \return CRC8_t The CRC of the data in EEPROM.
     */
    CRC8_t get_eeprom_crc() const;

    /**
     * Calculates the CRC of the data in memory.
     * \param[in]       previous_crc The crc to start with.  Optional.
     * \return CRC8_t The CRC of the data in memory.
     */
    CRC8_t calculate_crc(const CRC8_t previous_crc = 0) const;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
