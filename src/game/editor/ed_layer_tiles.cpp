#include <game/client/gc_mapres_tilemap.h>
#include <game/g_math.h>
#include "editor.hpp"

static void render_tilemap(TILE *tiles, int w, int h, float scale)
{
			//gfx_texture_set(img_get(tmap->image));
	float screen_x0, screen_y0, screen_x1, screen_y1;
	gfx_getscreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);

	// calculate the final pixelsize for the tiles	
	float tile_pixelsize = 1024/32.0f;
	float final_tilesize = scale/(screen_x1-screen_x0) * gfx_screenwidth();
	float final_tilesize_scale = final_tilesize/tile_pixelsize;
	
	gfx_quads_begin();
	
	int starty = (int)(screen_y0/scale)-1;
	int startx = (int)(screen_x0/scale)-1;
	int endy = (int)(screen_y1/scale)+1;
	int endx = (int)(screen_x1/scale)+1;
	
	// adjust the texture shift according to mipmap level
	float texsize = 1024.0f;
	float frac = (1.25f/texsize) * (1/final_tilesize_scale);
	float nudge = (0.5f/texsize) * (1/final_tilesize_scale);

	for(int y = starty; y < endy; y++)
		for(int x = startx; x < endx; x++)
		{
			int mx = x;
			int my = y;
			if(mx<0)
				continue; // mx = 0;
			if(mx>=w)
				continue; // mx = w-1;
			if(my<0)
				continue; // my = 0;
			if(my>=h)
				continue; // my = h-1;
			
			int c = mx + my*w;
				
			unsigned char index = tiles[c].index;
			if(index)
			{
				unsigned char flags = tiles[c].flags;
				int tx = index%16;
				int ty = index/16;
				int px0 = tx*(1024/16);
				int py0 = ty*(1024/16);
				int px1 = (tx+1)*(1024/16)-1;
				int py1 = (ty+1)*(1024/16)-1;
				
				float u0 = nudge + px0/texsize+frac;
				float v0 = nudge + py0/texsize+frac;
				float u1 = nudge + px1/texsize-frac;
				float v1 = nudge + py1/texsize-frac;
				
				if(flags&TILEFLAG_VFLIP)
				{
					float tmp = u0;
					u0 = u1;
					u1 = tmp;
				}

				if(flags&TILEFLAG_HFLIP)
				{
					float tmp = v0;
					v0 = v1;
					v1 = tmp;
				}
				
				gfx_quads_setsubset(u0,v0,u1,v1);

				gfx_quads_drawTL(x*scale, y*scale, scale, scale);
			}
		}
	
	gfx_quads_end();
}

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
	render_tilemap(tiles, width, height, 32.0f);
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


void LAYER_TILES::render_properties(RECT *toolbox)
{
	if(editor.props != PROPS_LAYER)
		return;
	
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
}
