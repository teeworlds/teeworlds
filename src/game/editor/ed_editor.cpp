/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <base/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
	#include <engine/e_common_interface.h>
	#include <engine/e_datafile.h>
	#include <engine/e_config.h>
	#include <engine/e_engine.h>
}

#include <game/client/ui.hpp>
#include <game/gamecore.hpp>
#include <game/client/render.hpp>

#include "ed_editor.hpp"
#include <game/client/lineinput.hpp>

static int checker_texture = 0;
static int background_texture = 0;
static int cursor_texture = 0;
static int entities_texture = 0;


static const void *ui_got_context = 0;

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
	
	use_clipping = 0;
	clip_x = 0;
	clip_y = 0;
	clip_w = 0;
	clip_h = 0;
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
	
	if(use_clipping)
	{
		float points[4];
		editor.map.game_group->mapping(points);
		float x0 = (clip_x - points[0]) / (points[2]-points[0]);
		float y0 = (clip_y - points[1]) / (points[3]-points[1]);
		float x1 = ((clip_x+clip_w) - points[0]) / (points[2]-points[0]);
		float y1 = ((clip_y+clip_h) - points[1]) / (points[3]-points[1]);
		
		gfx_clip_enable((int)(x0*gfx_screenwidth()), (int)(y0*gfx_screenheight()),
			(int)((x1-x0)*gfx_screenwidth()), (int)((y1-y0)*gfx_screenheight()));
	}
	
	for(int i = 0; i < layers.len(); i++)
	{
		if(layers[i]->visible && layers[i] != editor.map.game_layer)
		{
			if(editor.show_detail || !(layers[i]->flags&LAYERFLAG_DETAIL))
				layers[i]->render();
		}
	}
	
	gfx_clip_disable();
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

void EDITOR_IMAGE::analyse_tileflags()
{
	mem_zero(tileflags, sizeof(tileflags));
	
	int tw = width/16; // tilesizes
	int th = height/16;
	if ( tw == th ) {
		unsigned char *pixeldata = (unsigned char *)data;
		
		int tile_id = 0;
		for(int ty = 0; ty < 16; ty++)
			for(int tx = 0; tx < 16; tx++, tile_id++)
			{
				bool opaque = true;
				for(int x = 0; x < tw; x++)
					for(int y = 0; y < th; y++)
					{
						int p = (ty*tw+y)*width + tx*tw+x;
						if(pixeldata[p*4+3] < 250)
						{
							opaque = false;
							break;
						}
					}
					
				if(opaque)
					tileflags[tile_id] |= TILEFLAG_OPAQUE;
			}
	}
	
}

/********************************************************
 OTHER
*********************************************************/

// copied from gc_menu.cpp, should be more generalized
//extern int ui_do_edit_box(void *id, const RECT *rect, char *str, int str_size, float font_size, bool hidden=false);

int ui_do_edit_box(void *id, const RECT *rect, char *str, int str_size, float font_size, bool hidden=false)
{
    int inside = ui_mouse_inside(rect);
	int r = 0;
	static int at_index = 0;

	if(ui_last_active_item() == id)
	{
		int len = strlen(str);

		if (inside && ui_mouse_button(0))
		{
			int mx_rel = (int)(ui_mouse_x() - rect->x);

			for (int i = 1; i <= len; i++)
			{
				if (gfx_text_width(0, font_size, str, i) + 10 > mx_rel)
				{
					at_index = i - 1;
					break;
				}

				if (i == len)
					at_index = len;
			}
		}

		for(int i = 0; i < inp_num_events(); i++)
		{
			len = strlen(str);
			LINEINPUT::manipulate(inp_get_event(i), str, str_size, &len, &at_index);
		}
		
		r = 1;
	}

	bool just_got_active = false;
	
	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			if (ui_last_active_item() != id)
				just_got_active = true;
			ui_set_active_item(id);
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	RECT textbox = *rect;
	ui_draw_rect(&textbox, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
	ui_vmargin(&textbox, 5.0f, &textbox);
	
	const char *display_str = str;
	char stars[128];
	
	if(hidden)
	{
		unsigned s = strlen(str);
		if(s >= sizeof(stars))
			s = sizeof(stars)-1;
		memset(stars, '*', s);
		stars[s] = 0;
		display_str = stars;
	}

	ui_do_label(&textbox, display_str, font_size, -1);
	
	if (ui_last_active_item() == id && !just_got_active)
	{
		float w = gfx_text_width(0, font_size, display_str, at_index);
		textbox.x += w*ui_scale();
		ui_do_label(&textbox, "_", font_size, -1);
	}

	return r;
}

vec4 button_color_mul(const void *id)
{
	if(ui_active_item() == id)
		return vec4(1,1,1,0.5f);
	else if(ui_hot_item() == id)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

float ui_do_scrollbar_v(const void *id, const RECT *rect, float current)
{
	RECT handle;
	static float offset_y;
	ui_hsplit_t(rect, 33, &handle, 0);

	handle.y += (rect->h-handle.h)*current;

	/* logic */
    float ret = current;
    int inside = ui_mouse_inside(&handle);

	if(ui_active_item() == id)
	{
		if(!ui_mouse_button(0))
			ui_set_active_item(0);
		
		float min = rect->y;
		float max = rect->h-handle.h;
		float cur = ui_mouse_y()-offset_y;
		ret = (cur-min)/max;
		if(ret < 0.0f) ret = 0.0f;
		if(ret > 1.0f) ret = 1.0f;
	}
	else if(ui_hot_item() == id)
	{
		if(ui_mouse_button(0))
		{
			ui_set_active_item(id);
			offset_y = ui_mouse_y()-handle.y;
		}
	}
	
	if(inside)
		ui_set_hot_item(id);

	// render
	RECT rail;
	ui_vmargin(rect, 5.0f, &rail);
	ui_draw_rect(&rail, vec4(1,1,1,0.25f), 0, 0.0f);

	RECT slider = handle;
	slider.w = rail.x-slider.x;
	ui_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_L, 2.5f);
	slider.x = rail.x+rail.w;
	ui_draw_rect(&slider, vec4(1,1,1,0.25f), CORNER_R, 2.5f);

	slider = handle;
	ui_margin(&slider, 5.0f, &slider);
	ui_draw_rect(&slider, vec4(1,1,1,0.25f)*button_color_mul(id), CORNER_ALL, 2.5f);
	
    return ret;
}

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

void draw_editor_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_ALL, 3.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_file(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	if(ui_hot_item() == id)
		ui_draw_rect(r, get_button_color(id, checked), CORNER_ALL, 3.0f);
	
	RECT t = *r;
	ui_vmargin(&t, 5.0f, &t);
	ui_do_label(&t, text, 10, -1, -1);
}

static void draw_editor_button_menu(const void *id, const char *text, int checked, const RECT *rect, const void *extra)
{
	/*
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	if(ui_hot_item() == id)
		ui_draw_rect(r, get_button_color(id, checked), CORNER_ALL, 3.0f);
	*/

	RECT r = *rect;
	/*
	if(ui_popups[id == id)
	{
		ui_draw_rect(&r, vec4(0.5f,0.5f,0.5f,0.75f), CORNER_T, 3.0f);
		ui_margin(&r, 1.0f, &r);
		ui_draw_rect(&r, vec4(0,0,0,0.75f), CORNER_T, 3.0f);
	}
	else*/
		ui_draw_rect(&r, vec4(0.5f,0.5f,0.5f, 1.0f), CORNER_T, 3.0f);
	

	r = *rect;
	ui_vmargin(&r, 5.0f, &r);
	ui_do_label(&r, text, 10, -1, -1);
	
	//RECT t = *r;
}

void draw_editor_button_menuitem(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	if(ui_hot_item() == id || checked)
		ui_draw_rect(r, get_button_color(id, checked), CORNER_ALL, 3.0f);
	
	RECT t = *r;
	ui_vmargin(&t, 5.0f, &t);
	ui_do_label(&t, text, 10, -1, -1);
}

static void draw_editor_button_l(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_L, 3.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_m(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), 0, 3.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_editor_button_r(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item() == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_R, 3.0f);
	ui_do_label(r, text, 10, 0, -1);
}

static void draw_inc_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_R, 3.0f);
	ui_do_label(r, text?text:">", 10, 0, -1);
}

