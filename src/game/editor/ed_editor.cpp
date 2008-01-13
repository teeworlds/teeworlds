/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
	#include <engine/e_system.h>
	#include <engine/e_interface.h>
	#include <engine/e_datafile.h>
	#include <engine/e_config.h>
}

#include <game/client/gc_map_image.h>
#include <game/client/gc_ui.h>
#include <game/g_game.h>
#include <game/client/gc_render.h>

#include "ed_editor.hpp"

static int checker_texture = 0;
static int background_texture = 0;
static int cursor_texture = 0;
static int entities_texture = 0;

// backwards compatiblity
class mapres_image
{
public:
	int width;
	int height;
	int image_data;
};


struct mapres_tilemap
{
	int image;
	int width;
	int height;
	int x, y;
	int scale;
	int data;
	int main;
};

enum
{
	MAPRES_REGISTERED=0x8000,
	MAPRES_IMAGE=0x8001,
	MAPRES_TILEMAP=0x8002,
	MAPRES_COLLISIONMAP=0x8003,
	MAPRES_TEMP_THEME=0x8fff,
};



EDITOR editor;

LAYERGROUP::LAYERGROUP()
{
	name = "";
	visible = true;
	game_group = false;
	offset_x = 0;
	offset_y = 0;
	parallax_x = 100;
	parallax_y = 100;
}

LAYERGROUP::~LAYERGROUP()
{
	clear();
}

void LAYERGROUP::convert(RECT *rect)
{
	rect->x += offset_x;
	rect->y += offset_y;
}

void LAYERGROUP::mapping(float *points)
{
	mapscreen_to_world(
		editor.world_offset_x, editor.world_offset_y,
		parallax_x/100.0f, parallax_y/100.0f,
		offset_x, offset_y,
		gfx_screenaspect(), editor.world_zoom, points);
		
	points[0] += editor.editor_offset_x;
	points[1] += editor.editor_offset_y;
	points[2] += editor.editor_offset_x;
	points[3] += editor.editor_offset_y;
}

void LAYERGROUP::mapscreen()
{
	float points[4];
	mapping(points);
	gfx_mapscreen(points[0], points[1], points[2], points[3]);
}

void LAYERGROUP::render()
{
	mapscreen();
	
	for(int i = 0; i < layers.len(); i++)
	{
		if(layers[i]->visible && layers[i] != editor.game_layer)
			layers[i]->render();
	}
}

bool LAYERGROUP::is_empty() const { return layers.len() == 0; }
void LAYERGROUP::clear() { layers.deleteall(); }
void LAYERGROUP::add_layer(LAYER *l) { layers.add(l); }

void LAYERGROUP::delete_layer(int index)
{
	if(index < 0 || index >= layers.len()) return;
	delete layers[index];
	layers.removebyindex(index);
}	

void LAYERGROUP::get_size(float *w, float *h)
{
	*w = 0; *h = 0;
	for(int i = 0; i < layers.len(); i++)
	{
		float lw, lh;
		layers[i]->get_size(&lw, &lh);
		*w = max(*w, lw);
		*h = max(*h, lh);
	}
}


int LAYERGROUP::swap_layers(int index0, int index1)
{
	if(index0 < 0 || index0 >= layers.len()) return index0;
	if(index1 < 0 || index1 >= layers.len()) return index0;
	if(index0 == index1) return index0;
	swap(layers[index0], layers[index1]);
	return index1;
}	

/********************************************************
 OTHER
*********************************************************/
static void ui_draw_rect(const RECT *r, vec4 color, int corners, float rounding)
{
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(color.r, color.g, color.b, color.a);
	draw_round_rect_ext(r->x,r->y,r->w,r->h,rounding*ui_scale(), corners);
	gfx_quads_end();
}

// copied from gc_menu.cpp, should be more generalized
extern int ui_do_edit_box(void *id, const RECT *rect, char *str, int str_size, bool hidden=false);

static vec4 get_button_color(const void *id, int checked)
{
	if(checked < 0)
		return vec4(0,0,0,0.5f);
		
	if(checked > 0)
	{
		if(ui_hot_item() == id)
			return vec4(1,0,0,0.75f);
		return vec4(1,0,0,0.5f);
	}
	
	if(ui_hot_item() == id)
		return vec4(1,1,1,0.75f);
	return vec4(1,1,1,0.5f);
}

static void draw_editor_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_ALL, 5.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_l(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_L, 5.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_m(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), 0, 5.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_r(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_R, 5.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_inc_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_R, 5.0f);
	ui_do_label(r, ">", 10, 0, -1);
}

static void draw_dec_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_L, 5.0f);
	ui_do_label(r, "<", 10, 0, -1);
}



static void render_background(RECT view, int texture, float size, float brightness)
{
	gfx_texture_set(texture);
	gfx_blend_normal();
	gfx_quads_begin();
	gfx_setcolor(brightness,brightness,brightness,1.0f);
	gfx_quads_setsubset(0,0, view.w/size, view.h/size);
	gfx_quads_drawTL(view.x, view.y, view.w, view.h);
	gfx_quads_end();
}


static int selected_layer = 0;
static int selected_group = 0;
static int selected_quad = -1;
int selected_points = 0;
static int selected_envelope = 0;

static LAYERGROUP brush;
static LAYER_TILES tileset_picker(16, 16);

static int ui_do_value_selector(void *id, RECT *r, const char *label, int current, int min, int max, float scale)
{
    /* logic */
    static float value;
    int ret = 0;
    int inside = ui_mouse_inside(r);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
		{
			if(inside)
				ret = 1;
			editor.lock_mouse = false;
			ui_set_active_item(0);
		}
		else
		{
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
				value += editor.mouse_delta_x*0.05f;
			else
				value += editor.mouse_delta_x;
			
			if(fabs(value) > scale)
			{
				int count = (int)(value/scale);
				value = fmod(value, scale);
				current += count;
				if(current < min)
					current = min;
				if(current > max)
					current = max;
			}
		}
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			editor.lock_mouse = true;
			value = 0;
			ui_set_active_item(id);
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	// render
	char buf[128];
	sprintf(buf, "%s %d", label, current);
	ui_draw_rect(r, get_button_color(id, 0), CORNER_ALL, 5.0f);
	ui_do_label(r, buf, 12, 0, -1);
	return current;
}


LAYERGROUP *EDITOR::get_selected_group()
{
	if(selected_group >= 0 && selected_group < editor.map.groups.len())
		return editor.map.groups[selected_group];
	return 0x0;
}

LAYER *EDITOR::get_selected_layer(int index)
{
	LAYERGROUP *group = get_selected_group();
	if(!group)
		return 0x0;

	if(selected_layer >= 0 && selected_layer < editor.map.groups[selected_group]->layers.len())
		return group->layers[selected_layer];
	return 0x0;
}

LAYER *EDITOR::get_selected_layer_type(int index, int type)
{
	LAYER *p = get_selected_layer(index);
	if(p && p->type == type)
		return p;
	return 0x0;
}

QUAD *EDITOR::get_selected_quad()
{
	LAYER_QUADS *ql = (LAYER_QUADS *)get_selected_layer_type(0, LAYERTYPE_QUADS);
	if(!ql)
		return 0;
	if(selected_quad >= 0 && selected_quad < ql->quads.len())
		return &ql->quads[selected_quad];
	return 0;
}


