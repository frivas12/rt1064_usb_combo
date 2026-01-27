/* supervisor.h*/

#ifndef SRC_SYSTEM_DRIVERS_SUPERVISOR_SUPERVISOR_H_
#define SRC_SYSTEM_DRIVERS_SUPERVISOR_SUPERVISOR_H_

#ifdef __cplusplus

#include "heartbeat_watchdog.hh"
#include "event_watchdog.hh"
#include "slot_nums.h"

#endif

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern bool global_500ms_tick;
extern uint8_t fade_val;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
void supervisor_init(const bool cpld_programmed);

/**
 * Causes the power LED and the LED of any connected controller to blink for a
 * few seconds.
 * \param[in]       slot If provided, the LEDs assocated with the slot will also identify.
 */
void supervisor_identify(const slot_nums slot);

#ifdef __cplusplus
}

void supervisor_add_watchdog(event_watchdog * const p_watchdog);
void supervisor_add_watchdog(heartbeat_watchdog * const p_watchdog);

#endif

#endif /* SRC_SYSTEM_DRIVERS_SUPERVISOR_SUPERVISOR_H_ */
