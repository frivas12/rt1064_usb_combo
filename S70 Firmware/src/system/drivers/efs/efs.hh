// efs.hh

/**************************************************************************//**
 * \file efs.hh
 * \author Sean Benish
 * \brief Interface for the embedded file system.
 *
 * The embedded file system is a simple, compact way of storing data that
 * allows for scalable EEPROM mappings.
 *****************************************************************************/
#pragma once

#include "efs-handle.hh"

#include "FreeRTOS.h"

namespace efs
{

/*****************************************************************************
 * Defines
 *****************************************************************************/


// The number of files whose metadata should be cached.
// A file should be cached
#define EFS_FIRMWARE_CACHE_SIZE     

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct efs_header
{
    char identifier[3];
    uint8_t version;
    uint16_t page_size;
    uint16_t page_count;
    uint32_t eeprom_start_address;
    uint16_t header_pages;
    uint16_t _reserved14;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/
/**
 * The number of file metatdata that should be cached for the external user.
 * This should be at least 1 to speed up programming of files.
 */
constexpr std::size_t EFS_EXTERNAL_CACHE_SIZE = 2;



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * Initializes the system.
 * \note Must be initialized after the storage driver.
 */
void init();

/**
 * Marks all files and data pages on the EFS as free.
 * This will induce a lockdown on the file system until this is complete.
 * \warning This has SUPER user access.
 */
bool erase();

/**
 * Creates a file in the file system if it does not exist.
 * \param[in]       id The identifier of the file that should be created.
 * \param[in]       pages_to_occupy The number of EFS pages that the file should span.
 * \param[in]       attributes The attributes of the file.
 * \param[in]       timeout The amount of time to wait for a file.  If not defined, then wait forever.
 * \return true The file was created.
 * \return false The file was not created.
 */
bool create_file(
    const file_identifier_t id,
    const uint16_t pages_to_occupy,
    const file_attributes_t attributes = DEFAULT_ATTRIBUTES,
    const TickType_t timeout = portMAX_DELAY
);

/**
 * Checks to see if a file exists in the file system.
 * \note This is inefficient so use it sparingly.
 * \param[in]       id The identifier of the file to look for.
 * \param[in]       timeout The amount of time to wait for a file.  If not defined, then wait forever.
 * \return true The file was found within the given timeout.
 * \return false The files does not exists or the timeout was reached.
 */
bool does_file_exist(
    const file_identifier_t id,
    const TickType_t timeout = portMAX_DELAY
);

/**
 * Gets a handle to an already existing file.
 * If the file does not provide the actions specified, the handle will not be created.
 * \warning The end user should verify that the handle can perform the actions desired.
 * \param[in]       id The identifer of the file whose handle should be opened.
 * \param[in]       timeout The amount of time to wait for a file.  If not defined, then wait forever.
 * \param[in]       user The file-system user that is getting the handle.  Defaults to FIRMWARE.
 * \return A file handle.  The handle may be invalid (timeout or non-existant file).
 */
inline handle get_handle(
    const file_identifier_t id,
    const TickType_t timeout = portMAX_DELAY,
    const user_e user = FIRMWARE
) {
    return handle::create_handle(id, timeout, user);
}



/**
 * \return The maximum amount of files that the file system can allocate.
 */
uint16_t get_maximum_files();

/**
 * \return The number of files that can be allocated.
 */
uint16_t get_free_files();

/**
 * \return The number of pages that can be allocated by new files.
 */
uint16_t get_free_pages();

/**
 * \return Information about the EFS.
 */
efs_header get_header_info();

/**
 * Adds the passed identifier to the external cache.
 * \param[in]       id_to_cache The file identifier to be added to the user cache.
 * \param[in]       timeout The amount of time to wait for a file.  If not defined, then wait forever.
 */
void add_to_external_cache(const file_identifier_t id_to_cache, const TickType_t timeout);

// Testing-Specific functions to expose underlying behavior.
#ifdef _TESTING

file_metadata get_file_metadata(const file_identifier_t id);

#endif

}

//EOF
