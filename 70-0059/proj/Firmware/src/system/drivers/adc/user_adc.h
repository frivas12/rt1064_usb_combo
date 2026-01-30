/**
 * @file user_adc.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ADC_USER_ADC_H_
#define SRC_SYSTEM_DRIVERS_ADC_USER_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/** Reference voltage for AFEC in mv. */
#define VOLT_REF        (3300)
/** The maximal digital value */
#define MAX_DIGITAL     (4095UL)

/** AFEC0 channels */
#define TEMP_SENSOR  AFEC_CHANNEL_10


/** AFEC1 channels */
#define VIN_MONITOR  AFEC_CHANNEL_0


/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void config_afec(Afec *const afec, enum afec_channel_num afec_ch);
void init_adc(void);
uint16_t read_analog(Afec *const afec, enum afec_channel_num afec_ch);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ADC_USER_ADC_H_ */
