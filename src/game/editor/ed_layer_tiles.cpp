#include <game/g_math.h>
#include <game/generated/gc_data.h>
#include <game/client/gc_render.h>
#include "ed_editor.hpp"

LAYER_TILES::LAYER_TILES(int w, int h)
{
	type = LAYERTYPE_TILES;
	type_name = "Tiles";
	width = w;
	height = h;
	image = -1;
	tex_id = -1;
	game = 0;
	
	tiles = new TILE[width*height];
	mem_zero(tiles, width*height*sizeof(TILE));
}

LAYER_TILES::~LAYER_TILES()
{
	delete [] tiles;
}

void LAYER_TILES::make_palette()
{
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			tiles[y*width+x].index = y*16+x;
}

void LAYER_TILES::render()
{
	if(image >= 0 && image < editor.map.images.len())
		tex_id = editor.map.images[image]->tex_id;
	gfx_texture_set(tex_id);
	render_tilemap(tiles, width, height, 32.0f, 0);
}

int LAYER_TILES::convert_x(float x) const { return (int)(x/32.0f); }
int LAYER_TILES::convert_y(float y) const { return (int)(y/32.0f); }

void LAYER_TILES::convert(RECT rect, RECTi *out)
{
	out->x = convert_x(rect.x);
	out->y = convert_y(rect.y);
	out->w = convert_x(rect.x+rect.w+31) - out->x;
	out->h = convert_y(rect.y+rect.h+31) - out->y;
}

void LAYER_TILES::snap(RECT *rect)
{
	RECTi out;
	convert(*rect, &out);
	rect->x = out.x*32.0f;
	rect->y = out.y*32.0f;
	rect->w = out.w*32.0f;
	rect->h = out.h*32.0f;
}

void LAYER_TILES::clamp(RECTi *rect)
{
	if(rect->x < 0)
	{
		rect->w += rect->x;
		rect->x = 0;
	}
		
	if(rect->y < 0)
	{
		rect->h += rect->y;
		rect->y = 0;
	}
	
	if(rect->x+rect->w > width)
		rect->w = width-rect->x;

	if(rect->y+rect->h > height)
		rect->h = height-rect->y;
		
	if(rect->h < 0)
		rect->h = 0;
	if(rect->w < 0)
		rect->w = 0;
}

void LAYER_TILES::brush_selecting(RECT rect)
{
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(1,1,1,0.4f);
	snap(&rect);
	gfx_quads_drawTL(rect.x, rect.y, rect.w, rect.h);
	gfx_quads_end();
	
}

int LAYER_TILES::brush_grab(LAYERGROUP *brush, RECT rect)
{
	RECTi r;
	convert(rect, &r);
	clamp(&r);
	
	if(!r.w || !r.h)
		return 0;
	
	// create new layers
	LAYER_TILES *grabbed = new LAYER_TILES(r.w, r.h);
	grabbed->tex_id = tex_id;
	grabbed->image = image;
	brush->add_layer(grabbed);
	
	// copy the tiles
	for(int y = 0; y < r.h; y++)
		for(int x = 0; x < r.w; x++)
			grabbed->tiles[y*grabbed->width+x] = tiles[(r.y+y)*width+(r.x+x)];
	
	return 1;
}

void LAYER_TILES::brush_draw(LAYER *brush, float wx, float wy)
{
	//
	LAYER_TILES *l = (LAYER_TILES *)brush;
	int sx = convert_x(wx);
	int sy = convert_y(wy);
	
	for(int y = 0; y < l->height; y++)
		for(int x = 0; x < l->width; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			if(fx<0 || fx >= width || fy < 0 || fy >= height)
				continue;
				
			tiles[fy*width+fx] = l->tiles[y*l->width+x];
		}
}

void LAYER_TILES::brush_flip_x()
{
	for(int y = 0; y < height; y++)
		for(int x = 0; x < width/2; x++)
		{
			TILE tmp = tiles[y*width+x];
			tiles[y*width+x] = tiles[y*width+x+width-1-x];
			tiles[y*width+x+width-1-x] = tmp;
		}

	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			tiles[y*width+x].flags ^= TILEFLAG_VFLIP;
}

void LAYER_TILES::brush_flip_y()
{
	for(int y = 0; y < height/2; y++)
		for(int x = 0; x < width; x++)
		{
			TILE tmp = tiles[y*width+x];
			tiles[y*width+x] = tiles[(height-1-y)*width+x];
			tiles[(height-1-y)*width+x] = tmp;
		}

	for(int y = 0; y < height; y++)
		for(int x = 0; x < width; x++)
			tiles[y*width+x].flags ^= TILEFLAG_HFLIP;
}

void LAYER_TILES::resize(int new_w, int new_h)
{
	TILE *new_data = new TILE[new_w*new_h];
	mem_zero(new_data, new_w*new_h*sizeof(TILE));

	// copy old data	
	for(int y = 0; y < min(new_h, height); y++)
		mem_copy(&new_data[y*new_w], &tiles[y*width], min(width, new_w)*sizeof(TILE));
	
	// replace old
	delete [] tiles;
	tiles = new_data;
	width = new_w;
	height = new_h;
}


int LAYER_TILES::render_properties(RECT *toolbox)
{
	RECT button;
	ui_hsplit_b(toolbox, 12.0f, toolbox, &button);
	bool in_gamegroup = editor.game_group->layers.find(this) != -1;
	static int col_button = 0;
	if(do_editor_button(&col_button, "Make Collision", in_gamegroup?0:-1, &button, draw_editor_button, 0, "Constructs collision from the this layer"))
	{
		LAYER_TILES *gl = editor.game_layer;
		int w = min(gl->width, width);
		int h = min(gl->height, height);
		for(int y = 0; y < h; y++)
			for(int x = 0; x < w; x++)
			{
				if(gl->tiles[y*gl->width+x].index <= TILE_SOLID)
					gl->tiles[y*gl->width+x].index = tiles[y*width+x].index?TILE_SOLID:TILE_AIR;
			}
			
		return 1;
	}
	
	enum
	{
		PROP_IMAGE=0,
		PROP_WIDTH,
		PROP_HEIGHT,
		NUM_PROPS,
	};
	
	PROPERTY props[] = {
		{"Image", image, PROPTYPE_INT_STEP, 0, 0},
		{"Width", width, PROPTYPE_INT_STEP, 1, 1000000000},
		{"Height", height, PROPTYPE_INT_STEP, 1, 1000000000},
		{0},
	};
	
	static int ids[NUM_PROPS] = {0};
	int new_val = 0;
	int prop = editor.do_properties(toolbox, props, ids, &new_val);		
	
	if(prop == PROP_IMAGE)
		image = new_val%editor.map.images.len();
	else if(prop == PROP_WIDTH && new_val > 1)
		resize(new_val, height);
	else if(prop == PROP_HEIGHT && new_val > 1)
		resize(width, new_val);
	
	return 0;
}
