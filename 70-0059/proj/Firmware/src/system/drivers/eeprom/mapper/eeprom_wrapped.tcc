// eeprom_wrapped.tcc

/**************************************************************************//**
 * \file eeprom_wrapped.tcc
 * \author Sean Benish
 * \brief Static wrappers for EepromMapped objects.
 *****************************************************************************/
#pragma once

#include <type_traits>

#include "defs.hh"
#include "eeprom_mapper.hh"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
/**
 * A simple container for a statically-created object and it wrapper.
 * \tparam T The POD object to wrap.
 */
template<typename T>
    requires std::is_trivial_v<T> && std::is_standard_layout_v<T>
class EepromWrapped
{
public:
    T obj;
    EepromMapper mapping_data;

    EepromWrapped(const std::size_t eeprom_address) :
        mapping_data(eeprom_address, sizeof(T), &obj)
    {}
};

/**
 * A simpler container for statically-created POD objects and a slimmed eeprom mapper.
 * \tparam T 
 */
template<typename T>
    requires std::is_trivial_v<T> && std::is_standard_layout_v<T>
class EepromWrappedSlim
{
public:
    T obj;
    EepromMapperSlim mapping_data;

    EepromWrappedSlim (const std::size_t eeprom_address)
    {}
};


/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
