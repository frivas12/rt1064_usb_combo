/*
 * matrix_math.c
 *
 *  Created on: Sep 28, 2017
 *      Author: ELieser
 */

#include "matrix_math.h"

#include <arm_math.h>

static void reduction(float a[][6], int size, int pivot, int col);

float32_t pmSq(float32_t x)
{
	return x * x;
}

float32_t pmSqrt(float32_t x)
{
	float32_t a = x;
	uint32_t i = *(uint32_t *)&x;
	i = (i + 0x3f76cf62) >> 1;
	x = *(float32_t *)&i;
	x = (x + a / x) * 0.5;
	return x;


	//	float32_t root  = 0.0;
//	if (x > 0.0)
//	{
//		arm_sqrt_f32(x, &root);
//		return root;
//	}
//
//	if (x > SQRT_FUZZ)
//	{
//		return 0.0;
//	}
//	return 0.0;
}

uint8_t pmRpyMatConvert(PmRpy *rpy, PmRotationMatrix *m)
{
	float32_t sa, sb, sg;
	float32_t ca, cb, cg;

	sa = arm_sin_f32(rpy->y);
	sb = arm_sin_f32(rpy->p);
	sg = arm_sin_f32(rpy->r);

	ca = arm_cos_f32(rpy->y);
	cb = arm_cos_f32(rpy->p);
	cg = arm_cos_f32(rpy->r);

	m->x.x = ca * cb;
	m->y.x = ca * sb * sg - sa * cg;
	m->z.x = ca * sb * cg + sa * sg;

	m->x.y = sa * cb;
	m->y.y = sa * sb * sg + ca * cg;
	m->z.y = sa * sb * cg - ca * sg;

	m->x.z = -sb;
	m->y.z = cb * sg;
	m->z.z = cb * cg;

	return 0;
}


static void reduction(float a[][6], int size, int pivot, int col)
{
	int i, j;
	float factor;
	factor = a[pivot][col];

	for (i = 0; i < 2 * size; i++)
	{
		a[pivot][i] /= factor;
	}

	for (i = 0; i < size; i++)
	{
		if (i != pivot)
		{
			factor = a[i][col];
			for (j = 0; j < 2 * size; j++)
			{
				a[i][j] = a[i][j] - a[pivot][j] * factor;
			}
		}
	}
}

uint8_t pmRpyMatInvert(PmRotationMatrix *m, PmRotationMatrix *inv)
{
	float32_t matrix[3][6];
	uint8_t i, j;


	/*Fill in the right 3x3 with the identity matrix*/
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 6; j++)
		{
			if (j == i + 3)
			{
				matrix[i][j] = 1;
			}
			else
			{
				matrix[i][j] = 0;
			}
		}
	}

	/*Fill in the left part on matrix with our matrix to invert*/
	matrix[0][0] = m->x.x;
	matrix[0][1] = m->y.x;
	matrix[0][2] = m->z.x;
	matrix[1][0] = m->x.y;
	matrix[1][1] = m->y.y;
	matrix[1][2] = m->z.y;
	matrix[2][0] = m->x.z;
	matrix[2][1] = m->y.z;
	matrix[2][2] = m->z.z;

	for (i = 0; i < 3; i++)
	{
		reduction(matrix, 3, i, i);
	}

	inv->x.x = matrix[0][3];
	inv->y.x = matrix[0][4];
	inv->z.x = matrix[0][5];

	inv->x.y = matrix[1][3];
	inv->y.y = matrix[1][4];
	inv->z.y = matrix[1][5];

	inv->x.z = matrix[2][3];
	inv->y.z = matrix[2][4];
	inv->z.z = matrix[2][5];

	return 0;
}

uint8_t pmMatCartMult(PmRotationMatrix m, PmCartesian v, PmCartesian *vout)
{
	vout->x = m.x.x * v.x + m.y.x * v.y + m.z.x * v.z;
	vout->y = m.x.y * v.x + m.y.y * v.y + m.z.y * v.z;
	vout->z = m.x.z * v.x + m.y.z * v.y + m.z.z * v.z;

	return 0;
}

uint8_t pmCartCartAdd(PmCartesian v1, PmCartesian v2, PmCartesian *vout)
{
	vout->x = v1.x + v2.x;
	vout->y = v1.y + v2.y;
	vout->z = v1.z + v2.z;

	return 0;
}

uint8_t pmCartCartSub(PmCartesian v1, PmCartesian v2, PmCartesian *vout)
{
	vout->x = v1.x - v2.x;
	vout->y = v1.y - v2.y;
	vout->z = v1.z - v2.z;

	return 0;
}

