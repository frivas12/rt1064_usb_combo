/**
 * @file encoder_abs_biss_linear.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_BISS_LINEAR_H_
#define SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_BISS_LINEAR_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define BISS_ENC_SPI_MODE		_SPI_MODE_3
#define SPI_BISS_ENC_NO_READ 	BISS_ENC_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, (SLOT_CS(slot) + 1)
#define SPI_BISS_ENC_READ 		BISS_ENC_SPI_MODE, SPI_READ, CS_NO_TOGGLE, (SLOT_CS(slot) + 1)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void set_abs_biss_linear_encoder_position(uint8_t slot, int32_t counts);
int32_t get_abs_biss_linear_encoder_counts(uint8_t slot, Encoder *enc);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_BISS_LINEAR_H_ */
