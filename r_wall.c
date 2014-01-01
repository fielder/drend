#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vec.h"
#include "drend.h"
#include "render.h"

struct vert_s
{
	float xz[2];
};


struct plane_s
{
	float normal[2];
	float dist;
};


struct wall_s
{
	struct vert_s *verts[2];
	struct plane_s *plane;

	float floorheight;
	float ceilingheight;
};


struct drawspan_s
{
	short x;
	short top, bottom;
};


struct drawwall_s
{
	struct wall_s *wall;

	struct drawspan_s *spans;
	int num_spans;
};

/* ================================================================== */


/* green span */
struct gspan_s
{
	struct gspan_s *prev, *next;
	short top, bottom;
};

static struct gspan_s *r_gspans = NULL;
static struct gspan_s *r_gspans_pool = NULL;

/* these point to a buffer on the stack, as they don't need to be kept
 * around across frames */
struct drawspan_s *r_spans = NULL;
static struct drawspan_s *r_spans_end = NULL;


void
S_SpanCleanup (void)
{
	if (r_gspans != NULL)
	{
		free (r_gspans);
		r_gspans = NULL;
		r_gspans_pool = NULL;
	}
}


void
S_SpanInit (void)
{
	struct gspan_s *alloced;
	int i, count, sz;

	S_SpanCleanup ();

	/* estimate a probable max number of spans per bucket */
	count = vid.w * 24;

	sz = sizeof(*alloced) * count;

	alloced = malloc (sz);

	/* The first handful are reserved as each column's gspan
	 * list head. ie: one element per screen column just for
	 * linked-list management. */
	r_gspans = alloced;
	for (i = 0; i < vid.w; i++)
		r_gspans[i].prev = r_gspans[i].next = &r_gspans[i];

	r_gspans_pool = alloced + i;
	while (i < count)
	{
		alloced[i].next = (i == count - 1) ? NULL : &alloced[i + 1];
		i++;
	}

	printf ("%d byte gspan buffer\n", sz);
}


void
S_SpanBeginFrame (void *buf, int buflen)
{
	/* prepare the given span buffer */
	uintptr_t p = (uintptr_t)buf;

	while ((p % sizeof(struct drawspan_s)) != 0)
	{
		p++;
		buflen--;
	}

	r_spans = (struct drawspan_s *)p;
	r_spans_end = r_spans + (buflen / sizeof(struct drawspan_s));

	/* now gspans */
	struct gspan_s *gs, *head, *next;
	int i;

	for (i = 0, head = r_gspans; i < vid.w; i++, head++)
	{
		/* take any gspan still remaining on the column and toss
		 * back into the pool */
		while ((next = head->next) != head)
		{
			head->next = next->next;
			next->next = r_gspans_pool;
			r_gspans_pool = next;
		}

		/* reset the column with 1 fresh gspan */

		gs = r_gspans_pool;
		r_gspans_pool = gs->next;

		gs->top = 0;
		gs->bottom = vid.h - 1;
		gs->prev = gs->next = head;
		head->prev = head->next = gs;
	}
}


/*
 * Draw any remaining gspan on the screen. In normal operation, the
 * screen should be filled by the rendered world so there should never
 * be any gspans visible.
 */
void
S_RenderGSpans (void)
{
	const struct gspan_s *gs;
	int i;

	for (i = 0; i < vid.w; i++)
	{
		for (gs = r_gspans[i].next; gs != &r_gspans[i]; gs = gs->next)
		{
			int y = gs->top;
			while (y <= gs->bottom)
			{
				vid.rows[y][i] = 0x7e0;
				y++;
			}
		}
	}
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