static void draw_dec_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	if(ui_hot_item == id) if(extra) editor.tooltip = (const char *)extra;
	ui_draw_rect(r, get_button_color(id, checked), CORNER_L, 3.0f);
	ui_do_label(r, text?text:"<", 10, 0, -1);
}

enum
{
	BUTTON_CONTEXT=1,
};

int do_editor_button(const void *id, const char *text, int checked, const RECT *r, ui_draw_button_func draw_func, int flags, const char *tooltip)
{
	if(ui_mouse_inside(r))
	{
		if(flags&BUTTON_CONTEXT)
			ui_got_context = id;
		if(tooltip)
			editor.tooltip = tooltip;
	}
	
	return ui_do_button(id, text, checked, r, draw_func, 0);
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
	ui_do_label(r, buf, 10, 0, -1);
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

static void callback_open_map(const char *filename) { editor.load(filename); }
static void callback_append_map(const char *filename) { editor.append(filename); }
static void callback_save_map(const char *filename) { editor.save(filename); }

static void do_toolbar(RECT toolbar)
{
	RECT button;
	
	// ctrl+o to open
	if(inp_key_down('o') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL)))
		editor.invoke_file_dialog(LISTDIRTYPE_ALL, "Open Map", "Open", "maps/", "", callback_open_map);
	
	// ctrl+s to save
	if(inp_key_down('s') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL)))
		editor.invoke_file_dialog(LISTDIRTYPE_SAVE, "Save Map", "Save", "maps/", "", callback_save_map);

	// detail button
	ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
	static int hq_button = 0;
	if(do_editor_button(&hq_button, "Detail", editor.show_detail, &button, draw_editor_button, 0, "[ctrl+h] Toggle High Detail") ||
		(inp_key_down('h') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		editor.show_detail = !editor.show_detail;
	}

	ui_vsplit_l(&toolbar, 5.0f, 0, &toolbar);
	
	// animation button
	ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
	static int animate_button = 0;
	if(do_editor_button(&animate_button, "Anim", editor.animate, &button, draw_editor_button, 0, "[ctrl+m] Toggle animation") ||
		(inp_key_down('m') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		editor.animate_start = time_get();
		editor.animate = !editor.animate;
	}

	ui_vsplit_l(&toolbar, 5.0f, 0, &toolbar);

	// proof button
	ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
	static int proof_button = 0;
	if(do_editor_button(&proof_button, "Proof", editor.proof_borders, &button, draw_editor_button, 0, "[ctrl-p] Toggles proof borders. These borders represent what a player maximum can see.") ||
		(inp_key_down('p') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		editor.proof_borders = !editor.proof_borders;
	}

	ui_vsplit_l(&toolbar, 15.0f, 0, &toolbar);
	
	// zoom group
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_out_button = 0;
	if(do_editor_button(&zoom_out_button, "ZO", 0, &button, draw_editor_button_l, 0, "[NumPad-] Zoom out") || inp_key_down(KEY_KP_MINUS))
		editor.zoom_level += 50;
		
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_normal_button = 0;
	if(do_editor_button(&zoom_normal_button, "1:1", 0, &button, draw_editor_button_m, 0, "[NumPad*] Zoom to normal and remove editor offset") || inp_key_down(KEY_KP_MULTIPLY))
	{
		editor.editor_offset_x = 0;
		editor.editor_offset_y = 0;
		editor.zoom_level = 100;
	}
		
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int zoom_in_button = 0;
	if(do_editor_button(&zoom_in_button, "ZI", 0, &button, draw_editor_button_r, 0, "[NumPad+] Zoom in") || inp_key_down(KEY_KP_PLUS))
		editor.zoom_level -= 50;
	
	ui_vsplit_l(&toolbar, 15.0f, 0, &toolbar);
	
	// animation speed
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int anim_faster_button = 0;
	if(do_editor_button(&anim_faster_button, "A+", 0, &button, draw_editor_button_l, 0, "Increase animation speed"))
		editor.animate_speed += 0.5f;
	
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int anim_normal_button = 0;
	if(do_editor_button(&anim_normal_button, "1", 0, &button, draw_editor_button_m, 0, "Normal animation speed"))
		editor.animate_speed = 1.0f;
	
	ui_vsplit_l(&toolbar, 16.0f, &button, &toolbar);
	static int anim_slower_button = 0;
	if(do_editor_button(&anim_slower_button, "A-", 0, &button, draw_editor_button_r, 0, "Decrease animation speed"))
	{
		if(editor.animate_speed > 0.5f)
			editor.animate_speed -= 0.5f;
	}
	
	if(inp_key_presses(KEY_MOUSE_WHEEL_UP) && editor.dialog == DIALOG_NONE)
		editor.zoom_level -= 20;
		
	if(inp_key_presses(KEY_MOUSE_WHEEL_DOWN) && editor.dialog == DIALOG_NONE)
		editor.zoom_level += 20;
	
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
		if(do_editor_button(&flipx_button, "^X", enabled, &button, draw_editor_button_l, 0, "[N] Flip brush horizontal") || inp_key_down('n'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_x();
		}
			
		ui_vsplit_l(&toolbar, 20.0f, &button, &toolbar);
		static int flipy_button = 0;
		if(do_editor_button(&flipy_button, "^Y", enabled, &button, draw_editor_button_r, 0, "[M] Flip brush vertical") || inp_key_down('m'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_y();
		}

		// rotate buttons
		ui_vsplit_l(&toolbar, 20.0f, &button, &toolbar);
		
		ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
		static int rotation_amount = 90;
		rotation_amount = ui_do_value_selector(&rotation_amount, &button, "", rotation_amount, 1, 360, 2.0f);
		
		ui_vsplit_l(&toolbar, 5.0f, &button, &toolbar);
		ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
		static int ccw_button = 0;
		if(do_editor_button(&ccw_button, "CCW", enabled, &button, draw_editor_button_l, 0, "[R] Rotates the brush counter clockwise") || inp_key_down('r'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_rotate(-rotation_amount/360.0f*pi*2);
		}
			
		ui_vsplit_l(&toolbar, 30.0f, &button, &toolbar);
		static int cw_button = 0;
		if(do_editor_button(&cw_button, "CW", enabled, &button, draw_editor_button_r, 0, "[T] Rotates the brush clockwise") || inp_key_down('t'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_rotate(rotation_amount/360.0f*pi*2);
		}
	}

	// quad manipulation
	{
		// do add button
		ui_vsplit_l(&toolbar, 10.0f, &button, &toolbar);
		ui_vsplit_l(&toolbar, 60.0f, &button, &toolbar);
		static int new_button = 0;
		
		LAYER_QUADS *qlayer = (LAYER_QUADS *)editor.get_selected_layer_type(0, LAYERTYPE_QUADS);
		//LAYER_TILES *tlayer = (LAYER_TILES *)editor.get_selected_layer_type(0, LAYERTYPE_TILES);
		if(do_editor_button(&new_button, "Add Quad", qlayer?0:-1, &button, draw_editor_button, 0, "Adds a new quad"))
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
		OP_CONTEXT_MENU,
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
	if(editor.selected_quad == index)
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
		
		if(operation == OP_CONTEXT_MENU)
		{
			if(!ui_mouse_button(1))
			{
				static int quad_popup_id = 0;
				ui_invoke_popup_menu(&quad_popup_id, 0, ui_mouse_x(), ui_mouse_y(), 120, 150, popup_quad);
				editor.lock_mouse = false;
				operation = OP_NONE;
				ui_set_active_item(0);
			}
		}
		else
		{
			if(!ui_mouse_button(0))
			{
				editor.lock_mouse = false;
				operation = OP_NONE;
				ui_set_active_item(0);
			}
		}			

		gfx_setcolor(1,1,1,1);
	}
	else if(ui_hot_item() == id)
	{
		ui_got_context = id;
		
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
			editor.selected_quad = index;
			last_wx = wx;
			last_wy = wy;
		}
		
		if(ui_mouse_button(1))
		{
			editor.selected_quad = index;
			operation = OP_CONTEXT_MENU;
			ui_set_active_item(id);
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
	if(editor.selected_quad == quad_index && editor.selected_points&(1<<v))
	{
		gfx_setcolor(0,0,0,1);
		gfx_quads_draw(px, py, 7.0f, 7.0f);
	}
	
	enum
	{
		OP_NONE=0,
		OP_MOVEPOINT,
		OP_MOVEUV,
		OP_CONTEXT_MENU
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
					if(editor.selected_points&(1<<m))
					{
						q->points[m].x += f2fx(dx);
						q->points[m].y += f2fx(dy);
					}
			}
			else if(operation == OP_MOVEUV)
			{
				for(int m = 0; m < 4; m++)
					if(editor.selected_points&(1<<m))
					{
						q->texcoords[m].x += f2fx(dx*0.001f);
						q->texcoords[m].y += f2fx(dy*0.001f);
					}
			}
		}
		
		if(operation == OP_CONTEXT_MENU)
		{
			if(!ui_mouse_button(1))
			{
				static int point_popup_id = 0;
				ui_invoke_popup_menu(&point_popup_id, 0, ui_mouse_x(), ui_mouse_y(), 120, 150, popup_point);
				ui_set_active_item(0);
			}
		}
		else
		{
			if(!ui_mouse_button(0))
			{
				if(!moved)
				{
					if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
						editor.selected_points ^= 1<<v;
					else
						editor.selected_points = 1<<v;
				}
				editor.lock_mouse = false;
				ui_set_active_item(0);
			}
		}

		gfx_setcolor(1,1,1,1);
	}
	else if(ui_hot_item() == id)
	{
		ui_got_context = id;
		
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
				
			if(!(editor.selected_points&(1<<v)))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					editor.selected_points |= 1<<v;
				else
					editor.selected_points = 1<<v;
				moved = true;
			}
															
			editor.selected_quad = quad_index;
		}
		else if(ui_mouse_button(1))
		{
			operation = OP_CONTEXT_MENU;
			editor.selected_quad = quad_index;
			ui_set_active_item(id);
		}
	}
	else
		gfx_setcolor(1,0,0,1);
	
	gfx_quads_draw(px, py, 5.0f, 5.0f);	
}

static void do_map_editor(RECT view, RECT toolbar)
{
	//ui_clip_enable(&view);
	
	bool show_picker = inp_key_pressed(KEY_SPACE) != 0 && editor.dialog == DIALOG_NONE;

	// render all good stuff
	if(!show_picker)
	{
		for(int g = 0; g < editor.map.groups.len(); g++)
		{
			if(editor.map.groups[g]->visible)
				editor.map.groups[g]->render();
			//ui_clip_enable(&view);
		}
		
		// render the game above everything else
		if(editor.map.game_group->visible && editor.map.game_layer->visible)
		{
			editor.map.game_group->mapscreen();
			editor.map.game_layer->render();
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
		if(ui_active_item() == 0 && (ui_mouse_button(0) || ui_mouse_button(2)))
		{
			start_wx = wx;
			start_wy = wy;
			start_mx = mx;
			start_my = my;
					
			if(inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL) || ui_mouse_button(2))
			{
				if(inp_key_pressed(KEY_LSHIFT))
					operation = OP_PAN_EDITOR;
				else
					operation = OP_PAN_WORLD;
				ui_set_active_item(editor_id);
			}
		}

		// brush editing
		if(ui_hot_item() == editor_id)
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
						//editor.map.groups[selected_group]->mapscreen();
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
		
		// quad editing
		{
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

				// release mouse
				if(!ui_mouse_button(0))
				{
					operation = OP_NONE;
					ui_set_active_item(0);
				}
			}
		}
	}
	
	if(editor.get_selected_group() && editor.get_selected_group()->use_clipping)
	{
		LAYERGROUP *g = editor.map.game_group;
		g->mapscreen();
		
		gfx_texture_set(-1);
		gfx_lines_begin();

			RECT r;
			r.x = editor.get_selected_group()->clip_x;
			r.y = editor.get_selected_group()->clip_y;
			r.w = editor.get_selected_group()->clip_w;
			r.h = editor.get_selected_group()->clip_h;
			
			gfx_setcolor(1,0,0,1);
			gfx_lines_draw(r.x, r.y, r.x+r.w, r.y);
			gfx_lines_draw(r.x+r.w, r.y, r.x+r.w, r.y+r.h);
			gfx_lines_draw(r.x+r.w, r.y+r.h, r.x, r.y+r.h);
			gfx_lines_draw(r.x, r.y+r.h, r.x, r.y);
			
		gfx_lines_end();
	}

	// render screen sizes	
	if(editor.proof_borders)
	{
		LAYERGROUP *g = editor.map.game_group;
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

		if(1)
		{
			gfx_setcolor(1,0,0,1);
			for(int i = 0; i < 2; i++)
			{
				float points[4];
				float aspects[] = {4.0f/3.0f, 16.0f/10.0f, 5.0f/4.0f, 16.0f/9.0f};
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
				gfx_setcolor(0,1,0,1);
			}
		}
			
		gfx_lines_end();
	}
	
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	//ui_clip_disable();
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
			
			if(do_editor_button(&ids[i], 0, 0, &dec, draw_dec_button, 0, "Decrease"))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					*new_val = props[i].value-5;
				else
					*new_val = props[i].value-1;
				change = i;
			}
			if(do_editor_button(((char *)&ids[i])+1, 0, 0, &inc, draw_inc_button, 0, "Increase"))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					*new_val = props[i].value+5;
				else
					*new_val = props[i].value+1;
				change = i;
			}
		}
		else if(props[i].type == PROPTYPE_BOOL)
		{
			RECT no, yes;
			ui_vsplit_mid(&shifter, &no, &yes);
			if(do_editor_button(&ids[i], "No", !props[i].value, &no, draw_dec_button, 0, ""))
			{
				*new_val = 0;
				change = i;
			}
			if(do_editor_button(((char *)&ids[i])+1, "Yes", props[i].value, &yes, draw_inc_button, 0, ""))
			{
				*new_val = 1;
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
		else if(props[i].type == PROPTYPE_IMAGE)
		{
			char buf[64];
			if(props[i].value < 0)
				strcpy(buf, "None");
			else
				sprintf(buf, "%s",  editor.map.images[props[i].value]->name);
			
			if(do_editor_button(&ids[i], buf, 0, &shifter, draw_editor_button, 0, 0))
				popup_select_image_invoke(props[i].value, ui_mouse_x(), ui_mouse_y());
			
			int r = popup_select_image_result();
			if(r >= -1)
			{
				*new_val = r;
				change = i;
			}
		}
	}

	return change;
}

