#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include "gc_client.hpp"
#include "../layers.hpp"

struct FLOWCELL
{
	vec2 vel;
};

static FLOWCELL *cells = 0;
static int height = 0;
static int width = 0;
static int spacing = 16;

void flow_init()
{
	if(cells)
	{
		mem_free(cells);
		cells = 0;
	}
	
	MAPITEM_LAYER_TILEMAP *tilemap = layers_game_layer();
	width = tilemap->width*32/spacing;
	height = tilemap->height*32/spacing;

	// allocate and clear	
	cells = (FLOWCELL *)mem_alloc(sizeof(FLOWCELL)*width*height, 1);
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			cells[y*width+x].vel = vec2(0.0f, 0.0f);
}

void flow_update()
{
	if(!config.cl_flow)
		return;
		
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			cells[y*width+x].vel *= 0.85f;
}

void flow_dbg_render()
{
	if(!config.cl_flow)
		return;

	gfx_texture_set(-1);
	gfx_lines_begin();
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
		{
			vec2 pos(x*spacing, y*spacing);
			vec2 vel = cells[y*width+x].vel * 0.01f;
			gfx_lines_draw(pos.x, pos.y, pos.x+vel.x, pos.y+vel.y);
		}
		
	gfx_lines_end();
}

void flow_add(vec2 pos, vec2 vel, float size)
{
	if(!config.cl_flow)
		return;
		
	int x = (int)(pos.x / spacing);
	int y = (int)(pos.y / spacing);
	if(x < 0 || y < 0 || x >= width || y >= height)
		return;
	
	cells[y*width+x].vel += vel;
}

vec2 flow_get(vec2 pos)
{
	if(!config.cl_flow)
		return vec2(0,0);
	
	int x = (int)(pos.x / spacing);
	int y = (int)(pos.y / spacing);
	if(x < 0 || y < 0 || x >= width || y >= height)
		return vec2(0,0);
	
	return cells[y*width+x].vel;
}
