/**
 * @file virtual_motion.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_STEPPER_VIRTUAL_MOTION_VIRTUAL_MOTION_H_
#define SRC_SYSTEM_CARDS_STEPPER_VIRTUAL_MOTION_VIRTUAL_MOTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../drivers/math/matrix_math.h"
#include "synchronized_motion.h"
#include "hexapod.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void sm_clear_rotation_matrix(Sm_info *info);
void sm_get_rotation_matrix(Sm_info *info);
void service_virtual_motion(Sm_info *info);
void virtual_motion_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_CARDS_STEPPER_VIRTUAL_MOTION_VIRTUAL_MOTION_H_ */
