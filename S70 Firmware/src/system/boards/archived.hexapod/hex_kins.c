/*
 * hex_kins.c
 *
 *  Created on: Sep 28, 2017
 *      Author: ELieser
 */

#include <hex_kins.h>
#include "Debugging.h"
#include "../../drivers/math/matrix_math.h"

PmCartesian hex_b[NUM_STRUTS];
PmCartesian hex_a[NUM_STRUTS];

#if 1
/*****************NEMA 14 strut locations ******************/
//const int bANGLES[] = {46, 74, 166, 194, 286, 314};
const int bANGLES[] = {46, 74, 166, 194, 314, 286};  //swap the last two because the order of the struts is flipped
#define bRADIUS		134.62		//mm
//const int pANGLES[] = {16.5, 103.5, 136.5, 223.5, 256.5, 343.5};
const int pANGLES[] = {16.5, 103.5, 136.5, 223.5, 343.5, 256.5};  //swap the last two because the order of the struts is flipped
#define pRADIUS		78.74		//mm
#endif

#if 0
/*****************NEMA 8 strut locations ******************/
const int bANGLES[] = {44.35, 75.65, 164.35, 195.65, 315.65, 284.35};  //swap the last two because the order of the struts is flipped
#define bRADIUS		100.23		//mm
const int pANGLES[] = {15.43, 104.56, 135.43, 224.56, 344.56, 255.43};  //swap the last two because the order of the struts is flipped
#define pRADIUS		55.37		//mm
#endif

uint8_t rotate_pos_by_offs(EmcPose *offs)
{
	PmRpy rpy;
	PmRotationMatrix RMatrix;
	PmCartesian temp;

	/* define Rotation Matrix */
	rpy.r = offs->a * PI / 180.0;
	rpy.p = offs->b * PI / 180.0;
	rpy.y = offs->c * PI / 180.0;
	pmRpyMatConvert(&rpy, &RMatrix);

	pmMatCartMult(RMatrix, offs->tran, &temp);
	offs->tran.x = temp.x;
	offs->tran.y = temp.y;
	offs->tran.z = temp.z;

	return 0;
}

uint8_t kinematicsInverse(EmcPose pose, float32_t *joints){
	PmCartesian aw;
	PmCartesian temp;
	PmCartesian InvKinStrutVect;
	PmRotationMatrix RMatrix;
	PmRpy rpy;

	/* define Rotation Matrix */
	rpy.r = pose.a * PI / 180.0;  //convert to radians
	rpy.p = pose.b * PI / 180.0;
	rpy.y = pose.c * PI / 180.0;
	pmRpyMatConvert(&rpy, &RMatrix);

	/* enter for loop to calculate joints (strut lengths) */
	for (uint8_t i = 0; i < NUM_STRUTS; i++)
	{
		/* convert location of platform strut end from platform
		   to world coordinates */
		pmMatCartMult(RMatrix, hex_a[i], &temp);

		//add the pose transformation
		pmCartCartAdd(pose.tran, temp, &aw);

		/* define strut lengths */
		pmCartCartSub(aw, hex_b[i], &InvKinStrutVect);

		joints[i] = pmCartMag(InvKinStrutVect);
	}

	return 0;
}

