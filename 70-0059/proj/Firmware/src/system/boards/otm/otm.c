/**
 * @file otm.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "otm.h"
#include <pins.h>
#include "Debugging.h"
#include "25lc1024.h"

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

/**
 * @brief OTM board has static hardware
 * SLOT 1 Stepper card		- used for focus
 * SLOT 2 shutter card		- used for shutter 1
 * SLOT 3 shutter card		- used for shutter 2
 * SLOT 4 DAC card			- used for noise eater 1 & 2 and IPG laser power
 * SLOT 5 rs232 			- used for Ventus laser rs323
 * SLOT 6 scanner driver	- used to drive the galvos
 * SLOT 7 Sync outputs		- used for sync signals for scanners
 */
void otm_init(void)
{
	slots[SLOT_1].card_type =  ST_Stepper_type;
	slots[SLOT_2].card_type =  Shutter_type;
	slots[SLOT_3].card_type =  Shutter_type;
	slots[SLOT_4].card_type =  OTM_Dac_type;
	slots[SLOT_5].card_type = OTM_RS232_type;
//	slots[SLOT_5].card_type = NO_CARD_IN_SLOT;
	slots[SLOT_6].card_type = NO_CARD_IN_SLOT;
	slots[SLOT_7].card_type = NO_CARD_IN_SLOT;

//	zero_eeprom_position();

//	OTM_IPG_LaserPower(IPG_LASER_OFF);
//	slot[SLOT_4].otm->IPG__laser_power_cmd = IPG_LASER_OFF;

//	stepper_init(SLOT_1, slots[SLOT_1].card_type);
//	init_shutter(SLOT_2);
//	init_shutter(SLOT_3);

	// set sleep pin as output and set high to enable
//	set_pin_as_output(PIN_SMIO_1, ON);
//	set_pin_as_output(PIN_SMIO_2, ON);

//	OtmDacInit(SLOT_4);
//	OTM_ReadIpgMonitor();

}