static void do_toolbar(RECT toolbar)
{
	RECT button;

	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_out_button = 0;
	if(ui_do_button(&zoom_out_button, "ZO", 0, &button, draw_editor_button_l, "[NumPad-] Zoom out") || inp_key_down(KEY_KP_SUBTRACT))
		editor.zoom_level += 50;
		
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_normal_button = 0;
	if(ui_do_button(&zoom_normal_button, "1:1", 0, &button, draw_editor_button_m, "[NumPad*] Zoom to normal and remove editor offset") || inp_key_down(KEY_KP_MULTIPLY))
	{
		editor.editor_offset_x = 0;
		editor.editor_offset_y = 0;
		editor.zoom_level = 100;
	}
		
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_in_button = 0;
	if(ui_do_button(&zoom_in_button, "ZI", 0, &button, draw_editor_button_r, "[NumPad+] Zoom in") || inp_key_down(KEY_KP_ADD))
		editor.zoom_level -= 50;
	
	if(editor.zoom_level < 50)
		editor.zoom_level = 50;
	editor.world_zoom = editor.zoom_level/100.0f;

	ui_vsplit_l(&toolbar, 10.0f, &button, &toolbar);


	// brush manipulation
	{	
		int enabled = brush.is_empty()?-1:0;
		
		// flip buttons
		ui_vsplit_l(&toolbar, 20.0f, &button, &toolbar);
		static int flipx_button = 0;
		if(ui_do_button(&flipx_button, "^X", enabled, &button, draw_editor_button_l, "Flip brush horizontal"))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_x();
		}
			
		ui_vsplit_l(&toolbar, 20.0f, &button, &toolbar);
		static int flipy_button = 0;
		if(ui_do_button(&flipy_button, "^Y", enabled, &button, draw_editor_button_r, "Flip brush vertical"))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_y();
		}
	}

	// quad manipulation
	{
		// do add button
		ui_vsplit_l(&toolbar, 10.0f, &button, &toolbar);
		ui_vsplit_l(&toolbar, 60.0f, &button, &toolbar);
		static int new_button = 0;
		
		LAYER_QUADS *qlayer = (LAYER_QUADS *)editor.get_selected_layer_type(0, LAYERTYPE_QUADS);
		LAYER_TILES *tlayer = (LAYER_TILES *)editor.get_selected_layer_type(0, LAYERTYPE_TILES);
		if(ui_do_button(&new_button, "Add Quad", qlayer?0:-1, &button, draw_editor_button, "Adds a new quad"))
		{
			if(qlayer)
			{
				float mapping[4];
				LAYERGROUP *g = editor.get_selected_group();
				g->mapping(mapping);
				int add_x = f2fx(mapping[0] + (mapping[2]-mapping[0])/2);
				int add_y = f2fx(mapping[1] + (mapping[3]-mapping[1])/2);
				
				QUAD *q = qlayer->new_quad();
				for(int i = 0; i < 5; i++)
				{
					q->points[i].x += add_x;
					q->points[i].y += add_y;
				}
			}
		}

		ui_vsplit_l(&toolbar, 10.0f, &button, &toolbar);
		ui_vsplit_l(&toolbar, 60.0f, &button, &toolbar);
		static int sq_button = 0;
		if(ui_do_button(&sq_button, "Sq. Quad", editor.get_selected_quad()?0:-1, &button, draw_editor_button, "Squares the current quad"))
		{
			QUAD *q = editor.get_selected_quad();
			if(q)
			{
				int top = q->points[0].y;
				int left = q->points[0].x;
				int bottom = q->points[0].y;
				int right = q->points[0].x;
				
				for(int k = 1; k < 4; k++)
				{
					if(q->points[k].y < top) top = q->points[k].y;
					if(q->points[k].x < left) left = q->points[k].x;
					if(q->points[k].y > bottom) bottom = q->points[k].y;
					if(q->points[k].x > right) right = q->points[k].x;
				}
				
				q->points[0].x = left; q->points[0].y = top;
				q->points[1].x = right; q->points[1].y = top;
				q->points[2].x = left; q->points[2].y = bottom;
				q->points[3].x = right; q->points[3].y = bottom;
			}
		}

		ui_vsplit_l(&toolbar, 20.0f, &button, &toolbar);
		ui_vsplit_l(&toolbar, 60.0f, &button, &toolbar);
		bool in_gamegroup = editor.game_group->layers.find(tlayer) != -1;
		static int col_button = 0;
		if(ui_do_button(&col_button, "Make Col", (in_gamegroup&&tlayer)?0:-1, &button, draw_editor_button, "Constructs collision from the current layer"))
		{
			LAYER_TILES *gl = editor.game_layer;
			int w = min(gl->width, tlayer->width);
			int h = min(gl->height, tlayer->height);
			dbg_msg("", "w=%d h=%d", w, h);
			for(int y = 0; y < h; y++)
				for(int x = 0; x < w; x++)
				{
					if(gl->tiles[y*gl->width+x].index <= TILE_SOLID)
						gl->tiles[y*gl->width+x].index = tlayer->tiles[y*tlayer->width+x].index?TILE_SOLID:TILE_AIR;
				}
		}
	}
}

static void rotate(POINT *center, POINT *point, float rotation)
{
	int x = point->x - center->x;
	int y = point->y - center->y;
	point->x = (int)(x * cosf(rotation) - y * sinf(rotation) + center->x);
	point->y = (int)(x * sinf(rotation) + y * cosf(rotation) + center->y);
}

static void do_quad(QUAD *q, int index)
{
	enum
	{
		OP_NONE=0,
		OP_MOVE_ALL,
		OP_MOVE_PIVOT,
		OP_ROTATE,
	};
	
	// some basic values
	void *id = &q->points[4]; // use pivot addr as id
	static POINT rotate_points[4];
	static float last_wx;
	static float last_wy;
	static int operation = OP_NONE;
	static float rotate_angle = 0;
	float wx = ui_mouse_world_x();
	float wy = ui_mouse_world_y();
	
	// get pivot
	float center_x = fx2f(q->points[4].x);
	float center_y = fx2f(q->points[4].y);

	float dx = (center_x - wx);
	float dy = (center_y - wy);
	if(dx*dx+dy*dy < 10*10)
		ui_set_hot_item(id);

	// draw selection background	
	if(selected_quad == index)
	{
		gfx_setcolor(0,0,0,1);
		gfx_quads_draw(center_x, center_y, 7.0f, 7.0f);
	}
	
	if(ui_active_item() == id)
	{
		// check if we only should move pivot
		if(operation == OP_MOVE_PIVOT)
		{
			q->points[4].x += f2fx(wx-last_wx);
			q->points[4].y += f2fx(wy-last_wy);
		}
		else if(operation == OP_MOVE_ALL)
		{
			// move all points including pivot
			for(int v = 0; v < 5; v++)
			{
				q->points[v].x += f2fx(wx-last_wx);
				q->points[v].y += f2fx(wy-last_wy);
			}
		}
		else if(operation == OP_ROTATE)
		{
			for(int v = 0; v < 4; v++)
			{
				q->points[v] = rotate_points[v];
				rotate(&q->points[4], &q->points[v], rotate_angle);
			}
		}
		
		rotate_angle += (editor.mouse_delta_x) * 0.002f;
		last_wx = wx;
		last_wy = wy;
		
		if(!ui_mouse_button(0))
		{
			editor.lock_mouse = false;
			operation = OP_NONE;
			ui_set_active_item(0);
		}

		gfx_setcolor(1,1,1,1);
	}
	else if(ui_hot_item() == id)
	{
		gfx_setcolor(1,1,1,1);
		editor.tooltip = "Left mouse button to move. Hold shift to move pivot. Hold ctrl to rotate";
		
		if(ui_mouse_button(0))
		{
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
				operation = OP_MOVE_PIVOT;
			else if(inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))
			{
				editor.lock_mouse = true;
				operation = OP_ROTATE;
				rotate_angle = 0;
				rotate_points[0] = q->points[0];
				rotate_points[1] = q->points[1];
				rotate_points[2] = q->points[2];
				rotate_points[3] = q->points[3];
			}
			else
				operation = OP_MOVE_ALL;
				
			ui_set_active_item(id);
			selected_quad = index;
			editor.props = PROPS_QUAD;
			last_wx = wx;
			last_wy = wy;
		}
	}
	else
		gfx_setcolor(0,1,0,1);

	gfx_quads_draw(center_x, center_y, 5.0f, 5.0f);
}

static void do_quad_point(QUAD *q, int quad_index, int v)
{
	void *id = &q->points[v];

	float wx = ui_mouse_world_x();
	float wy = ui_mouse_world_y();
	
	float px = fx2f(q->points[v].x);
	float py = fx2f(q->points[v].y);
	
	float dx = (px - wx);
	float dy = (py - wy);
	if(dx*dx+dy*dy < 10*10)
		ui_set_hot_item(id);

	// draw selection background	
	if(selected_quad == quad_index && selected_points&(1<<v))
	{
		gfx_setcolor(0,0,0,1);
		gfx_quads_draw(px, py, 7.0f, 7.0f);
	}
	
	enum
	{
		OP_NONE=0,
		OP_MOVEPOINT,
		OP_MOVEUV
	};
	
	static bool moved;
	static int operation = OP_NONE;

	if(ui_active_item() == id)
	{
		float dx = editor.mouse_delta_wx;
		float dy = editor.mouse_delta_wy;
		if(!moved)
		{
			if(dx*dx+dy*dy > 0.5f)
				moved = true;
		}
		
		if(moved)
		{
			if(operation == OP_MOVEPOINT)
			{
				for(int m = 0; m < 4; m++)
					if(selected_points&(1<<m))
					{
						q->points[m].x += f2fx(dx);
						q->points[m].y += f2fx(dy);
					}
			}
			else if(operation == OP_MOVEUV)
			{
				for(int m = 0; m < 4; m++)
					if(selected_points&(1<<m))
					{
						q->texcoords[m].x += f2fx(dx*0.001f);
						q->texcoords[m].y += f2fx(dy*0.001f);
					}
			}
		}
		
		if(!ui_mouse_button(0))
		{
			if(!moved)
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					selected_points ^= 1<<v;
				else
					selected_points = 1<<v;
					
				editor.props = PROPS_QUAD_POINT;
			}
			editor.lock_mouse = false;
			ui_set_active_item(0);
		}

		gfx_setcolor(1,1,1,1);
	}
	else if(ui_hot_item() == id)
	{
		gfx_setcolor(1,1,1,1);
		editor.tooltip = "Left mouse button to move. Hold shift to move the texture.";
		
		if(ui_mouse_button(0))
		{
			ui_set_active_item(id);
			moved = false;
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
			{
				operation = OP_MOVEUV;
				editor.lock_mouse = true;
			}
			else
				operation = OP_MOVEPOINT;
				
			if(!(selected_points&(1<<v)))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					selected_points |= 1<<v;
				else
					selected_points = 1<<v;
				moved = true;
			}
															
			editor.props = PROPS_QUAD_POINT;
			selected_quad = quad_index;
		}
	}
	else
		gfx_setcolor(1,0,0,1);
	
	gfx_quads_draw(px, py, 5.0f, 5.0f);	
}

