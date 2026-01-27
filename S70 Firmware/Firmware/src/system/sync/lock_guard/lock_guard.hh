// lock_guard.hh

/**************************************************************************//**
 * \file lock_guard.hh
 * \author Sean Benish
 * \brief RAII lock object.
 *
 * Implements an RAII object for semaphore/mutex locking.  The object locks at
 * construction and unlocks when it goes out of scope.
 *****************************************************************************/

#ifndef SRC_SYSTEM_SYNC_LOCK_GUARD_LOCK_GUARD_HH_
#define SRC_SYSTEM_SYNC_LOCK_GUARD_LOCK_GUARD_HH_

#include "FreeRTOS.h"
#include "semphr.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/



/*****************************************************************************
 * Data Types
 *****************************************************************************/

/**
 * RAII-implementation for FreeRTOS semaphores/mutexes.
 * Uses portMAX_DELAY when locking.
 */
class lock_guard
{
private:
    const SemaphoreHandle_t _handle;
public:
    lock_guard(const SemaphoreHandle_t handle);
    lock_guard(const lock_guard&) = delete;
    ~lock_guard();
};

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* SRC_SYSTEM_SYNC_LOCK_GUARD_LOCK_GUARD_HH_ */
//EOF
