#include <stdio.h>

extern "C" {
	#include <engine/system.h>
	#include <engine/client/ui.h>
	#include <engine/interface.h>
	#include <engine/datafile.h>
	#include <engine/config.h>
}

#include <game/client/mapres_image.h>
#include <game/client/mapres_tilemap.h>
//#include "game/mapres_col.h"
#include <game/mapres.h>
#include <game/game.h>

static int font_texture = 0;
static int checker_texture = 0;

struct ent_type
{
	const char *name;
	int id;
	int item_id;
};

static ent_type ent_types[] = {
	{"spawn", MAPRES_SPAWNPOINT, 0},
	{"gun", MAPRES_ITEM, ITEM_WEAPON_GUN},
	{"shotgun", MAPRES_ITEM, ITEM_WEAPON_SHOTGUN},
	{"rocket", MAPRES_ITEM, ITEM_WEAPON_ROCKET},
	{"sniper", MAPRES_ITEM, ITEM_WEAPON_SNIPER},
	{"hammer", MAPRES_ITEM, ITEM_WEAPON_HAMMER},
	{"health", MAPRES_ITEM, ITEM_HEALTH},
	{"armor", MAPRES_ITEM, ITEM_ARMOR},
	{"ninja", MAPRES_ITEM, ITEM_NINJA},
	{0, 0}
};


/********************************************************
 ENTITIES                                                
*********************************************************/
struct entity
{
	int type;
	int x, y;
};

static const int MAX_ENTITIES = 1024;
static entity entites[MAX_ENTITIES];
static int num_entities = 0;

static int ents_count()
{
	return num_entities;
}

static entity *ents_get(int index)
{
	return &entites[index];
}

static int ents_delete(int index)
{
	if(index < 0 || index >= ents_count())
		return -1;
	num_entities--;
	entites[index] = entites[num_entities];
	return 0;
}

static int ents_new(int type, int x, int y)
{
	entites[num_entities].type = type;
	entites[num_entities].x = x;
	entites[num_entities].y = y;
	num_entities++;
	return num_entities-1;
}

/********************************************************
 TILEMAP                                                 
*********************************************************/
struct tile
{
	unsigned char index;
	unsigned char flags;
};

struct tilemap
{
	int width;
	int height;
	tile *tiles;
};

// allocates a new tilemap and inits the structure
static void tilemap_new(tilemap *tm, int width, int height)
{
	unsigned size = sizeof(tile)*width*height;
	mem_zero(tm, sizeof(tilemap));
	
	tm->width = width;
	tm->height = height;
	tm->tiles = (tile *)mem_alloc(size, 1);
	mem_zero(tm->tiles, size);
}

// resizes an tilemap, copies the old data
static void tilemap_resize(tilemap *tm, int new_width, int new_height, int snap)
{
	if(new_width <= 0) new_width = 1;
	if(new_height <= 0) new_height = 1;
		
	new_width = (new_width+snap-1)/snap*snap;
	new_height = (new_height+snap-1)/snap*snap;
	
	unsigned size = sizeof(tile)*new_width*new_height;
	tile *newtiles = (tile *)mem_alloc(size, 1);
	mem_zero(newtiles, size);

	// copy old tiles
	int w = new_width < tm->width ? new_width : tm->width;
	int h = new_height < tm->height ? new_height : tm->height;
	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++)
			newtiles[y*new_width+x] = tm->tiles[y*tm->width+x];
		
	// free old tiles and set new values
	mem_free(tm->tiles);
	tm->tiles = newtiles;
	tm->width = new_width;
	tm->height = new_height;
}

static void tilemap_destroy(tilemap *tm)
{
	mem_free(tm->tiles);
	tm->tiles = 0;
	tm->width = 0;
	tm->height = 0;
}

static int tilemap_blit(tilemap *dst, tilemap *src, int x, int y)
{
	int count = 0;
	// TODO: performance of this could be better
	for(int iy = 0; iy < src->height; iy++)
		for(int ix = 0; ix < src->width; ix++)
		{
			if(x+ix >= dst->width || y+iy >= dst->height)
				continue;
			if(x+ix < 0 || y+iy < 0)
				continue;
			
			count++;
			dst->tiles[(y+iy)*dst->width + x+ix] = src->tiles[iy*src->width + ix];
		}
	return count;
}

/********************************************************
 LAYERS                                                  
*********************************************************/
struct layer
{
	tilemap tm;
	int tileset_id;
	int visible;
	int main_layer;
};

static const int MAX_LAYERS = 64;
static layer layers[MAX_LAYERS];
static int num_layers = 0;
static int current_layer = 0;

