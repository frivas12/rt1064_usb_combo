/**
 * \file spi-transfer-handle.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-09-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "spi-transfer-handle.hh"

using namespace drivers::spi;

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
transfer_handle handle_factory::create_handle(spi_modes mode, bool toggle,
                                              uint8_t chip_select) {
    acquire_lock();

    if (spi_start_transfer(mode, toggle, chip_select) != SPI_OK) {
        for (;;)
            ;
    }

    return transfer_handle{};
}

locking_transfer_handle handle_factory::create_handle(yield_lock_to_handle_t,
                                                      spi_modes mode,
                                                      bool toggle,
                                                      uint8_t chip_select) {
    acquire_lock();

    if (spi_start_transfer(mode, toggle, chip_select) != SPI_OK) {
        for (;;)
            ;
    }

    _owns_lock = false;
    return locking_transfer_handle(xSPI_Semaphore, std::adopt_lock);
}

transfer_handle handle_factory::operator()(spi_modes mode, bool toggle,
                                           uint8_t chip_select) {
    return create_handle(mode, toggle, chip_select);
}

locking_transfer_handle handle_factory::operator()(yield_lock_to_handle_t,
                                                   spi_modes mode, bool toggle,
                                                   uint8_t chip_select) {
    return create_handle(yield_lock_to_handle, mode, toggle, chip_select);
}

void handle_factory::acquire_lock() {
    if (has_lock()) {
        return;
    }

    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
    _owns_lock = true;
}
bool handle_factory::acquire_lock(TickType_t timeout) {
    if (!has_lock()) {
        _owns_lock = xSemaphoreTake(xSPI_Semaphore, timeout) == pdTRUE;
    }

    return has_lock();
}
void handle_factory::release_lock() {
    if (!has_lock()) {
        return;
    }

    xSemaphoreGive(xSPI_Semaphore);
}

handle_factory::handle_factory() : _owns_lock(true) {
    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
}

handle_factory::handle_factory(std::defer_lock_t) : _owns_lock(false) {}

handle_factory::handle_factory(std::adopt_lock_t) : _owns_lock(true) {}

handle_factory::handle_factory(std::try_to_lock_t, TickType_t lock_timeout)
    : _owns_lock(xSemaphoreTake(xSPI_Semaphore, lock_timeout) == pdTRUE) {}

handle_factory::handle_factory(handle_factory &&other)
    : _owns_lock(other._owns_lock) {
    other._owns_lock = false;
}

handle_factory::~handle_factory() { release_lock(); }

void transfer_handle::write(std::byte value) {
    spi_partial_write_array(reinterpret_cast<const uint8_t *>(&value), 1);
}
void transfer_handle::write(std::span<const std::byte> buffer) {
    spi_partial_write_array(reinterpret_cast<const uint8_t *>(buffer.data()),
                      buffer.size());
}

std::byte transfer_handle::transfer(std::byte value) {
    spi_partial_transfer_array(reinterpret_cast<uint8_t *>(&value), 1);
    return value;
}
void transfer_handle::transfer(std::span<std::byte> buffer) {
    spi_partial_transfer_array(reinterpret_cast<uint8_t *>(buffer.data()),
                         buffer.size());
}

transfer_handle::transfer_handle(transfer_handle &&other)
    : _owns_lifetime(other._owns_lifetime) {
    other._owns_lifetime = false;
}

transfer_handle::~transfer_handle() { end_transfer_lifetime(); }

locking_transfer_handle::~locking_transfer_handle() {
    if (end_transfer_lifetime()) {
        xSemaphoreGive(_lock);
    }
}
/*****************************************************************************
 * Private Functions
 *****************************************************************************/
bool transfer_handle::end_transfer_lifetime() {
    if (!_owns_lifetime) {
        return false;
    }

    spi_end_transfer();
    _owns_lifetime = false;
    return true;
}

locking_transfer_handle
locking_transfer_handle::create(SemaphoreHandle_t lock) {
    xSemaphoreTake(lock, portMAX_DELAY);
    return locking_transfer_handle(lock, std::adopt_lock);
}

locking_transfer_handle::locking_transfer_handle(SemaphoreHandle_t lock,
                                                 std::adopt_lock_t)
    : _lock(lock) {}

// EOF
