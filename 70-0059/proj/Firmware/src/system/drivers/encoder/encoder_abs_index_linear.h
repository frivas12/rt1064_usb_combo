/**
 * @file encoder_abs_index_linear.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_INDEX_LINEAR_H_
#define SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_INDEX_LINEAR_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void set_abs_index_linear_encoder_position(uint8_t slot, int32_t counts);
int32_t get_abs_index_linear_encoder_counts(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ENCODER_ENCODER_ABS_INDEX_LINEAR_H_ */