static void render_layers(RECT toolbox, RECT toolbar, RECT view)
{
	RECT layersbox = toolbox;

	if(!editor.gui_active)
		return;
			
	RECT slot, button;
	char buf[64];

	int valid_group = 0;
	int valid_layer = 0;
	if(editor.selected_group >= 0 && editor.selected_group < editor.map.groups.len())
		valid_group = 1;

	if(valid_group && editor.selected_layer >= 0 && editor.selected_layer < editor.map.groups[editor.selected_group]->layers.len())
		valid_layer = 1;
		
	// render layers	
	{
		for(int g = 0; g < editor.map.groups.len(); g++)
		{
			RECT visible_toggle;
			ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);
			ui_vsplit_l(&slot, 12, &visible_toggle, &slot);
			if(do_editor_button(&editor.map.groups[g]->visible, editor.map.groups[g]->visible?"V":"H", 0, &visible_toggle, draw_editor_button_l, 0, "Toggle group visibility"))
				editor.map.groups[g]->visible = !editor.map.groups[g]->visible;

			sprintf(buf, "#%d %s", g, editor.map.groups[g]->name);
			if(int result = do_editor_button(&editor.map.groups[g], buf, g==editor.selected_group, &slot, draw_editor_button_r,
				BUTTON_CONTEXT, "Select group. Right click for properties."))
			{
				editor.selected_group = g;
				editor.selected_layer = 0;
				
				static int group_popup_id = 0;
				if(result == 2)
					ui_invoke_popup_menu(&group_popup_id, 0, ui_mouse_x(), ui_mouse_y(), 120, 200, popup_group);
			}
			
			
			ui_hsplit_t(&layersbox, 2.0f, &slot, &layersbox);
			
			for(int i = 0; i < editor.map.groups[g]->layers.len(); i++)
			{
				//visible
				ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);
				ui_vsplit_l(&slot, 12.0f, 0, &button);
				ui_vsplit_l(&button, 15, &visible_toggle, &button);

				if(do_editor_button(&editor.map.groups[g]->layers[i]->visible, editor.map.groups[g]->layers[i]->visible?"V":"H", 0, &visible_toggle, draw_editor_button_l, 0, "Toggle layer visibility"))
					editor.map.groups[g]->layers[i]->visible = !editor.map.groups[g]->layers[i]->visible;

				sprintf(buf, "#%d %s ", i, editor.map.groups[g]->layers[i]->type_name);
				if(int result = do_editor_button(editor.map.groups[g]->layers[i], buf, g==editor.selected_group&&i==editor.selected_layer, &button, draw_editor_button_r,
					BUTTON_CONTEXT, "Select layer. Right click for properties."))
				{
					editor.selected_layer = i;
					editor.selected_group = g;
					static int layer_popup_id = 0;
					if(result == 2)
						ui_invoke_popup_menu(&layer_popup_id, 0, ui_mouse_x(), ui_mouse_y(), 120, 150, popup_layer);
				}
				
				
				ui_hsplit_t(&layersbox, 2.0f, &slot, &layersbox);
			}
			ui_hsplit_t(&layersbox, 5.0f, &slot, &layersbox);
		}
	}
	

	{
		ui_hsplit_t(&layersbox, 12.0f, &slot, &layersbox);

		static int new_group_button = 0;
		if(do_editor_button(&new_group_button, "Add Group", 0, &slot, draw_editor_button, 0, "Adds a new group"))
		{
			editor.map.new_group();
			editor.selected_group = editor.map.groups.len()-1;
		}
	}

	ui_hsplit_t(&layersbox, 5.0f, &slot, &layersbox);
	
}

