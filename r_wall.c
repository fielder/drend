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
	short x, y;
	short len;
};


struct drawwall_s
{
	struct wall_s *wall;

	struct drawspan_s *spans;
	int num_spans;
};




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
