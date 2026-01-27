/**
 * @file usr_interrupt.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USE_INTERRUPT_USR_INTERRUPT_H_
#define SRC_SYSTEM_DRIVERS_USE_INTERRUPT_USR_INTERRUPT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "slot_nums.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void init_interrupts(void);
void setup_slot_interrupt_CPLD(slot_nums slot);
void setup_slot_interrupt_SMIO(slot_nums slot);
void cpld_slot_1_handler(uint32_t t1, uint32_t t2);
void cpld_slot_2_handler(uint32_t t1, uint32_t t2);
void cpld_slot_3_handler(uint32_t t1, uint32_t t2);
void cpld_slot_4_handler(uint32_t t1, uint32_t t2);
void cpld_slot_5_handler(uint32_t t1, uint32_t t2);
void cpld_slot_6_handler(uint32_t t1, uint32_t t2);
void cpld_slot_7_handler(uint32_t t1, uint32_t t2);
void smio_slot_1_handler(uint32_t t1, uint32_t t2);
void smio_slot_2_handler(uint32_t t1, uint32_t t2);
void smio_slot_3_handler(uint32_t t1, uint32_t t2);
void smio_slot_4_handler(uint32_t t1, uint32_t t2);
void smio_slot_5_handler(uint32_t t1, uint32_t t2);
void smio_slot_6_handler(uint32_t t1, uint32_t t2);
void smio_slot_7_handler(uint32_t t1, uint32_t t2);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USE_INTERRUPT_USR_INTERRUPT_H_ */
