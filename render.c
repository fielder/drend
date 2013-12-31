#include "vec.h"
#include "drend.h"
#include "render.h"


static void
CalcCamera (void)
{
//	Vec_RotationMatrix (-camera.angle, camera.xform);

	//TODO: xform
	//TODO: vplanes
#if 0
	float cam2world[3][3];
	float v[3];
	struct viewplane_s *p;
	float ang;

	/* make transformation matrix */
	Vec_Copy (cam.angles, v);
	Vec_Scale (v, -1.0);
	Vec_AnglesMatrix (v, cam.xform, "xyz");

	/* get view vectors */
	Vec_Copy (cam.xform[0], cam.left);
	Vec_Copy (cam.xform[1], cam.up);
	Vec_Copy (cam.xform[2], cam.forward);

	/* set up view planes */

	/* view to world transformation matrix */
	Vec_AnglesMatrix (cam.angles, cam2world, "zyx");

	p = &cam.vplanes[VPLANE_LEFT];
	ang = (cam.fov_x / 2.0);
	v[0] = -cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_RIGHT];
	ang = (cam.fov_x / 2.0);
	v[0] = cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_TOP];
	ang = (cam.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = -cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_BOTTOM];
	ang = (cam.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);
#endif
}


void
R_DrawScene (void)
{
	CalcCamera ();

	//...
}


void
R_RenderScene (void)
{
	//...
}
