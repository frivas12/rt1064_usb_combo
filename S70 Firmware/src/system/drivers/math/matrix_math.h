/*
 * matrix_math.h
 *
 *  Created on: Sep 28, 2017
 *      Author: ELieser
 */

#ifndef SRC_DRIVERS_MATRIX_MATH_H_
#define SRC_DRIVERS_MATRIX_MATH_H_

#include <stdint.h>
#include <arm_math.h>

//#define PI 			3.14159
///PI already defined in arm_math
#define SQRT_FUZZ 	-1.0e-6		// how close to 0 before math_sqrt(); is error

typedef struct  __attribute__((packed))
{
	float32_t x;
	float32_t y;
	float32_t z;
} PmCartesian;

typedef struct  __attribute__((packed))
{
	PmCartesian x;
	PmCartesian y;
	PmCartesian z;
} PmRotationMatrix;

typedef struct  __attribute__((packed))
{
	float32_t r;
	float32_t p;
	float32_t y;
} PmRpy;

float32_t pmSq(float32_t x);
float32_t pmSqrt(float32_t x);
uint8_t pmRpyMatConvert(PmRpy *rpy, PmRotationMatrix *m);
uint8_t pmRpyMatInvert(PmRotationMatrix *m, PmRotationMatrix *R_inverseMatrix);
uint8_t pmMatCartMult(PmRotationMatrix m, PmCartesian v, PmCartesian *vout);
uint8_t pmCartCartAdd(PmCartesian v1, PmCartesian v2, PmCartesian *vout);
uint8_t pmCartCartSub(PmCartesian v1, PmCartesian v2, PmCartesian *vout);
float32_t pmCartMag(PmCartesian v);
uint8_t pmCartUnit(PmCartesian v, PmCartesian *vout);
uint8_t pmMatInvert(float32_t J[6][6], float32_t InvJ[6][6]);
uint8_t pmMatMult(float32_t J[6][6], float32_t *x, float32_t *Ans);
uint8_t pmCartCartCross(PmCartesian v1, PmCartesian v2, PmCartesian *vout);

#endif /* SRC_DRIVERS_MATRIX_MATH_H_ */