uint8_t kinematicsForward(float32_t *joints, EmcPose *pose)
{
	EmcPose guess_pose;
	uint16_t iterNum = 0;
	uint16_t iterMax = 20;
	float32_t error = 1.0;
	float32_t tolerance = 0.0001;  //when this is 10nm it often fails to converge due to float32 resolution limitations
	float32_t test_joints[6];

	PmCartesian aw;
	PmCartesian RMatrix_a, RMatrix_a_cross_Strut;
	PmCartesian InvKinStrutVect, InvKinStrutVectUnit;
	PmRotationMatrix RMatrix;
	PmRpy rpy;
	float32_t Jacobian[NUM_STRUTS][NUM_STRUTS];
	float32_t InverseJacobian[NUM_STRUTS][NUM_STRUTS];
	float32_t StrutLengthDiff[NUM_STRUTS];
	float32_t delta[NUM_STRUTS];

	guess_pose.tran.x = pose->tran.x;
	guess_pose.tran.y = pose->tran.y;
	guess_pose.tran.z = pose->tran.z;
	guess_pose.a = pose->a;
	guess_pose.b = pose->b;
	guess_pose.c = pose->c;

	while(iterNum < iterMax)
	{
		error = 0;

		rpy.r = guess_pose.a * PI / 180.0;  //convert to radians
		rpy.p = guess_pose.b * PI / 180.0;
		rpy.y = guess_pose.c * PI / 180.0;
		pmRpyMatConvert(&rpy, &RMatrix);

		for (uint8_t i = 0; i < NUM_STRUTS; i++)
		{
			pmMatCartMult(RMatrix, hex_a[i], &RMatrix_a);
			pmCartCartAdd(guess_pose.tran, RMatrix_a, &aw);
			pmCartCartSub(aw, hex_b[i], &InvKinStrutVect);
			test_joints[i] = pmCartMag(InvKinStrutVect);
			StrutLengthDiff[i] = test_joints[i] - joints[i];
			if(StrutLengthDiff[i] < 0.0)
				error -= StrutLengthDiff[i];
			else
				error += StrutLengthDiff[i];

			pmCartUnit(InvKinStrutVect, &InvKinStrutVectUnit);

			/* Determine RMatrix_a_cross_strut */
			pmCartCartCross(RMatrix_a, InvKinStrutVectUnit, &RMatrix_a_cross_Strut);

			/* Build Inverse Jacobian Matrix */
			InverseJacobian[i][0] = InvKinStrutVectUnit.x;
			InverseJacobian[i][1] = InvKinStrutVectUnit.y;
			InverseJacobian[i][2] = InvKinStrutVectUnit.z;
			InverseJacobian[i][3] = RMatrix_a_cross_Strut.x;
			InverseJacobian[i][4] = RMatrix_a_cross_Strut.y;
			InverseJacobian[i][5] = RMatrix_a_cross_Strut.z;
		}

		if(error < tolerance){
			pose->tran.x = guess_pose.tran.x;
			pose->tran.y = guess_pose.tran.y;
			pose->tran.z = guess_pose.tran.z;
			pose->a = guess_pose.a;
			pose->b = guess_pose.b;
			pose->c = guess_pose.c;
			return true;
		}

		/* invert Inverse Jacobian */
		pmMatInvert(InverseJacobian, Jacobian);

		/* multiply Jacobian by LegLengthDiff */
		pmMatMult(Jacobian, StrutLengthDiff, delta);

		guess_pose.tran.x -= delta[0];
		guess_pose.tran.y -= delta[1];
		guess_pose.tran.z -= delta[2];
		guess_pose.a -= delta[3] * 180 / PI;
		guess_pose.b -= delta[4] * 180 / PI;
		guess_pose.c -= delta[5] * 180 / PI;


		iterNum++;

	}
	//debug_print("Fwd Kinematic calculations failed to converge./r/n");
	return false;

}

uint8_t configure_haldata()
{

	for(uint8_t c = 0; c < NUM_STRUTS; c++)
	{
		hex_b[c].x = bRADIUS * arm_cos_f32(bANGLES[c] * PI / 180);
		hex_b[c].y = bRADIUS * arm_sin_f32(bANGLES[c] * PI / 180);
		hex_b[c].z = 0;
	}

	for(uint8_t c = 0; c < NUM_STRUTS; c++)
	{
		hex_a[c].x = pRADIUS * arm_cos_f32(pANGLES[c] * PI / 180);
		hex_a[c].y = pRADIUS * arm_sin_f32(pANGLES[c] * PI / 180);
		hex_a[c].z = 0;
	}

	return 0;
}

