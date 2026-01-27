// efs-handle.hh

// clang-format off
/**************************************************************************/ /**
* \file efs-handle.hh
* \author Sean Benish
* \brief Handle base class for the embedded file system.
*****************************************************************************/
// clang-format on
#pragma once

#include "FreeRTOS.h"
#include "efs-pods.hh"
#include "spi-transfer-handle.hh"

#include <ranges>
#include <span>

namespace efs {

/*****************************************************************************
 * Defines
 *****************************************************************************/
/**
 * The number of bytes that a handle will read before it quickly yields the
 * SPI semaphore and takes it again.  This reduces the worst-case delay
 * significantly with a minor increase in average read time.
 * When set to 0, this optimization is ignored.
 */
static constexpr std::size_t HANDLE_READ_YIELD_BYTES = 64;

/*****************************************************************************
 * Data Types
 *****************************************************************************/
/**
 * Creates a handle to a file.
 * The handle follows a RAII pattern, where only one handle to a file can
 * be created.
 */
class handle {
  private:
    /// @brief Metadata for the file handle.
    file_metadata _metadata;

    /// @brief Read-only source for the file handle.
    user_e _source;

    /// @brief Set when the handle is valid (points to a valid file).
    bool _open;

  protected:
    /**
     * Creates an invalid handle.
     * \param[in]       source The user that accessed this file.
     */
    handle(const user_e source);

    /**
     * Create a valid handle to a given file.
     * \param[in]       source  The user that accessed this file.
     * \param[in]       r_metadata
     */
    handle(const user_e source, const file_metadata &r_metadata);

    // Needs to be virtual for cache ownership transfer.
    virtual void release_ownership();

  public:
    /**
     * Gets a handle to an already existing file.
     * If the file does not provide the actions specified, the handle will not
     * be created. \warning The end user should verify that the handle can
     * perform the actions desired. \param[in]       id The identifer of the
     * file whose handle should be opened. \param[in]       timeout The amount
     * of time to wait for a file.  If not defined, then wait forever.
     * \param[in]       user The file-system user that is getting the handle.
     * Defaults to FIRMWARE. \return A file handle.  The handle may be invalid
     * (timeout or non-existant file).
     */
    static handle create_handle(const file_identifier_t id,
                                const TickType_t timeout = portMAX_DELAY,
                                const user_e user = FIRMWARE);

    handle(handle &&r_handle);
    handle &operator=(handle &&r_handle);

    handle(const handle &r_handle) = delete;
    handle &operator=(const handle &r_handle) = delete;

    ~handle();

    inline bool is_valid() const { return _open; }

    inline bool can_read() const {
        switch (_source) {
        case user_e::SUPER:
            return true;
        case user_e::FIRMWARE:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_FIRMWARE_READ) > 0;
        case user_e::EXTERNAL:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_EXTERNAL_READ) > 0;
        }
        return false;
    }

    inline bool can_write() const {
        switch (_source) {
        case user_e::SUPER:
            return true;
        case user_e::FIRMWARE:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_FIRMWARE_WRITE) > 0;
        case user_e::EXTERNAL:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_EXTERNAL_WRITE) > 0;
        }
        return false;
    }

    inline bool can_delete() const {
        switch (_source) {
        case user_e::SUPER:
            return true;
        case user_e::FIRMWARE:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_FIRMWARE_DELETE) > 0;
        case user_e::EXTERNAL:
            return (_metadata.attr & FILE_ATTRIBUTES_MASK_EXTERNAL_DELETE) > 0;
        }
        return false;
    }

    inline file_attributes_t get_attr() const { return _metadata.attr; }

    inline user_e get_user() const { return _source; }

    inline file_identifier_t get_id() const { return _metadata.id; }

    inline uint16_t get_page_length() const { return _metadata.length; }

    /**
     * Closes the handle
     */
    void close();

    /**
     * Frees the file from the system if able.
     * If successful, the handle becomes invalid.
     * \return true The file was successfully freed from the file system.
     * \return false The file was not valid or invalid access.
     */
    bool delete_file();

    /**
     * Reads data from the file to a pointer.
     * \param[in]       file_address The address in the file to read data from.
     * \param[out]      p_dest The pointer where data will be written to.
     * \param[in]       bytes_to_read The maximum number of bytes to read from
     *                                the file.
     * \return The number of bytes actually written to the destination
     * pointer.
     */
    inline uint32_t read(const uint32_t file_address, void *const p_dest,
                         const uint32_t bytes_to_read) const {
        drivers::spi::handle_factory factory;
        std::byte *const ptr = reinterpret_cast<std::byte *>(p_dest);
        return read(factory, file_address,
                    std::span<std::byte>(
                        std::ranges::subrange(ptr, ptr + bytes_to_read)));
    }

    /**
     * Reads data from the file to a span.
     * \param[in]       factory The SPI driver factory to use for ownership.
     * \param[in]       file_address The address in the file to read data from.
     * \param[out]      dest The region to write data to.
     * \return The number of bytes actually written in the destination span.
     */
    uint32_t read(drivers::spi::handle_factory &factory,
                  uint32_t file_address, std::span<std::byte> dest) const;

    /**
     * Writes data from a pointer to the file.
     * \param[in]       file_address The address in the file to write data to.
     * \param[out]      p_src The pointer where data will be read from.
     * \param[in]       bytes_to_write The maximum number of bytes to write to
     *                                 the file.
     * \return The number of bytes actually written to the file.
     */
    inline uint32_t write(const uint32_t file_address, const void *const p_src,
                          const uint32_t bytes_to_write) const {
        drivers::spi::handle_factory factory;
        const std::byte *const ptr = reinterpret_cast<const std::byte *>(p_src);
        return write(factory, file_address,
                     std::span<const std::byte>(
                         std::ranges::subrange(ptr, ptr + bytes_to_write)));
    }

    /**
     * Writes data from a span to the file.
     * \param[in]       factory The SPI driver factory to use for ownership.
     * \param[in]       file_address The address in the file to write data to.
     * \param[in]       source The location where the data shall be obtained
     * from. \return The number of bytes actually written to the file.
     */
    uint32_t write(drivers::spi::handle_factory &factory,
                   uint32_t file_address,
                   std::span<const std::byte> src) const;

    // TODO:  Refactor eeprom mapper and create a file memory mapper.
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

} // namespace efs

// EOF