static int layers_remove(int index)
{
	if(index < 0 || index >= num_layers)
		return 0;
	
	// free the memory
	mem_free(layers[index].tm.tiles);
	
	// move the layers
	for(int i = index; i < num_layers-1; i++)
		layers[i] = layers[i+1];
	num_layers--;
	return 1;
}

static int layers_count()
{
	return num_layers;
}

static layer *layers_get_current()
{
	return &layers[current_layer];
}

static layer *layers_get(int index)
{
	return &layers[index];
}

static int layers_new(int w, int h)
{
	if(num_layers+1 >= MAX_LAYERS)
		return -1;
	
	tilemap_new(&layers[num_layers].tm, w, h);
	layers[num_layers].tileset_id = -1;
	layers[num_layers].visible = 1;
	layers[num_layers].main_layer = 0;
	num_layers++;
	return num_layers-1;
}

static int layers_swap(int index1, int index2)
{
	// swap the two layers
	layer temp = layers[index1];
	layers[index1] = layers[index2];
	layers[index2] = temp;
	return -1;
}

static int layers_moveup(int index)
{
	if(index < 0 || index >= num_layers)
		return index;
	
	if(index == 0)
		return 0;
	
	layers_swap(index, index-1);
	return index-1;
}

static int layers_movedown(int index)
{
	if(index < 0 || index >= num_layers)
		return index;
	
	if(index+1 == num_layers)
		return index;
	
	layers_swap(index, index+1);
	return index+1;
}

/********************************************************
 TILESET                                                 
*********************************************************/
struct tileset
{
	int tex_id;
	IMAGE_INFO img;
};

static const int MAX_TILESETS = 64;
static tileset tilesets[MAX_TILESETS];
static int num_tilesets = 0;

static int tilesets_new()
{
	tilesets[num_tilesets].img.width = 0;
	tilesets[num_tilesets].img.height = 0;
	tilesets[num_tilesets].img.data = 0;
	tilesets[num_tilesets].tex_id = -1;// gfx_load_texture_raw(img.width, img.height, img.data);
	num_tilesets++;
	return num_tilesets-1;
}


static void tilesets_delete(int index)
{
	// remove the tileset
	if(tilesets[index].tex_id >= 0)
		gfx_unload_texture(tilesets[index].tex_id);
		
	for(int i = index; i < num_tilesets-1; i++)
		tilesets[i] = tilesets[i+1];
	num_tilesets--;
	
	// adjust
	for(int i = 0; i < layers_count(); i++)
	{
		if(layers_get(i)->tileset_id == index)
			layers_get(i)->tileset_id = -1;
		else if(layers_get(i)->tileset_id > index)
			layers_get(i)->tileset_id--;
	}
}

static void tilesets_clear()
{
	for(int i = 0; i < num_tilesets; i++)
	{
		if(tilesets[i].tex_id >= 0)
			gfx_unload_texture(tilesets[i].tex_id);
		mem_free(tilesets[i].img.data);
	}
	num_tilesets = 0;
	
}

static int tilesets_set_img(int index, int w, int h, void *data)
{
	tilesets[index].img.width = w;
	tilesets[index].img.height = h;
	
	if(tilesets[index].img.data)
		mem_free(tilesets[index].img.data);
	tilesets[index].img.data = data;
	tilesets[index].tex_id = gfx_load_texture_raw(w, h, IMG_RGBA, data);
	return index;
}

static int tilesets_count()
{
	return num_tilesets;
}

static tileset *tilesets_get(int index)
{
	return &tilesets[index];
}

/********************************************************
 UI                                                      
*********************************************************/
static void render_tilemap(tilemap *tm, float sx, float sy, float scale)
{
	float frac = (1.0f/1024.0f); //2.0f;
	gfx_quads_begin();
	for(int y = 0; y < tm->height; y++)
		for(int x = 0; x < tm->width; x++)
		{
			unsigned char d = tm->tiles[y*tm->width+x].index;
			if(d)
			{
				gfx_quads_setsubset(
					(d%16)/16.0f+frac,
					(d/16)/16.0f+frac,
					(d%16)/16.0f+1.0f/16.0f-frac,
					(d/16)/16.0f+1.0f/16.0f-frac);
				gfx_quads_drawTL(sx+x*scale, sy+y*scale, scale, scale);
			}

			//gfx_quads_setsubset(x/16.0f,y/16.0f,(x+1)/16.0f,(y+1)/16.0f);
			//gfx_quads_drawTL(sx+x*w,sy+y*h,w,h);
		}
	gfx_quads_end();
}

/********************************************************
 EDITOR                                                  
*********************************************************/