static void extract_name(const char *filename, char *name)
{
	int len = strlen(filename);
	int start = len;
	int end = len;
	
	while(start > 0)
	{
		start--;
		if(filename[start] == '/' || filename[start] == '\\')
		{
			start++;
			break;
		}
	}
	
	end = start;
	for(int i = start; i < len; i++)
	{
		if(filename[i] == '.')
			end = i;
	}
	
	if(end == start)
		end = len;
	
	int final_len = end-start;
	mem_copy(name, &filename[start], final_len);
	name[final_len] = 0;
	dbg_msg("", "%s %s %d %d", filename, name, start, end);
}

static void replace_image(const char *filename)
{
	EDITOR_IMAGE imginfo;
	if(!gfx_load_png(&imginfo, filename))
		return;
	
	EDITOR_IMAGE *img = editor.map.images[editor.selected_image];
	gfx_unload_texture(img->tex_id);
	*img = imginfo;
	extract_name(filename, img->name);
	img->tex_id = gfx_load_texture_raw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO, 0);
}

static void add_image(const char *filename)
{
	EDITOR_IMAGE imginfo;
	if(!gfx_load_png(&imginfo, filename))
		return;

	EDITOR_IMAGE *img = new EDITOR_IMAGE;
	*img = imginfo;
	img->tex_id = gfx_load_texture_raw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO, 0);
	img->external = 1; // external by default
	extract_name(filename, img->name);
	editor.map.images.add(img);
}


