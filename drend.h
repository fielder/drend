#ifndef __TMAP2_H__
#define __TMAP2_H__

#include <stdint.h>
#include <SDL_keysym.h>

typedef uint16_t pixel_t;

enum
{
	VPLANE_LEFT,
	VPLANE_RIGHT,
};

struct viewplane_s
{
	float normal[2];
	float dist;
};

struct camera_s
{
	float center_x;
	float center_y;

	float fov_x; /* radians */
	float fov_y; /* radians */
	float dist;

	float pos[2];
	float altitude;
	float angle; /* radians */

	float left[2];
	float forward[2];

	float xform[2][2]; /* world-to-camera */

	struct viewplane_s vplanes[2];
};

enum
{
	MBUTTON_LEFT,
	MBUTTON_MIDDLE,
	MBUTTON_RIGHT,
	MBUTTON_WHEEL_UP,
	MBUTTON_WHEEL_DOWN,
	MBUTTON_LAST,
};

struct input_s
{
	struct
	{
		int delta[2];
		struct
		{
			int state[MBUTTON_LAST];
			int press[MBUTTON_LAST];
			int release[MBUTTON_LAST];
		} button;
	} mouse;

	struct
	{
		int state[SDLK_LAST];
		int press[SDLK_LAST];
		int release[SDLK_LAST];
	} key;
};

struct video_s
{
	int w, h;
	pixel_t *bouncebuf;
	pixel_t **rows;
	int scale;
	pixel_t red, green, blue;
};

extern struct camera_s camera;
extern struct input_s input;
extern struct video_s vid;

extern void
Quit (void);

#endif /* __TMAP2_H__ */
