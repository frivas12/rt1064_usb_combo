/**
 * @file ping_pong.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_BUFFERS_PING_PONG_H_
#define SRC_SYSTEM_DRIVERS_BUFFERS_PING_PONG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "ftdi.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
#define PARSING_BUFFER			2

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void swap_buffers(fifo_t * f_interrupt, uint8_t * interrupt_buf_sel, uint8_t * task_buf_sel);


#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_BUFFERS_PING_PONG_H_ */
