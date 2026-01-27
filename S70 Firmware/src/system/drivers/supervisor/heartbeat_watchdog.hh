// heartbeat_watchdog.hh

/**************************************************************************//**
 * \file heartbeat_watchdog.hh
 * \author Sean Benish
 *
 * A FreeRTOS task-based watchdog, allowing for a supervisor to be
 * aware of possible lockups on tasks.
 * 
 * Heartbeat watchdogs ensure that tasks that operate in a period mannor
 * loop within a given interval.
 * 
 * \note No internal synchronization objects are used, as they are quite heavy.
 * As a result, the time is measured between the end of one call to beat() and
 * the next.
 *****************************************************************************/
#pragma once

#include "FreeRTOS.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

class heartbeat_watchdog
{
private:
    TickType_t _TIMEOUT;
    TickType_t _last_heartbeat;

public:

    /**
     * Constructs a heartbeat watchdog.
     * \param[in]       heartbeat_internal The maximum amount of time (in system
     *                  ticks) that can elapse before a fault is indicated.
     */
    heartbeat_watchdog(TickType_t heartbeat_interval);

    /**
     * Checks if the task meets its criteria.
     * \warning Do NOT use while in an ISR.
     * \return true The task meets its interval criteria.
     * \return false The task failed its interval criteria.  Action required.
     */
    bool valid() const;
    bool valid(TickType_t now) const;

    /**
     * Changes the allowed heartbeat interval of the watchdog.  Useful to bound
     * configuration steps that may take longer.
     * \param[in]       heartbeat_interval The new heartbeat interval to use.
     */
    void set_heartbeat_interval(TickType_t heartbeat_interval);

    /**
     * Sent by the task to indicate that it is still alive.
     */
    void beat();

};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