static int modify_index_deleted_index;
static void modify_index_deleted(int *index)
{
	if(*index == modify_index_deleted_index)
		*index = -1;
	else if(*index > modify_index_deleted_index)
		*index = *index - 1;
}

static int popup_image(RECT view)
{
	static int replace_button = 0;	
	static int remove_button = 0;	

	RECT slot;
	ui_hsplit_t(&view, 2.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	EDITOR_IMAGE *img = editor.map.images[editor.selected_image];
	
	static int external_button = 0;
	if(img->external)
	{
		if(do_editor_button(&external_button, "Embedd", 0, &slot, draw_editor_button_menuitem, 0, "Embedds the image into the map file."))
		{
			img->external = 0;
			return 1;
		}
	}
	else
	{		
		if(do_editor_button(&external_button, "Make external", 0, &slot, draw_editor_button_menuitem, 0, "Removes the image from the map file."))
		{
			img->external = 1;
			return 1;
		}
	}

	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&replace_button, "Replace", 0, &slot, draw_editor_button_menuitem, 0, "Replaces the image with a new one"))
	{
		editor.invoke_file_dialog(LISTDIRTYPE_ALL, "Replace Image", "Replace", "mapres/", "", replace_image);
		return 1;
	}

	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&remove_button, "Remove", 0, &slot, draw_editor_button_menuitem, 0, "Removes the image from the map"))
	{
		delete img;
		editor.map.images.removebyindex(editor.selected_image);
		modify_index_deleted_index = editor.selected_image;
		editor.map.modify_image_index(modify_index_deleted);
		return 1;
	}

	return 0;
}