float32_t pmCartMag(PmCartesian v)
{
	float32_t d = 0;
	d = pmSqrt(pmSq(v.x) + pmSq(v.y) + pmSq(v.z));

	return d;
}

uint8_t pmCartUnit(PmCartesian v, PmCartesian *vout)
{
	double size = pmSqrt(pmSq(v.x) + pmSq(v.y) + pmSq(v.z));

	if (fpclassify(size) == FP_ZERO)
	{ return 1; }

	vout->x = v.x / size;
	vout->y = v.y / size;
	vout->z = v.z / size;

	return 0;
}

uint8_t pmMatInvert(float32_t J[6][6], float32_t InvJ[6][6])
{
	float32_t JAug[12][12];
	double m, temp;
	int j, k, n;

	/* This function determines the inverse of a 6x6 matrix using
	   Gauss-Jordan elimination */

	/* Augment the Identity matrix to the Jacobian matrix */

	for (j = 0; j <= 5; ++j)
	{
	   for (k = 0; k <= 5; ++k)
	   {     /* Assign J matrix to first 6 columns of AugJ */
		   JAug[j][k] = J[j][k];
	   }
	   for (k = 6; k <= 11; ++k)
	   {    /* Assign I matrix to last six columns of AugJ */
		   if (k - 6 == j)
		   {
			   JAug[j][k] = 1;
		   }
		   else
		   {
			   JAug[j][k] = 0;
		   }
	   }
	}

	/* Perform Gauss elimination */
	for (k = 0; k <= 4; ++k)
	{               /* Pivot        */
	   if ((JAug[k][k] < 0.01) && (JAug[k][k] > -0.01))
	   {
		   for (j = k + 1; j <= 5; ++j)
		   {
			   if ((JAug[j][k] > 0.01) || (JAug[j][k] < -0.01))
			   {
				   for (n = 0; n <= 11; ++n)
				   {
					   temp = JAug[k][n];
					   JAug[k][n] = JAug[j][n];
					   JAug[j][n] = temp;
				   }
				   break;
			   }
		   }
	   }
	   for (j = k + 1; j <= 5; ++j)
	   {            /* Pivot */
		   m = -JAug[j][k] / JAug[k][k];
		   for (n = 0; n <= 11; ++n)
		   {
			   JAug[j][n] = JAug[j][n] + m * JAug[k][n];   /* (Row j) + m * (Row k) */
			   if ((JAug[j][n] < 0.000001) && (JAug[j][n] > -0.000001))
			   {
				   JAug[j][n] = 0;
			   }
		   }
	   }
	}
	   /* Normalization of Diagonal Terms */
	for (j = 0; j <= 5; ++j)
	{
	  m = 1 / JAug[j][j];
	  for (k = 0; k <= 11; ++k)
	  {
		  JAug[j][k] = m * JAug[j][k];
	  }
	}

	/* Perform Gauss Jordan Steps */
	for (k = 5; k >= 0; --k)
	{
	  for (j = k - 1; j >= 0; --j)
	  {
		  m = -JAug[j][k] / JAug[k][k];
		  for (n = 0; n <= 11; ++n)
		  {
			  JAug[j][n] = JAug[j][n] + m * JAug[k][n];
		  }
	  }
	}

	/* Assign last 6 columns of JAug to InvJ */
	for (j = 0; j <= 5; ++j)
	{
	  for (k = 0; k <= 5; ++k)
	  {
		  InvJ[j][k] = JAug[j][k + 6];

	  }
	}

	return 0;         /* FIXME-- check divisors for 0 above */
   }

uint8_t pmMatMult(float32_t J[6][6], float32_t *x, float32_t *Ans)
{
	int j, k;
	for (j = 0; j <= 5; ++j)
	{
		Ans[j] = 0;
		for (k = 0; k <= 5; ++k)
		{
			Ans[j] = J[j][k] * x[k] + Ans[j];
		}
	}
	return 0;
}

uint8_t pmCartCartCross(PmCartesian v1, PmCartesian v2, PmCartesian *vout)
{
	if (fpclassify(vout->z - v1.z) == FP_ZERO || fpclassify(vout->z - v2.z) == FP_ZERO)
	{
		return 0;
	}
	vout->x = v1.y * v2.z - v1.z * v2.y;
	vout->y = v1.z * v2.x - v1.x * v2.z;
	vout->z = v1.x * v2.y - v1.y * v2.x;

	return 0;
}
