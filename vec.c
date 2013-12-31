/*
 * Slow, crusty, archaic math "library"
 */

#include <math.h>

#include "vec.h"


void
Vec_Clear (float v[2])
{
	v[0] = 0.0;
	v[1] = 0.0;
}


void
Vec_Copy (const float src[2], float out[2])
{
	out[0] = src[0];
	out[1] = src[1];
}


void
Vec_Scale (float v[2], float s)
{
	v[0] *= s;
	v[1] *= s;
}


void
Vec_Add (const float a[2], const float b[2], float out[2])
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
}


void
Vec_Subtract (const float a[2], const float b[2], float out[2])
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
}


float
Vec_Dot (const float a[2], const float b[2])
{
	return a[0] * b[0] + a[1] * b[1];
}


void
Vec_Normalize (float v[2])
{
	float len;

	len = sqrt(v[0] * v[0] + v[1] * v[1]);
	if (len == 0.0)
	{
		v[0] = 0.0;
		v[1] = 0.0;
	}
	else
	{
		v[0] /= len;
		v[1] /= len;
	}
}


float
Vec_Length (const float v[2])
{
	return sqrt(v[0] * v[0] + v[1] * v[1]);
}


void
Vec_Transform (float xform[2][2], const float v[2], float out[2])
{
	out[0] = xform[0][0] * v[0] + xform[0][1] * v[1];
	out[1] = xform[1][0] * v[0] + xform[1][1] * v[1];
}


void
Vec_MakeNormal (const float v1[2],
		const float v2[2],
		float normal[2],
		float *dist)
{
	normal[0] =  (v2[1] - v1[1]);
	normal[1] = -(v2[0] - v1[0]);

	Vec_Normalize (normal);

	*dist = Vec_Dot (normal, v1);
}


void
Vec_IdentityMatrix (float mat[2][2])
{
	mat[0][0] = 1.0; mat[0][1] = 0.0;
	mat[1][0] = 0.0; mat[1][1] = 1.0;
}


void
Vec_MultMatrix (float a[2][2], float b[2][2], float out[2][2])
{
	out[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0];
	out[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1];
	out[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0];
	out[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1];
}


void
Vec_RotationMatrix (float angle, float out[2][2])
{
	double c = cos(angle);
	double s = sin(angle);
	out[0][0] = c;
	out[0][1] = -s;
	out[1][0] = s;
	out[1][1] = c;
}
