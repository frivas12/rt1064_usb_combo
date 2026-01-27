/**
 * @file fifo.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_BUFFERS_FIFO_H_
#define SRC_SYSTEM_DRIVERS_BUFFERS_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <compiler.h>
#include "stdint.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct
{
	uint8_t * buf;
	volatile uint16_t head;
	volatile uint16_t tail;
	volatile uint16_t size;
} fifo_t;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void fifo_reset(fifo_t * f, uint8_t * buf, uint16_t size);
uint8_t fifo_peak(fifo_t * f, uint16_t pos);
int fifo_read(fifo_t * f, uint8_t * buf, uint16_t nbytes);
int fifo_write(fifo_t * f, uint8_t * buf, uint16_t nbytes);
uint16_t fifo_data_available(fifo_t * f);
uint8_t fifo_tail_inc(fifo_t * f, uint8_t num, bool move_tail, uint16_t buff_size);
uint8_t fifo_tail_dec(fifo_t * f, uint8_t num, bool move_tail, uint16_t buff_size);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_BUFFERS_FIFO_H_ */