static void do_map_editor(RECT view, RECT toolbar)
{
	// do the toolbar
	if(editor.gui_active)
		do_toolbar(toolbar);
	
	ui_clip_enable(&view);
	
	bool show_picker = inp_key_pressed(KEY_SPACE) != 0;

	// render all good stuff
	if(!show_picker)
	{
		for(int g = 0; g < editor.map.groups.len(); g++)
		{
			if(editor.map.groups[g]->visible)
				editor.map.groups[g]->render();
		}
		
		// render the game above everything else
		if(editor.game_group->visible && editor.game_layer->visible)
		{
			editor.game_group->mapscreen();
			editor.game_layer->render();
		}
	}

	static void *editor_id = (void *)&editor_id;
	int inside = ui_mouse_inside(&view);

	// fetch mouse position		
	float wx = ui_mouse_world_x();
	float wy = ui_mouse_world_y();
	float mx = ui_mouse_x();
	float my = ui_mouse_y();
	
	static float start_wx = 0;
	static float start_wy = 0;
	static float start_mx = 0;
	static float start_my = 0;
	
	enum
	{
		OP_NONE=0,
		OP_BRUSH_GRAB,
		OP_BRUSH_DRAW,
		OP_PAN_WORLD,
		OP_PAN_EDITOR,
	};

	// remap the screen so it can display the whole tileset
	if(show_picker)
	{
		RECT screen = *ui_screen();
		float size = 32.0*16.0f;
		float w = size*(screen.w/view.w);
		float h = size*(screen.h/view.h);
		float x = -(view.x/screen.w)*w;
		float y = -(view.y/screen.h)*h;
		wx = x+w*mx/screen.w;
		wy = y+h*my/screen.h;
		gfx_mapscreen(x, y, x+w, y+h);
		LAYER_TILES *t = (LAYER_TILES *)editor.get_selected_layer_type(0, LAYERTYPE_TILES);
		if(t)
		{
			tileset_picker.image = t->image;
			tileset_picker.tex_id = t->tex_id;
			tileset_picker.render();
		}
	}
	
	static int operation = OP_NONE;
	
	// draw layer borders
	LAYER *edit_layers[16];
	int num_edit_layers = 0;
	num_edit_layers = 0;
	
	if(show_picker)
	{
		edit_layers[0] = &tileset_picker;
		num_edit_layers++;
	}
	else
	{
		edit_layers[0] = editor.get_selected_layer(0);
		if(edit_layers[0])
			num_edit_layers++;

		LAYERGROUP *g = editor.get_selected_group();
		if(g)
		{
			g->mapscreen();
			
			for(int i = 0; i < num_edit_layers; i++)
			{
				if(edit_layers[i]->type != LAYERTYPE_TILES)
					continue;
					
				float w, h;
				edit_layers[i]->get_size(&w, &h);

				gfx_texture_set(-1);
				gfx_lines_begin();
				gfx_lines_draw(0,0, w,0);
				gfx_lines_draw(w,0, w,h);
				gfx_lines_draw(w,h, 0,h);
				gfx_lines_draw(0,h, 0,0);
				gfx_lines_end();
			}
		}
	}
		
	if(inside)
	{
		ui_set_hot_item(editor_id);
		
		// do global operations like pan and zoom
		if(ui_active_item() == 0 && ui_mouse_button(0))
		{
			start_wx = wx;
			start_wy = wy;
			start_mx = mx;
			start_my = my;
					
			if(inp_key_pressed(KEY_LALT))
			{
				if(inp_key_pressed(KEY_LSHIFT))
					operation = OP_PAN_EDITOR;
				else
					operation = OP_PAN_WORLD;
				ui_set_active_item(editor_id);
			}
		}

		// brush editing
		{
			if(brush.is_empty())
				editor.tooltip = "Use left mouse button to drag and create a brush.";
			else
				editor.tooltip = "Use left mouse button to paint with the brush. Right button clears the brush.";

			if(ui_active_item() == editor_id)
			{
				RECT r;
				r.x = start_wx;
				r.y = start_wy;
				r.w = wx-start_wx;
				r.h = wy-start_wy;
				if(r.w < 0)
				{
					r.x += r.w;
					r.w = -r.w;
				}

				if(r.h < 0)
				{
					r.y += r.h;
					r.h = -r.h;
				}
				
				if(operation == OP_BRUSH_DRAW)
				{						
					if(!brush.is_empty())
					{
						// draw with brush
						for(int k = 0; k < num_edit_layers; k++)
						{
							if(edit_layers[k]->type == brush.layers[0]->type)
								edit_layers[k]->brush_draw(brush.layers[0], wx, wy);
						}
					}
				}
				else if(operation == OP_BRUSH_GRAB)
				{
					if(!ui_mouse_button(0))
					{
						// grab brush
						dbg_msg("editor", "grabbing %f %f %f %f", r.x, r.y, r.w, r.h);
						
						// TODO: do all layers
						int grabs = 0;
						for(int k = 0; k < num_edit_layers; k++)
							grabs += edit_layers[k]->brush_grab(&brush, r);
						if(grabs == 0)
							brush.clear();
					}
					else
					{
						editor.map.groups[selected_group]->mapscreen();
						for(int k = 0; k < num_edit_layers; k++)
							edit_layers[k]->brush_selecting(r);
						gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
					}
				}
			}
			else
			{
				if(ui_mouse_button(1))
					brush.clear();
					
				if(ui_mouse_button(0) && operation == OP_NONE)
				{
					ui_set_active_item(editor_id);
					
					if(brush.is_empty())
						operation = OP_BRUSH_GRAB;
					else
					{
						operation = OP_BRUSH_DRAW;
						for(int k = 0; k < num_edit_layers; k++)
						{
							if(edit_layers[k]->type == brush.layers[0]->type)
								edit_layers[k]->brush_place(brush.layers[0], wx, wy);
						}
						
					}
				}
				
				if(!brush.is_empty())
				{
					brush.offset_x = -(int)wx;
					brush.offset_y = -(int)wy;
					for(int i = 0; i < brush.layers.len(); i++)
					{
						if(brush.layers[i]->type == LAYERTYPE_TILES)
						{
							brush.offset_x = -(int)(wx/32.0f)*32;
							brush.offset_y = -(int)(wy/32.0f)*32;
							break;
						}
					}
				
					LAYERGROUP *g = editor.get_selected_group();
					brush.offset_x += g->offset_x;
					brush.offset_y += g->offset_y;
					brush.parallax_x = g->parallax_x;
					brush.parallax_y = g->parallax_y;
					brush.render();
					float w, h;
					brush.get_size(&w, &h);
					
					gfx_texture_set(-1);
					gfx_lines_begin();
					gfx_lines_draw(0,0, w,0);
					gfx_lines_draw(w,0, w,h);
					gfx_lines_draw(w,h, 0,h);
					gfx_lines_draw(0,h, 0,0);
					gfx_lines_end();
					
				}
			}
		}
		
		if(!show_picker && brush.is_empty())
		{
			// fetch layers
			LAYERGROUP *g = editor.get_selected_group();
			if(g)
				g->mapscreen();
				
			for(int k = 0; k < num_edit_layers; k++)
			{
				if(edit_layers[k]->type == LAYERTYPE_QUADS)
				{
					LAYER_QUADS *layer = (LAYER_QUADS *)edit_layers[k];
	
					gfx_texture_set(-1);
					gfx_quads_begin();				
					for(int i = 0; i < layer->quads.len(); i++)
					{
						for(int v = 0; v < 4; v++)
							do_quad_point(&layer->quads[i], i, v);
							
						do_quad(&layer->quads[i], i);
					}
					gfx_quads_end();
				}
			}
			
			gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
		}		
		
		// do panning
		if(ui_active_item() == editor_id)
		{
			if(operation == OP_PAN_WORLD)
			{
				editor.world_offset_x -= editor.mouse_delta_x*editor.world_zoom;
				editor.world_offset_y -= editor.mouse_delta_y*editor.world_zoom;
			}
			else if(operation == OP_PAN_EDITOR)
			{
				editor.editor_offset_x -= editor.mouse_delta_x*editor.world_zoom;
				editor.editor_offset_y -= editor.mouse_delta_y*editor.world_zoom;
			}
		}

		
		// release mouse
		if(!ui_mouse_button(0))
		{
			operation = OP_NONE;
			ui_set_active_item(0);
		}
		
	}

	// render screen sizes	
	if(editor.proof_borders)
	{
		LAYERGROUP *g = editor.game_group;
		g->mapscreen();
		
		gfx_texture_set(-1);
		gfx_lines_begin();
		
		float last_points[4];
		float start = 1.0f; //9.0f/16.0f;
		float end = 16.0f/9.0f;
		const int num_steps = 20;
		for(int i = 0; i <= num_steps; i++)
		{
			float points[4];
			float aspect = start + (end-start)*(i/(float)num_steps);
			
			mapscreen_to_world(
				editor.world_offset_x, editor.world_offset_y,
				1.0f, 1.0f, 0.0f, 0.0f, aspect, 1.0f, points);
			
			if(i == 0)
			{
				gfx_lines_draw(points[0], points[1], points[2], points[1]);
				gfx_lines_draw(points[0], points[3], points[2], points[3]);
			}

			if(i != 0)
			{
				gfx_lines_draw(points[0], points[1], last_points[0], last_points[1]);
				gfx_lines_draw(points[2], points[1], last_points[2], last_points[1]);
				gfx_lines_draw(points[0], points[3], last_points[0], last_points[3]);
				gfx_lines_draw(points[2], points[3], last_points[2], last_points[3]);
			}

			if(i == num_steps)
			{
				gfx_lines_draw(points[0], points[1], points[0], points[3]);
				gfx_lines_draw(points[2], points[1], points[2], points[3]);
			}
			
			mem_copy(last_points, points, sizeof(points));
		}

		if(0)
		{
			gfx_setcolor(1.0f,0,0,1);		
			for(int i = 0; i < 4; i++)
			{
				float points[4];
				float aspects[] = {4.0f/3.0f, 5.0f/4.0f, 16.0f/10.0f, 16.0f/9.0f};
				float aspect = aspects[i];
				
				mapscreen_to_world(
					editor.world_offset_x, editor.world_offset_y,
					1.0f, 1.0f, 0.0f, 0.0f, aspect, 1.0f, points);
				
				RECT r;
				r.x = points[0];
				r.y = points[1];
				r.w = points[2]-points[0];
				r.h = points[3]-points[1];
				
				gfx_lines_draw(r.x, r.y, r.x+r.w, r.y);
				gfx_lines_draw(r.x+r.w, r.y, r.x+r.w, r.y+r.h);
				gfx_lines_draw(r.x+r.w, r.y+r.h, r.x, r.y+r.h);
				gfx_lines_draw(r.x, r.y+r.h, r.x, r.y);
			}
		}
			
		gfx_lines_end();
	}
	
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	ui_clip_disable();
}

