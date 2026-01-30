// save_constructor.hh

/**************************************************************************//**
 * \file save_constructor.hh
 * \author Sean Benish
 *
 * Stub for save size, used in the compiler.
 *****************************************************************************/

#pragma once

#include "defs.h"
#include "structure_id.hh"

namespace save_constructor
{

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
    /**
     * Gets the number of sub devices that a slot card type will use.
     * \param[in]       slot_type The slot card's type.
     * \return The number of subdevices for a slot card.
     */
    uint8_t get_subdevice_count(const slot_types slot_type);

    /**
     * Gets the size of the configuration structure (in bytes).
     * \param[in]       id The structure ID for the configuration structure.
     * \return The amount of bytes that the configuration structure takes up.
     * 0 for invalid.
     */
    uint32_t get_config_size(const struct_id_t id);

    /**
     * Gets the size of the total number of configurations for a given slot type (in bytes).
     * \param[in]       type The slot type.
     * \return The amount of bytes that the slot type's configuration array takes up.
     * 0 for invalid.
     */
    uint32_t get_slot_save_size(const slot_types type);

}

//EOF
