/**
 * @file encoder_abs_magnetic_rotation.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_MAGNETIC_ROTATION_H_
#define SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_MAGNETIC_ROTATION_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define MAG_SENSOR_SPI_MODE		_SPI_MODE_2
#define SPI_MAG_SENSOR_NO_READ 	MAG_SENSOR_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, (SLOT_CS(slot) + 1)
#define SPI_MAG_SENSOR_READ 	MAG_SENSOR_SPI_MODE, SPI_READ, CS_NO_TOGGLE, (SLOT_CS(slot) + 1)

#define OPTICAL_PWR_OFF			0
#define OPTICAL_PWR_ON			1

#define MAG_PWR_OFF			1	// High Off, Low on
#define MAG_PWR_ON			0

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
//void abs_magnetic_rotation_encoder_power(uint8_t slot, bool state);
//void set_abs_magnetic_rotation_encoder_position(uint8_t slot, int32_t counts);
int32_t get_abs_magnetic_rotation_encoder_counts(uint8_t slot, Encoder *enc);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_MAGNETIC_ROTATION_H_ */
