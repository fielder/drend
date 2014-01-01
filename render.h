#ifndef __RENDER_H__
#define __RENDER_H__

/* r_main.c */

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_DrawScene (void);

extern void
R_RenderScene (void);


/* r_wall.c */

extern void
S_SpanCleanup (void);

extern void
S_SpanInit (void);

extern void
S_SpanBeginFrame (void *buf, int buflen);

extern void
S_RenderGSpans (void);

#endif /* __RENDER_H__ */
