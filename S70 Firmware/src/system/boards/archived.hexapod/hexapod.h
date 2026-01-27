/**
 * @file hexapod.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_HEXAPOD_HEXAPOD_H_
#define SRC_SYSTEM_HEXAPOD_HEXAPOD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hex_kins.h"
#include "synchronized_motion.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void hexapod_init(void);
void hexapod_init_struct(Sm_info *info);
void service_hexapod(Sm_info *info);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_HEXAPOD_HEXAPOD_H_ */