int EDITOR::do_properties(RECT *toolbox, PROPERTY *props, int *ids, int *new_val)
{
	int change = -1;

	for(int i = 0; props[i].name; i++)
	{
		RECT slot;
		ui_hsplit_t(toolbox, 13.0f, &slot, toolbox);
		RECT label, shifter;
		ui_vsplit_mid(&slot, &label, &shifter);
		ui_hmargin(&shifter, 1.0f, &shifter);
		ui_do_label(&label, props[i].name, 10.0f, -1, -1);
		
		if(props[i].type == PROPTYPE_INT_STEP)
		{
			RECT inc, dec;
			char buf[64];
			
			ui_vsplit_r(&shifter, 10.0f, &shifter, &inc);
			ui_vsplit_l(&shifter, 10.0f, &dec, &shifter);
			sprintf(buf, "%d", props[i].value);
			ui_draw_rect(&shifter, vec4(1,1,1,0.5f), 0, 0.0f);
			ui_do_label(&shifter, buf, 10.0f, 0, -1);
			
			if(ui_do_button(&ids[i], 0, 0, &dec, draw_dec_button, "Decrease"))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					*new_val = props[i].value-5;
				else
					*new_val = props[i].value-1;
				change = i;
			}
			if(ui_do_button(((char *)&ids[i])+1, 0, 0, &inc, draw_inc_button, "Increase"))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					*new_val = props[i].value+5;
				else
					*new_val = props[i].value+1;
				change = i;
			}
		}
		else if(props[i].type == PROPTYPE_INT_SCROLL)
		{
			int new_value = ui_do_value_selector(&ids[i], &shifter, "", props[i].value, props[i].min, props[i].max, 1.0f);
			if(new_value != props[i].value)
			{
				*new_val = new_value;
				change = i;
			}
		}
		else if(props[i].type == PROPTYPE_COLOR)
		{
			static const char *texts[4] = {"R", "G", "B", "A"};
			static int shift[] = {24, 16, 8, 0};
			int new_color = 0;
			
			for(int c = 0; c < 4; c++)
			{
				int v = (props[i].value >> shift[c])&0xff;
				new_color |= ui_do_value_selector(((char *)&ids[i])+c, &shifter, texts[c], v, 0, 255, 1.0f)<<shift[c];

				if(c != 3)
				{
					ui_hsplit_t(toolbox, 13.0f, &slot, toolbox);
					ui_vsplit_mid(&slot, 0, &shifter);
					ui_hmargin(&shifter, 1.0f, &shifter);
				}
			}
			
			if(new_color != props[i].value)
			{
				*new_val = new_color;
				change = i;
			}
		}
	}
	
	return change;
}

