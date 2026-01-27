/*
 * hexkins.h
 *
 *  Created on: Sep 28, 2017
 *      Author: ELieser
 */

#ifndef SRC_DRIVERS_HEXAPOD_HEX_KINS_H_
#define SRC_DRIVERS_HEXAPOD_HEX_KINS_H_

#include <synchronized_motion.h>
#include "../../drivers/math/matrix_math.h"
#include "../../drivers/math/matrix_math.h"

#define NUM_STRUTS			6
#define X_OFFS				0//-211
#define Y_OFFS				0
#define Z_OFFS				0
#define STAGE_HEIGHT		340 //NEMA8 is 220


/* declare arrays for base and platform coordinates */
extern PmCartesian hex_b[NUM_STRUTS];
extern PmCartesian hex_a[NUM_STRUTS];


uint8_t rotate_pos_by_offs(EmcPose *offs);
uint8_t kinematicsInverse(EmcPose pose, float32_t *joints);
uint8_t kinematicsForward(float32_t *joints, EmcPose *pose);
uint8_t configure_haldata(void);

#endif /* SRC_DRIVERS_HEXAPOD_HEX_KINS_H_ */
