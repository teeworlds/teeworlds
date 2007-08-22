#include <engine/system.h>
#include <game/vmath.h>
#include <game/math.h>
#include <math.h>
#include "../engine/interface.h"
#include "mapres_col.h"
#include "mapres.h"

/*
	Simple collision rutines!
*/
struct collision
{
	int w, h;
	unsigned char *data;
};

static collision col;
static int global_dividor;

int col_init(int dividor)
{
	mapres_collision *c = (mapres_collision*)map_find_item(MAPRES_COLLISIONMAP,0);
	if(!c)
	{
		dbg_msg("mapres_col", "failed!");
		return 0;
	}
	col.w = c->width;
	col.h = c->height;
	global_dividor = dividor;
	col.data = (unsigned char *)map_get_data(c->data_index);
	return col.data ? 1 : 0;
}

int col_check_point(int x, int y)
{
	int nx = x/global_dividor;
	int ny = y/global_dividor;
	if(nx < 0 || nx >= col.w || ny >= col.h)
		return 1;
	
	if(y < 0)
		return 0; // up == sky == free
	
	return col.data[ny*col.w+nx];
}

// TODO: rewrite this smarter!
bool col_intersect_line(vec2 pos0, vec2 pos1, vec2 *out)
{
	float d = distance(pos0, pos1);
	
	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 pos = mix(pos0, pos1, a);
		if(col_check_point((int)pos.x, (int)pos.y))
		{
			if(out)
				*out = pos;
			return true;
		}
	}
	if(out)
		*out = pos1;
	return false;
}
