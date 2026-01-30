// lut_manager.hh

#ifndef _SRC_SYSTEM_DRIVERS_LUT_LUT_MANAGER_HH_
#define _SRC_SYSTEM_DRIVERS_LUT_LUT_MANAGER_HH_

/**************************************************************************//**
 * \file lut_manager.hh
 * \author Sean Benish
 * \brief Driver for the Look-Up Table (LUT) manager.
 *
 * Handles the allocation, deallocation, and reallocation of LUTs in a system.
 *****************************************************************************/

#include "defs.h"
#include "lut_types.hh"

namespace lut_manager
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

    constexpr uint16_t MAXIMUM_PROGRAMMING_PAYLOAD_SIZE = 64;

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

    /**
     * Sets the LUT manager's file lock on the LUT files.
     * Locked file prevent the user from accessing them.
     * \param[in]       lock When set, the LUT files will be inaccessible unless the LUT manager is used.
     *                       When reset, then the default behavior in the EFS is used.
     */
    void set_lock(const bool lock);

    /**
     * \return true The LUTs are locked from access.  They must be read from the LUT manager.
     * \return false The LUT are unlocked, so other users may manipulate the files.
     */
    bool get_lock();

    /**
     * Attempts to load data from the LUT and into the save pointer based on the signature provided.
     * \warning p_signature must match the LUT signature of identified table.
     * \warning p_save must have enough space to hold the data specified by the signature.
     * \param[in]       lut The identifier for the LUT to load data from.
     * \param[in]       p_key Pointer to the memory that stored the specified LUT's key.
     * \param[out]      p_save Pointer to the memory to save any loaded data into.
     * \return OKAY on succcess.  Otherwise, a specific error.
     */
    LUT_ERROR load_data (const LUT_ID lut, const void * const p_key, void * const p_save);

    void init();

    typedef config_signature_t config_lut_key_t;
    typedef struct __attribute__((packed))
    {
        slot_types running_slot_card;
        device_signature_t connected_device;
    } device_lut_key_t;

}

#endif

//EOF
