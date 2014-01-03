#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "drend.h"
#include "render.h"

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

	r_spans = NULL;
	r_spans_end = NULL;
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
				vid.rows[y][i] = 0x3c0;
				y++;
			}
		}
	}
}


static void
PushSpan (short x, short top, short bottom)
{
	if (r_spans != r_spans_end)
	{
		r_spans->x = x;
		r_spans->top = top;
		r_spans->len = bottom - top + 1;
		r_spans++;
	}
}


void
S_ClipAndEmitSpan (int x, int y1, int y2)
{
	short X = x, Y1 = y1, Y2 = y2;
	struct gspan_s *head = &r_gspans[X];
	struct gspan_s *gs, *next, *new;

	if (x < 0 || x >= vid.w || y1 > y2)
		return;

	gs = head->next;
	while (1)
	{
		if (gs == head)
		{
			break;
		}
		else if (gs->bottom < Y1)
		{
			gs = gs->next;
		}
		else if (gs->top > Y2)
		{
			break;
		}
		else if (gs->top < Y1)
		{
			/* the gspan hangs off the top of the span,
			 * split the gspan into 2 */

			if (r_gspans_pool == NULL)
				return;

			new = r_gspans_pool;
			r_gspans_pool = r_gspans_pool->next;
			new->top = Y1;
			new->bottom = gs->bottom;
			gs->bottom = Y1 - 1;
			new->prev = gs;
			new->next = gs->next;
			new->prev->next = new;
			new->next->prev = new;
			gs = new;
		}
		else if (gs->bottom <= Y2)
		{
			/* the gspan lies entirely within the span,
			 * emit a drawable span */

			PushSpan (X, gs->top, gs->bottom);
			next = gs->next;
			gs->prev->next = gs->next;
			gs->next->prev = gs->prev;
			gs->next = r_gspans_pool;
			r_gspans_pool = gs;
			gs = next;
		}
		else
		{
			/* the gspan hangs off the bottom of the span,
			 * split the gspan */

			if (r_gspans_pool == NULL)
				return;

			new = r_gspans_pool;
			r_gspans_pool = r_gspans_pool->next;
			new->top = Y2 + 1;
			new->bottom = gs->bottom;
			gs->bottom = Y2;
			new->prev = gs;
			new->next = gs->next;
			new->prev->next = new;
			new->next->prev = new;
		}
	}
}



