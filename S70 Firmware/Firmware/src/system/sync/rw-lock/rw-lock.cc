#include "./rw-lock.hh"

#include "FreeRTOSConfig.h"
#include "lock_guard.hh"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"

using namespace sync;
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
rw_lock::reader_reference::reader_reference(rw_lock& parent)
    : _parent(parent) {}

rw_lock::writer_reference::writer_reference(rw_lock& parent)
    : _parent(parent) {}

rw_lock::rw_lock()
    : _write_lock(xSemaphoreCreateMutex())
    , _read_lock(xSemaphoreCreateMutex())
    , _readers(0) {
    configASSERT(_write_lock);
    configASSERT(_read_lock);
}

rw_lock::~rw_lock() {
    vSemaphoreDelete(_write_lock);
    vSemaphoreDelete(_read_lock);
}

bool rw_lock::try_lock_reader(TickType_t timeout) {
    if (pdTRUE != xSemaphoreTake(_read_lock, timeout)) {
        return false;
    }
    if (++_readers == 1) {
        if (pdTRUE != xSemaphoreTake(_write_lock, timeout)) {
            --_readers;
            xSemaphoreGive(_read_lock);
            return false;
        }
        xSemaphoreGive(_read_lock);
    }

    return true;
}

bool rw_lock::try_lock_writer(TickType_t timeout) {
    return xSemaphoreTake(_write_lock, portMAX_DELAY) == pdTRUE;
}

void rw_lock::lock_reader() {
    auto _ = lock_guard(_read_lock);
    if (++_readers == 1) {
        xSemaphoreTake(_write_lock, portMAX_DELAY);
    }
}

void rw_lock::lock_writer() { xSemaphoreTake(_write_lock, portMAX_DELAY); }

void rw_lock::unlock_reader() {
    auto _ = lock_guard(_read_lock);
    if (--_readers == 0) {
        xSemaphoreGive(_write_lock);
    }
}

void rw_lock::unlock_writer() { xSemaphoreGive(_write_lock); }

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