static void render_layers(RECT toolbox, RECT toolbar, RECT view)
{
	RECT layersbox = toolbox;
	RECT propsbox;

	do_map_editor(view, toolbar);
	
	if(!editor.gui_active)
		return;
			
	RECT slot, button;
	char buf[64];

	int valid_group = 0;
	int valid_layer = 0;
	if(selected_group >= 0 && selected_group < editor.map.groups.len())
		valid_group = 1;

	if(valid_group && selected_layer >= 0 && selected_layer < editor.map.groups[selected_group]->layers.len())
		valid_layer = 1;
		
	int valid_group_mask = valid_group ? 0 : -1;
	int valid_layer_mask = valid_layer ? 0 : -1;
	
	{
		ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);
		
		// new layer group
		ui_vsplit_l(&slot, 12.0f, &button, &slot);
		static int new_layer_group_button = 0;
		if(ui_do_button(&new_layer_group_button, "G+", 0, &button, draw_editor_button, "New group"))
		{
			editor.map.new_group();
			selected_group = editor.map.groups.len()-1;
			editor.props = PROPS_GROUP;
		}
			
		// new tile layer
		ui_vsplit_l(&slot, 10.0f, &button, &slot);
		ui_vsplit_l(&slot, 12.0f, &button, &slot);
		static int new_tile_layer_button = 0;
		if(ui_do_button(&new_tile_layer_button, "T+", valid_group_mask, &button, draw_editor_button, "New tile layer"))
		{
			LAYER *l = new LAYER_TILES(50, 50);
			editor.map.groups[selected_group]->add_layer(l);
			selected_layer = editor.map.groups[selected_group]->layers.len()-1;
			editor.props = PROPS_LAYER;
		}

		// new quad layer
		ui_vsplit_l(&slot, 2.0f, &button, &slot);
		ui_vsplit_l(&slot, 12.0f, &button, &slot);
		static int new_quad_layer_button = 0;
		if(ui_do_button(&new_quad_layer_button, "Q+", valid_group_mask, &button, draw_editor_button, "New quad layer"))
		{
			LAYER *l = new LAYER_QUADS;
			editor.map.groups[selected_group]->add_layer(l);
			selected_layer = editor.map.groups[selected_group]->layers.len()-1;
			editor.props = PROPS_LAYER;
		}

		// remove layer
		ui_vsplit_r(&slot, 12.0f, &slot, &button);
		static int delete_layer_button = 0;
		if(ui_do_button(&delete_layer_button, "L-", valid_layer_mask, &button, draw_editor_button, "Delete layer"))
			editor.map.groups[selected_group]->delete_layer(selected_layer);

		// remove group
		ui_vsplit_r(&slot, 2.0f, &slot, &button);
		ui_vsplit_r(&slot, 12.0f, &slot, &button);
		static int delete_group_button = 0;
		if(ui_do_button(&delete_group_button, "G-", valid_group_mask, &button, draw_editor_button, "Delete group"))
			editor.map.delete_group(selected_group);
	}

	ui_hsplit_t(&layersbox, 5.0f, &slot, &layersbox);
	
	// render layers	
	{
		for(int g = 0; g < editor.map.groups.len(); g++)
		{
			RECT visible_toggle;
			ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);
			ui_vsplit_l(&slot, 12, &visible_toggle, &slot);
			if(ui_do_button(&editor.map.groups[g]->visible, editor.map.groups[g]->visible?"V":"H", 0, &visible_toggle, draw_editor_button_l, "Toggle group visibility"))
				editor.map.groups[g]->visible = !editor.map.groups[g]->visible;

			sprintf(buf, "#%d %s", g, editor.map.groups[g]->name);
			if(ui_do_button(&editor.map.groups[g], buf, g==selected_group, &slot, draw_editor_button_r, "Select group"))
			{
				selected_group = g;
				selected_layer = 0;
				editor.props = PROPS_GROUP;
			}
			
			ui_hsplit_t(&layersbox, 2.0f, &slot, &layersbox);
			
			for(int i = 0; i < editor.map.groups[g]->layers.len(); i++)
			{
				//visible
				ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);
				ui_vsplit_l(&slot, 12.0f, 0, &button);
				ui_vsplit_l(&button, 15, &visible_toggle, &button);

				if(ui_do_button(&editor.map.groups[g]->layers[i]->visible, editor.map.groups[g]->layers[i]->visible?"V":"H", 0, &visible_toggle, draw_editor_button_l, "Toggle layer visibility"))
					editor.map.groups[g]->layers[i]->visible = !editor.map.groups[g]->layers[i]->visible;

				sprintf(buf, "#%d %s ", i, editor.map.groups[g]->layers[i]->type_name);
				if(ui_do_button(editor.map.groups[g]->layers[i], buf, g==selected_group&&i==selected_layer, &button, draw_editor_button_r, "Select layer"))
				{
					selected_layer = i;
					selected_group = g;
					editor.props = PROPS_LAYER;
				}
				ui_hsplit_t(&layersbox, 2.0f, &slot, &layersbox);
			}
			ui_hsplit_t(&layersbox, 5.0f, &slot, &layersbox);
		}
	}
	
	propsbox = layersbox;
	
	// group properties
	if(editor.props == PROPS_GROUP && valid_group)
	{
		ui_hsplit_t(&propsbox, 12.0f, &slot, &propsbox);
		ui_do_label(&slot, "Group Props", 12.0f, -1, -1);
	
		enum
		{
			PROP_ORDER=0,
			PROP_POS_X,
			PROP_POS_Y,
			PROP_PARA_X,
			PROP_PARA_Y,
			NUM_PROPS,
		};
		
		PROPERTY props[] = {
			{"Order", selected_group, PROPTYPE_INT_STEP, 0, editor.map.groups.len()-1},
			{"Pos X", -editor.map.groups[selected_group]->offset_x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
			{"Pos Y", -editor.map.groups[selected_group]->offset_y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
			{"Para X", editor.map.groups[selected_group]->parallax_x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
			{"Para Y", editor.map.groups[selected_group]->parallax_y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
			{0},
		};
		
		static int ids[NUM_PROPS] = {0};
		int new_val = 0;
		
		// cut the properties that isn't needed
		if(editor.get_selected_group()->game_group)
			props[PROP_POS_X].name = 0;
			
		int prop = editor.do_properties(&propsbox, props, ids, &new_val);
		if(prop == PROP_ORDER)
			selected_group = editor.map.swap_groups(selected_group, new_val);
			
		// these can not be changed on the game group
		if(!editor.get_selected_group()->game_group)
		{
			if(prop == PROP_PARA_X)
				editor.map.groups[selected_group]->parallax_x = new_val;
			else if(prop == PROP_PARA_Y)
				editor.map.groups[selected_group]->parallax_y = new_val;
			else if(prop == PROP_POS_X)
				editor.map.groups[selected_group]->offset_x = -new_val;
			else if(prop == PROP_POS_Y)
				editor.map.groups[selected_group]->offset_y = -new_val;
		}
	}
	
	// layer properties
	if(editor.get_selected_layer(0))
	{
		LAYERGROUP *current_group = editor.map.groups[selected_group];
		LAYER *current_layer = editor.get_selected_layer(0);
		
		if(editor.props == PROPS_LAYER)
		{
			ui_hsplit_t(&propsbox, 15.0f, &slot, &propsbox);
			ui_do_label(&slot, "Layer Props", 12.0f, -1, -1);
			
			enum
			{
				PROP_GROUP=0,
				PROP_ORDER,
				NUM_PROPS,
			};
			
			PROPERTY props[] = {
				{"Group", selected_group, PROPTYPE_INT_STEP, 0, editor.map.groups.len()-1},
				{"Order", selected_layer, PROPTYPE_INT_STEP, 0, current_group->layers.len()},
				{0},
			};
			
			static int ids[NUM_PROPS] = {0};
			int new_val = 0;
			int prop = editor.do_properties(&propsbox, props, ids, &new_val);		
			
			if(prop == PROP_ORDER)
				selected_layer = current_group->swap_layers(selected_layer, new_val);
			else if(prop == PROP_GROUP && current_layer->type != LAYERTYPE_GAME)
			{
				if(new_val >= 0 && new_val < editor.map.groups.len())
				{
					current_group->layers.remove(current_layer);
					editor.map.groups[new_val]->layers.add(current_layer);
					selected_group = new_val;
					selected_layer = editor.map.groups[new_val]->layers.len()-1;
				}
			}
		}
			
		current_layer->render_properties(&propsbox);
	}
}

static void render_images(RECT toolbox, RECT view)
{
	static int selected_image = 0;
	
	for(int i = 0; i < editor.map.images.len(); i++)
	{
		char buf[128];
		sprintf(buf, "#%d %dx%d", i, editor.map.images[i]->width, editor.map.images[i]->height);
		RECT slot;
		ui_hsplit_t(&toolbox, 15.0f, &slot, &toolbox);
		
		if(ui_do_button(&editor.map.images[i], buf, selected_image == i, &slot, draw_editor_button, "Select image"))
			selected_image = i;
		
		ui_hsplit_t(&toolbox, 2.0f, 0, &toolbox);
		
		// render image
		if(selected_image == i)
		{
			RECT r;
			ui_margin(&view, 10.0f, &r);
			if(r.h < r.w)
				r.w = r.h;
			else
				r.h = r.w;
			gfx_texture_set(editor.map.images[i]->tex_id);
			gfx_blend_normal();
			gfx_quads_begin();
			gfx_quads_drawTL(r.x, r.y, r.w, r.h);
			gfx_quads_end();
			
		}
	}
	
	RECT slot;
	ui_hsplit_t(&toolbox, 5.0f, &slot, &toolbox);
	ui_hsplit_t(&toolbox, 15.0f, &slot, &toolbox);

	// new image
	static int new_image_button = 0;
	if(ui_do_button(&new_image_button, "(Load New Image)", 0, &slot, draw_editor_button, "Load a new image to use in the map"))
		editor.dialog = DIALOG_LOAD_IMAGE;

	ui_hsplit_t(&toolbox, 15.0f, &slot, &toolbox);
}
	
static void editor_listdir_callback(const char *name, int is_dir, void *user)
{
	if(name[0] == '.') // skip this shit!
		return;
	
	RECT *view = (RECT *)user;
	RECT button;
	ui_hsplit_t(view, 15.0f, &button, view);
	ui_hsplit_t(view, 2.0f, 0, view);
	
	if(ui_do_button((void*)(10+(int)button.y), name, 0, &button, draw_editor_button, 0))
	{
		char buf[512];
		sprintf(buf, "tilesets/%s", name);
		
		IMAGE imginfo;
		if(!gfx_load_png(&imginfo, buf))
			return;
		
		IMAGE *img = new IMAGE;
		*img = imginfo;
		img->tex_id = gfx_load_texture_raw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO);
		editor.map.images.add(img);
		
		//tilesets_set_img(tilesets_new(), img.width, img.height, img.data);
		editor.dialog = DIALOG_NONE;
	}
}

static void render_dialog_load_image()
{
	// GUI coordsys
	gfx_clear(0.25f,0.25f,0.25f);
		
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	
	RECT view = *ui_screen();
	fs_listdir("tilesets", editor_listdir_callback, &view);
	
	if(inp_key_pressed(KEY_ESC))
		editor.dialog = DIALOG_NONE;
}

static void render_modebar(RECT view)
{
	RECT button;

	// mode buttons
	{
		ui_vsplit_l(&view, 40.0f, &button, &view);
		static int map_button = 0;
		if(ui_do_button(&map_button, "Map", editor.mode == MODE_MAP, &button, draw_editor_button_l, "Switch to edit global map settings"))
			editor.mode = MODE_MAP;
		
		ui_vsplit_l(&view, 40.0f, &button, &view);
		static int tile_button = 0;
		if(ui_do_button(&tile_button, "Layers", editor.mode == MODE_LAYERS, &button, draw_editor_button_m, "Switch to edit layers"))
			editor.mode = MODE_LAYERS;

		ui_vsplit_l(&view, 40.0f, &button, &view);
		static int img_button = 0;
		if(ui_do_button(&img_button, "Images", editor.mode == MODE_IMAGES, &button, draw_editor_button_r, "Switch to manage images"))
			editor.mode = MODE_IMAGES;
	}

	ui_vsplit_l(&view, 5.0f, 0, &view);
	
	// animate button
	ui_vsplit_l(&view, 30.0f, &button, &view);
	static int animate_button = 0;
	if(ui_do_button(&animate_button, "Anim", editor.animate, &button, draw_editor_button, "[ctrl+m] Toggle animation") ||
		(inp_key_down('M') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		editor.animate_start = time_get();
		editor.animate = !editor.animate;
	}

	ui_vsplit_l(&view, 5.0f, 0, &view);

	// proof button
	ui_vsplit_l(&view, 30.0f, &button, &view);
	static int proof_button = 0;
	if(ui_do_button(&proof_button, "Proof", editor.proof_borders, &button, draw_editor_button, "[ctrl-p] Toggles proof borders. These borders represent what a player maximum can see.") ||
		(inp_key_down('P') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		editor.proof_borders = !editor.proof_borders;
	}
	
	// spacing
	//ui_vsplit_l(&view, 10.0f, 0, &view);
}

static void render_statusbar(RECT view)
{
	RECT button;
	ui_vsplit_r(&view, 60.0f, &view, &button);
	static int envelope_button = 0;
	if(ui_do_button(&envelope_button, "Envelopes", editor.show_envelope_editor, &button, draw_editor_button, "Toggles the envelope editor"))
		editor.show_envelope_editor = (editor.show_envelope_editor+1)%4;
	
	if(editor.tooltip)
		ui_do_label(&view, editor.tooltip, 12.0f, -1, -1);
}

static void render_envelopeeditor(RECT view)
{
	if(selected_envelope < 0) selected_envelope = 0;
	if(selected_envelope >= editor.map.envelopes.len()) selected_envelope--;

	ENVELOPE *envelope = 0;
	if(selected_envelope >= 0 && selected_envelope < editor.map.envelopes.len())
		envelope = editor.map.envelopes[selected_envelope];

	bool show_colorbar = false;
	if(envelope && envelope->channels == 4)
		show_colorbar = true;

	RECT toolbar, curvebar, colorbar;
	ui_hsplit_t(&view, 20.0f, &toolbar, &view);
	ui_hsplit_t(&view, 20.0f, &curvebar, &view);
	ui_margin(&toolbar, 2.0f, &toolbar);
	ui_margin(&curvebar, 2.0f, &curvebar);

	if(show_colorbar)
	{
		ui_hsplit_t(&view, 20.0f, &colorbar, &view);
		ui_margin(&colorbar, 2.0f, &colorbar);
		render_background(colorbar, checker_texture, 16.0f, 1.0f);
	}

	render_background(view, checker_texture, 32.0f, 0.1f);

	// do the toolbar
	{
		RECT button;
		ENVELOPE *new_env = 0;
		
		ui_vsplit_r(&toolbar, 50.0f, &toolbar, &button);
		static int new_4d_button = 0;
		if(ui_do_button(&new_4d_button, "Color+", 0, &button, draw_editor_button, "Creates a new color envelope"))
			new_env = editor.map.new_envelope(4);

		ui_vsplit_r(&toolbar, 5.0f, &toolbar, &button);
		ui_vsplit_r(&toolbar, 50.0f, &toolbar, &button);
		static int new_2d_button = 0;
		if(ui_do_button(&new_2d_button, "Pos.+", 0, &button, draw_editor_button, "Creates a new pos envelope"))
			new_env = editor.map.new_envelope(3);
		
		if(new_env) // add the default points
		{
			if(new_env->channels == 4)
			{
				new_env->add_point(0, 1,1,1,1);
				new_env->add_point(1000, 1,1,1,1);
			}
			else
			{
				new_env->add_point(0, 0);
				new_env->add_point(1000, 0);
			}
		}
		
		RECT shifter, inc, dec;
		ui_vsplit_l(&toolbar, 60.0f, &shifter, &toolbar);
		ui_vsplit_r(&shifter, 15.0f, &shifter, &inc);
		ui_vsplit_l(&shifter, 15.0f, &dec, &shifter);
		char buf[512];
		sprintf(buf, "%d/%d", selected_envelope+1, editor.map.envelopes.len());
		ui_draw_rect(&shifter, vec4(1,1,1,0.5f), 0, 0.0f);
		ui_do_label(&shifter, buf, 14.0f, 0, -1);
		
		static int prev_button = 0;
		if(ui_do_button(&prev_button, 0, 0, &dec, draw_dec_button, "Previous Envelope"))
			selected_envelope--;
		
		static int next_button = 0;
		if(ui_do_button(&next_button, 0, 0, &inc, draw_inc_button, "Next Envelope"))
			selected_envelope++;
			
		if(envelope)
		{
			ui_vsplit_l(&toolbar, 15.0f, &button, &toolbar);
			ui_vsplit_l(&toolbar, 35.0f, &button, &toolbar);
			ui_do_label(&button, "Name:", 14.0f, -1, -1);

			ui_vsplit_l(&toolbar, 80.0f, &button, &toolbar);
			static int name_box = 0;
			ui_do_edit_box(&name_box, &button, envelope->name, sizeof(envelope->name));
		}
	}
	
	if(envelope)
	{
		static array<int> selection;
		static int envelope_editor_id = 0;
		static int active_channels = 0xf;
		
		if(envelope)
		{
			RECT button;	
			
			ui_vsplit_l(&toolbar, 15.0f, &button, &toolbar);

			static const char *names[4][4] = {
				{"X", "", "", ""},
				{"X", "Y", "", ""},
				{"X", "Y", "R", ""},
				{"R", "G", "B", "A"},
			};
			
			static int channel_buttons[4] = {0};
			int bit = 1;
			ui_draw_button_func draw_func;
			
			for(int i = 0; i < envelope->channels; i++, bit<<=1)
			{
				ui_vsplit_l(&toolbar, 15.0f, &button, &toolbar);
				
				if(i == 0) draw_func = draw_editor_button_l;
				else if(i == envelope->channels-1) draw_func = draw_editor_button_r;
				else draw_func = draw_editor_button_m;
				
				if(ui_do_button(&channel_buttons[i], names[envelope->channels-1][i], active_channels&bit, &button, draw_func, 0))
					active_channels ^= bit;
			}
		}		
		
		float end_time = envelope->end_time();
		if(end_time < 1)
			end_time = 1;
		
		envelope->find_top_bottom();
		float top = envelope->top;
		float bottom = envelope->bottom;
		
		if(top < 1)
			top = 1;
		if(bottom >= 0)
			bottom = 0;
		
		float timescale = end_time/view.w;
		float valuescale = (top-bottom)/view.h;
		
		if(ui_mouse_inside(&view))
			ui_set_hot_item(&envelope_editor_id);
			
		if(ui_hot_item() == &envelope_editor_id)
		{
			// do stuff
			if(envelope)
			{
				if(ui_mouse_button_clicked(1))
				{
					// add point
					int time = (int)(((ui_mouse_x()-view.x)*timescale)*1000.0f);
					//float env_y = (ui_mouse_y()-view.y)/timescale;
					envelope->add_point(time,
						f2fx(envelope->eval(time, 0)),
						f2fx(envelope->eval(time, 1)),
						f2fx(envelope->eval(time, 2)),
						f2fx(envelope->eval(time, 3)));
				}
				
				editor.tooltip = "Press right mouse button to create a new point";
			}
		}

		vec3 colors[] = {vec3(1,0.2f,0.2f), vec3(0.2f,1,0.2f), vec3(0.2f,0.2f,1), vec3(1,1,0.2f)};

		// render lines
		{
			gfx_texture_set(-1);
			gfx_lines_begin();
			for(int c = 0; c < envelope->channels; c++)
			{
				if(active_channels&(1<<c))
					gfx_setcolor(colors[c].r,colors[c].g,colors[c].b,1);
				else
					gfx_setcolor(colors[c].r*0.5f,colors[c].g*0.5f,colors[c].b*0.5f,1);
				
				float prev_x = 0;
				float prev_value = envelope->eval(0.000001f, c);
				int steps = (int)((view.w/ui_screen()->w) * gfx_screenwidth());
				for(int i = 1; i <= steps; i++)
				{
					float a = i/(float)steps;
					float v = envelope->eval(a*end_time, c);
					v = (v-bottom)/(top-bottom);
					
					gfx_lines_draw(view.x + prev_x*view.w, view.y+view.h - prev_value*view.h, view.x + a*view.w, view.y+view.h - v*view.h);
					prev_x = a;
					prev_value = v;
				}
			}
			gfx_lines_end();
		}
		
		// render curve options
		{
			for(int i = 0; i < envelope->points.len()-1; i++)
			{
				float t0 = envelope->points[i].time/1000.0f/end_time;
				float t1 = envelope->points[i+1].time/1000.0f/end_time;

				//dbg_msg("", "%f", end_time);
				
				RECT v;
				v.x = curvebar.x + (t0+(t1-t0)*0.5f) * curvebar.w;
				v.y = curvebar.y;
				v.h = curvebar.h;
				v.w = curvebar.h;
				v.x -= v.w/2;
				void *id = &envelope->points[i].curvetype;
				const char *type_name[] = {
					"N", "L", "S", "F", "M"
					};
				
				if(ui_do_button(id, type_name[envelope->points[i].curvetype], 0, &v, draw_editor_button, "Switch curve type"))
					envelope->points[i].curvetype = (envelope->points[i].curvetype+1)%NUM_CURVETYPES;
			}
		}
		
		// render colorbar
		if(show_colorbar)
		{
			gfx_texture_set(-1);
			gfx_quads_begin();
			for(int i = 0; i < envelope->points.len()-1; i++)
			{
				float r0 = fx2f(envelope->points[i].values[0]);
				float g0 = fx2f(envelope->points[i].values[1]);
				float b0 = fx2f(envelope->points[i].values[2]);
				float a0 = fx2f(envelope->points[i].values[3]);
				float r1 = fx2f(envelope->points[i+1].values[0]);
				float g1 = fx2f(envelope->points[i+1].values[1]);
				float b1 = fx2f(envelope->points[i+1].values[2]);
				float a1 = fx2f(envelope->points[i+1].values[3]);

				gfx_setcolorvertex(0, r0, g0, b0, a0);
				gfx_setcolorvertex(1, r1, g1, b1, a1);
				gfx_setcolorvertex(2, r1, g1, b1, a1);
				gfx_setcolorvertex(3, r0, g0, b0, a0);

				float x0 = envelope->points[i].time/1000.0f/end_time;
//				float y0 = (fx2f(envelope->points[i].values[c])-bottom)/(top-bottom);
				float x1 = envelope->points[i+1].time/1000.0f/end_time;
				//float y1 = (fx2f(envelope->points[i+1].values[c])-bottom)/(top-bottom);
				RECT v;
				v.x = colorbar.x + x0*colorbar.w;
				v.y = colorbar.y;
				v.w = (x1-x0)*colorbar.w;
				v.h = colorbar.h;
				
				gfx_quads_drawTL(v.x, v.y, v.w, v.h);
			}
			gfx_quads_end();
		}
		
		// render handles
		{
			static bool move = false;
			
			int current_value = 0, current_time = 0;
			
			gfx_texture_set(-1);
			gfx_quads_begin();
			for(int c = 0; c < envelope->channels; c++)
			{
				if(!(active_channels&(1<<c)))
					continue;
				
				for(int i = 0; i < envelope->points.len(); i++)
				{
					float x0 = envelope->points[i].time/1000.0f/end_time;
					float y0 = (fx2f(envelope->points[i].values[c])-bottom)/(top-bottom);
					RECT final;
					final.x = view.x + x0*view.w;
					final.y = view.y+view.h - y0*view.h;
					final.x -= 2.0f;
					final.y -= 2.0f;
					final.w = 4.0f;
					final.h = 4.0f;
					
					void *id = &envelope->points[i].values[c];
					
					if(ui_mouse_inside(&final))
						ui_set_hot_item(id);
						
					float colormod = 1.0f;

					if(ui_active_item() == id)
					{
						if(!ui_mouse_button(0))
						{
							ui_set_active_item(0);
							move = false;
						}
						else
						{
							envelope->points[i].values[c] -= f2fx(editor.mouse_delta_y*valuescale);
							if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
							{
								if(i != 0)
								{
									envelope->points[i].time += (int)((editor.mouse_delta_x*timescale)*1000.0f);
									if(envelope->points[i].time < envelope->points[i-1].time)
										envelope->points[i].time = envelope->points[i-1].time + 1;
									if(i+1 != envelope->points.len() && envelope->points[i].time > envelope->points[i+1].time)
										envelope->points[i].time = envelope->points[i+1].time - 1;
								}
							}
						}
						
						colormod = 100.0f;
						gfx_setcolor(1,1,1,1);
					}
					else if(ui_hot_item() == id)
					{
						if(ui_mouse_button(0))
						{
							selection.clear();
							selection.add(i);
							ui_set_active_item(id);
						}

						// remove point
						if(ui_mouse_button_clicked(1))
							envelope->points.removebyindex(i);
							
						colormod = 100.0f;
						gfx_setcolor(1,0.75f,0.75f,1);
						editor.tooltip = "Left mouse to drag. Hold shift to alter time point aswell. Right click to delete.";
					}

					if(ui_active_item() == id || ui_hot_item() == id)
					{
						current_time = envelope->points[i].time;
						current_value = envelope->points[i].values[c];
					}
					
					gfx_setcolor(colors[c].r*colormod, colors[c].g*colormod, colors[c].b*colormod, 1.0f);
					gfx_quads_drawTL(final.x, final.y, final.w, final.h);
				}
			}
			gfx_quads_end();

			char buf[512];
			sprintf(buf, "%.3f %.3f", current_time/1000.0f, fx2f(current_value));
			ui_do_label(&toolbar, buf, 14.0f, 0, -1);
		}
	}
}

static void editor_render()
{
	// basic start
	gfx_clear(1.0f,0.0f,1.0f);
	RECT view = *ui_screen();
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	
	// reset tip
	editor.tooltip = 0;
	
	// render checker
	render_background(view, checker_texture, 32.0f, 1.0f);
	
	RECT modebar, toolbar, statusbar, envelope_editor, propsbar;
	
	if(editor.gui_active)
	{
		
		ui_hsplit_t(&view, 16.0f, &toolbar, &view);
		ui_vsplit_l(&view, 80.0f, &propsbar, &view);
		ui_hsplit_b(&view, 16.0f, &view, &statusbar);

				
		float brightness = 0.25f;

		render_background(propsbar, background_texture, 128.0f, brightness);
		ui_margin(&propsbar, 2.0f, &propsbar);
		
		render_background(toolbar, background_texture, 128.0f, brightness);
		ui_margin(&toolbar, 2.0f, &toolbar);
		ui_vsplit_l(&toolbar, 220.0f, &modebar, &toolbar);

		render_background(statusbar, background_texture, 128.0f, brightness);
		ui_margin(&statusbar, 2.0f, &statusbar);
		
		if(editor.show_envelope_editor)
		{
			float size = 125.0f;
			if(editor.show_envelope_editor == 2)
				size *= 2.0f;
			else if(editor.show_envelope_editor == 3)
				size *= 3.0f;
			ui_hsplit_b(&view, size, &view, &envelope_editor);
			render_background(envelope_editor, background_texture, 128.0f, brightness);
			ui_margin(&envelope_editor, 2.0f, &envelope_editor);
		}
	}
	
	if(editor.dialog == DIALOG_LOAD_IMAGE)
		render_dialog_load_image();
	else if(editor.mode == MODE_LAYERS)
		render_layers(propsbar, toolbar, view);
	else if(editor.mode == MODE_IMAGES)
		render_images(propsbar, view);
		
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);

	if(editor.gui_active)
	{
		render_modebar(modebar);
		if(editor.show_envelope_editor)
			render_envelopeeditor(envelope_editor);
		render_statusbar(statusbar);
	}
	
	//do_propsdialog();

	// render butt ugly mouse cursor
	float mx = ui_mouse_x();
	float my = ui_mouse_y();
	gfx_texture_set(cursor_texture);
	gfx_quads_begin();
	gfx_quads_drawTL(mx,my, 16.0f, 16.0f);
	gfx_quads_end();	
}

void editor_reset(bool create_default=true)
{
	editor.map.groups.deleteall();
	editor.map.envelopes.deleteall();
	editor.map.images.deleteall();
	
	editor.game_layer = 0;
	editor.game_group = 0;

	// create default layers
	if(create_default)
	{
		editor.make_game_group(editor.map.new_group());
		editor.make_game_layer(new LAYER_GAME(50, 50));
		editor.game_group->add_layer(editor.game_layer);
	}
}

void EDITOR::make_game_layer(LAYER *layer)
{
	editor.game_layer = (LAYER_GAME *)layer;
	editor.game_layer->tex_id = entities_texture;
	editor.game_layer->readonly = true;
}

void EDITOR::make_game_group(LAYERGROUP *group)
{
	editor.game_group = group;
	editor.game_group->game_group = true;
	editor.game_group->name = "Game";
}

template<typename T>
static int make_version(int i, const T &v)
{ return (i<<16)+sizeof(T); }

void editor_load_old(DATAFILE *df)
{
	// load tilemaps
	int game_width = 0;
	int game_height = 0;
	{
		int start, num;
		datafile_get_type(df, MAPRES_TILEMAP, &start, &num);
		for(int t = 0; t < num; t++)
		{
			mapres_tilemap *tmap = (mapres_tilemap *)datafile_get_item(df, start+t,0,0);
			
			LAYER_TILES *l = new LAYER_TILES(tmap->width, tmap->height);
			
			if(tmap->main)
			{
				// move game layer to correct position
				for(int i = 0; i < editor.map.groups[0]->layers.len()-1; i++)
				{
					if(editor.map.groups[0]->layers[i] == editor.game_layer)
						editor.map.groups[0]->swap_layers(i, i+1);
				}
				
				game_width = tmap->width;
				game_height = tmap->height;
			}

			// add new layer
			editor.map.groups[0]->add_layer(l);

			// process the data
			unsigned char *src_data = (unsigned char *)datafile_get_data(df, tmap->data);
			TILE *dst_data = l->tiles;
			
			for(int y = 0; y < tmap->height; y++)
				for(int x = 0; x < tmap->width; x++, dst_data++, src_data+=2)
				{
					dst_data->index = src_data[0];
					dst_data->flags = src_data[1];
				}
				
			l->image = tmap->image;
		}
	}
	
	// load images
	{
		int start, count;
		datafile_get_type(df, MAPRES_IMAGE, &start, &count);
		for(int i = 0; i < count; i++)
		{
			mapres_image *imgres = (mapres_image *)datafile_get_item(df, start+i, 0, 0);
			void *data = datafile_get_data(df, imgres->image_data);

			IMAGE *img = new IMAGE;
			img->width = imgres->width;
			img->height = imgres->height;
			img->format = IMG_RGBA;
			
			// copy image data
			img->data = mem_alloc(img->width*img->height*4, 1);
			mem_copy(img->data, data, img->width*img->height*4);
			img->tex_id = gfx_load_texture_raw(img->width, img->height, img->format, img->data, IMG_AUTO);
			editor.map.images.add(img);
			
			// unload image
			datafile_unload_data(df, imgres->image_data);
		}
	}
	
	// load entities
	{
		LAYER_GAME *g = editor.game_layer;
		g->resize(game_width, game_height);
		for(int t = MAPRES_ENTS_START; t < MAPRES_ENTS_END; t++)
		{
			// fetch entities of this class
			int start, num;
			datafile_get_type(df, t, &start, &num);


			for(int i = 0; i < num; i++)
			{
				mapres_entity *e = (mapres_entity *)datafile_get_item(df, start+i,0,0);
				int x = e->x/32;
				int y = e->y/32;
				int id = -1;
					
				if(t == MAPRES_SPAWNPOINT) id = ENTITY_SPAWN;
				else if(t == MAPRES_SPAWNPOINT_RED) id = ENTITY_SPAWN_RED;
				else if(t == MAPRES_SPAWNPOINT_BLUE) id = ENTITY_SPAWN_BLUE;
				else if(t == MAPRES_FLAGSTAND_RED) id = ENTITY_FLAGSTAND_RED;
				else if(t == MAPRES_FLAGSTAND_BLUE) id = ENTITY_FLAGSTAND_BLUE;
				else if(t == MAPRES_ITEM)
				{
					if(e->data[0] == ITEM_WEAPON_SHOTGUN) id = ENTITY_WEAPON_SHOTGUN;
					else if(e->data[0] == ITEM_WEAPON_ROCKET) id = ENTITY_WEAPON_ROCKET;
					else if(e->data[0] == ITEM_NINJA) id = ENTITY_POWERUP_NINJA;
					else if(e->data[0] == ITEM_ARMOR) id = ENTITY_ARMOR_1;
					else if(e->data[0] == ITEM_HEALTH) id = ENTITY_HEALTH_1;
				}
						
				if(id > 0 && x >= 0 && x < g->width && y >= 0 && y < g->height)
					g->tiles[y*g->width+x].index = id;
			}
		}
	}
}

int EDITOR::save(const char *filename)
{
	dbg_msg("editor", "saving to '%s'...", filename);
	DATAFILE_OUT *df = datafile_create(filename);
	if(!df)
	{
		dbg_msg("editor", "failed to open file '%s'...", filename);
		return 0;
	}
		
	// save version
	{
		MAPITEM_VERSION item;
		item.version = 1;
		datafile_add_item(df, MAPITEMTYPE_VERSION, 0, sizeof(item), &item);
	}

	// save images
	for(int i = 0; i < map.images.len(); i++)
	{
		IMAGE *img = map.images[i];
		MAPITEM_IMAGE item;
		item.version = 1;
		
		item.width = img->width;
		item.height = img->height;
		item.external = 0;
		item.image_name = -1;
		item.image_data = datafile_add_data(df, item.width*item.height*4, img->data);
		datafile_add_item(df, MAPITEMTYPE_IMAGE, i, sizeof(item), &item);
	}
	
	// save layers
	int layer_count = 0;
	for(int g = 0; g < map.groups.len(); g++)
	{
		LAYERGROUP *group = map.groups[g];
		MAPITEM_GROUP gitem;
		gitem.version = 1;
		
		gitem.parallax_x = group->parallax_x;
		gitem.parallax_y = group->parallax_y;
		gitem.offset_x = group->offset_x;
		gitem.offset_y = group->offset_y;
		gitem.start_layer = layer_count;
		gitem.num_layers = 0;
		
		for(int l = 0; l < group->layers.len(); l++)
		{
			if(group->layers[l]->type == LAYERTYPE_TILES)
			{
				dbg_msg("editor", "saving tiles layer");
				LAYER_TILES *layer = (LAYER_TILES *)group->layers[l];
				MAPITEM_LAYER_TILEMAP item;
				item.version = 2;
				
				item.layer.flags = 0;
				item.layer.type = layer->type;
				
				item.color.r = 255; // not in use right now
				item.color.g = 255;
				item.color.b = 255;
				item.color.a = 255;
				item.color_env = -1;
				item.color_env_offset = 0;
				
				item.width = layer->width;
				item.height = layer->height;
				item.flags = layer->game;
				item.image = layer->image;
				item.data = datafile_add_data(df, layer->width*layer->height*sizeof(TILE), layer->tiles);
				datafile_add_item(df, MAPITEMTYPE_LAYER, layer_count, sizeof(item), &item);
				
				gitem.num_layers++;
				layer_count++;
			}
			else if(group->layers[l]->type == LAYERTYPE_QUADS)
			{
				dbg_msg("editor", "saving quads layer");
				LAYER_QUADS *layer = (LAYER_QUADS *)group->layers[l];
				if(layer->quads.len())
				{
					MAPITEM_LAYER_QUADS item;
					item.version = 1;
					item.layer.flags = 0;
					item.layer.type = layer->type;
					item.image = layer->image;
					
					// add the data
					item.num_quads = layer->quads.len();
					item.data = datafile_add_data_swapped(df, layer->quads.len()*sizeof(QUAD), layer->quads.getptr());
					datafile_add_item(df, MAPITEMTYPE_LAYER, layer_count, sizeof(item), &item);
					
					// clean up
					//mem_free(quads);

					gitem.num_layers++;
					layer_count++;
				}
			}
		}
		
		datafile_add_item(df, MAPITEMTYPE_GROUP, g, sizeof(gitem), &gitem);
	}
	
	// finish the data file
	datafile_finish(df);
	dbg_msg("editor", "done");
	return 1;
}


int EDITOR::load(const char *filename)
{
	DATAFILE *df = datafile_load(filename);
	if(!df)
		return 0;

	// check version
	MAPITEM_VERSION *item = (MAPITEM_VERSION *)datafile_find_item(df, MAPITEMTYPE_VERSION, 0);
	if(!item)
	{
		// import old map
		editor_reset();
		editor_load_old(df);
	}
	else if(item->version == 1)
	{
		editor_reset(false);
		
		// load images
		{
			int start, num;
			datafile_get_type(df, MAPITEMTYPE_IMAGE, &start, &num);
			for(int i = 0; i < num; i++)
			{
				MAPITEM_IMAGE *item = (MAPITEM_IMAGE *)datafile_get_item(df, start+i, 0, 0);
				void *data = datafile_get_data(df, item->image_data);
				
				IMAGE *img = new IMAGE;
				img->width = item->width;
				img->height = item->height;
				img->format = IMG_RGBA;
				
				// copy image data
				img->data = mem_alloc(img->width*img->height*4, 1);
				mem_copy(img->data, data, img->width*img->height*4);
				img->tex_id = gfx_load_texture_raw(img->width, img->height, img->format, img->data, IMG_AUTO);
				editor.map.images.add(img);
				
				// unload image
				datafile_unload_data(df, item->image_data);
			}
		}
		
		// load groups
		{
			int layers_start, layers_num;
			datafile_get_type(df, MAPITEMTYPE_LAYER, &layers_start, &layers_num);
			
			int start, num;
			datafile_get_type(df, MAPITEMTYPE_GROUP, &start, &num);
			for(int g = 0; g < num; g++)
			{
				MAPITEM_GROUP *gitem = (MAPITEM_GROUP *)datafile_get_item(df, start+g, 0, 0);
				LAYERGROUP *group = map.new_group();
				group->parallax_x = gitem->parallax_x;
				group->parallax_y = gitem->parallax_y;
				group->offset_x = gitem->offset_x;
				group->offset_y = gitem->offset_y;
				
				for(int l = 0; l < gitem->num_layers; l++)
				{
					MAPITEM_LAYER *layer_item = (MAPITEM_LAYER *)datafile_get_item(df, layers_start+gitem->start_layer+l, 0, 0);
					if(!layer_item)
						continue;
						
					if(layer_item->type == LAYERTYPE_TILES)
					{
						MAPITEM_LAYER_TILEMAP *tilemap_item = (MAPITEM_LAYER_TILEMAP *)layer_item;
						LAYER_TILES *tiles = 0;
						
						if(tilemap_item->flags&1)
						{
							tiles = new LAYER_GAME(tilemap_item->width, tilemap_item->height);
							editor.make_game_layer(tiles);
							make_game_group(group);
						}
						else
							tiles = new LAYER_TILES(tilemap_item->width, tilemap_item->height);
						
						group->add_layer(tiles);
						void *data = datafile_get_data(df, tilemap_item->data);
						tiles->image = tilemap_item->image;
						tiles->game = tilemap_item->flags&1;
						
						mem_copy(tiles->tiles, data, tiles->width*tiles->height*sizeof(TILE));
						
						if(tiles->game && tilemap_item->version == make_version(1, *tilemap_item))
						{
							for(int i = 0; i < tiles->width*tiles->height; i++)
							{
								if(tiles->tiles[i].index)
									tiles->tiles[i].index += ENTITY_OFFSET;
							}
						}
						
						datafile_unload_data(df, tilemap_item->data);
					}
					else if(layer_item->type == LAYERTYPE_QUADS)
					{
						MAPITEM_LAYER_QUADS *quads_item = (MAPITEM_LAYER_QUADS *)layer_item;
						LAYER_QUADS *layer = new LAYER_QUADS;
						layer->image = quads_item->image;
						if(layer->image < -1 || layer->image >= map.images.len())
							layer->image = -1;
						void *data = datafile_get_data_swapped(df, quads_item->data);
						group->add_layer(layer);
						layer->quads.setsize(quads_item->num_quads);
						mem_copy(layer->quads.getptr(), data, sizeof(QUAD)*quads_item->num_quads);
						datafile_unload_data(df, quads_item->data);
					}
				}
			}
		}
	}
	
	datafile_unload(df);
	
	return 0;
}


extern "C" void editor_init()
{
	checker_texture = gfx_load_texture("data/editor/checker.png", IMG_AUTO);
	background_texture = gfx_load_texture("data/editor/background.png", IMG_AUTO);
	cursor_texture = gfx_load_texture("data/editor/cursor.png", IMG_AUTO);
	entities_texture = gfx_load_texture("data/editor/entities.png", IMG_AUTO);
	
	tileset_picker.make_palette();
	tileset_picker.readonly = true;
	
	editor_reset();
	//editor.load("debug_test.map");
	
#if 0
	IMAGE *img = new IMAGE;
	gfx_load_png(img, "tilesets/grassland_main.png");
	img->tex_id = gfx_load_texture_raw(img->width, img->height, img->format, img->data);
	editor.map.images.add(img);
	

	ENVELOPE *e = editor.map.new_envelope(4);
	e->add_point(0, 0, 0);
	e->add_point(1000, f2fx(1), f2fx(0.75f));
	e->add_point(2000, f2fx(0.75f), f2fx(1));
	e->add_point(3000, 0, 0);
	
	editor.animate = true;
	editor.animate_start = time_get();
	
	editor.show_envelope_editor = 1;
#endif

/*
	if(1)
	{
		float w, h;
		float amount = 1300*1000;
		float max = 1500;
		dbg_msg("", "%f %f %f %f", (900*(5/4.0f))*900.0f, (900*(4/3.0f))*900.0f, (900*(16/9.0f))*900.0f, (900*(16/10.0f))*900.0f);
		dbg_msg("", "%f", 900*(16/9.0f));
		calc_screen_params(amount, max, max, 5.0f/4.0f, &w, &h); dbg_msg("", "5:4 %f %f %f", w, h, w*h);
		calc_screen_params(amount, max, max, 4.0f/3.0f, &w, &h); dbg_msg("", "4:3 %f %f %f", w, h, w*h);
		calc_screen_params(amount, max, max, 16.0f/9.0f, &w, &h); dbg_msg("", "16:9 %f %f %f", w, h, w*h);
		calc_screen_params(amount, max, max, 16.0f/10.0f, &w, &h); dbg_msg("", "16:10 %f %f %f", w, h, w*h);
		
		calc_screen_params(amount, max, max, 9.0f/16.0f, &w, &h); dbg_msg("", "%f %f %f", w, h, w*h);
		calc_screen_params(amount, max, max, 16.0f/3.0f, &w, &h); dbg_msg("", "%f %f %f", w, h, w*h);
		calc_screen_params(amount, max, max, 3.0f/16.0f, &w, &h); dbg_msg("", "%f %f %f", w, h, w*h);
	}*/
}

extern "C" void editor_update_and_render()
{
	static int mouse_x = 0;
	static int mouse_y = 0;

	editor.animate_time = (time_get()-editor.animate_start)/(float)time_freq();

	// handle mouse movement
	float mx, my, mwx, mwy;
	int rx, ry;
	{
		inp_mouse_relative(&rx, &ry);
		editor.mouse_delta_x = rx;
		editor.mouse_delta_y = ry;
		
		if(!editor.lock_mouse)
		{
			mouse_x += rx;
			mouse_y += ry;
		}
		
		if(mouse_x < 0) mouse_x = 0;
		if(mouse_y < 0) mouse_y = 0;
		if(mouse_x > ui_screen()->w) mouse_x = (int)ui_screen()->w;
		if(mouse_y > ui_screen()->h) mouse_y = (int)ui_screen()->h;

		// update the ui
		mx = mouse_x;
		my = mouse_y;
		mwx = 0;
		mwy = 0;
		
		// fix correct world x and y
		LAYERGROUP *g = editor.get_selected_group();
		if(g)
		{
			float points[4];
			g->mapping(points);

			float world_width = points[2]-points[0];
			float world_height = points[3]-points[1];
			
			mwx = points[0] + world_width * (mouse_x/ui_screen()->w);
			mwy = points[1] + world_height * (mouse_y/ui_screen()->h);
			editor.mouse_delta_wx = editor.mouse_delta_x*(world_width / ui_screen()->w);
			editor.mouse_delta_wy = editor.mouse_delta_y*(world_height / ui_screen()->h);
		}
		
		int buttons = 0;
		if(inp_key_pressed(KEY_MOUSE_1)) buttons |= 1;
		if(inp_key_pressed(KEY_MOUSE_2)) buttons |= 2;
		if(inp_key_pressed(KEY_MOUSE_3)) buttons |= 4;
		
		ui_update(mx,my,mwx,mwy,buttons);
	}
	
	// toggle gui
	if(inp_key_down(KEY_TAB))
		editor.gui_active = !editor.gui_active;

	if(inp_key_down(KEY_F5))
		editor.save("data/maps/debug_test2.map");
	
	if(inp_key_down(KEY_F8))
		editor.load("data/maps/debug_test.map");
	
	editor_render();
	inp_clear_events();
}

