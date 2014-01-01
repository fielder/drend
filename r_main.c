#include <string.h>
#include <math.h>

#include "vec.h"
#include "drend.h"
#include "render.h"

const float fov_x = 90.0;


void
R_Init (void)
{
	camera.center_x = vid.w / 2.0;
	camera.center_y = vid.h / 2.0;

	camera.fov_x = fov_x * (M_PI / 180.0);
	camera.dist = (vid.w / 2.0) / tan(camera.fov_x / 2.0);
	camera.fov_y = 2.0 * atan((vid.h / 2.0) / camera.dist);

	Vec_Clear (camera.pos);
	camera.altitude = 0.0;
	camera.angle = 0.0;
}


void
R_Shutdown (void)
{
}


static void
CalcCamera (void)
{
	double s, c;
	float n[2];
	float cam2world[2][2];

	Vec_RotationMatrix (-camera.angle, cam2world);

	s = sin (camera.fov_x / 2.0);
	c = cos (camera.fov_x / 2.0);

	n[0] = -c;
	n[1] = s;
	Vec_Transform (cam2world, n, camera.vplanes[VPLANE_LEFT].normal);
	camera.vplanes[VPLANE_LEFT].dist = Vec_Dot (camera.vplanes[VPLANE_LEFT].normal, camera.pos);

	n[0] = c;
	n[1] = s;
	Vec_Transform (cam2world, n, camera.vplanes[VPLANE_RIGHT].normal);
	camera.vplanes[VPLANE_RIGHT].dist = Vec_Dot (camera.vplanes[VPLANE_RIGHT].normal, camera.pos);

	/* world-to-camera transformation matrix */
	Vec_RotationMatrix (camera.angle, camera.xform);
}


/*
 * Traverse the world and set up structures to prepare drawing to the
 * frame buffer.
 */
void
R_DrawScene (void)
{
	CalcCamera ();

	//...
}


static void
PutPixel (int x, int y, int c)
{
	if (x >= 0 && x < vid.w && y >= 0 && y < vid.h)
		vid.rows[y][x] = c & 0xffff;
}


static void
RenderPoint (float x, float y, float z)
{
	float v[2], local[2], out[2];
	v[0] = x;
	v[1] = z;

	{
		struct viewplane_s *p;
		p = &camera.vplanes[VPLANE_LEFT];
		if (Vec_Dot(v, p->normal) - p->dist < (1 / 32.0))
			return;
		p = &camera.vplanes[VPLANE_RIGHT];
		if (Vec_Dot(v, p->normal) - p->dist < (1 / 32.0))
			return;
	}

//	PutPixel (vid.w / 2 - v[0], vid.h / 2 - v[1], 0xffff);

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[1] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[1]);
		int sy = camera.center_y - camera.dist * ((y - camera.altitude) / out[1]);
		PutPixel (sx, sy, 0xffff);
	}
}


/*
 * Draw to the frame buffer.
 */
void
R_RenderScene (void)
{
	int x, z;
	for (x = -50; x < 50; x++)
	{
		for (z = -50; z < 50; z++)
			RenderPoint(x*4, 0, z*4);
	}
	//...
}
