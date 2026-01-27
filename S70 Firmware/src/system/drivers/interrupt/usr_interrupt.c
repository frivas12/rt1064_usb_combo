/**
 * @file usr_interrupt.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "usr_interrupt.h"
#include <pins.h>
#include "pio.h"


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void enable_CPLD_handler(uint8_t slot);
static void enable_SMIO_handler(uint8_t slot);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void enable_CPLD_handler(uint8_t slot)
{
	switch (slot)
	{
		case SLOT_1:
			set_pin_as_input(PIN_INT_1);
			pio_enable_interrupt(PIOD, PIO_PD10);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD10, PIO_IT_RISE_EDGE, cpld_slot_1_handler);
			break;
		case SLOT_2:
			set_pin_as_input(PIN_INT_2);
			pio_enable_interrupt(PIOD, PIO_PD1);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD1, PIO_IT_RISE_EDGE, cpld_slot_2_handler);
			break;
		case SLOT_3:
			set_pin_as_input(PIN_INT_3);
			pio_enable_interrupt(PIOA, PIO_PA11);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA11, PIO_IT_RISE_EDGE, cpld_slot_3_handler);
			break;
		case SLOT_4:
			set_pin_as_input(PIN_INT_4);
			pio_enable_interrupt(PIOA, PIO_PA26);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA26, PIO_IT_RISE_EDGE, cpld_slot_4_handler);
			break;
		case SLOT_5:
			set_pin_as_input(PIN_INT_5);
			pio_enable_interrupt(PIOD, PIO_PD28);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD28, PIO_IT_RISE_EDGE, cpld_slot_5_handler);
			break;
		case SLOT_6:
			set_pin_as_input(PIN_INT_6);
			pio_enable_interrupt(PIOA, PIO_PA24);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA24, PIO_IT_RISE_EDGE, cpld_slot_6_handler);
			break;
		case SLOT_7:
			set_pin_as_input(PIN_INT_7);
			pio_enable_interrupt(PIOD, PIO_PD18);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD18, PIO_IT_RISE_EDGE, cpld_slot_7_handler);
			break;
	}
	// set flag to read the interrupt at bootup
	slots[slot].interrupt_flag_cpld = true;
}

static void enable_SMIO_handler(uint8_t slot)
{
	switch (slot)
	{
		case SLOT_1:
			set_pin_as_input(PIN_SMIO_1);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD26, PIO_IT_FALL_EDGE, smio_slot_1_handler);
			pio_enable_interrupt(PIOD, PIO_PD26);
			break;
		case SLOT_2:
			set_pin_as_input(PIN_SMIO_2);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD25, PIO_IT_FALL_EDGE, smio_slot_2_handler);
			pio_enable_interrupt(PIOD, PIO_PD25);
			break;
		case SLOT_3:
			set_pin_as_input(PIN_SMIO_3);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD2, PIO_IT_FALL_EDGE, smio_slot_3_handler);
			pio_enable_interrupt(PIOD, PIO_PD2);
			break;
		case SLOT_4:
			set_pin_as_input(PIN_SMIO_4);
			pio_handler_set(PIOD, ID_PIOD, PIO_PD24, PIO_IT_FALL_EDGE, smio_slot_4_handler);
			pio_enable_interrupt(PIOD, PIO_PD24);
			break;
		case SLOT_5:
			set_pin_as_input(PIN_SMIO_5);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA13, PIO_IT_FALL_EDGE, smio_slot_5_handler);
			pio_enable_interrupt(PIOA, PIO_PA13);
			break;
		case SLOT_6:
			set_pin_as_input(PIN_SMIO_6);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA14, PIO_IT_FALL_EDGE, smio_slot_6_handler);
			pio_enable_interrupt(PIOA, PIO_PA14);
			break;
		case SLOT_7:
			set_pin_as_input(PIN_SMIO_7);
			pio_handler_set(PIOA, ID_PIOA, PIO_PA25, PIO_IT_FALL_EDGE, smio_slot_7_handler);
			pio_enable_interrupt(PIOA, PIO_PA25);
			break;
	}
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/
void init_interrupts(void)
{
	/* Enable PIO controller IRQs. */
	NVIC_EnableIRQ((IRQn_Type)ID_PIOA);
	NVIC_EnableIRQ((IRQn_Type)ID_PIOD);

	// must set the interrupt priority lower priority than configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
	pio_handler_set_priority(PIOA, ID_PIOA, 7);
	pio_handler_set_priority(PIOD, ID_PIOD, 7);
}

void setup_slot_interrupt_CPLD(slot_nums slot)
{
	enable_CPLD_handler(slot);
}

void setup_slot_interrupt_SMIO(slot_nums slot)
{
	enable_SMIO_handler(slot);
}

/**
 * @brief  Interrupt handler for the interrupts coming in from the CPLD.
 */
void cpld_slot_1_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_1].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_1].p_interrupt_cpld_handler(SLOT_1,NULL);
	}
}

void cpld_slot_2_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_2].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_2].p_interrupt_cpld_handler(SLOT_2,NULL);
	}
}

void cpld_slot_3_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_3].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_3].p_interrupt_cpld_handler(SLOT_3,NULL);
	}
}

void cpld_slot_4_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_4].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_4].p_interrupt_cpld_handler(SLOT_4,NULL);
	}
}

void cpld_slot_5_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_5].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_5].p_interrupt_cpld_handler(SLOT_5,NULL);
	}
}

void cpld_slot_6_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_6].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_6].p_interrupt_cpld_handler(SLOT_6,NULL);
	}
}

void cpld_slot_7_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_7].p_interrupt_cpld_handler != NULL)
	{
		slots[SLOT_7].p_interrupt_cpld_handler(SLOT_7,NULL);
	}
}

/**
 * Interrupt handlers for the interrupt coming in from slot to uC.
 * @param slot
 */
void smio_slot_1_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_1].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_1].p_interrupt_smio_handler(SLOT_1, NULL);
	}
}

void smio_slot_2_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_2].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_2].p_interrupt_smio_handler(SLOT_2, NULL);
	}
}

void smio_slot_3_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_3].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_3].p_interrupt_smio_handler(SLOT_3, NULL);
	}
}

void smio_slot_4_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_4].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_4].p_interrupt_smio_handler(SLOT_4, NULL);
	}
}

void smio_slot_5_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_5].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_5].p_interrupt_smio_handler(SLOT_5, NULL);
	}
}

void smio_slot_6_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_6].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_6].p_interrupt_smio_handler(SLOT_6, NULL);
	}
}

void smio_slot_7_handler(uint32_t t1, uint32_t t2)
{
	if (slots[SLOT_7].p_interrupt_smio_handler != NULL)
	{
		slots[SLOT_7].p_interrupt_smio_handler(SLOT_7, NULL);
	}
}

