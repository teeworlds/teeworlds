/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>
#include <base/math.hpp>
#include <base/vmath.hpp>

#include <math.h>
#include <engine/e_common_interface.h>
#include <game/mapitems.hpp>
#include <game/layers.hpp>
#include <game/collision.hpp>

static TILE *tiles;
static int width = 0;
static int height = 0;

int col_width() { return width; }
int col_height() { return height; }

int col_init()
{
	width = layers_game_layer()->width;
	height = layers_game_layer()->height;
	tiles = (TILE *)map_get_data(layers_game_layer()->data);
	
	for(int i = 0; i < width*height; i++)
	{
		int index = tiles[i].index;
		
		if(index > 128)
			continue;
		
		if(index == TILE_DEATH)
			tiles[i].index = COLFLAG_DEATH;
		else if(index == TILE_SOLID)
			tiles[i].index = COLFLAG_SOLID;
		else if(index == TILE_NOHOOK)
			tiles[i].index = COLFLAG_SOLID|COLFLAG_NOHOOK;
		else
			tiles[i].index = 0;
	}
				
	return 1;
}


int col_get(int x, int y)
{
	int nx = clamp(x/32, 0, width-1);
	int ny = clamp(y/32, 0, height-1);
	
	if(tiles[ny*width+nx].index > 128)
		return 0;
	return tiles[ny*width+nx].index;
}

int col_is_solid(int x, int y)
{
	return col_get(x,y)&COLFLAG_SOLID;
}


// TODO: rewrite this smarter!
int col_intersect_line(vec2 pos0, vec2 pos1, vec2 *out)
{
	float d = distance(pos0, pos1);
	
	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 pos = mix(pos0, pos1, a);
		if(col_is_solid((int)pos.x, (int)pos.y))
		{
			if(out)
				*out = pos;
			return col_get((int)pos.x, (int)pos.y);
		}
	}
	if(out)
		*out = pos1;
	return 0;
}