static void ui_do_frame(float x, float y, float w, float h)
{
	static int frame_id = 0;
    int inside = ui_mouse_inside(x,y,w,h);
	if(inside)
		ui_set_hot_item(&frame_id);
		
	gfx_texture_set(-1);
	gfx_blend_normal();
	gfx_quads_begin();
	gfx_quads_setcolor(0.0f, 0.0f, 0.0f, 1.0f);
	gfx_quads_drawTL(x, y, w, h);
	gfx_quads_end();
	gfx_blend_normal();	
}

static tilemap brush = {0};
static tilemap chooser = {0};
static float world_offset_x = 0, world_offset_y = 0;
static int world_zoom = 3;
static const char *editor_filename = 0;
static int editor_mode = 0; // 0 == tiles, 1 == ents
static int editor_selected_ent = -1;

static const int TILEMAPFLAG_READONLY = 1;
static const int TILEMAPFLAG_UISPACE = 2;

static int ui_do_tilemap(void *id, tilemap *tm, int flags, float x, float y, float scale)
{
	/*
	int do_input = 1;
	if(inp_key_pressed(input::lalt) || inp_key_pressed(input::ralt))
		do_input = 0;*/
	
	
	float mx = ui_mouse_world_x();
	float my = ui_mouse_world_y();
	if(flags&TILEMAPFLAG_UISPACE)
	{
		mx = ui_mouse_x();
		my = ui_mouse_y();
	}
	
	int tmx = (int)((mx-x)/scale); // tilemap x
	int tmy = (int)((my-y)/scale); // tilemap y
	
	static int start_tmx, start_tmy;
	static int grabbing = 0;
	
	//float start_tmx_wx = start_tmx*scale+x;
	//float start_tmx_wy = start_tmy*scale+y;
	
	int select_x = 0;
	int select_y = 0;
	int select_w = 1;
	int select_h = 1;
	float select_wx = 0;
	float select_wy = 0;
	float select_ww = 0;
	float select_wh = 0;
	
	if(ui_hot_item() == id)
	{
		int x0 = start_tmx;
		int y0 = start_tmy;
		int x1 = tmx;
		int y1 = tmy;
		
		if(x1 < x0)
		{
			int tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		if(y1 < y0)
		{
			int tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		select_w = x1-x0;
		select_h = y1-y0;
		select_w++;
		select_h++;
		select_x = x0;
		select_y = y0;
		
		select_wx = select_x*scale+x;
		select_wy = select_y*scale+y;
		select_ww = select_w*scale;
		select_wh = select_h*scale;
	}
	
	// ui_do_tilemap always tries to steal the focus
	ui_set_hot_item(id);
	
	// render the tilemap
	render_tilemap(tm, x, y, scale);
	
	if(ui_hot_item() == id)
	{
		if(brush.tiles != 0)
		{
			// draw brush
			render_tilemap(&brush, (tmx-brush.width/2)*scale, (tmy-brush.height/2)*scale, scale);
			
			gfx_texture_set(-1);
			gfx_blend_additive();
			gfx_quads_begin();
			gfx_quads_setcolor(1.0f, 0.0f, 0.0f, 0.25f);
			gfx_quads_drawTL((tmx-brush.width/2)*scale, (tmy-brush.height/2)*scale, brush.width*scale, brush.height*scale);
			gfx_quads_end();
			gfx_blend_normal();
		}

		if(grabbing == 0)
		{
			if(ui_mouse_button(0))
			{
				//grabbing = 1;
				start_tmx = (int)((mx-x)/scale);
				start_tmy = (int)((my-y)/scale);
				if(brush.tiles == 0)
				{
					dbg_msg("editor", "grabbing...");
					grabbing = 1; // grab tiles
				}
				else
				{
					// paint
					if(!(flags&TILEMAPFLAG_READONLY))
					{
						layer *l = layers_get_current();
						int px = tmx-brush.width/2;
						int py = tmy-brush.height/2;
						tilemap_blit(&l->tm, &brush, px, py);
						//dbg_msg("editor", "painted %d tiles at (%d,%d)", c, px, py);
					}
				}
			}
		}
		else
		{
			gfx_texture_set(-1);
			gfx_blend_additive();
			gfx_quads_begin();
			gfx_quads_setcolor(1.0f, 1.0f, 1.0f, 0.25f);
			gfx_quads_drawTL(select_wx, select_wy, select_ww, select_wh);
			gfx_quads_end();
			gfx_blend_normal();
			
			if(!ui_mouse_button(0))
			{
				grabbing = 0;
				
				if(brush.tiles == 0)
				{
					// create brush
					dbg_msg("editor", "creating brush w=%d h=%d p0=(%d,%d)", select_w, select_h, select_x, select_y);
					tilemap_new(&brush, select_w, select_h);
					
					// copy data
					for(int y = 0; y < select_h; y++)
						for(int x = 0; x < select_w; x++)
							brush.tiles[y*select_w+x] = tm->tiles[(select_y+y)*tm->width + select_x+x];
				}
			}
		}
	}
	
	// raw rect around tilemap
	gfx_texture_set(-1);
	gfx_quads_begin();
	float w = tm->width*scale;
	float h = tm->height*scale;
	gfx_quads_drawTL(x-2, y-2, 2, h+4);
	gfx_quads_drawTL(x-2, y-2, w+4, 2);
	gfx_quads_drawTL(x+w, y-2, 2, h+4);
	gfx_quads_drawTL(x-2, y+h, w+4, 2);
	gfx_quads_end();
	
	return 0;
}

static int ui_do_entity(void *id, entity *ent, int selected)
{
	float x = (float)ent->x;
	float y = (float)ent->y;
	float w = 16;
	float h = 16;
	
	float mx = ui_mouse_world_x();
	float my = ui_mouse_world_y();
	
	int inside = 0;
	int r = 0;
	if(mx > x-w/2 && mx < x+w/2 && my > y-h/2 && my < y+h/2)
		inside = 1;
	
	if(inside)
		ui_set_hot_item(id);
	
	if(ui_hot_item() == id && ui_mouse_button(0))
	{
		ui_set_active_item(id);
		r = 1;
	}
	
	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
		ent->x = (int)ui_mouse_world_x();
		ent->y = (int)ui_mouse_world_y();
		ent->x = (ent->x/32)*32+16;
		ent->y = (ent->y/32)*32+16;
	}
	
	// raw rect around tilemap
	gfx_texture_set(-1);
	gfx_quads_begin();
	if(selected)
		gfx_quads_setcolor(1.0f, 0.5f, 0.5f, 0.95f);
	else if(ui_hot_item() == id)
		gfx_quads_setcolor(1.0f, 1.0f, 1.0f, 0.95f);
	else
		gfx_quads_setcolor(0.75f, 0.75f, 0.75f, 0.95f);
		
	gfx_quads_drawTL(x-w/2, y-w/2, w, h);
	gfx_quads_end();
	
    gfx_texture_set(font_texture);
	if(ent->type >= 0)
		gfx_quads_text(x-4, y-h/2-4, 24.0f, ent_types[ent->type].name);
	
	return r;
}


static int editor_reset()
{
	// delete all layers
	while(layers_count())
		layers_remove(layers_count()-1);
	
	tilemap_destroy(&brush);
	current_layer = 0;
	
	tilesets_clear();
	
	// init chooser
	static tile chooser_tiles[256];
	for(int i = 0; i < 256; i++)
	{
		chooser_tiles[i].index = i;
		chooser_tiles[i].flags = 0;
	}
	
	chooser.width = 16;
	chooser.height = 16;
	chooser.tiles = chooser_tiles;
	
	return 0;
}

void draw_editor_button(void *id, const char *text, int checked, float x, float y, float w, float h, void *extra)
{
    gfx_blend_normal();
    gfx_texture_set(-1);
    gfx_quads_begin(); 
    if(ui_hot_item() == id)
        gfx_quads_setcolor(1,1,1,1);
    else if(checked)
        gfx_quads_setcolor(0.75f,0.5f,0.5f,1);
    else
        gfx_quads_setcolor(0.5f,0.5f,0.5f,1);

    gfx_quads_drawTL(x,y,w,h);
    gfx_quads_end();
    gfx_pretty_text(x+1, y-1, 6.5f, text, -1);
}

static int editor_loadimage = -1;

static void editor_listdir_callback(const char *name, int is_dir, void *user)
{
	if(name[0] == '.') // skip this shit!
		return;
	
	int *y = (int*)user;
	if(ui_do_button((void*)(*y + 1), name, 0, 10, 10 + *y * 8, 100, 6, draw_editor_button, 0))
	{
		char buf[512];
		sprintf(buf, "tilesets/%s", name);
		
		IMAGE_INFO img;
		if(!gfx_load_png(&img, buf))
			return;
		
		tilesets_set_img(editor_loadimage, img.width, img.height, img.data);
		editor_loadimage = -1;
	}
	*y += 1;
}

static void editor_render_loadfile_dialog()
{
	gfx_clear(0.2f,0.2f,0.8f);
	// GUI coordsys
	gfx_mapscreen(0,0,400.0f,300.0f);
	
	int index = 0;
	fs_listdir("tilesets", editor_listdir_callback, &index);
	
	if(inp_key_pressed(KEY_ESC))
		editor_loadimage = -1;
}

static void editor_render()
{
	if(editor_loadimage != -1)
	{
		editor_render_loadfile_dialog();
		return;
	}
	
	// background color
	gfx_clear(0.2f,0.2f,0.8f);
	
	// world coordsys
	float zoom = world_zoom;
	gfx_mapscreen(world_offset_x,world_offset_y,world_offset_x+400.0f*zoom,world_offset_y+300.0f*zoom);

	for(int i = 0; i < layers_count(); i++)
	{
		layer *l = layers_get(i);
		
		gfx_texture_set(-1);
		if(l->tileset_id >= 0 && l->tileset_id < tilesets_count())
			gfx_texture_set(tilesets_get(l->tileset_id)->tex_id);
		
		if(editor_mode == 0)
		{
			if(l == layers_get_current())
			{
				// do current layer
				ui_do_tilemap(&l->tm, &l->tm, 0, 0, 0, 32.0f);
			}
			else if(l->visible)
			{
				// render layer
				render_tilemap(&l->tm, 0, 0, 32.0f);
			}
		}
		else
			render_tilemap(&l->tm, 0, 0, 32.0f);			
	}

	if(editor_mode == 1)
	{
		// ents mode
		for(int i = 0; i < ents_count(); i++)
		{
			if(ui_do_entity(ents_get(i), ents_get(i), i == editor_selected_ent))
				editor_selected_ent = i;
			
		}
	}
	
	// GUI coordsys
	gfx_mapscreen(0,0,400.0f,300.0f);

	// toolbox
	float toolbox_width = 50.0f;
	
	ui_do_frame(0, 0, toolbox_width, 300);
	
	if(editor_mode == 0)
	{
		float layerbox_x = 0;
		float layerbox_y = 0;
		int count = 1;
		int main_layer = -1;
		for(int i = 0; i < layers_count(); i++)
		{
			layer *l = layers_get(i);
			char buf[128];
			if(l->main_layer)
			{
				main_layer = i;
				sprintf(buf, "Main\n(%dx%d)", l->tm.width, l->tm.height);
				count = 1;
			}
			else
			{
				if(main_layer == -1)
					sprintf(buf, "Bg %d\n(%dx%d)", count, l->tm.width, l->tm.height);
				else
					sprintf(buf, "Fg %d\n(%dx%d)", count, l->tm.width, l->tm.height);
				count++;
			}
			
			// show / hide layer
			const char *text = " ";
			if(layers_get(i)->visible)
				text = "V";

			if(ui_do_button(&layers_get(i)->visible, text, 0, layerbox_x, layerbox_y+i*14, 6, 6, draw_editor_button, 0))
				layers_get(i)->visible = layers_get(i)->visible^1;
			
			// layer bytton
			if(ui_do_button(&layers_get(i)->tileset_id, buf, current_layer == i, layerbox_x+8, layerbox_y+i*14, toolbox_width-8, 12, draw_editor_button, 0))
				current_layer = i;
		}
		
		// draw buttons
		{
			static int push_button, pull_button;
			float y = 150;
			float x = 0;
			if(ui_do_button(&push_button, "push", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				current_layer = layers_moveup(current_layer);
			y += 7;
			
			if(ui_do_button(&pull_button, "pull", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				current_layer = layers_movedown(current_layer);
			y += 10;

			static int w_inc, w_dec;
			int resize_amount = 10;
			if(ui_do_button(&w_dec, "width-", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				tilemap_resize(&layers_get_current()->tm, layers_get_current()->tm.width-resize_amount, layers_get_current()->tm.height, resize_amount);
			y += 7;
			if(ui_do_button(&w_inc, "width+", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				tilemap_resize(&layers_get_current()->tm, layers_get_current()->tm.width+resize_amount, layers_get_current()->tm.height, resize_amount);
			y += 10;

			static int h_inc, h_dec;
			if(ui_do_button(&h_dec, "height-", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				tilemap_resize(&layers_get_current()->tm, layers_get_current()->tm.width, layers_get_current()->tm.height-resize_amount, resize_amount);
			y += 7;
			if(ui_do_button(&h_inc, "height+", 0, x, y, toolbox_width, 6, draw_editor_button, 0))
				tilemap_resize(&layers_get_current()->tm, layers_get_current()->tm.width, layers_get_current()->tm.height+resize_amount, resize_amount);
			y += 10;
		}


		float tilesetsbox_x = 0;
		float tilesetsbox_y = 230;
		for(int i = 0; i < tilesets_count(); i++)
		{
			char buf[128];
			sprintf(buf, "#%d %dx%d", i, tilesets_get(i)->img.width, tilesets_get(i)->img.height);
			if(ui_do_button(&tilesets_get(i)->img.width, "L", layers_get(current_layer)->tileset_id == i, tilesetsbox_x, tilesetsbox_y+i*7, 6, 6, draw_editor_button, 0))
			{
				// load image
				editor_loadimage = i;
			}

			if(ui_do_button(tilesets_get(i), buf, layers_get(current_layer)->tileset_id == i, tilesetsbox_x+16, tilesetsbox_y+i*7, toolbox_width-16, 6, draw_editor_button, 0))
			{
				// select tileset for layer
				dbg_msg("editor", "settings tileset %d=%d", current_layer, i);
				layers_get(current_layer)->tileset_id = i;
			}

			if(ui_do_button(&tilesets_get(i)->img.height, "D", layers_get(current_layer)->tileset_id == i, tilesetsbox_x+8, tilesetsbox_y+i*7, 6, 6, draw_editor_button, 0)
				&& (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL)))
			{
				dbg_msg("editor", "deleting tileset %d", i);
				tilesets_delete(i);
				i--;
			}
		}

		// (add) button for tilesets
		static int add_tileset;
		if(ui_do_button(&add_tileset, "(Add)", 0, tilesetsbox_x, tilesetsbox_y+tilesets_count()*7+3, toolbox_width, 6, draw_editor_button, 0))
			tilesets_new();
		
		if(brush.tiles != 0)
		{
			// right mouse button or C clears the brush
			if(ui_mouse_button(1) || inp_key_pressed('C'))
				tilemap_destroy(&brush);
		}
		
		if(inp_key_pressed(KEY_SPACE))
		{
			// render chooser
			float chooser_x = toolbox_width+10.0f;
			float chooser_y = 10.0f;
			
			
			gfx_texture_set(checker_texture);
			gfx_blend_normal();
			gfx_quads_begin();
			gfx_quads_setcolor(1.0f, 1.0f, 1.0f, 1.0f);
			gfx_quads_setsubset(0,0,32.0f, 32.0f);
			gfx_quads_drawTL(chooser_x, chooser_y, 16*16.0f, 16*16.0f);
			gfx_quads_end();
			gfx_blend_normal();	
				
			gfx_texture_set(-1);
			layer *l = layers_get_current();
			if(l && l->tileset_id >= 0 && l->tileset_id < tilesets_count())
				gfx_texture_set(tilesets_get(l->tileset_id)->tex_id);
			ui_do_tilemap(&chooser, &chooser, TILEMAPFLAG_READONLY|TILEMAPFLAG_UISPACE, chooser_x, chooser_y, 16.0f);
		}
	}
	else
	{
		int current_type = -1;
		if(editor_selected_ent >= 0 && editor_selected_ent < ents_count())
			current_type = ents_get(editor_selected_ent)->type;
		
		float y = 0;
		for(int i = 0; ent_types[i].name; i++)
		{
			if(ui_do_button(&ent_types[i], ent_types[i].name, current_type==i, 0, y, toolbox_width, 6, draw_editor_button, 0))
			{
				if(editor_selected_ent >= 0 && editor_selected_ent < ents_count())
					ents_get(editor_selected_ent)->type = i;
			}
			y += 8;
		}

		y += 8;
		static int add, del;
		if(ui_do_button(&add, "Add", 0, 0, y, toolbox_width, 6, draw_editor_button, 0))
		{
			int x = (int)(world_offset_x+400*zoom/2)/32*32+16;
			int y = (int)(world_offset_y+300*zoom/2)/32*32+16;
			ents_new(0, x, y);
		}
		
		y += 8;
		if(ui_do_button(&del, "Del", 0, 0, y, toolbox_width, 6, draw_editor_button, 0))
			ents_delete(editor_selected_ent);
	}
}

int editor_load(const char *filename)
{
	DATAFILE *df = datafile_load(filename);
	if(!df)
		return 0;
	
	// load tilesets
	{
		int start, count;
		datafile_get_type(df, MAPRES_IMAGE, &start, &count);
		for(int i = 0; i < count; i++)
		{
			mapres_image *img = (mapres_image *)datafile_get_item(df, start+i, 0, 0);
			void *data = datafile_get_data(df, img->image_data);
			int id = tilesets_new();
			void *data_cpy = mem_alloc(img->width*img->height*4, 1);
			mem_copy(data_cpy, data, img->width*img->height*4);
			tilesets_set_img(id, img->width, img->height, data_cpy);
		}
	}
	
	// load tilemaps
	{
		int start, num;
		datafile_get_type(df, MAPRES_TILEMAP, &start, &num);
		for(int t = 0; t < num; t++)
		{
			mapres_tilemap *tmap = (mapres_tilemap *)datafile_get_item(df, start+t,0,0);
			//unsigned char *data = (unsigned char *)datafile_get_data(df, tmap->data);
			
			layer *l = layers_get(layers_new(tmap->width, tmap->height));
			mem_copy(l->tm.tiles, datafile_get_data(df, tmap->data), tmap->width*tmap->height*2);
			l->tileset_id = tmap->image;
			l->main_layer = tmap->main;
			
			// force a main layer
			if(num == 1)
				l->main_layer = 1;
		}
	}
	
	// load entities
	{
		int type = -1;
		for(int i = 0; ent_types[i].name; i++)
		{
			if(ent_types[i].id == MAPRES_SPAWNPOINT)
			{
				type = i;
				break;
			}
		}
		
		int start, num;
		datafile_get_type(df, MAPRES_SPAWNPOINT, &start, &num);
		for(int t = 0; t < num; t++)
		{
			mapres_spawnpoint *sp = (mapres_spawnpoint *)datafile_get_item(df, start+t,0,0);
			ents_new(type, sp->x, sp->y);
		}
	}
	
	{
		int start, num;
		datafile_get_type(df, MAPRES_ITEM, &start, &num);
		for(int t = 0; t < num; t++)
		{
			mapres_item *it = (mapres_item *)datafile_get_item(df, start+t,0,0);
			
			int type = -1;
			for(int i = 0; ent_types[i].name; i++)
			{
				if(ent_types[i].id == MAPRES_ITEM && ent_types[i].item_id == it->type)
				{
					type = i;
					break;
				}
			}
		
			ents_new(type, it->x, it->y);
		}
	}	
	return 1;
}

int editor_save(const char *filename)
{
	DATAFILE_OUT *df = datafile_create(filename);

	// add tilesets
	for(int i = 0; i < tilesets_count(); i++)
	{
		mapres_image img;
		tileset *ts = tilesets_get(i);
		img.width = ts->img.width;
		img.height = ts->img.height;
		img.image_data = datafile_add_data(df, ts->img.width*ts->img.height*4, ts->img.data);
		datafile_add_item(df, MAPRES_IMAGE, i, sizeof(img), &img);
	}
	
	// add tilemaps
	for(int i = 0; i < layers_count(); i++)
	{
		layer *l = layers_get(i);
		mapres_tilemap tm;
		tm.image = l->tileset_id;
		tm.width = l->tm.width;
		tm.height = l->tm.height;
		tm.x = 0;
		tm.y = 0;
		tm.main = l->main_layer;
		tm.scale = 1<<16;
		tm.data = datafile_add_data(df, l->tm.width*l->tm.height*2, l->tm.tiles);
		datafile_add_item(df, MAPRES_TILEMAP, i, sizeof(tm), &tm);
	}
	
	// add collision
	char *collisiondata = 0x0;
	for(int i = 0; i < layers_count(); i++)
	{
		layer *l = layers_get(i);
		if(l->main_layer)
		{
			mapres_collision col;
			col.width = l->tm.width;
			col.height = l->tm.height;
			
			collisiondata = (char *)mem_alloc(col.width*col.height, 1);
			for(int y = 0, c = 0; y < col.height; y++)
				for(int x = 0; x < col.width; x++, c++)
				{
					if(l->tm.tiles[c].index)
						collisiondata[c] = 1;
					else
						collisiondata[c] = 0;
				}
			
			col.data_index = datafile_add_data(df, col.width*col.height, collisiondata);
			datafile_add_item(df, MAPRES_COLLISIONMAP, 0, sizeof(col), &col);
			break;
		}
	}
	
	// add spawnpoints
	for(int i = 0, id = 0; i < ents_count(); i++)
	{
		entity *ent = ents_get(i);
		if(ent->type >= 0 && ent_types[ent->type].id == MAPRES_SPAWNPOINT)
		{
			mapres_spawnpoint sp;
			sp.x = ent->x;
			sp.y = ent->y;
			sp.type = 0;
			datafile_add_item(df, MAPRES_SPAWNPOINT, id, sizeof(sp), &sp);
			id++;
		}
	}

	// add items
	for(int i = 0, id = 0; i < ents_count(); i++)
	{
		entity *ent = ents_get(i);
		if(ent->type >= 0 && ent_types[ent->type].id == MAPRES_ITEM)
		{
			mapres_item it;
			it.x = ent->x;
			it.y = ent->y;
			it.type = ent_types[ent->type].item_id;
			dbg_msg("editor", "i mapped=%d type=%x", ent->type, it.type);
			datafile_add_item(df, MAPRES_ITEM, id, sizeof(it), &it);
			id++;
		}
	}
	
	// finish and clean up
	datafile_finish(df);
	mem_free(collisiondata);
	
	return 0;
}

static int editor_loop()
{
	int mouse_x = 0;
	int mouse_y = 0;
	
	//input::set_mouse_mode(input::mode_relative);
	
	while(!(inp_key_pressed(KEY_LCTRL) && inp_key_pressed('Q')))
	{	
		// update input
		inp_update();
		
		// handle mouse movement
		float mx, my, mwx, mwy;
		int rx, ry;
		{
			inp_mouse_relative(&rx, &ry);
			mouse_x += rx;
			mouse_y += ry;
			if(mouse_x < 0) mouse_x = 0;
			if(mouse_y < 0) mouse_y = 0;
			if(mouse_x > gfx_screenwidth()) mouse_x = gfx_screenwidth();
			if(mouse_y > gfx_screenheight()) mouse_y = gfx_screenheight();

			// update the ui
			mx = (mouse_x/(float)gfx_screenwidth())*400.0f;
			my = (mouse_y/(float)gfx_screenheight())*300.0f;
			mwx = world_offset_x+mx*world_zoom; // adjust to zoom and offset
			mwy = world_offset_y+my*world_zoom; // adjust to zoom and offset
			
			int buttons = 0;
			if(inp_key_pressed(KEY_MOUSE_1)) buttons |= 1;
			if(inp_key_pressed(KEY_MOUSE_2)) buttons |= 2;
			if(inp_key_pressed(KEY_MOUSE_3)) buttons |= 4;
			
			ui_update(mx,my,mwx,mwy,buttons);
		}
		
		//
		editor_render();

		if(inp_key_pressed(KEY_LALT) || inp_key_pressed(KEY_RALT))
		{
			static int moveid;
			ui_set_hot_item(&moveid);
			if(inp_key_pressed(KEY_MOUSE_1))
			{
				world_offset_x -= rx*2;
				world_offset_y -= ry*2;
			}
		}
		
		
		// render butt ugly mouse cursor
		gfx_texture_set(-1);
		gfx_quads_begin();
		gfx_quads_setcolor(0,0,0,1);
		gfx_quads_draw_freeform(mx,my,mx,my,
								mx+7,my,
								mx,my+7);
		gfx_quads_setcolor(1,1,1,1);
		gfx_quads_draw_freeform(mx+1,my+1,mx+1,my+1,
								mx+5,my+1,
								mx+1,my+5);
		gfx_quads_end();
		
		// swap the buffers
		gfx_swap();
		
		//
		/*
		if(inp_key_pressed(KEY_F1))
			input::set_mouse_mode(input::mode_absolute);
		if(inp_key_pressed(KEY_F2))
			input::set_mouse_mode(input::mode_relative);
			*/

		// mode switch
		if(inp_key_down(KEY_TAB))
			editor_mode ^= 1;
		
		// zoom in
		if(inp_key_down(KEY_KP_ADD))
		{
			world_zoom--;
			if(world_zoom < 3)
				world_zoom = 3;
		}
		
		// zoom out
		if(inp_key_down(KEY_KP_SUBTRACT))
		{
			world_zoom++;
			if(world_zoom > 8)
				world_zoom = 8;
		}
		
		if(inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))
		{
			if(inp_key_down('L'))
			{
				int w = 50, h = 50;
				for(int i = 0; i < layers_count(); i++)
				{
					layer *l = layers_get(i);	
					if(l->main_layer)
					{
						w = l->tm.width;
						h = l->tm.height;
						break;
					}
				}					
				// copy main layer size
				layers_new(w, h);
			}
			
			if(inp_key_down('S'))
			{
				dbg_msg("editor", "save");
				editor_save(editor_filename);
			}

		}
		
		if(inp_key_down(KEY_F5))
		{
			dbg_msg("editor", "quick save");
			editor_save("quicksave.map");
		}
		
		if(inp_key_down(KEY_F8))
		{
			dbg_msg("editor", "quick load");
			int s = current_layer;
			editor_reset();
			editor_load("quicksave.map");
			current_layer = s;
			if(current_layer >= layers_count())
				current_layer = layers_count();
				
		}
		
		// be nice
		thread_sleep(1);
	}
	
	return 0;
}

extern void modmenu_init();

extern "C" int editor_main(int argc, char **argv)
{
	dbg_msg("editor", "starting...");
#ifdef CONF_PLATFORM_MACOSX	
	config_load("~/teewars");
#else
	config_load("default.cfg");
#endif
	
	// parse arguments
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] == 0 && argc - i > 1)
		{
			// -e NAME
			i++;
			editor_filename = argv[i];
		}
	}
	
	if(!editor_filename)
	{
		dbg_msg("editor", "no filename given");
		return -1;
	}
	
	if(!gfx_init())
		return -1;
	
	modmenu_init();
	
	// reset and start
	font_texture = gfx_load_texture("data/debug_font.png");
	checker_texture = gfx_load_texture("data/checker.png");
	editor_reset();

	// load or new
	if(!editor_load(editor_filename))
	{
		layer *l = layers_get(layers_new(50, 50));
		l->main_layer = 1;
	}
	
	editor_loop();
	
	return 0;
}
