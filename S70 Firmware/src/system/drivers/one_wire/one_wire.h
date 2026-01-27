/**
 * @file one_wire.h
 *
 * @brief Functions for DS24B33
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ONE_WIRE_ONE_WIRE_H_
#define SRC_SYSTEM_DRIVERS_ONE_WIRE_ONE_WIRE_H_

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
void ow_byte_wr(uint8_t b);
uint8_t ow_byte_rd(bool * const p_timed_out);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ONE_WIRE_ONE_WIRE_H_ */
