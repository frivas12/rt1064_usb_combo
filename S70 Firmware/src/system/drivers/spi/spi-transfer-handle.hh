/**
 * \file spi-transfer-handle.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-08-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <span>
#include <mutex>

#include "user_spi.h"
#include "slot_nums.h"

#include "FreeRTOS.h"
#include "semphr.h"

namespace drivers::spi {

class transfer_handle;
class locking_transfer_handle;

/**
 * A thread-safe factory for transfer_handles.
 */
class handle_factory {
  public:
    /// @brief Type flag that indicates that the factory should yield
    ///        the lock to the transfer handle instead of keeping it.
    struct yield_lock_to_handle_t {};
    static constexpr yield_lock_to_handle_t yield_lock_to_handle{};

    /**
     * Creates a transfer handle that begins an SPI transfer.
     * \warning The lifetime of the transfer handle (if created) may not exceed
     *          the lifetime of the factory.
     * If the lock is not owned, this will block until the lock becomes owned.
     * \param[in]       mode The spi mode.
     * \param[in]       toggle If the CS line needs to toggle between byte
     * transfers. \param[in]       chip_select The chip select enumeration to
     * use. \return If the transfer starts, a transfer handle in an optional.
     * \return If the transfer does not start, an empty optional.
     */
    transfer_handle create_handle(spi_modes mode, bool toggle,
                                  uint8_t chip_select);

    /**
     * Creates a transfer handle that begins an SPI transfer and yields the lock
     * to it. Use this is the lifetime of the transfer handle will exceed the
     * lifetime of the factory. If the lock is not owned, this will block until
     * the lock becomes owned. \param[in]       mode The spi mode. \param[in]
     * toggle If the CS line needs to toggle between byte transfers. \param[in]
     * chip_select The chip select enumeration to use. \return If the transfer
     * starts, a transfer handle owning the lock in an optional. \return If the
     * transfer does not start, an empty optional.
     */
    locking_transfer_handle create_handle(yield_lock_to_handle_t,
                                          spi_modes mode, bool toggle,
                                          uint8_t chip_select);

    /// \brief Invokes create_handle.
    transfer_handle operator()(spi_modes mode, bool toggle,
                               uint8_t chip_select);

    /// \brief Invokes create_handle.
    inline locking_transfer_handle operator()(yield_lock_to_handle_t,
                                              spi_modes mode, bool toggle,
                                              uint8_t chip_select);

    inline bool has_lock() const { return _owns_lock; }

    /// \brief Takes the lock if not owned.
    void acquire_lock();

    /// \brief Takes the lock if not owned within the \param timeout.
    /// \return If the factory has the lock.
    bool acquire_lock(TickType_t timeout);

    /// @brief Releases the lock.
    /// \warning No handles may be alive when this is called.
    void release_lock();

    /// \brief Creates a factory and locks the SPI lock.
    handle_factory();

    /// \brief Creates a factory and does not lock the SPI lock.
    explicit handle_factory(std::defer_lock_t);

    /// @brief Creates a factory and takes ownership of the already-owned SPI
    /// lock.
    explicit handle_factory(std::adopt_lock_t);

    /// @brief Creates a factory that may own the lock if the lock was acquired
    /// within the \param lock_timeout (default: no wait).
    explicit handle_factory(std::try_to_lock_t, TickType_t lock_timeout = 0);

    explicit handle_factory(const handle_factory &) = delete;
    explicit handle_factory(handle_factory &&);
    handle_factory &operator=(handle_factory &&) = delete;

    ~handle_factory();

  private:
    bool _owns_lock;
};

/**
 * A handle to the SPI for an SPI channel during a transfer operation.
 * The transfer operation ends at the end of this object's lifetime.
 * This object is not thread safe.
 */
class transfer_handle {
  public:
    void write(std::byte value);
    void write(std::span<const std::byte> buffer);

    std::byte transfer(std::byte value);
    void transfer(std::span<std::byte> buffer);

    explicit transfer_handle(const transfer_handle &) = delete;
    explicit transfer_handle(transfer_handle &&);
    ~transfer_handle();

  protected:
    transfer_handle() = default;

    /// @brief Allows derived classes to end the SPI transfer.
    ///        Useful in derived classes that need to do an action
    ///        after the transfer is released (due to derived
    ///        destructions being called first).
    /// \return If the transfer was ended.
    bool end_transfer_lifetime();

    friend class handle_factory;

  private:
    bool _owns_lifetime = true;
};

/**
 * An RAII type for a transfer handle that wraps a mutex.
 * Takes ownership of the lock for the lifetime of the object.
 */
class locking_transfer_handle : public transfer_handle {
  public:
    explicit locking_transfer_handle(locking_transfer_handle &&other) = default;

    /// @brief Ends the transfer and (if it does) releases the lock.
    ~locking_transfer_handle();

  protected:
    /// \brief Locks the lock and return the handle.
    static locking_transfer_handle create(SemaphoreHandle_t lock);

    /// @brief Takes ownership of the lock (locked by external code, ownership
    /// released to this).
    explicit locking_transfer_handle(SemaphoreHandle_t lock, std::adopt_lock_t);

    friend class handle_factory;

  private:
    SemaphoreHandle_t _lock;
};

constexpr inline uint8_t
slot_to_chip_select(slot_nums slot, bool secondary_cs = false) noexcept {
    return 97 + 2 * static_cast<uint8_t>(slot) + (secondary_cs ? 1 : 0);
}

} // namespace drivers::spi

// EOF
