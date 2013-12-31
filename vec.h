#ifndef __VEC_H__
#define __VEC_H__

extern void
Vec_Clear (float v[2]);

extern void
Vec_Copy (const float src[2], float out[2]);

extern void
Vec_Scale (float v[2], float s);

extern void
Vec_Add (const float a[2], const float b[2], float out[2]);

extern void
Vec_Subtract (const float a[2], const float b[2], float out[2]);

extern float
Vec_Dot (const float a[2], const float b[2]);

extern void
Vec_Normalize (float v[2]);

extern float
Vec_Length (const float v[2]);

extern void
Vec_Transform (float xform[2][2], const float v[2], float out[2]);

extern void
Vec_MakeNormal (const float v1[2],
		const float v2[2],
		float normal[2],
		float *dist);

extern void
Vec_IdentityMatrix (float mat[2][2]);

extern void
Vec_MultMatrix (float a[2][2], float b[2][2], float out[2][2]);

extern void
Vec_RotationMatrix (float angle, float out[2][2]);

#endif /* __VEC_H__ */
