/**
 * @file fifo.c
 *
 * @brief Functions for ???
 *
 */
#include "../buffers/fifo.h"

#include <asf.h>
#include "string.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/*This initializes the FIFO structure with the given buffer and size*/
void fifo_reset(fifo_t * f, uint8_t * buf, uint16_t size)
{
	f->head = 0;
	f->tail = 0;
	f->size = size;
	memset(f->buf, '\0', size);
//	f->buf = buf;
}

/*Reads a byte at position pos but does not move the tail*/
uint8_t fifo_peak(fifo_t * f, uint16_t pos)
{
	return f->buf[pos];
}

/*This reads nbytes bytes from the FIFO The number of bytes read is returned*/
int fifo_read(fifo_t * f, uint8_t * buf, uint16_t nbytes)
{
//	irqflags_t flags;
//	flags = cpu_irq_save();

	int i;
	uint16_t y = 0;
	for (i = 0; i < nbytes; i++)
	{
		if (f->tail != f->head)
		{ //see if any data is available
			buf[y] = f->buf[f->tail];  //grab a byte from the buffer
			y++;
			f->tail++;  //increment the tail
			if (f->tail == f->size)
			{  //check for wrap-around
				f->tail = 0;
			}
		}
		else
		{
//			cpu_irq_restore(flags);
			return i; //number of bytes read
		}
	}
//	cpu_irq_restore(flags);
	return nbytes;
}

/*This writes up to nbytes bytes to the FIFO. If the head runs in to the tail, not
 * all bytes are written. The number of bytes written is returned*/
int fifo_write(fifo_t * f, uint8_t * buf, uint16_t nbytes)
{
//	irqflags_t flags;
//	flags = cpu_irq_save();

	int i;
	uint8_t * p;
	p = buf;
	for (i = 0; i < nbytes; i++)
	{
		//first check to see if there is space in the buffer
		if ((f->head + 1 == f->tail)
				|| ((f->head + 1 == f->size) && (f->tail == 0)))
		{
//			cpu_irq_restore(flags);
			return i; //no more room
		}
		else
		{
			f->buf[f->head] = *p++;
			f->head++;  //increment the head
			if (f->head == f->size)
			{  //check for wrap-around
				f->head = 0;
			}
		}
	}
//	cpu_irq_restore(flags);
	return nbytes;
}

uint16_t fifo_data_available(fifo_t * f)
{
//	irqflags_t flags;
//	flags = cpu_irq_save();

	if (f->head >= f->tail)
	{
//		cpu_irq_restore(flags);
		return (f->head - f->tail);
	}
	else
	{
//		cpu_irq_restore(flags);
		return (f->size - f->tail + f->head);
	}
}

uint8_t fifo_tail_inc(fifo_t * f, uint8_t num, bool move_tail, uint16_t buff_size)
{
	uint16_t temp_tail = f->tail;

	temp_tail += num;
	if (temp_tail >= buff_size)
		temp_tail -= buff_size;

	if (move_tail)
	{
		f->tail = temp_tail;
	}
	return temp_tail;
}

uint8_t fifo_tail_dec(fifo_t * f, uint8_t num, bool move_tail, uint16_t buff_size)
{
	uint16_t temp_tail = f->tail;

	if (temp_tail >= num)
		temp_tail -= num;
	else
		temp_tail = buff_size - num + f->tail;

	if (move_tail)
	{
		f->tail = temp_tail;
	}
	return temp_tail;
}