static void render_images(RECT toolbox, RECT toolbar, RECT view)
{
	for(int e = 0; e < 2; e++) // two passes, first embedded, then external
	{
		RECT slot;
		ui_hsplit_t(&toolbox, 15.0f, &slot, &toolbox);
		if(e == 0)
			ui_do_label(&slot, "Embedded", 12.0f, 0);
		else
			ui_do_label(&slot, "External", 12.0f, 0);
		
		for(int i = 0; i < editor.map.images.len(); i++)
		{
			if((e && !editor.map.images[i]->external) ||
				(!e && editor.map.images[i]->external))
			{
				continue;
			}
			
			char buf[128];
			sprintf(buf, "%s", editor.map.images[i]->name);
			ui_hsplit_t(&toolbox, 12.0f, &slot, &toolbox);
			
			if(int result = do_editor_button(&editor.map.images[i], buf, editor.selected_image == i, &slot, draw_editor_button,
				BUTTON_CONTEXT, "Select image"))
			{
				editor.selected_image = i;
				
				static int popup_image_id = 0;
				if(result == 2)
					ui_invoke_popup_menu(&popup_image_id, 0, ui_mouse_x(), ui_mouse_y(), 120, 80, popup_image);
			}
			
			ui_hsplit_t(&toolbox, 2.0f, 0, &toolbox);
			
			// render image
			if(editor.selected_image == i)
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
	}
	
	RECT slot;
	ui_hsplit_t(&toolbox, 5.0f, &slot, &toolbox);
	
	// new image
	static int new_image_button = 0;
	ui_hsplit_t(&toolbox, 10.0f, &slot, &toolbox);
	ui_hsplit_t(&toolbox, 12.0f, &slot, &toolbox);
	if(do_editor_button(&new_image_button, "Add", 0, &slot, draw_editor_button, 0, "Load a new image to use in the map"))
		editor.invoke_file_dialog(LISTDIRTYPE_ALL, "Add Image", "Add", "mapres/", "", add_image);
}


static int file_dialog_dirtypes = 0;
static const char *file_dialog_title = 0;
static const char *file_dialog_button_text = 0;
static void (*file_dialog_func)(const char *filename);
static char file_dialog_filename[512] = {0};
static char file_dialog_path[512] = {0};
static char file_dialog_complete_filename[512] = {0};
static int files_num = 0;
int files_startat = 0;
int files_cur = 0;
int files_stopat = 999;

static void editor_listdir_callback(const char *name, int is_dir, void *user)
{
	if(name[0] == '.' || is_dir) // skip this shit!
		return;
	
	if(files_cur > files_num)
		files_num = files_cur;
	
	files_cur++;
	if(files_cur-1 < files_startat || files_cur > files_stopat)
		return;
	
	RECT *view = (RECT *)user;
	RECT button;
	ui_hsplit_t(view, 15.0f, &button, view);
	ui_hsplit_t(view, 2.0f, 0, view);
	//char buf[512];
	
	if(do_editor_button((void*)(10+(int)button.y), name, 0, &button, draw_editor_button_file, 0, 0))
	{
		strncpy(file_dialog_filename, name, sizeof(file_dialog_filename));
		
		file_dialog_complete_filename[0] = 0;
		strcat(file_dialog_complete_filename, file_dialog_path);
		strcat(file_dialog_complete_filename, file_dialog_filename);

		if(inp_mouse_doubleclick())
		{
			if(file_dialog_func)
				file_dialog_func(file_dialog_complete_filename);
			editor.dialog = DIALOG_NONE;
		}
	}
}

static void render_file_dialog()
{
	// GUI coordsys
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	
	RECT view = *ui_screen();
	ui_draw_rect(&view, vec4(0,0,0,0.25f), 0, 0);
	ui_vmargin(&view, 150.0f, &view);
	ui_hmargin(&view, 50.0f, &view);
	ui_draw_rect(&view, vec4(0,0,0,0.75f), CORNER_ALL, 5.0f);
	ui_margin(&view, 10.0f, &view);

	RECT title, filebox, filebox_label, buttonbar, scroll;
	ui_hsplit_t(&view, 18.0f, &title, &view);
	ui_hsplit_t(&view, 5.0f, 0, &view); // some spacing
	ui_hsplit_b(&view, 14.0f, &view, &buttonbar);
	ui_hsplit_b(&view, 10.0f, &view, 0); // some spacing
	ui_hsplit_b(&view, 14.0f, &view, &filebox);
	ui_vsplit_l(&filebox, 50.0f, &filebox_label, &filebox);
	ui_vsplit_r(&view, 15.0f, &view, &scroll);
	
	// title
	ui_draw_rect(&title, vec4(1,1,1,0.25f), CORNER_ALL, 5.0f);
	ui_vmargin(&title, 10.0f, &title);
	ui_do_label(&title, file_dialog_title, 14.0f, -1, -1);
	
	// filebox
	ui_do_label(&filebox_label, "Filename:", 10.0f, -1, -1);
	
	static int filebox_id = 0;
	ui_do_edit_box(&filebox_id, &filebox, file_dialog_filename, sizeof(file_dialog_filename), 10.0f);

	file_dialog_complete_filename[0] = 0;
	strcat(file_dialog_complete_filename, file_dialog_path);
	strcat(file_dialog_complete_filename, file_dialog_filename);
	
	int num = (int)(view.h/17.0);
	static float scrollvalue = 0;
	static int scrollbar = 0;
	ui_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui_do_scrollbar_v(&scrollbar, &scroll, scrollvalue);
	
	int scrollnum = files_num-num+10;
	if(scrollnum > 0)
	{
		if(inp_key_presses(KEY_MOUSE_WHEEL_UP))
			scrollvalue -= 3.0f/scrollnum;
		if(inp_key_presses(KEY_MOUSE_WHEEL_DOWN))
			scrollvalue += 3.0f/scrollnum;
			
		if(scrollvalue < 0) scrollvalue = 0;
		if(scrollvalue > 1) scrollvalue = 1;
	}
	else
		scrollnum = 0;
	
	files_startat = (int)(scrollnum*scrollvalue);
	if(files_startat < 0)
		files_startat = 0;
		
	files_stopat = files_startat+num;
	
	files_cur = 0;
	
	// set clipping
	ui_clip_enable(&view);
	
	// the list
	engine_listdir(file_dialog_dirtypes, file_dialog_path, editor_listdir_callback, &view);
	
	// disable clipping again
	ui_clip_disable();
	
	// the buttons
	static int ok_button = 0;	
	static int cancel_button = 0;	

	RECT button;
	ui_vsplit_r(&buttonbar, 50.0f, &buttonbar, &button);
	if(do_editor_button(&ok_button, file_dialog_button_text, 0, &button, draw_editor_button, 0, 0) || inp_key_pressed(KEY_RETURN))
	{
		if(file_dialog_func)
			file_dialog_func(file_dialog_complete_filename);
		editor.dialog = DIALOG_NONE;
	}

	ui_vsplit_r(&buttonbar, 40.0f, &buttonbar, &button);
	ui_vsplit_r(&buttonbar, 50.0f, &buttonbar, &button);
	if(do_editor_button(&cancel_button, "Cancel", 0, &button, draw_editor_button, 0, 0) || inp_key_pressed(KEY_ESCAPE))
		editor.dialog = DIALOG_NONE;
}

void EDITOR::invoke_file_dialog(int listdirtypes, const char *title, const char *button_text,
	const char *basepath, const char *default_name,
	void (*func)(const char *filename))
{
	file_dialog_dirtypes = listdirtypes;
	file_dialog_title = title;
	file_dialog_button_text = button_text;
	file_dialog_func = func;
	file_dialog_filename[0] = 0;
	file_dialog_path[0] = 0;
	
	if(default_name)
		strncpy(file_dialog_filename, default_name, sizeof(file_dialog_filename));
	if(basepath)
		strncpy(file_dialog_path, basepath, sizeof(file_dialog_path));
		
	editor.dialog = DIALOG_FILE;
}



static void render_modebar(RECT view)
{
	RECT button;

	// mode buttons
	{
		ui_vsplit_l(&view, 40.0f, &button, &view);
		static int tile_button = 0;
		if(do_editor_button(&tile_button, "Layers", editor.mode == MODE_LAYERS, &button, draw_editor_button_m, 0, "Switch to edit layers."))
			editor.mode = MODE_LAYERS;

		ui_vsplit_l(&view, 40.0f, &button, &view);
		static int img_button = 0;
		if(do_editor_button(&img_button, "Images", editor.mode == MODE_IMAGES, &button, draw_editor_button_r, 0, "Switch to manage images."))
			editor.mode = MODE_IMAGES;
	}

	ui_vsplit_l(&view, 5.0f, 0, &view);
	
	// spacing
	//ui_vsplit_l(&view, 10.0f, 0, &view);
}

static void render_statusbar(RECT view)
{
	RECT button;
	ui_vsplit_r(&view, 60.0f, &view, &button);
	static int envelope_button = 0;
	if(do_editor_button(&envelope_button, "Envelopes", editor.show_envelope_editor, &button, draw_editor_button, 0, "Toggles the envelope editor."))
		editor.show_envelope_editor = (editor.show_envelope_editor+1)%4;
	
	if(editor.tooltip)
	{
		if(ui_got_context && ui_got_context == ui_hot_item())
		{
			char buf[512];
			sprintf(buf, "%s Right click for context menu.", editor.tooltip);
			ui_do_label(&view, buf, 10.0f, -1, -1);
		}
		else
			ui_do_label(&view, editor.tooltip, 10.0f, -1, -1);
	}
}

static void render_envelopeeditor(RECT view)
{
	if(editor.selected_envelope < 0) editor.selected_envelope = 0;
	if(editor.selected_envelope >= editor.map.envelopes.len()) editor.selected_envelope--;

	ENVELOPE *envelope = 0;
	if(editor.selected_envelope >= 0 && editor.selected_envelope < editor.map.envelopes.len())
		envelope = editor.map.envelopes[editor.selected_envelope];

	bool show_colorbar = false;
	if(envelope && envelope->channels == 4)
		show_colorbar = true;

	RECT toolbar, curvebar, colorbar;
	ui_hsplit_t(&view, 15.0f, &toolbar, &view);
	ui_hsplit_t(&view, 15.0f, &curvebar, &view);
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
		if(do_editor_button(&new_4d_button, "Color+", 0, &button, draw_editor_button, 0, "Creates a new color envelope"))
			new_env = editor.map.new_envelope(4);

		ui_vsplit_r(&toolbar, 5.0f, &toolbar, &button);
		ui_vsplit_r(&toolbar, 50.0f, &toolbar, &button);
		static int new_2d_button = 0;
		if(do_editor_button(&new_2d_button, "Pos.+", 0, &button, draw_editor_button, 0, "Creates a new pos envelope"))
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
		sprintf(buf, "%d/%d", editor.selected_envelope+1, editor.map.envelopes.len());
		ui_draw_rect(&shifter, vec4(1,1,1,0.5f), 0, 0.0f);
		ui_do_label(&shifter, buf, 10.0f, 0, -1);
		
		static int prev_button = 0;
		if(do_editor_button(&prev_button, 0, 0, &dec, draw_dec_button, 0, "Previous Envelope"))
			editor.selected_envelope--;
		
		static int next_button = 0;
		if(do_editor_button(&next_button, 0, 0, &inc, draw_inc_button, 0, "Next Envelope"))
			editor.selected_envelope++;
			
		if(envelope)
		{
			ui_vsplit_l(&toolbar, 15.0f, &button, &toolbar);
			ui_vsplit_l(&toolbar, 35.0f, &button, &toolbar);
			ui_do_label(&button, "Name:", 10.0f, -1, -1);

			ui_vsplit_l(&toolbar, 80.0f, &button, &toolbar);
			
			static int name_box = 0;
			ui_do_edit_box(&name_box, &button, envelope->name, sizeof(envelope->name), 10.0f);
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
				
				if(do_editor_button(&channel_buttons[i], names[envelope->channels-1][i], active_channels&bit, &button, draw_func, 0, 0))
					active_channels ^= bit;
			}
		}		
		
		float end_time = envelope->end_time();
		if(end_time < 1)
			end_time = 1;
		
		envelope->find_top_bottom(active_channels);
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
					float channels[4];
					envelope->eval(time, channels);
					envelope->add_point(time,
						f2fx(channels[0]), f2fx(channels[1]),
						f2fx(channels[2]), f2fx(channels[3]));
				}
				
				editor.tooltip = "Press right mouse button to create a new point";
			}
		}

		vec3 colors[] = {vec3(1,0.2f,0.2f), vec3(0.2f,1,0.2f), vec3(0.2f,0.2f,1), vec3(1,1,0.2f)};

		// render lines
		{
			ui_clip_enable(&view);
			gfx_texture_set(-1);
			gfx_lines_begin();
			for(int c = 0; c < envelope->channels; c++)
			{
				if(active_channels&(1<<c))
					gfx_setcolor(colors[c].r,colors[c].g,colors[c].b,1);
				else
					gfx_setcolor(colors[c].r*0.5f,colors[c].g*0.5f,colors[c].b*0.5f,1);
				
				float prev_x = 0;
				float results[4];
				envelope->eval(0.000001f, results);
				float prev_value = results[c];
				
				int steps = (int)((view.w/ui_screen()->w) * gfx_screenwidth());
				for(int i = 1; i <= steps; i++)
				{
					float a = i/(float)steps;
					envelope->eval(a*end_time, results);
					float v = results[c];
					v = (v-bottom)/(top-bottom);
					
					gfx_lines_draw(view.x + prev_x*view.w, view.y+view.h - prev_value*view.h, view.x + a*view.w, view.y+view.h - v*view.h);
					prev_x = a;
					prev_value = v;
				}
			}
			gfx_lines_end();
			ui_clip_disable();
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
				
				if(do_editor_button(id, type_name[envelope->points[i].curvetype], 0, &v, draw_editor_button, 0, "Switch curve type"))
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
			ui_do_label(&toolbar, buf, 10.0f, 0, -1);
		}
	}
}

