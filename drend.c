#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "vec.h"
#include "drend.h"
#include "render.h"

#if 1
/* wasd-style on a kinesis advantage w/ dvorak */
const int bind_forward = '.';
const int bind_back = 'e';
const int bind_left = 'o';
const int bind_right = 'u';
#else
/* qwerty-style using arrows */
const int bind_forward = SDLK_UP;
const int bind_back = SDLK_DOWN;
const int bind_left = SDLK_LEFT;
const int bind_right = SDLK_RIGHT;
#endif

struct camera_s camera;
struct input_s input;
struct video_s vid;

static SDL_Surface *sdl_surf = NULL;

float frametime;

static struct
{
	float rate;
	int framecount;
	Uint32 calc_start;
} fps;

const float flyspeed = 64.0;


static void
Shutdown (void)
{
	R_Shutdown ();

	if (sdl_surf != NULL)
	{
		SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);

		SDL_FreeSurface (sdl_surf);
		sdl_surf = NULL;
	}

	if (vid.bouncebuf != NULL)
	{
		free (vid.bouncebuf);
		vid.bouncebuf = NULL;
	}

	if (vid.rows != NULL)
	{
		free (vid.rows);
		vid.rows = NULL;
	}

	SDL_Quit ();
}


void
Quit (void)
{
	Shutdown ();
	exit (EXIT_SUCCESS);
}


static void
InitVideo (void)
{
	Uint32 flags;
	int y;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		fprintf (stderr, "ERROR: video init failed\n");
		Quit ();
	}

	printf ("Setting mode %dx%dx%d\n", vid.w, vid.h, (int)sizeof(pixel_t) * 8);
	printf ("Scaling %dx\n", vid.scale);

	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;
	if ((sdl_surf = SDL_SetVideoMode(vid.w * vid.scale, vid.h * vid.scale, sizeof(pixel_t) * 8, flags)) == NULL)
	{
		fprintf (stderr, "ERROR: failed setting video mode\n");
		Quit ();
	}
	printf ("Set %dx%dx%d video mode\n", sdl_surf->w, sdl_surf->h, sdl_surf->format->BytesPerPixel * 8);
	printf ("RGB mask: 0x%x 0x%x 0x%x\n", sdl_surf->format->Rmask, sdl_surf->format->Gmask, sdl_surf->format->Bmask);

	vid.rows = malloc (vid.h * sizeof(*vid.rows));

	if (vid.scale == 1)
	{
		for (y = 0; y < vid.h; y++)
			vid.rows[y] = (pixel_t *)((uintptr_t)sdl_surf->pixels + y * sdl_surf->pitch);
	}
	else
	{
		vid.bouncebuf = malloc (vid.w * vid.h * sizeof(pixel_t));
		for (y = 0; y < vid.h; y++)
			vid.rows[y] = vid.bouncebuf + y * vid.w;
	}
}


static int _mouse_grabbed = 0;
static int _mouse_ignore_move = 1;

static void
FetchInput (void)
{
	SDL_Event sdlev;

	memset (input.key.press, 0, sizeof(input.key.press));
	memset (input.key.release, 0, sizeof(input.key.release));

	input.mouse.delta[0] = 0;
	input.mouse.delta[1] = 0;
	memset (input.mouse.button.press, 0, sizeof(input.mouse.button.press));
	memset (input.mouse.button.release, 0, sizeof(input.mouse.button.release));

	while (SDL_PollEvent(&sdlev))
	{
		switch (sdlev.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				int k = sdlev.key.keysym.sym;
				if (k >= 0 && k < sizeof(input.key.state) / sizeof(input.key.state[0]))
				{
					if (sdlev.type == SDL_KEYDOWN)
					{
						input.key.state[k] = 1;
						input.key.press[k] = 1;
					}
					else
					{
						input.key.state[k] = 0;
						input.key.release[k] = 1;
					}
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int b = -1;
				switch (sdlev.button.button)
				{
					case 1: b = MBUTTON_LEFT; break;
					case 2: b = MBUTTON_MIDDLE; break;
					case 3: b = MBUTTON_RIGHT; break;
					case 4: b = MBUTTON_WHEEL_UP; break;
					case 5: b = MBUTTON_WHEEL_DOWN; break;
					default: break;
				}

				if (b != -1)
				{
					if (sdlev.type == SDL_MOUSEBUTTONDOWN)
					{
						input.mouse.button.state[b] = 1;
						input.mouse.button.press[b] = 1;
					}
					else
					{
						input.mouse.button.state[b] = 0;
						input.mouse.button.release[b] = 1;
					}
				}

				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (_mouse_grabbed)
				{
					/* when grabbing the mouse, the initial
					 * delta we get can be HUGE, so ignore
					 * the first mouse movement after grabbing
					 * input */
					if (_mouse_ignore_move == 0)
					{
						//FIXME: a grabbed mouse under vmware gives jacked deltas
						input.mouse.delta[0] = sdlev.motion.xrel;
						input.mouse.delta[1] = sdlev.motion.yrel;
					}
					else
					{
						_mouse_ignore_move = 0;
					}
				}
				break;
			}

			case SDL_QUIT:
				Quit ();
				break;

			default:
				break;
		}
	}
}


