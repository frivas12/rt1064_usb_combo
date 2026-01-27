// save_constructor.hh

/**************************************************************************//**
 * \file save_constructor.hh
 * \author Sean Benish
 * \brief Driver for the save constructor system.
 *
 * Handles the instatiaton of configurations for slot card devices.
 *****************************************************************************/

#pragma once

#include "save_size.hh"

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
     * Calculates the CRC for the provided signature and save information.
     * \param[in]       saved_signature The signature of the device whose data is saved.
     * \param[in]       p_save The entirety of the data saved for the device's configuration.
     * \param[in]       save_size The amount (in bytes) of data saved for the device.
     * \return The CRC of the saved data that will pass validation checks.
     */
    CRC8_t validate(const device_signature_t saved_signature, const void * const p_save,
        uint32_t save_size);

    /**
     * Checks if the CRC saved into storage equals the CRC calculated.
     * Can be used to verify that a save structure has valid data before loading values into memory.
     * \param[in]       saved_signature The signature of the device whose data is saved.
     * \param[in]       p_save The entirety of the data saved for the device's configuration.
     * \param[in]       save_size The amount (in bytes) of data saved for the device.
     * \param[in]       saved_crc The CRC saved in storage.
     * \return true The signature and save data match the CRC that was saved.
     * \return false The signature and/or save data mismatched with the CRC.  Corruption may have occurred.
     */
    inline bool check_valid(const device_signature_t saved_signature, const void * const p_save,
        uint32_t save_size, const CRC8_t saved_crc)
    {
        return save_constructor::validate(saved_signature, p_save, save_size) == saved_crc;
    }

    /**
     * Searches the custom entries and config look-up table for a serialized config
     * that matches the signature provided.
     * \pre buffer must have enough space to hold the serialized config.
     *      See \ref{get_config_size} for checking this constraint.
     * \return The number of bytes copied into the buffer.
     *         0 when no signature was found.
     */
    uint32_t construct_single(const config_signature_t &signature,
                                                void *const buffer,
                                                void *const custom_entries,
                                                uint16_t custom_entries_size);

    inline uint32_t construct_single(
        const config_signature_t &signature,
        void *const buffer)
    { return construct_single(signature, buffer, nullptr, 0); }


    /**
     * Provided a buffer to hold all of the signatures, this will pack all of the configuration
     * structures into the provided buffer, with the first config signature provided being the 
     * first to be loaded into the structure.
     * \warning p_buffer must have enough space to hold all of the signature data.
     * \warning Some structures have more config signatures than config structures in their save structure,
     * as they may split up their save structure in terms of read/write frequency.
     * \param[in]       p_config_signatures Array of configuration signatures to load.
     * \param[in]       signature_count Amount of configuration signatures in the array.
     * \param[out]      p_buffer Buffer where the configuration structure instances will be instantiated.
     * \return true All of the configuration structures were initialized.
     * \return false At least one configuration structures failed to be initilaized.
     */
    bool construct(const config_signature_t * p_config_signatures, uint16_t signature_count,
        void * const p_buffer, void * const p_custom_entires, const uint16_t custom_entires_length);

    inline bool construct(const config_signature_t * p_config_signatures, uint16_t signature_count,
        void * const p_buffer)
    {
        return construct(p_config_signatures, signature_count, p_buffer, nullptr, 0);
    }

}

//EOF