static int popup_menu_file(RECT view)
{
	static int new_map_button = 0;
	static int save_button = 0;
	static int save_as_button = 0;
	static int open_button = 0;
	static int append_button = 0;
	static int exit_button = 0;

	RECT slot;
	ui_hsplit_t(&view, 2.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&new_map_button, "New", 0, &slot, draw_editor_button_menuitem, 0, "Creates a new map"))
	{
		editor.reset();
		return 1;
	}

	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&open_button, "Open", 0, &slot, draw_editor_button_menuitem, 0, "Opens a map for editing"))
	{
		editor.invoke_file_dialog(LISTDIRTYPE_ALL, "Open Map", "Open", "maps/", "", callback_open_map);
		return 1;
	}

	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&append_button, "Append", 0, &slot, draw_editor_button_menuitem, 0, "Opens a map and adds everything from that map to the current one"))
	{
		editor.invoke_file_dialog(LISTDIRTYPE_ALL, "Append Map", "Append", "maps/", "", callback_append_map);
		return 1;
	}

	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&save_button, "Save (NOT IMPL)", 0, &slot, draw_editor_button_menuitem, 0, "Saves the current map"))
	{
		return 1;
	}

	ui_hsplit_t(&view, 2.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&save_as_button, "Save As", 0, &slot, draw_editor_button_menuitem, 0, "Saves the current map under a new name"))
	{
		editor.invoke_file_dialog(LISTDIRTYPE_SAVE, "Save Map", "Save", "maps/", "", callback_save_map);
		return 1;
	}
	
	ui_hsplit_t(&view, 10.0f, &slot, &view);
	ui_hsplit_t(&view, 12.0f, &slot, &view);
	if(do_editor_button(&exit_button, "Exit", 0, &slot, draw_editor_button_menuitem, 0, "Exits from the editor"))
	{
		config.cl_editor = 0;
		return 1;
	}	
		
	return 0;
}

static void render_menubar(RECT menubar)
{
	static RECT file /*, view, help*/;

	ui_vsplit_l(&menubar, 60.0f, &file, &menubar);
	if(do_editor_button(&file, "File", 0, &file, draw_editor_button_menu, 0, 0))
		ui_invoke_popup_menu(&file, 1, file.x, file.y+file.h-1.0f, 120, 150, popup_menu_file);
	
	/*
	ui_vsplit_l(&menubar, 5.0f, 0, &menubar);
	ui_vsplit_l(&menubar, 60.0f, &view, &menubar);
	if(do_editor_button(&view, "View", 0, &view, draw_editor_button_menu, 0, 0))
		(void)0;

	ui_vsplit_l(&menubar, 5.0f, 0, &menubar);
	ui_vsplit_l(&menubar, 60.0f, &help, &menubar);
	if(do_editor_button(&help, "Help", 0, &help, draw_editor_button_menu, 0, 0))
		(void)0;
		*/
}