static void
SetGrab (int grab)
{
	if (grab && !_mouse_grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_ON);
		SDL_ShowCursor (SDL_DISABLE);
		_mouse_grabbed = 1;
		_mouse_ignore_move = 1;
	}
	else if (!grab && _mouse_grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);
		_mouse_grabbed = 0;
		_mouse_ignore_move = 1;
	}
}


static void
ToggleGrab (void)
{
	SetGrab (!_mouse_grabbed);
}


static void
LockSurface (void)
{
	if (SDL_MUSTLOCK(sdl_surf))
	{
		if (SDL_LockSurface(sdl_surf) != 0)
		{
			fprintf (stderr, "ERROR: surface lock failed\n");
			Quit ();
		}
	}
}


static void
UnLockSurface (void)
{
	if (SDL_MUSTLOCK(sdl_surf))
		SDL_UnlockSurface (sdl_surf);
}


static void
CameraMovement (void)
{
	float mat[2][2];
	float v[2];
	int left, forward, up;
	float speed = flyspeed;

	/* angle */

	camera.angle += -input.mouse.delta[0] * (camera.fov_x / vid.w);
	while (camera.angle >= 2.0 * M_PI)
		camera.angle -= 2.0 * M_PI;
	while (camera.angle < 0.0)
		camera.angle += 2.0 * M_PI;

	/* view vectors */

	Vec_RotationMatrix (camera.angle, mat);
	Vec_Copy (mat[0], camera.left);
	Vec_Copy (mat[1], camera.forward);

	/* movement */

	if (input.key.state[SDLK_LSHIFT] || input.key.state[SDLK_RSHIFT])
		speed *= 1.5;

	left = 0;
	left += input.key.state[bind_left] ? 1 : 0;
	left -= input.key.state[bind_right] ? 1 : 0;
	Vec_Copy (camera.left, v);
	Vec_Scale (v, left * speed * frametime);
	Vec_Add (camera.pos, v, camera.pos);

	forward = 0;
	forward += input.key.state[bind_forward] ? 1 : 0;
	forward -= input.key.state[bind_back] ? 1 : 0;
	Vec_Copy (camera.forward, v);
	Vec_Scale (v, forward * speed * frametime);
	Vec_Add (camera.pos, v, camera.pos);

	up = 0;
	up += input.mouse.button.state[MBUTTON_RIGHT] ? 1 : 0;
	up -= input.mouse.button.state[MBUTTON_MIDDLE] ? 1 : 0;
	camera.altitude += up * speed * frametime;
}


static void
RunInput (void)
{
	FetchInput ();

	if (input.key.release[27])
		Quit ();

	if (input.key.release['g'])
		ToggleGrab ();

	if (input.key.release['f'])
		printf ("%g\n", fps.rate);

	if (input.key.release['c'])
	{
		printf ("(%g %g %g)\n", camera.pos[0], camera.altitude, camera.pos[1]);
		printf ("angle: %g\n", camera.angle);
		printf ("left: (%g %g)\n", camera.left[0], camera.left[1]);
		printf ("forward: (%g %g)\n", camera.forward[0], camera.forward[1]);
		printf ("\n");
	}

	CameraMovement ();
}


