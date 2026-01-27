// event_watchdog.hh

/**************************************************************************//**
 * \file event_watchdog.hh
 * \author Sean Benish
 * 
 * A FreeRTOS task-based watchdog, allowing for a supervisor to be
 * aware of possible lockups on tasks.
 * 
 * Event-based watchdogs ensure that tasks that operate in the
 * producer-consumer model finish their tasks within an allotted time.
 * 
 * \note No internal synchronization objects are used, as they are quite heavy.
 * As a result, the object measures the time between the end of the start()
 * call and the end of the stop() call.
 *****************************************************************************/
#pragma once

#include "FreeRTOS.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

class event_watchdog
{
private:
    const TickType_t _TIMEOUT;      ///> The number of ticks before a fault is indicated.
    TickType_t _handler_started;    ///> The timestamp when the handler began.
    bool _handler_running;          ///> If the event handler is running.

public:

    /**
     * Constructs the watchdog.
     * \param[in]       timeout The amount of time (in system ticks) that must elapse before resetting to
     *                          indicate a fault.
     */
    event_watchdog(TickType_t timeout);

    /**
     * Checks if the task meets its criteria.
     * \warning Do NOT use while in an ISR.
     * \return true The task meets its timeout criteria.
     * \return false The task failed its timeout criteria.  Action required.
     */
    bool valid() const;
    bool valid(TickType_t now) const;

    /**
     * Sent to indicate that the handler has begun.
     * \warning Do NOT use while in an ISR.
     */
    void start();

    /**
     * Sent to indicate that the handler has finished.
     * \warning Do NOT use while in an ISR.
     */
    void stop();
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
