#include <game/mapitems.hpp>
#include <game/layers.hpp>
#include "flow.hpp"

FLOW::FLOW()
{
	cells = 0;
	height = 0;
	width = 0;
	spacing = 16;
}
	
void FLOW::dbg_render()
{
	if(!cells)
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

void FLOW::init()
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
	cells = (CELL *)mem_alloc(sizeof(CELL)*width*height, 1);
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			cells[y*width+x].vel = vec2(0.0f, 0.0f);
}

void FLOW::update()
{
	if(!cells)
		return;
		
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			cells[y*width+x].vel *= 0.85f;
}

vec2 FLOW::get(vec2 pos)
{
	if(!cells)
		return vec2(0,0);
	
	int x = (int)(pos.x / spacing);
	int y = (int)(pos.y / spacing);
	if(x < 0 || y < 0 || x >= width || y >= height)
		return vec2(0,0);
	
	return cells[y*width+x].vel;	
}

void FLOW::add(vec2 pos, vec2 vel, float size)
{
	if(!cells)
		return;
		
	int x = (int)(pos.x / spacing);
	int y = (int)(pos.y / spacing);
	if(x < 0 || y < 0 || x >= width || y >= height)
		return;
	
	cells[y*width+x].vel += vel;
}