static void
CopyScreenScaled (void)
{
	//FIXME: make work w/ various bpp depths

	if (vid.scale == 2)
	{
		int x, y;
		pixel_t *src;
		uint32_t *t1, *t2, *d1, *d2, pix;
		int dstride = sdl_surf->pitch / sizeof(*d1);

		src = vid.bouncebuf;
		d1 = sdl_surf->pixels;
		d2 = d1 + dstride;
		for (y = 0; y < vid.h; y++)
		{
			t1 = d1;
			t2 = d2;
			for (x = 0; x < vid.w; x++)
			{
				pix = *src++;
				pix |= pix << 16;
				*t1++ = pix;
				*t2++ = pix;
			}
			d1 += dstride * 2;
			d2 += dstride * 2;
		}
	}
	else if (vid.scale == 3)
	{
		int x, y;
		pixel_t *src;
		uint16_t *t1, *t2, *t3, *d1, *d2, *d3, pix;
		int dstride = sdl_surf->pitch / sizeof(*d1);

		src = vid.bouncebuf;
		d1 = sdl_surf->pixels;
		d2 = d1 + dstride;
		d3 = d2 + dstride;
		for (y = 0; y < vid.h; y++)
		{
			t1 = d1;
			t2 = d2;
			t3 = d3;
			for (x = 0; x < vid.w; x++)
			{
				pix = *src++;
				*t1++ = pix; *t1++ = pix; *t1++ = pix;
				*t2++ = pix; *t2++ = pix; *t2++ = pix;
				*t3++ = pix; *t3++ = pix; *t3++ = pix;
			}
			d1 += dstride * 3;
			d2 += dstride * 3;
			d3 += dstride * 3;
		}
	}
	else if (vid.scale == 4)
	{
		int x, y;
		pixel_t *src;
		uint32_t *t1, *t2, *t3, *t4, *d1, *d2, *d3, *d4, pix;
		int dstride = sdl_surf->pitch / sizeof(*d1);

		src = vid.bouncebuf;
		d1 = sdl_surf->pixels;
		d2 = d1 + dstride;
		d3 = d2 + dstride;
		d4 = d3 + dstride;
		for (y = 0; y < vid.h; y++)
		{
			t1 = d1;
			t2 = d2;
			t3 = d3;
			t4 = d4;
			for (x = 0; x < vid.w; x++)
			{
				pix = *src++;
				pix |= pix << 16;
				*t1++ = pix; *t1++ = pix;
				*t2++ = pix; *t2++ = pix;
				*t3++ = pix; *t3++ = pix;
				*t4++ = pix; *t4++ = pix;
			}
			d1 += dstride * 4;
			d2 += dstride * 4;
			d3 += dstride * 4;
			d4 += dstride * 4;
		}
	}
	else
	{
	}
}


static void
ClearScreen (int color)
{
	int y;
	for (y = 0; y < vid.h; y++)
		memset (vid.rows[y], color, vid.w * sizeof(*vid.rows[y]));
}


static void
Refresh (void)
{
	char spanbuf[0x8000];
	char wallbuf[0x8000];

	S_SpanBeginFrame (spanbuf, sizeof(spanbuf));
	R_BeginWallFrame (wallbuf, sizeof(wallbuf));

	/* do as much as we can _before_ touching the frame buffer */
	R_DrawScene ();

	if (vid.scale == 1)
	{
		/* draw directly to the video surface */

		LockSurface ();

		ClearScreen (0x00);
		R_RenderScene ();

		UnLockSurface ();
	}
	else
	{
		/* draw to the bounce buffer first */
		ClearScreen (0x00);
		R_RenderScene ();

		/* then scale it directly to the video surface */
		LockSurface ();
		CopyScreenScaled ();
		UnLockSurface ();
	}

	SDL_Flip (sdl_surf);

	fps.framecount++;

	/* calculate the framerate */
	{
		Uint32 now = SDL_GetTicks();

		if ((now - fps.calc_start) > 250)
		{
			fps.rate = fps.framecount / ((now - fps.calc_start) / 1000.0);
			fps.framecount = 0;
			fps.calc_start = now;
		}
	}
}


int
main (int argc, const char **argv)
{
	Uint32 last, duration, now;

	if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0)
	{
		fprintf (stderr, "ERROR: SDL init failed\n");
		exit (EXIT_FAILURE);
	}

	memset (&vid, 0, sizeof(vid));
	vid.w = 320;
	vid.h = 240;
	vid.scale = 3;

	InitVideo ();
	R_Init ();

	last = SDL_GetTicks ();
	for (;;)
	{
		do
		{
			now = SDL_GetTicks ();
			duration = now - last;
		} while (duration < 1);
		frametime = duration / 1000.0;
		last = now;

		RunInput ();
		Refresh ();
	}

	return 0;
}
