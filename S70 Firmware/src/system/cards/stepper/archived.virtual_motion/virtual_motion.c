/**
 * @file virtual_motion.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "virtual_motion.h"
#include "synchronized_motion.h"
#include <arm_math.h>
#include "../../../drivers/math/matrix_math.h"
#include "Debugging.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Function that return dot product of two vector array.
 * @param vect_A
 * @param vect_B
 * @return
 */
// static float32_t dotProduct(PmCartesian v1, PmCartesian v2)
// {
// 	float32_t product = 0;

// 	product = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
// 	return product;
// }

static float32_t Sq(float32_t x)
{
	return x * x;
}

// static float32_t magnitude(PmCartesian v)
// {
// 	float32_t d = 0;
// 	d = sqrt(Sq(v.x) + Sq(v.y) + Sq(v.z));

// 	return d;
// }

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void sm_clear_rotation_matrix(Sm_info *info)
{
	/* Reset the structures*/
	memset(&info->save.params.RMatrix, 0, sizeof(PmRotationMatrix));
}

void sm_get_rotation_matrix(Sm_info *info)
{
	PmCartesian p1, p2, p3;
	PmCartesian v13;
	PmCartesian x_prime;  /*new x axis*/
	PmCartesian y_prime;  /*new y axis*/
	PmCartesian z_prime;  /*vector normal to plane*/
	float32_t roll_angle;
	float32_t yaw_angle;
	float32_t pitch_angle;
	PmRpy rpy;

	p1.x = info->PointsMatrix.x.x;
	p1.y = info->PointsMatrix.x.y;
	p1.z = info->PointsMatrix.x.z;

	p2.x = info->PointsMatrix.y.x;
	p2.y = info->PointsMatrix.y.y;
	p2.z = info->PointsMatrix.y.z;

	p3.x = info->PointsMatrix.z.x;
	p3.y = info->PointsMatrix.z.y;
	p3.z = info->PointsMatrix.z.z;

	/* Note: p1 to p2 will be the new x' axis.
	 * To find the angles for roll, pitch, and yaw we need to first find the vector orthogonal to
	 * the plane made up of points p1,p2,p3.
	 * To do this, we need to get the two vectors on the plane that go from p1 to p2 and p1 to p3.
	 * To do this we subtract components from p2 and p1 to get vector v12 (x_prime) which is on the plane and
	 * also subtract components from p3 and p1 to get vector v13 which is also on the plane.
	 * Then take the cross product of these vector to get the orthogonal vector normal to this plane.
	 * Now we have 2 orthogonal vectors v12 (x_prime) and z' (z_prime) but we need one more.  For this one we
	 * take the cross product of these 2 vectors to get the 3rd orthogonal vector need to get the
	 * roll, pitch, and yaw.
	 *
	 * */

	/*Get vector v12 on the plane from p1, p2*/
	pmCartCartSub(p1, p2, &x_prime);

	/*Get vector v13 on the plane  from p1, p3*/
	pmCartCartSub(p3, p2, &v13);

	/*Get the cross product of v13 & v12 to find the normal to the plane*/
	pmCartCartCross(x_prime, v13, &z_prime);

	if(z_prime.z < 0)
	{
		z_prime.x *= -1;
		z_prime.y *= -1;
		z_prime.z *= -1;
	}
	
	/*Get the cross product of x_prime & z_prime to find the 3rd normal vector*/
	pmCartCartCross(x_prime, z_prime, &y_prime);

	if(y_prime.z < 0)
	{
		y_prime.x *= -1;
		y_prime.y *= -1;
		y_prime.z *= -1;
	}

	/*Pitch angle, rotation about x */
	pitch_angle = atan2(-x_prime.z,sqrt(Sq(x_prime.x) + Sq(x_prime.y)));

	/*Yaw angle, rotation about z*/
	yaw_angle = atan2((x_prime.y/cos(pitch_angle)), (x_prime.x/cos(pitch_angle))); 

	/*Roll angle , rotation about y*/
	roll_angle = atan2((-x_prime.z/cos(pitch_angle)), (x_prime.x/cos(pitch_angle)));

	rpy.r = roll_angle; 	/*Angle about y*/
	rpy.p = pitch_angle; 	/*Angle about x*/
	rpy.y = yaw_angle; 		/*Angle about z*/

	if (info->save.params.config.sm_type == VIRTUAL_AXIS_SM_TYPE)
	{
		rpy.r = 0; 		/*Angle about x*/
		rpy.p = 0;		/*Angle about y*/
	}

	/*Get the rotation matrix*/
	pmRpyMatConvert(&rpy, &info->save.params.RMatrix);

}

void service_virtual_motion(Sm_info *info)
{
	PmCartesian move_delta;
	PmCartesian out;

	move_delta.x = info->sm_data.pose_command.tran.x;
	move_delta.y = info->sm_data.pose_command.tran.y;
	move_delta.z = info->sm_data.pose_command.tran.z;
	pmMatCartMult(info->save.params.RMatrix, move_delta, &out);

	/*struts are in */
	info->sm_data.struts[info->save.params.config.x_slot] = out.x;
	info->sm_data.struts[info->save.params.config.y_slot] = out.y;
	info->sm_data.struts[info->save.params.config.z_slot] = out.z;
}