void EDITOR::render()
{
	// basic start
	gfx_clear(1.0f,0.0f,1.0f);
	RECT view = *ui_screen();
	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
	
	// reset tip
	editor.tooltip = 0;
	
	// render checker
	render_background(view, checker_texture, 32.0f, 1.0f);
	
	RECT menubar, modebar, toolbar, statusbar, envelope_editor, toolbox;
	
	if(editor.gui_active)
	{
		
		ui_hsplit_t(&view, 16.0f, &menubar, &view);
		ui_vsplit_l(&view, 80.0f, &toolbox, &view);
		ui_hsplit_t(&view, 16.0f, &toolbar, &view);
		ui_hsplit_b(&view, 16.0f, &view, &statusbar);

		if(editor.show_envelope_editor)
		{
			float size = 125.0f;
			if(editor.show_envelope_editor == 2)
				size *= 2.0f;
			else if(editor.show_envelope_editor == 3)
				size *= 3.0f;
			ui_hsplit_b(&view, size, &view, &envelope_editor);
		}
	}
	
	//	a little hack for now
	if(editor.mode == MODE_LAYERS)
		do_map_editor(view, toolbar);
	
	if(editor.gui_active)
	{
		float brightness = 0.25f;
		render_background(menubar, background_texture, 128.0f, brightness*0);
		ui_margin(&menubar, 2.0f, &menubar);

		render_background(toolbox, background_texture, 128.0f, brightness);
		ui_margin(&toolbox, 2.0f, &toolbox);
		
		render_background(toolbar, background_texture, 128.0f, brightness);
		ui_margin(&toolbar, 2.0f, &toolbar);
		ui_vsplit_l(&toolbar, 150.0f, &modebar, &toolbar);

		render_background(statusbar, background_texture, 128.0f, brightness);
		ui_margin(&statusbar, 2.0f, &statusbar);
		
		// do the toolbar
		if(editor.mode == MODE_LAYERS)
			do_toolbar(toolbar);
		
		if(editor.show_envelope_editor)
		{
			render_background(envelope_editor, background_texture, 128.0f, brightness);
			ui_margin(&envelope_editor, 2.0f, &envelope_editor);
		}
	}
		
	
	if(editor.mode == MODE_LAYERS)
		render_layers(toolbox, toolbar, view);
	else if(editor.mode == MODE_IMAGES)
		render_images(toolbox, toolbar, view);

	gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);

	if(editor.gui_active)
	{
		render_menubar(menubar);
		
		render_modebar(modebar);
		if(editor.show_envelope_editor)
			render_envelopeeditor(envelope_editor);
	}

	if(editor.dialog == DIALOG_FILE)
	{
		static int null_ui_target = 0;
		ui_set_hot_item(&null_ui_target);
		render_file_dialog();
	}
	
	
	ui_do_popup_menu();

	if(editor.gui_active)
		render_statusbar(statusbar);

	//
	if(config.ed_showkeys)
	{
		gfx_mapscreen(ui_screen()->x, ui_screen()->y, ui_screen()->w, ui_screen()->h);
		TEXT_CURSOR cursor;
		gfx_text_set_cursor(&cursor, view.x+10, view.y+view.h-24-10, 24.0f, TEXTFLAG_RENDER);
		
		int nkeys = 0;
		for(int i = 0; i < KEY_LAST; i++)
		{
			if(inp_key_pressed(i))
			{
				if(nkeys)
					gfx_text_ex(&cursor, " + ", -1);
				gfx_text_ex(&cursor, inp_key_name(i), -1);
				nkeys++;
			}
		}
	}
	
	if (editor.show_mouse_pointer)
	{
		// render butt ugly mouse cursor
		float mx = ui_mouse_x();
		float my = ui_mouse_y();
		gfx_texture_set(cursor_texture);
		gfx_quads_begin();
		if(ui_got_context == ui_hot_item())
			gfx_setcolor(1,0,0,1);
		gfx_quads_drawTL(mx,my, 16.0f, 16.0f);
		gfx_quads_end();
	}
	
}

void EDITOR::reset(bool create_default)
{
	editor.map.clean();

	// create default layers
	if(create_default)
		editor.map.create_default(entities_texture);
	
	/*
	{
	}*/
	
	selected_layer = 0;
	selected_group = 0;
	selected_quad = -1;
	selected_points = 0;
	selected_envelope = 0;
	selected_image = 0;
}

void MAP::make_game_layer(LAYER *layer)
{
	game_layer = (LAYER_GAME *)layer;
	game_layer->tex_id = entities_texture;
}

void MAP::make_game_group(LAYERGROUP *group)
{
	game_group = group;
	game_group->game_group = true;
	game_group->name = "Game";
}



void MAP::clean()
{
	groups.deleteall();
	envelopes.deleteall();
	images.deleteall();
	
	game_layer = 0x0;
	game_group = 0x0;
}

void MAP::create_default(int entities_texture)
{
	make_game_group(new_group());
	make_game_layer(new LAYER_GAME(50, 50));
	game_group->add_layer(game_layer);
}

extern "C" void editor_init()
{
	checker_texture = gfx_load_texture("editor/checker.png", IMG_AUTO, 0);
	background_texture = gfx_load_texture("editor/background.png", IMG_AUTO, 0);
	cursor_texture = gfx_load_texture("editor/cursor.png", IMG_AUTO, 0);
	entities_texture = gfx_load_texture("editor/entities.png", IMG_AUTO, 0);
	
	tileset_picker.make_palette();
	tileset_picker.readonly = true;
	
	editor.reset();
}

extern "C" void editor_update_and_render()
{
	static int mouse_x = 0;
	static int mouse_y = 0;
	
	if(editor.animate)
		editor.animate_time = (time_get()-editor.animate_start)/(float)time_freq();
	else
		editor.animate_time = 0;
	ui_got_context = 0;

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
		editor.save("maps/debug_test2.map");

	if(inp_key_down(KEY_F6))
		editor.load("maps/debug_test2.map");
	
	if(inp_key_down(KEY_F8))
		editor.load("maps/debug_test.map");
	
	if(inp_key_down(KEY_F10))
		editor.show_mouse_pointer = false;
	
	editor.render();
	
	if(inp_key_down(KEY_F10))
	{
		gfx_screenshot();
		editor.show_mouse_pointer = true;
	}
	
	inp_clear_events();
}

