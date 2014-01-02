#ifndef __RENDER_H__
#define __RENDER_H__

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
	short top;
	short len;
};

struct drawwall_s
{
	struct wall_s *wall;

	struct drawspan_s *spans;
	int num_spans;
};


/* r_main.c */

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_DrawScene (void);

extern void
R_RenderScene (void);


/* r_span.c */

extern struct drawspan_s *r_spans;

extern void
S_SpanCleanup (void);

extern void
S_SpanInit (void);

extern void
S_SpanBeginFrame (void *buf, int buflen);

extern void
S_RenderGSpans (void);

extern void
S_ClipAndEmitSpan (int x, int y1, int y2);


/* r_wall.c */

extern struct drawwall_s *r_walls;

extern void
R_BeginWallFrame (void *buf, int buflen);

extern void
R_DrawWall (struct wall_s *w);

#endif /* __RENDER_H__ */
