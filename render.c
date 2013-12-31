#include <math.h>

#include "vec.h"
#include "drend.h"
#include "render.h"


static void
CalcCamera (void)
{
	double s, c;

	Vec_RotationMatrix (camera.angle, camera.xform);

	s = sin (camera.fov_x / 2.0);
	c = cos (camera.fov_x / 2.0);

	camera.vplanes[VPLANE_LEFT].normal[0] = -c;
	camera.vplanes[VPLANE_LEFT].normal[1] = s;
	camera.vplanes[VPLANE_LEFT].dist = Vec_Dot (camera.vplanes[VPLANE_LEFT].normal, camera.pos);

	camera.vplanes[VPLANE_RIGHT].normal[0] = c;
	camera.vplanes[VPLANE_RIGHT].normal[1] = s;
	camera.vplanes[VPLANE_RIGHT].dist = Vec_Dot (camera.vplanes[VPLANE_RIGHT].normal, camera.pos);
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
