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

//	top1_i = floor (top1_f + 0.5);
//	bottom1_i = floor (bottom1_f + 0.5) - 1;
//	top2_i = floor (top2_f + 0.5);
//	bottom2_i = floor (bottom2_f + 0.5) - 1;

//	S_ClipAndEmitSpan (x1_i, top1_i, bottom1_i);
//	S_ClipAndEmitSpan (x2_i, top2_i, bottom2_i);

}


// - back-face check w/ 0.01 epsilon
// - clip the line against the view frustum
// - translate both vertices to view space
// - find screen x for each vert: center_x - viewdist * (transvert[0] / transvert[1])
// - snap x's to pixel centers and save off coords
// - if the wall doesn't span a pixel's center, it's not visible, return
// - for each vert:
//   - get the wall's height in screen space: (viewdist * (wallheight / transvert[1]))
//   - find the wall's ceiling in screen space: center_y - viewdist * ((ceiling - cam.altitude) / transvert[1])
//   - screen space floor = screen space ceiling + screen space height
// - find dy/dx for ceiling & floor
// - adjust both x coords to snap onto pixel centers, adjusting respective screen y coords appropriately
// - reject if top is below, or bottom is above the screen
// - convert coords & deltas to fixed-point
//   - x1 = 
