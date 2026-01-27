/**
 * @file user_adc.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "user_adc.h"
#include "supervisor.h"
#include "pins.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
/** Low threshold */
static uint16_t gs_us_low_threshold = 0;
/** High threshold */
static uint16_t gs_us_high_threshold = 0;

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
void config_afec(Afec *const afec, enum afec_channel_num afec_ch)
{
	struct afec_config afec_cfg;
	struct afec_ch_config afec_ch_cfg;

	gs_us_low_threshold = 0x0;
	gs_us_high_threshold = 2300;

	afec_enable(AFEC0);
	afec_get_config_defaults(&afec_cfg);

//	afec_cfg.stm = false;

	afec_init(AFEC0, &afec_cfg);
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);
	afec_ch_get_config_defaults(&afec_ch_cfg);

	/*
	 * Because the internal AFEC offset is 0x200, it should cancel it and shift
	 * down to 0.
	 */
	afec_channel_set_analog_offset(AFEC0, TEMP_SENSOR, 0x200);

	afec_ch_cfg.gain = AFEC_GAINVALUE_0;

	afec_ch_set_config(AFEC0, TEMP_SENSOR, &afec_ch_cfg);
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);

//	afec_set_comparison_mode(AFEC0, AFEC_CMP_MODE_3, TEMP_SENSOR, 0);
//	afec_set_comparison_window(AFEC0, gs_us_low_threshold, gs_us_high_threshold);

	/* Enable channel. */
	afec_channel_enable(AFEC0, TEMP_SENSOR);

//	afec_set_callback(AFEC0, AFEC_INTERRUPT_COMP_ERROR, afec_print_comp_result, 1);

}

void init_adc(void)
{
	struct afec_config afec_cfg;
	struct afec_ch_config afec_ch_cfg;

	set_pin_as_input(PIN_TEMP_SENSOR);
	set_pin_as_input(PIN_VIN_MONITOR);

	afec_get_config_defaults(&afec_cfg);
	afec_ch_get_config_defaults(&afec_ch_cfg);

	afec_ch_cfg.diff = false;
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;


	/****** AFEC0 ******/
	afec_enable(AFEC0);
	afec_init(AFEC0, &afec_cfg);
	afec_ch_set_config(AFEC0, TEMP_SENSOR, &afec_ch_cfg);
	/*
	 * Because the internal AFEC offset is 512, it should cancel it and shift
	 * down to 0.
	 */
	afec_channel_set_analog_offset(AFEC0, TEMP_SENSOR, 512);//528); //522
	afec_set_trigger(AFEC0, AFEC_TRIG_SW);
	afec_channel_enable(AFEC0, TEMP_SENSOR);
//	afec_set_callback(AFEC0, AFEC_INTERRUPT_DATA_READY, adc_temperature_ready, 1);

	/*CPU temperature sensor*/
	afec_ch_set_config(AFEC0, AFEC_TEMPERATURE_SENSOR, &afec_ch_cfg);
	afec_channel_set_analog_offset(AFEC0, AFEC_TEMPERATURE_SENSOR, 512);
	afec_set_trigger(AFEC0, AFEC_TRIG_SW);
	afec_channel_enable(AFEC0, AFEC_TEMPERATURE_SENSOR);


	/****** AFEC1 ******/
	afec_enable(AFEC1);
	afec_init(AFEC1, &afec_cfg);
	afec_ch_set_config(AFEC1, VIN_MONITOR, &afec_ch_cfg);
	/*
	 * Because the internal AFEC offset is 512, it should cancel it and shift
	 * down to 0.
	 */
	afec_channel_set_analog_offset(AFEC1, VIN_MONITOR, 512); // 582 ,512
	afec_set_trigger(AFEC1, AFEC_TRIG_SW);
	afec_channel_enable(AFEC1, VIN_MONITOR);
//	afec_set_callback(AFEC1, AFEC_INTERRUPT_DATA_READY, adc_vin_ready, 1);
}


uint16_t read_analog(Afec *const afec, enum afec_channel_num afec_ch)
{
	afec_start_software_conversion(afec);
	while((afec_get_interrupt_status(afec) & AFEC_ISR_DRDY) != AFEC_ISR_DRDY);
	return afec_channel_get_value(afec, afec_ch);
}
