/**
 * @file encoder_abs_magnetic_rotation.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include <encoder.h>
#include <encoder_abs_magnetic_rotation.h>
#include <cpld.h>
#include <pins.h>
#include "user_spi.h"

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
#if 0
void abs_magnetic_rotation_encoder_power(uint8_t slot, bool state)
{
	// turns the power to the mag sensor on or off
	switch ((uint8_t) slot)
	{
	case SLOT_1:
		pin_state(PIN_ADC_1, state);
		break;

	case SLOT_2:
		pin_state(PIN_ADC_2, state);
		break;
	case SLOT_3:
		pin_state(PIN_ADC_3, state);
		break;
	case SLOT_4:
		pin_state(PIN_ADC_4, state);
		break;
	case SLOT_5:
		pin_state(PIN_ADC_5, state);
		break;
	case SLOT_6:
		pin_state(PIN_ADC_6, state);
		break;
	case SLOT_7:
		pin_state(PIN_ADC_1, state);
		break;
	}

	if(state == MAG_PWR_OFF)
		encoder_reset_nsl(slot, 0);
	else
		encoder_reset_nsl(slot, 1);
}
#endif

int32_t get_abs_magnetic_rotation_encoder_counts(uint8_t slot, Encoder *enc)
{
#define MAG_SENSOR_ERROR_MASK

	uint32_t counts;
	uint8_t temp;

	/*read the encoder*/
	read_reg(C_READ_SSI_MAG, slot, &temp, &counts);

	enc->mlo = Tst_bits(temp, 2); /*False ok, magnet too close*/
	enc->mhi = Tst_bits(temp, 4); /*False ok, magnet too far or EPI turret is out*/
	enc->m_ready = Tst_bits(temp, 8); /*true ok*/

	if (!enc->m_ready || enc->mlo || enc->mhi)
	{
		counts = enc->enc_pos;
		enc->mag_good = false;
	}
	else
		enc->mag_good = true;

	return counts;
}


