#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "vec.h"
#include "drend.h"
#include "render.h"

#define BACKFACE_EPSILON 0.01

struct drawwall_s *r_walls = NULL;
static struct drawwall_s *r_walls_start = NULL;
static struct drawwall_s *r_walls_end = NULL;


void
R_BeginWallFrame (void *buf, int buflen)
{
	/* prepare the given wall buffer */
	uintptr_t w = (uintptr_t)buf;

	while ((w % sizeof(struct drawwall_s)) != 0)
	{
		w++;
		buflen--;
	}
	r_walls_start = r_walls = (struct drawwall_s *)w;
	r_walls_end = r_walls_start + (buflen / sizeof(struct drawwall_s));
}


struct vert_s _v1 = { { 32, 128 } };
struct vert_s _v2 = { { -32, 128 } };
struct plane_s _p = { { 0, -1 }, -128 };
struct wall_s _w = { { &_v1, &_v2 }, &_p, 0, 64 };

static float *r_p1, *r_p2;
static float *r_clip;


static int
ClipWall (struct viewplane_s *plane)
{
	float d1, d2, frac;

	d1 = Vec_Dot (plane->normal, r_p1) - plane->dist;
	d2 = Vec_Dot (plane->normal, r_p2) - plane->dist;

	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* wall runs from front -> back */

			frac = d1 / (d1 - d2);
			r_clip[0] = r_p1[0] + frac * (r_p2[0] - r_p1[0]);
			r_clip[1] = r_p1[1] + frac * (r_p2[1] - r_p1[1]);

			r_p2 = r_clip;
			r_clip += 2;
		}
		else
		{
			/* both vertices on the front side */
		}
	}
	else
	{
		if (d2 < 0.0)
		{
			/* both vertices behind a plane; the
			 * wall is fully clipped away */
			return 0;
		}
		else
		{
			/* wall runs from back -> front */

			frac = d1 / (d1 - d2);
			r_clip[0] = r_p1[0] + frac * (r_p2[0] - r_p1[0]);
			r_clip[1] = r_p1[1] + frac * (r_p2[1] - r_p1[1]);

			r_p1 = r_clip;
			r_clip += 2;
		}
	}

	return 1;
}


static void
ScanWall (int x1, int top1, int bot1, int x2, int top_dy, int bot_dy)
{
	while (x1 <= x2)
	{
		S_ClipAndEmitSpan (x1, top1 >> 20, bot1 >> 20);

		top1 += top_dy;
		bot1 += bot_dy;
		x1++;
	}
}


#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

void
R_DrawWall (struct wall_s *w)
{
	float clipverts[2 * 2];
	float local[2];

	float out1[2];
	float top1_f, bottom1_f;
	float x1_f;
	int x1_i; //, top1_i, bottom1_i;

	float out2[2];
	float top2_f, bottom2_f;
	float x2_f;
	int x2_i; //, top2_i, bottom2_i;

	float sheight;
	float top_dy, bottom_dy;

	w = &_w;

	/* no more draw walls */
	if (r_walls == r_walls_end)
		return;

	if (w->floorheight >= w->ceilingheight)
		return;

	if (Vec_Dot(camera.pos, w->plane->normal) - w->plane->dist < BACKFACE_EPSILON)
		return;

	r_p1 = w->verts[0]->xz;
	r_p2 = w->verts[1]->xz;
	r_clip = clipverts;

	if (!ClipWall(&camera.vplanes[VPLANE_LEFT]))
		return;
	if (!ClipWall(&camera.vplanes[VPLANE_RIGHT]))
		return;

	Vec_Subtract (r_p1, camera.pos, local);
	Vec_Transform (camera.xform, local, out1);

	Vec_Subtract (r_p2, camera.pos, local);
	Vec_Transform (camera.xform, local, out2);

	/* the left exactly on a pixel center will not include that
	 * column in the wall, but the next column */
	x1_f = camera.center_x - camera.dist * (out1[0] / out1[1]);
	x1_i = floor (x1_f + 0.5);

	/* the right exactly on a pixel center will include that
	 * column */
	x2_f = camera.center_x - camera.dist * (out2[0] / out2[1]);
	x2_i = floor (x2_f + 0.5) - 1;

	/* doesn't cross any pixel center */
	if (x1_i > x2_i)
		return;

	sheight = camera.dist * ((w->ceilingheight - w->floorheight) / out1[1]);
	top1_f = camera.center_y - camera.dist * ((w->ceilingheight - camera.altitude) / out1[1]);
	bottom1_f = top1_f + sheight;

	sheight = camera.dist * ((w->ceilingheight - w->floorheight) / out2[1]);
	top2_f = camera.center_y - camera.dist * ((w->ceilingheight - camera.altitude) / out2[1]);
	bottom2_f = top2_f + sheight;

	top_dy = (top2_f - top1_f) / (x2_f - x1_f);
	bottom_dy = (bottom2_f - bottom1_f) / (x2_f - x1_f);

	/* shift the projected x coords to align with the center of its
	 * pixel and adjust the top & bottom accordingly */
	top1_f = top1_f + top_dy * ((x1_i + 0.5) - x1_f);
	top2_f = top2_f + top_dy * ((x2_i + 0.5) - x2_f);
	bottom1_f = bottom1_f + bottom_dy * ((x1_i + 0.5) - x1_f);
	bottom2_f = bottom2_f + bottom_dy * ((x2_i + 0.5) - x2_f);

	/* our vertical fill rule says a column's top on a pixel center
	 * gets the pixel, so this affects the following reject tests */

	/* quickly reject if the floor is off the top of the screen */
	if (MAX(bottom1_f, bottom2_f) <= 0.0)
		return;
	/* quickly reject if the ceiling is off the bottom of the screen */
	if (MIN(top1_f, top2_f) > vid.h - 0.5)
		return;

	ScanWall (	x1_i, top1_f * 0x100000, bottom1_f * 0x100000,
			x2_i, top_dy * 0x100000, bottom_dy * 0x100000);
}
