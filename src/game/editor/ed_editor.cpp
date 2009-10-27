/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <base/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <engine/e_common_interface.h>
#include <engine/e_datafile.h>
#include <engine/e_config.h>
#include <engine/e_engine.h>
#include <engine/client/graphics.h>

#include <game/client/ui.hpp>
#include <game/gamecore.hpp>
#include <game/client/render.hpp>

#include "ed_editor.hpp"
#include <game/client/lineinput.hpp>

static int checker_texture = 0;
static int background_texture = 0;
static int cursor_texture = 0;
static int entities_texture = 0;

enum
{
	BUTTON_CONTEXT=1,
};



EDITOR_IMAGE::~EDITOR_IMAGE()
{
	editor->Graphics()->UnloadTexture(tex_id);
}

static const void *ui_got_context = 0;

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

void LAYERGROUP::convert(CUIRect *rect)
{
	rect->x += offset_x;
	rect->y += offset_y;
}

void LAYERGROUP::mapping(float *points)
{
	m_pMap->editor->RenderTools()->mapscreen_to_world(
		m_pMap->editor->world_offset_x, m_pMap->editor->world_offset_y,
		parallax_x/100.0f, parallax_y/100.0f,
		offset_x, offset_y,
		m_pMap->editor->Graphics()->ScreenAspect(), m_pMap->editor->world_zoom, points);
		
	points[0] += m_pMap->editor->editor_offset_x;
	points[1] += m_pMap->editor->editor_offset_y;
	points[2] += m_pMap->editor->editor_offset_x;
	points[3] += m_pMap->editor->editor_offset_y;
}

void LAYERGROUP::mapscreen()
{
	float points[4];
	mapping(points);
	m_pMap->editor->Graphics()->MapScreen(points[0], points[1], points[2], points[3]);
}

void LAYERGROUP::render()
{
	mapscreen();
	IGraphics *pGraphics = m_pMap->editor->Graphics();
	
	if(use_clipping)
	{
		float points[4];
		m_pMap->game_group->mapping(points);
		float x0 = (clip_x - points[0]) / (points[2]-points[0]);
		float y0 = (clip_y - points[1]) / (points[3]-points[1]);
		float x1 = ((clip_x+clip_w) - points[0]) / (points[2]-points[0]);
		float y1 = ((clip_y+clip_h) - points[1]) / (points[3]-points[1]);
		
		pGraphics->ClipEnable((int)(x0*pGraphics->ScreenWidth()), (int)(y0*pGraphics->ScreenHeight()),
			(int)((x1-x0)*pGraphics->ScreenWidth()), (int)((y1-y0)*pGraphics->ScreenHeight()));
	}
	
	for(int i = 0; i < layers.len(); i++)
	{
		if(layers[i]->visible && layers[i] != m_pMap->game_layer)
		{
			if(m_pMap->editor->show_detail || !(layers[i]->flags&LAYERFLAG_DETAIL))
				layers[i]->render();
		}
	}
	
	pGraphics->ClipDisable();
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
//extern int ui_do_edit_box(void *id, const CUIRect *rect, char *str, int str_size, float font_size, bool hidden=false);

int EDITOR::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, bool Hidden)
{
    int Inside = UI()->MouseInside(pRect);
	int ReturnValue = 0;
	static int AtIndex = 0;

	if(UI()->LastActiveItem() == pID)
	{
		int Len = strlen(pStr);
			
		if(Inside && UI()->MouseButton(0))
		{
			int mx_rel = (int)(UI()->MouseX() - pRect->x);

			for (int i = 1; i <= Len; i++)
			{
				if (gfx_text_width(0, FontSize, pStr, i) + 10 > mx_rel)
				{
					AtIndex = i - 1;
					break;
				}

				if (i == Len)
					AtIndex = Len;
			}
		}

		for(int i = 0; i < inp_num_events(); i++)
		{
			Len = strlen(pStr);
			LINEINPUT::manipulate(inp_get_event(i), pStr, StrSize, &Len, &AtIndex);
		}
	}

	bool JustGotActive = false;
	
	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pID)
				JustGotActive = true;
			UI()->SetActiveItem(pID);
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	RenderTools()->DrawUIRect(&Textbox, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
	Textbox.VMargin(5.0f, &Textbox);
	
	const char *pDisplayStr = pStr;
	char aStars[128];
	
	if(Hidden)
	{
		unsigned s = strlen(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		memset(aStars, '*', s);
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, -1);
	
	if (UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = gfx_text_width(0, FontSize, pDisplayStr, AtIndex);
		Textbox.x += w*UI()->Scale();
		UI()->DoLabel(&Textbox, "_", FontSize, -1);
	}

	return ReturnValue;
}

/*
int ui_do_edit_box(void *id, const CUIRect *rect, char *str, int str_size, float font_size, bool hidden=false)
{
    int inside = UI()->MouseInside(rect);
	int r = 0;
	static int at_index = 0;

	if(UI()->LastActiveItem() == id)
	{
		int len = strlen(str);

		if (inside && UI()->MouseButton(0))
		{
			int mx_rel = (int)(UI()->MouseX() - rect->x);

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
	
	if(UI()->ActiveItem() == id)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
	}
	else if(UI()->HotItem() == id)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != id)
				just_got_active = true;
			UI()->SetActiveItem(id);
		}
	}
	
	if(inside)
		UI()->SetHotItem(id);

	CUIRect textbox = *rect;
	RenderTools()->DrawUIRect(&textbox, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
	textbox.VMargin(5.0f, &textbox);
	
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

	UI()->DoLabel(&textbox, display_str, font_size, -1);
	
	if (UI()->LastActiveItem() == id && !just_got_active)
	{
		float w = gfx_text_width(0, font_size, display_str, at_index);
		textbox.x += w*UI()->Scale();
		UI()->DoLabel(&textbox, "_", font_size, -1);
	}

	return r;
}
*/

vec4 EDITOR::button_color_mul(const void *id)
{
	if(UI()->ActiveItem() == id)
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == id)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

float EDITOR::ui_do_scrollbar_v(const void *id, const CUIRect *rect, float current)
{
	CUIRect handle;
	static float offset_y;
	rect->HSplitTop(33, &handle, 0);

	handle.y += (rect->h-handle.h)*current;

	/* logic */
    float ret = current;
    int inside = UI()->MouseInside(&handle);

	if(UI()->ActiveItem() == id)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
		
		float min = rect->y;
		float max = rect->h-handle.h;
		float cur = UI()->MouseY()-offset_y;
		ret = (cur-min)/max;
		if(ret < 0.0f) ret = 0.0f;
		if(ret > 1.0f) ret = 1.0f;
	}
	else if(UI()->HotItem() == id)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(id);
			offset_y = UI()->MouseY()-handle.y;
		}
	}
	
	if(inside)
		UI()->SetHotItem(id);

	// render
	CUIRect rail;
	rect->VMargin(5.0f, &rail);
	RenderTools()->DrawUIRect(&rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect slider = handle;
	slider.w = rail.x-slider.x;
	RenderTools()->DrawUIRect(&slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
	slider.x = rail.x+rail.w;
	RenderTools()->DrawUIRect(&slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);

	slider = handle;
	slider.Margin(5.0f, &slider);
	RenderTools()->DrawUIRect(&slider, vec4(1,1,1,0.25f)*button_color_mul(id), CUI::CORNER_ALL, 2.5f);
	
    return ret;
}

vec4 EDITOR::get_button_color(const void *id, int checked)
{
	if(checked < 0)
		return vec4(0,0,0,0.5f);
		
	if(checked > 0)
	{
		if(UI()->HotItem() == id)
			return vec4(1,0,0,0.75f);
		return vec4(1,0,0,0.5f);
	}
	
	if(UI()->HotItem() == id)
		return vec4(1,1,1,0.75f);
	return vec4(1,1,1,0.5f);
}

int EDITOR::DoButton_Editor_Common(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(UI()->MouseInside(pRect))
	{
		if(Flags&BUTTON_CONTEXT)
			ui_got_context = pID;
		if(tooltip)
			tooltip = pToolTip;
	}
	
	if(UI()->HotItem() == pID && pToolTip)
		tooltip = (const char *)pToolTip;
	
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);

	// Draw here
	//return UI()->DoButton(id, text, checked, r, draw_func, 0);
}


int EDITOR::DoButton_Editor(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_ALL, 3.0f);
	UI()->DoLabel(pRect, pText, 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int EDITOR::DoButton_File(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(UI()->HotItem() == pID)
		RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_ALL, 3.0f);
	
	CUIRect t = *pRect;
	t.VMargin(5.0f, &t);
	UI()->DoLabel(&t, pText, 10, -1, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

//static void draw_editor_button_menu(const void *id, const char *text, int checked, const CUIRect *rect, const void *extra)
int EDITOR::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	/*
	if(UI()->HotItem() == id) if(extra) editor.tooltip = (const char *)extra;
	if(UI()->HotItem() == id)
		RenderTools()->DrawUIRect(r, get_button_color(id, checked), CUI::CORNER_ALL, 3.0f);
	*/

	CUIRect r = *pRect;
	/*
	if(ui_popups[id == id)
	{
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), CUI::CORNER_T, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), CUI::CORNER_T, 3.0f);
	}
	else*/
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f, 1.0f), CUI::CORNER_T, 3.0f);
	

	r = *pRect;
	r.VMargin(5.0f, &r);
	UI()->DoLabel(&r, pText, 10, -1, -1);
	
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
	
	//CUIRect t = *r;
}

int EDITOR::DoButton_MenuItem(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(UI()->HotItem() == pID || Checked)
		RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_ALL, 3.0f);
	
	CUIRect t = *pRect;
	t.VMargin(5.0f, &t);
	UI()->DoLabel(&t, pText, 10, -1, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, 0, 0);
}

int EDITOR::DoButton_ButtonL(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_L, 3.0f);
	UI()->DoLabel(pRect, pText, 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int EDITOR::DoButton_ButtonM(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), 0, 3.0f);
	UI()->DoLabel(pRect, pText, 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int EDITOR::DoButton_ButtonR(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_R, 3.0f);
	UI()->DoLabel(pRect, pText, 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int EDITOR::DoButton_ButtonInc(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_R, 3.0f);
	UI()->DoLabel(pRect, pText?pText:">", 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int EDITOR::DoButton_ButtonDec(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, get_button_color(pID, Checked), CUI::CORNER_L, 3.0f);
	UI()->DoLabel(pRect, pText?pText:"<", 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

/*
static void draw_editor_button_l(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	RenderTools()->DrawUIRect(r, get_button_color(id, checked), CUI::CORNER_L, 3.0f);
	UI()->DoLabel(r, text, 10, 0, -1);
}

static void draw_editor_button_m(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(UI()->HotItem() == id) if(extra) editor.tooltip = (const char *)extra;
	RenderTools()->DrawUIRect(r, get_button_color(id, checked), 0, 3.0f);
	UI()->DoLabel(r, text, 10, 0, -1);
}

static void draw_editor_button_r(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(UI()->HotItem() == id) if(extra) editor.tooltip = (const char *)extra;
	RenderTools()->DrawUIRect(r, get_button_color(id, checked), CUI::CORNER_R, 3.0f);
	UI()->DoLabel(r, text, 10, 0, -1);
}

static void draw_inc_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(UI()->HotItem == id) if(extra) editor.tooltip = (const char *)extra;
	RenderTools()->DrawUIRect(r, get_button_color(id, checked), CUI::CORNER_R, 3.0f);
	UI()->DoLabel(r, text?text:">", 10, 0, -1);
}

static void draw_dec_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(UI()->HotItem == id) if(extra) editor.tooltip = (const char *)extra;
	RenderTools()->DrawUIRect(r, get_button_color(id, checked), CUI::CORNER_L, 3.0f);
	UI()->DoLabel(r, text?text:"<", 10, 0, -1);
}

int do_editor_button(const void *id, const char *text, int checked, const CUIRect *r, ui_draw_button_func draw_func, int flags, const char *tooltip)
{
	if(UI()->MouseInside(r))
	{
		if(flags&BUTTON_CONTEXT)
			ui_got_context = id;
		if(tooltip)
			editor.tooltip = tooltip;
	}
	
	return UI()->DoButton(id, text, checked, r, draw_func, 0);
}*/


void EDITOR::render_background(CUIRect view, int texture, float size, float brightness)
{
	Graphics()->TextureSet(texture);
	Graphics()->BlendNormal();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(brightness,brightness,brightness,1.0f);
	Graphics()->QuadsSetSubset(0,0, view.w/size, view.h/size);
	Graphics()->QuadsDrawTL(view.x, view.y, view.w, view.h);
	Graphics()->QuadsEnd();
}

static LAYERGROUP brush;
static LAYER_TILES tileset_picker(16, 16);

int EDITOR::ui_do_value_selector(void *id, CUIRect *r, const char *label, int current, int min, int max, float scale)
{
    /* logic */
    static float value;
    int ret = 0;
    int inside = UI()->MouseInside(r);

	if(UI()->ActiveItem() == id)
	{
		if(!UI()->MouseButton(0))
		{
			if(inside)
				ret = 1;
			lock_mouse = false;
			UI()->SetActiveItem(0);
		}
		else
		{
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
				value += mouse_delta_x*0.05f;
			else
				value += mouse_delta_x;
			
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
	else if(UI()->HotItem() == id)
	{
		if(UI()->MouseButton(0))
		{
			lock_mouse = true;
			value = 0;
			UI()->SetActiveItem(id);
		}
	}
	
	if(inside)
		UI()->SetHotItem(id);

	// render
	char buf[128];
	sprintf(buf, "%s %d", label, current);
	RenderTools()->DrawUIRect(r, get_button_color(id, 0), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(r, buf, 10, 0, -1);
	return current;
}

LAYERGROUP *EDITOR::get_selected_group()
{
	if(selected_group >= 0 && selected_group < map.groups.len())
		return map.groups[selected_group];
	return 0x0;
}

LAYER *EDITOR::get_selected_layer(int index)
{
	LAYERGROUP *group = get_selected_group();
	if(!group)
		return 0x0;

	if(selected_layer >= 0 && selected_layer < map.groups[selected_group]->layers.len())
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

static void callback_open_map(const char *filename, void *user) { ((EDITOR*)user)->load(filename); }
static void callback_append_map(const char *filename, void *user) { ((EDITOR*)user)->append(filename); }
static void callback_save_map(const char *filename, void *user) { ((EDITOR*)user)->save(filename); }

void EDITOR::do_toolbar(CUIRect toolbar)
{
	CUIRect button;
	
	// ctrl+o to open
	if(inp_key_down('o') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL)))
		invoke_file_dialog(LISTDIRTYPE_ALL, "Open Map", "Open", "maps/", "", callback_open_map, this);
	
	// ctrl+s to save
	if(inp_key_down('s') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL)))
		invoke_file_dialog(LISTDIRTYPE_SAVE, "Save Map", "Save", "maps/", "", callback_save_map, this);

	// detail button
	toolbar.VSplitLeft(30.0f, &button, &toolbar);
	static int hq_button = 0;
	if(DoButton_Editor(&hq_button, "Detail", show_detail, &button, 0, "[ctrl+h] Toggle High Detail") ||
		(inp_key_down('h') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		show_detail = !show_detail;
	}

	toolbar.VSplitLeft(5.0f, 0, &toolbar);
	
	// animation button
	toolbar.VSplitLeft(30.0f, &button, &toolbar);
	static int animate_button = 0;
	if(DoButton_Editor(&animate_button, "Anim", animate, &button, 0, "[ctrl+m] Toggle animation") ||
		(inp_key_down('m') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		animate_start = time_get();
		animate = !animate;
	}

	toolbar.VSplitLeft(5.0f, 0, &toolbar);

	// proof button
	toolbar.VSplitLeft(30.0f, &button, &toolbar);
	static int proof_button = 0;
	if(DoButton_Editor(&proof_button, "Proof", proof_borders, &button, 0, "[ctrl-p] Toggles proof borders. These borders represent what a player maximum can see.") ||
		(inp_key_down('p') && (inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))))
	{
		proof_borders = !proof_borders;
	}

	toolbar.VSplitLeft(15.0f, 0, &toolbar);
	
	// zoom group
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int zoom_out_button = 0;
	if(DoButton_ButtonL(&zoom_out_button, "ZO", 0, &button, 0, "[NumPad-] Zoom out") || inp_key_down(KEY_KP_MINUS))
		zoom_level += 50;
		
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int zoom_normal_button = 0;
	if(DoButton_ButtonM(&zoom_normal_button, "1:1", 0, &button, 0, "[NumPad*] Zoom to normal and remove editor offset") || inp_key_down(KEY_KP_MULTIPLY))
	{
		editor_offset_x = 0;
		editor_offset_y = 0;
		zoom_level = 100;
	}
		
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int zoom_in_button = 0;
	if(DoButton_ButtonR(&zoom_in_button, "ZI", 0, &button, 0, "[NumPad+] Zoom in") || inp_key_down(KEY_KP_PLUS))
		zoom_level -= 50;
	
	toolbar.VSplitLeft(15.0f, 0, &toolbar);
	
	// animation speed
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int anim_faster_button = 0;
	if(DoButton_ButtonL(&anim_faster_button, "A+", 0, &button, 0, "Increase animation speed"))
		animate_speed += 0.5f;
	
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int anim_normal_button = 0;
	if(DoButton_ButtonM(&anim_normal_button, "1", 0, &button, 0, "Normal animation speed"))
		animate_speed = 1.0f;
	
	toolbar.VSplitLeft(16.0f, &button, &toolbar);
	static int anim_slower_button = 0;
	if(DoButton_ButtonR(&anim_slower_button, "A-", 0, &button, 0, "Decrease animation speed"))
	{
		if(animate_speed > 0.5f)
			animate_speed -= 0.5f;
	}
	
	if(inp_key_presses(KEY_MOUSE_WHEEL_UP) && dialog == DIALOG_NONE)
		zoom_level -= 20;
		
	if(inp_key_presses(KEY_MOUSE_WHEEL_DOWN) && dialog == DIALOG_NONE)
		zoom_level += 20;
	
	if(zoom_level < 50)
		zoom_level = 50;
	world_zoom = zoom_level/100.0f;

	toolbar.VSplitLeft(10.0f, &button, &toolbar);


	// brush manipulation
	{	
		int enabled = brush.is_empty()?-1:0;
		
		// flip buttons
		toolbar.VSplitLeft(20.0f, &button, &toolbar);
		static int flipx_button = 0;
		if(DoButton_ButtonL(&flipx_button, "^X", enabled, &button, 0, "[N] Flip brush horizontal") || inp_key_down('n'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_x();
		}
			
		toolbar.VSplitLeft(20.0f, &button, &toolbar);
		static int flipy_button = 0;
		if(DoButton_ButtonR(&flipy_button, "^Y", enabled, &button, 0, "[M] Flip brush vertical") || inp_key_down('m'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_flip_y();
		}

		// rotate buttons
		toolbar.VSplitLeft(20.0f, &button, &toolbar);
		
		toolbar.VSplitLeft(30.0f, &button, &toolbar);
		static int rotation_amount = 90;
		rotation_amount = ui_do_value_selector(&rotation_amount, &button, "", rotation_amount, 1, 360, 2.0f);
		
		toolbar.VSplitLeft(5.0f, &button, &toolbar);
		toolbar.VSplitLeft(30.0f, &button, &toolbar);
		static int ccw_button = 0;
		if(DoButton_ButtonL(&ccw_button, "CCW", enabled, &button, 0, "[R] Rotates the brush counter clockwise") || inp_key_down('r'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_rotate(-rotation_amount/360.0f*pi*2);
		}
			
		toolbar.VSplitLeft(30.0f, &button, &toolbar);
		static int cw_button = 0;
		if(DoButton_ButtonR(&cw_button, "CW", enabled, &button, 0, "[T] Rotates the brush clockwise") || inp_key_down('t'))
		{
			for(int i = 0; i < brush.layers.len(); i++)
				brush.layers[i]->brush_rotate(rotation_amount/360.0f*pi*2);
		}
	}

	// quad manipulation
	{
		// do add button
		toolbar.VSplitLeft(10.0f, &button, &toolbar);
		toolbar.VSplitLeft(60.0f, &button, &toolbar);
		static int new_button = 0;
		
		LAYER_QUADS *qlayer = (LAYER_QUADS *)get_selected_layer_type(0, LAYERTYPE_QUADS);
		//LAYER_TILES *tlayer = (LAYER_TILES *)get_selected_layer_type(0, LAYERTYPE_TILES);
		if(DoButton_Editor(&new_button, "Add Quad", qlayer?0:-1, &button, 0, "Adds a new quad"))
		{
			if(qlayer)
			{
				float mapping[4];
				LAYERGROUP *g = get_selected_group();
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

void EDITOR::do_quad(QUAD *q, int index)
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
	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();
	
	// get pivot
	float center_x = fx2f(q->points[4].x);
	float center_y = fx2f(q->points[4].y);

	float dx = (center_x - wx);
	float dy = (center_y - wy);
	if(dx*dx+dy*dy < 10*10)
		UI()->SetHotItem(id);

	// draw selection background	
	if(selected_quad == index)
	{
		Graphics()->SetColor(0,0,0,1);
		Graphics()->QuadsDraw(center_x, center_y, 7.0f, 7.0f);
	}
	
	if(UI()->ActiveItem() == id)
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
		
		rotate_angle += (mouse_delta_x) * 0.002f;
		last_wx = wx;
		last_wy = wy;
		
		if(operation == OP_CONTEXT_MENU)
		{
			if(!UI()->MouseButton(1))
			{
				static int quad_popup_id = 0;
				ui_invoke_popup_menu(&quad_popup_id, 0, UI()->MouseX(), UI()->MouseY(), 120, 150, popup_quad);
				lock_mouse = false;
				operation = OP_NONE;
				UI()->SetActiveItem(0);
			}
		}
		else
		{
			if(!UI()->MouseButton(0))
			{
				lock_mouse = false;
				operation = OP_NONE;
				UI()->SetActiveItem(0);
			}
		}			

		Graphics()->SetColor(1,1,1,1);
	}
	else if(UI()->HotItem() == id)
	{
		ui_got_context = id;
		
		Graphics()->SetColor(1,1,1,1);
		tooltip = "Left mouse button to move. Hold shift to move pivot. Hold ctrl to rotate";
		
		if(UI()->MouseButton(0))
		{
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
				operation = OP_MOVE_PIVOT;
			else if(inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL))
			{
				lock_mouse = true;
				operation = OP_ROTATE;
				rotate_angle = 0;
				rotate_points[0] = q->points[0];
				rotate_points[1] = q->points[1];
				rotate_points[2] = q->points[2];
				rotate_points[3] = q->points[3];
			}
			else
				operation = OP_MOVE_ALL;
				
			UI()->SetActiveItem(id);
			selected_quad = index;
			last_wx = wx;
			last_wy = wy;
		}
		
		if(UI()->MouseButton(1))
		{
			selected_quad = index;
			operation = OP_CONTEXT_MENU;
			UI()->SetActiveItem(id);
		}
	}
	else
		Graphics()->SetColor(0,1,0,1);

	Graphics()->QuadsDraw(center_x, center_y, 5.0f, 5.0f);
}

void EDITOR::do_quad_point(QUAD *q, int quad_index, int v)
{
	void *id = &q->points[v];

	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();
	
	float px = fx2f(q->points[v].x);
	float py = fx2f(q->points[v].y);
	
	float dx = (px - wx);
	float dy = (py - wy);
	if(dx*dx+dy*dy < 10*10)
		UI()->SetHotItem(id);

	// draw selection background	
	if(selected_quad == quad_index && selected_points&(1<<v))
	{
		Graphics()->SetColor(0,0,0,1);
		Graphics()->QuadsDraw(px, py, 7.0f, 7.0f);
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

	if(UI()->ActiveItem() == id)
	{
		float dx = mouse_delta_wx;
		float dy = mouse_delta_wy;
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
		
		if(operation == OP_CONTEXT_MENU)
		{
			if(!UI()->MouseButton(1))
			{
				static int point_popup_id = 0;
				ui_invoke_popup_menu(&point_popup_id, 0, UI()->MouseX(), UI()->MouseY(), 120, 150, popup_point);
				UI()->SetActiveItem(0);
			}
		}
		else
		{
			if(!UI()->MouseButton(0))
			{
				if(!moved)
				{
					if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
						selected_points ^= 1<<v;
					else
						selected_points = 1<<v;
				}
				lock_mouse = false;
				UI()->SetActiveItem(0);
			}
		}

		Graphics()->SetColor(1,1,1,1);
	}
	else if(UI()->HotItem() == id)
	{
		ui_got_context = id;
		
		Graphics()->SetColor(1,1,1,1);
		tooltip = "Left mouse button to move. Hold shift to move the texture.";
		
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(id);
			moved = false;
			if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
			{
				operation = OP_MOVEUV;
				lock_mouse = true;
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
															
			selected_quad = quad_index;
		}
		else if(UI()->MouseButton(1))
		{
			operation = OP_CONTEXT_MENU;
			selected_quad = quad_index;
			UI()->SetActiveItem(id);
		}
	}
	else
		Graphics()->SetColor(1,0,0,1);
	
	Graphics()->QuadsDraw(px, py, 5.0f, 5.0f);	
}

void EDITOR::do_map_editor(CUIRect view, CUIRect toolbar)
{
	//UI()->ClipEnable(&view);
	
	bool show_picker = inp_key_pressed(KEY_SPACE) != 0 && dialog == DIALOG_NONE;

	// render all good stuff
	if(!show_picker)
	{
		for(int g = 0; g < map.groups.len(); g++)
		{
			if(map.groups[g]->visible)
				map.groups[g]->render();
			//UI()->ClipEnable(&view);
		}
		
		// render the game above everything else
		if(map.game_group->visible && map.game_layer->visible)
		{
			map.game_group->mapscreen();
			map.game_layer->render();
		}
	}

	static void *editor_id = (void *)&editor_id;
	int inside = UI()->MouseInside(&view);

	// fetch mouse position
	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();
	float mx = UI()->MouseX();
	float my = UI()->MouseY();
	
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
		CUIRect screen = *UI()->Screen();
		float size = 32.0*16.0f;
		float w = size*(screen.w/view.w);
		float h = size*(screen.h/view.h);
		float x = -(view.x/screen.w)*w;
		float y = -(view.y/screen.h)*h;
		wx = x+w*mx/screen.w;
		wy = y+h*my/screen.h;
		Graphics()->MapScreen(x, y, x+w, y+h);
		LAYER_TILES *t = (LAYER_TILES *)get_selected_layer_type(0, LAYERTYPE_TILES);
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
		edit_layers[0] = get_selected_layer(0);
		if(edit_layers[0])
			num_edit_layers++;

		LAYERGROUP *g = get_selected_group();
		if(g)
		{
			g->mapscreen();
			
			for(int i = 0; i < num_edit_layers; i++)
			{
				if(edit_layers[i]->type != LAYERTYPE_TILES)
					continue;
					
				float w, h;
				edit_layers[i]->get_size(&w, &h);

				Graphics()->TextureSet(-1);
				Graphics()->LinesBegin();
				Graphics()->LinesDraw(0,0, w,0);
				Graphics()->LinesDraw(w,0, w,h);
				Graphics()->LinesDraw(w,h, 0,h);
				Graphics()->LinesDraw(0,h, 0,0);
				Graphics()->LinesEnd();
			}
		}
	}
		
	if(inside)
	{
		UI()->SetHotItem(editor_id);
				
		// do global operations like pan and zoom
		if(UI()->ActiveItem() == 0 && (UI()->MouseButton(0) || UI()->MouseButton(2)))
		{
			start_wx = wx;
			start_wy = wy;
			start_mx = mx;
			start_my = my;
					
			if(inp_key_pressed(KEY_LCTRL) || inp_key_pressed(KEY_RCTRL) || UI()->MouseButton(2))
			{
				if(inp_key_pressed(KEY_LSHIFT))
					operation = OP_PAN_EDITOR;
				else
					operation = OP_PAN_WORLD;
				UI()->SetActiveItem(editor_id);
			}
		}

		// brush editing
		if(UI()->HotItem() == editor_id)
		{
			if(brush.is_empty())
				tooltip = "Use left mouse button to drag and create a brush.";
			else
				tooltip = "Use left mouse button to paint with the brush. Right button clears the brush.";

			if(UI()->ActiveItem() == editor_id)
			{
				CUIRect r;
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
					if(!UI()->MouseButton(0))
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
						Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
					}
				}
			}
			else
			{
				if(UI()->MouseButton(1))
					brush.clear();
					
				if(UI()->MouseButton(0) && operation == OP_NONE)
				{
					UI()->SetActiveItem(editor_id);
					
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
				
					LAYERGROUP *g = get_selected_group();
					brush.offset_x += g->offset_x;
					brush.offset_y += g->offset_y;
					brush.parallax_x = g->parallax_x;
					brush.parallax_y = g->parallax_y;
					brush.render();
					float w, h;
					brush.get_size(&w, &h);
					
					Graphics()->TextureSet(-1);
					Graphics()->LinesBegin();
					Graphics()->LinesDraw(0,0, w,0);
					Graphics()->LinesDraw(w,0, w,h);
					Graphics()->LinesDraw(w,h, 0,h);
					Graphics()->LinesDraw(0,h, 0,0);
					Graphics()->LinesEnd();
					
				}
			}
		}
		
		// quad editing
		{
			if(!show_picker && brush.is_empty())
			{
				// fetch layers
				LAYERGROUP *g = get_selected_group();
				if(g)
					g->mapscreen();
					
				for(int k = 0; k < num_edit_layers; k++)
				{
					if(edit_layers[k]->type == LAYERTYPE_QUADS)
					{
						LAYER_QUADS *layer = (LAYER_QUADS *)edit_layers[k];
		
						Graphics()->TextureSet(-1);
						Graphics()->QuadsBegin();				
						for(int i = 0; i < layer->quads.len(); i++)
						{
							for(int v = 0; v < 4; v++)
								do_quad_point(&layer->quads[i], i, v);
								
							do_quad(&layer->quads[i], i);
						}
						Graphics()->QuadsEnd();
					}
				}
				
				Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
			}		
			
			// do panning
			if(UI()->ActiveItem() == editor_id)
			{
				if(operation == OP_PAN_WORLD)
				{
					world_offset_x -= mouse_delta_x*world_zoom;
					world_offset_y -= mouse_delta_y*world_zoom;
				}
				else if(operation == OP_PAN_EDITOR)
				{
					editor_offset_x -= mouse_delta_x*world_zoom;
					editor_offset_y -= mouse_delta_y*world_zoom;
				}

				// release mouse
				if(!UI()->MouseButton(0))
				{
					operation = OP_NONE;
					UI()->SetActiveItem(0);
				}
			}
		}
	}
	
	if(get_selected_group() && get_selected_group()->use_clipping)
	{
		LAYERGROUP *g = map.game_group;
		g->mapscreen();
		
		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();

			CUIRect r;
			r.x = get_selected_group()->clip_x;
			r.y = get_selected_group()->clip_y;
			r.w = get_selected_group()->clip_w;
			r.h = get_selected_group()->clip_h;
			
			Graphics()->SetColor(1,0,0,1);
			Graphics()->LinesDraw(r.x, r.y, r.x+r.w, r.y);
			Graphics()->LinesDraw(r.x+r.w, r.y, r.x+r.w, r.y+r.h);
			Graphics()->LinesDraw(r.x+r.w, r.y+r.h, r.x, r.y+r.h);
			Graphics()->LinesDraw(r.x, r.y+r.h, r.x, r.y);
			
		Graphics()->LinesEnd();
	}

	// render screen sizes	
	if(proof_borders)
	{
		LAYERGROUP *g = map.game_group;
		g->mapscreen();
		
		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();
		
		float last_points[4];
		float start = 1.0f; //9.0f/16.0f;
		float end = 16.0f/9.0f;
		const int num_steps = 20;
		for(int i = 0; i <= num_steps; i++)
		{
			float points[4];
			float aspect = start + (end-start)*(i/(float)num_steps);
			
			RenderTools()->mapscreen_to_world(
				world_offset_x, world_offset_y,
				1.0f, 1.0f, 0.0f, 0.0f, aspect, 1.0f, points);
			
			if(i == 0)
			{
				Graphics()->LinesDraw(points[0], points[1], points[2], points[1]);
				Graphics()->LinesDraw(points[0], points[3], points[2], points[3]);
			}

			if(i != 0)
			{
				Graphics()->LinesDraw(points[0], points[1], last_points[0], last_points[1]);
				Graphics()->LinesDraw(points[2], points[1], last_points[2], last_points[1]);
				Graphics()->LinesDraw(points[0], points[3], last_points[0], last_points[3]);
				Graphics()->LinesDraw(points[2], points[3], last_points[2], last_points[3]);
			}

			if(i == num_steps)
			{
				Graphics()->LinesDraw(points[0], points[1], points[0], points[3]);
				Graphics()->LinesDraw(points[2], points[1], points[2], points[3]);
			}
			
			mem_copy(last_points, points, sizeof(points));
		}

		if(1)
		{
			Graphics()->SetColor(1,0,0,1);
			for(int i = 0; i < 2; i++)
			{
				float points[4];
				float aspects[] = {4.0f/3.0f, 16.0f/10.0f, 5.0f/4.0f, 16.0f/9.0f};
				float aspect = aspects[i];
				
				RenderTools()->mapscreen_to_world(
					world_offset_x, world_offset_y,
					1.0f, 1.0f, 0.0f, 0.0f, aspect, 1.0f, points);
				
				CUIRect r;
				r.x = points[0];
				r.y = points[1];
				r.w = points[2]-points[0];
				r.h = points[3]-points[1];
				
				Graphics()->LinesDraw(r.x, r.y, r.x+r.w, r.y);
				Graphics()->LinesDraw(r.x+r.w, r.y, r.x+r.w, r.y+r.h);
				Graphics()->LinesDraw(r.x+r.w, r.y+r.h, r.x, r.y+r.h);
				Graphics()->LinesDraw(r.x, r.y+r.h, r.x, r.y);
				Graphics()->SetColor(0,1,0,1);
			}
		}
			
		Graphics()->LinesEnd();
	}
	
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
	//UI()->ClipDisable();
}


int EDITOR::do_properties(CUIRect *toolbox, PROPERTY *props, int *ids, int *new_val)
{
	int change = -1;

	for(int i = 0; props[i].name; i++)
	{
		CUIRect slot;
		toolbox->HSplitTop(13.0f, &slot, toolbox);
		CUIRect label, shifter;
		slot.VSplitMid(&label, &shifter);
		shifter.HMargin(1.0f, &shifter);
		UI()->DoLabel(&label, props[i].name, 10.0f, -1, -1);
		
		if(props[i].type == PROPTYPE_INT_STEP)
		{
			CUIRect inc, dec;
			char buf[64];
			
			shifter.VSplitRight(10.0f, &shifter, &inc);
			shifter.VSplitLeft(10.0f, &dec, &shifter);
			sprintf(buf, "%d", props[i].value);
			RenderTools()->DrawUIRect(&shifter, vec4(1,1,1,0.5f), 0, 0.0f);
			UI()->DoLabel(&shifter, buf, 10.0f, 0, -1);
			
			if(DoButton_ButtonDec(&ids[i], 0, 0, &dec, 0, "Decrease"))
			{
				if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
					*new_val = props[i].value-5;
				else
					*new_val = props[i].value-1;
				change = i;
			}
			if(DoButton_ButtonInc(((char *)&ids[i])+1, 0, 0, &inc, 0, "Increase"))
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
			CUIRect no, yes;
			shifter.VSplitMid(&no, &yes);
			if(DoButton_ButtonDec(&ids[i], "No", !props[i].value, &no, 0, ""))
			{
				*new_val = 0;
				change = i;
			}
			if(DoButton_ButtonInc(((char *)&ids[i])+1, "Yes", props[i].value, &yes, 0, ""))
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
					toolbox->HSplitTop(13.0f, &slot, toolbox);
					slot.VSplitMid(0, &shifter);
					shifter.HMargin(1.0f, &shifter);
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
				sprintf(buf, "%s",  map.images[props[i].value]->name);
			
			if(DoButton_Editor(&ids[i], buf, 0, &shifter, 0, 0))
				popup_select_image_invoke(props[i].value, UI()->MouseX(), UI()->MouseY());
			
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

void EDITOR::render_layers(CUIRect toolbox, CUIRect toolbar, CUIRect view)
{
	CUIRect layersbox = toolbox;

	if(!gui_active)
		return;
			
	CUIRect slot, button;
	char buf[64];

	int valid_group = 0;
	int valid_layer = 0;
	if(selected_group >= 0 && selected_group < map.groups.len())
		valid_group = 1;

	if(valid_group && selected_layer >= 0 && selected_layer < map.groups[selected_group]->layers.len())
		valid_layer = 1;
		
	// render layers	
	{
		for(int g = 0; g < map.groups.len(); g++)
		{
			CUIRect visible_toggle;
			layersbox.HSplitTop(12.0f, &slot, &layersbox);
			slot.VSplitLeft(12, &visible_toggle, &slot);
			if(DoButton_ButtonL(&map.groups[g]->visible, map.groups[g]->visible?"V":"H", 0, &visible_toggle, 0, "Toggle group visibility"))
				map.groups[g]->visible = !map.groups[g]->visible;

			sprintf(buf, "#%d %s", g, map.groups[g]->name);
			if(int result = DoButton_ButtonR(&map.groups[g], buf, g==selected_group, &slot,
				BUTTON_CONTEXT, "Select group. Right click for properties."))
			{
				selected_group = g;
				selected_layer = 0;
				
				static int group_popup_id = 0;
				if(result == 2)
					ui_invoke_popup_menu(&group_popup_id, 0, UI()->MouseX(), UI()->MouseY(), 120, 200, popup_group);
			}
			
			
			layersbox.HSplitTop(2.0f, &slot, &layersbox);
			
			for(int i = 0; i < map.groups[g]->layers.len(); i++)
			{
				//visible
				layersbox.HSplitTop(12.0f, &slot, &layersbox);
				slot.VSplitLeft(12.0f, 0, &button);
				button.VSplitLeft(15, &visible_toggle, &button);

				if(DoButton_ButtonL(&map.groups[g]->layers[i]->visible, map.groups[g]->layers[i]->visible?"V":"H", 0, &visible_toggle, 0, "Toggle layer visibility"))
					map.groups[g]->layers[i]->visible = !map.groups[g]->layers[i]->visible;

				sprintf(buf, "#%d %s ", i, map.groups[g]->layers[i]->type_name);
				if(int result = DoButton_ButtonR(map.groups[g]->layers[i], buf, g==selected_group&&i==selected_layer, &button,
					BUTTON_CONTEXT, "Select layer. Right click for properties."))
				{
					selected_layer = i;
					selected_group = g;
					static int layer_popup_id = 0;
					if(result == 2)
						ui_invoke_popup_menu(&layer_popup_id, 0, UI()->MouseX(), UI()->MouseY(), 120, 150, popup_layer);
				}
				
				
				layersbox.HSplitTop(2.0f, &slot, &layersbox);
			}
			layersbox.HSplitTop(5.0f, &slot, &layersbox);
		}
	}
	

	{
		layersbox.HSplitTop(12.0f, &slot, &layersbox);

		static int new_group_button = 0;
		if(DoButton_Editor(&new_group_button, "Add Group", 0, &slot, 0, "Adds a new group"))
		{
			map.new_group();
			selected_group = map.groups.len()-1;
		}
	}

	layersbox.HSplitTop(5.0f, &slot, &layersbox);
	
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

void EDITOR::replace_image(const char *filename, void *user)
{
	EDITOR *editor = (EDITOR *)user;
	EDITOR_IMAGE imginfo(editor);
	if(!editor->Graphics()->LoadPNG(&imginfo, filename))
		return;
	
	EDITOR_IMAGE *img = editor->map.images[editor->selected_image];
	editor->Graphics()->UnloadTexture(img->tex_id);
	*img = imginfo;
	extract_name(filename, img->name);
	img->tex_id = editor->Graphics()->LoadTextureRaw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO, 0);
}

void EDITOR::add_image(const char *filename, void *user)
{
	EDITOR *editor = (EDITOR *)user;
	EDITOR_IMAGE imginfo(editor);
	if(!editor->Graphics()->LoadPNG(&imginfo, filename))
		return;

	EDITOR_IMAGE *img = new EDITOR_IMAGE(editor);
	*img = imginfo;
	img->tex_id = editor->Graphics()->LoadTextureRaw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO, 0);
	img->external = 1; // external by default
	extract_name(filename, img->name);
	editor->map.images.add(img);
}


static int modify_index_deleted_index;
static void modify_index_deleted(int *index)
{
	if(*index == modify_index_deleted_index)
		*index = -1;
	else if(*index > modify_index_deleted_index)
		*index = *index - 1;
}

int EDITOR::popup_image(EDITOR *pEditor, CUIRect view)
{
	static int replace_button = 0;	
	static int remove_button = 0;	

	CUIRect slot;
	view.HSplitTop(2.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	EDITOR_IMAGE *img = pEditor->map.images[pEditor->selected_image];
	
	static int external_button = 0;
	if(img->external)
	{
		if(pEditor->DoButton_MenuItem(&external_button, "Embedd", 0, &slot, 0, "Embedds the image into the map file."))
		{
			img->external = 0;
			return 1;
		}
	}
	else
	{		
		if(pEditor->DoButton_MenuItem(&external_button, "Make external", 0, &slot, 0, "Removes the image from the map file."))
		{
			img->external = 1;
			return 1;
		}
	}

	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&replace_button, "Replace", 0, &slot, 0, "Replaces the image with a new one"))
	{
		pEditor->invoke_file_dialog(LISTDIRTYPE_ALL, "Replace Image", "Replace", "mapres/", "", replace_image, pEditor);
		return 1;
	}

	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&remove_button, "Remove", 0, &slot, 0, "Removes the image from the map"))
	{
		delete img;
		pEditor->map.images.removebyindex(pEditor->selected_image);
		modify_index_deleted_index = pEditor->selected_image;
		pEditor->map.modify_image_index(modify_index_deleted);
		return 1;
	}

	return 0;
}


void EDITOR::render_images(CUIRect toolbox, CUIRect toolbar, CUIRect view)
{
	for(int e = 0; e < 2; e++) // two passes, first embedded, then external
	{
		CUIRect slot;
		toolbox.HSplitTop(15.0f, &slot, &toolbox);
		if(e == 0)
			UI()->DoLabel(&slot, "Embedded", 12.0f, 0);
		else
			UI()->DoLabel(&slot, "External", 12.0f, 0);
		
		for(int i = 0; i < map.images.len(); i++)
		{
			if((e && !map.images[i]->external) ||
				(!e && map.images[i]->external))
			{
				continue;
			}
			
			char buf[128];
			sprintf(buf, "%s", map.images[i]->name);
			toolbox.HSplitTop(12.0f, &slot, &toolbox);
			
			if(int result = DoButton_Editor(&map.images[i], buf, selected_image == i, &slot,
				BUTTON_CONTEXT, "Select image"))
			{
				selected_image = i;
				
				static int popup_image_id = 0;
				if(result == 2)
					ui_invoke_popup_menu(&popup_image_id, 0, UI()->MouseX(), UI()->MouseY(), 120, 80, popup_image);
			}
			
			toolbox.HSplitTop(2.0f, 0, &toolbox);
			
			// render image
			if(selected_image == i)
			{
				CUIRect r;
				view.Margin(10.0f, &r);
				if(r.h < r.w)
					r.w = r.h;
				else
					r.h = r.w;
				Graphics()->TextureSet(map.images[i]->tex_id);
				Graphics()->BlendNormal();
				Graphics()->QuadsBegin();
				Graphics()->QuadsDrawTL(r.x, r.y, r.w, r.h);
				Graphics()->QuadsEnd();
				
			}
		}
	}
	
	CUIRect slot;
	toolbox.HSplitTop(5.0f, &slot, &toolbox);
	
	// new image
	static int new_image_button = 0;
	toolbox.HSplitTop(10.0f, &slot, &toolbox);
	toolbox.HSplitTop(12.0f, &slot, &toolbox);
	if(DoButton_Editor(&new_image_button, "Add", 0, &slot, 0, "Load a new image to use in the map"))
		invoke_file_dialog(LISTDIRTYPE_ALL, "Add Image", "Add", "mapres/", "", add_image, this);
}


static int file_dialog_dirtypes = 0;
static const char *file_dialog_title = 0;
static const char *file_dialog_button_text = 0;
static void (*file_dialog_func)(const char *filename, void *user);
static void *file_dialog_user = 0;
static char file_dialog_filename[512] = {0};
static char file_dialog_path[512] = {0};
static char file_dialog_complete_filename[512] = {0};
static int files_num = 0;
int files_startat = 0;
int files_cur = 0;
int files_stopat = 999;

struct LISTDIRINFO
{
	CUIRect *rect;
	EDITOR *editor;
};

static void editor_listdir_callback(const char *name, int is_dir, void *user)
{
	if(name[0] == '.' || is_dir) // skip this shit!
		return;
	
	if(files_cur > files_num)
		files_num = files_cur;
	
	files_cur++;
	if(files_cur-1 < files_startat || files_cur > files_stopat)
		return;
	
	LISTDIRINFO *info = (LISTDIRINFO *)user;
	CUIRect *view = info->rect;
	CUIRect button;
	view->HSplitTop(15.0f, &button, view);
	view->HSplitTop(2.0f, 0, view);
	//char buf[512];
	
	if(info->editor->DoButton_File((void*)(10+(int)button.y), name, 0, &button, 0, 0))
	{
		strncpy(file_dialog_filename, name, sizeof(file_dialog_filename));
		
		file_dialog_complete_filename[0] = 0;
		strcat(file_dialog_complete_filename, file_dialog_path);
		strcat(file_dialog_complete_filename, file_dialog_filename);

		if(inp_mouse_doubleclick())
		{
			if(file_dialog_func)
				file_dialog_func(file_dialog_complete_filename, user);
			info->editor->dialog = DIALOG_NONE;
		}
	}
}

void EDITOR::render_file_dialog()
{
	// GUI coordsys
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
	
	CUIRect view = *UI()->Screen();
	RenderTools()->DrawUIRect(&view, vec4(0,0,0,0.25f), 0, 0);
	view.VMargin(150.0f, &view);
	view.HMargin(50.0f, &view);
	RenderTools()->DrawUIRect(&view, vec4(0,0,0,0.75f), CUI::CORNER_ALL, 5.0f);
	view.Margin(10.0f, &view);

	CUIRect title, filebox, filebox_label, buttonbar, scroll;
	view.HSplitTop(18.0f, &title, &view);
	view.HSplitTop(5.0f, 0, &view); // some spacing
	view.HSplitBottom(14.0f, &view, &buttonbar);
	view.HSplitBottom(10.0f, &view, 0); // some spacing
	view.HSplitBottom(14.0f, &view, &filebox);
	filebox.VSplitLeft(50.0f, &filebox_label, &filebox);
	view.VSplitRight(15.0f, &view, &scroll);
	
	// title
	RenderTools()->DrawUIRect(&title, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 5.0f);
	title.VMargin(10.0f, &title);
	UI()->DoLabel(&title, file_dialog_title, 14.0f, -1, -1);
	
	// filebox
	UI()->DoLabel(&filebox_label, "Filename:", 10.0f, -1, -1);
	
	static int filebox_id = 0;
	DoEditBox(&filebox_id, &filebox, file_dialog_filename, sizeof(file_dialog_filename), 10.0f);

	file_dialog_complete_filename[0] = 0;
	strcat(file_dialog_complete_filename, file_dialog_path);
	strcat(file_dialog_complete_filename, file_dialog_filename);
	
	int num = (int)(view.h/17.0);
	static float scrollvalue = 0;
	static int scrollbar = 0;
	scroll.HMargin(5.0f, &scroll);
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
	UI()->ClipEnable(&view);
	
	// the list
	LISTDIRINFO info;
	info.rect = &view;
	info.editor = this;
	engine_listdir(file_dialog_dirtypes, file_dialog_path, editor_listdir_callback, &info);
	
	// disable clipping again
	UI()->ClipDisable();
	
	// the buttons
	static int ok_button = 0;	
	static int cancel_button = 0;	

	CUIRect button;
	buttonbar.VSplitRight(50.0f, &buttonbar, &button);
	if(DoButton_Editor(&ok_button, file_dialog_button_text, 0, &button, 0, 0) || inp_key_pressed(KEY_RETURN))
	{
		if(file_dialog_func)
			file_dialog_func(file_dialog_complete_filename, file_dialog_user);
		dialog = DIALOG_NONE;
	}

	buttonbar.VSplitRight(40.0f, &buttonbar, &button);
	buttonbar.VSplitRight(50.0f, &buttonbar, &button);
	if(DoButton_Editor(&cancel_button, "Cancel", 0, &button, 0, 0) || inp_key_pressed(KEY_ESCAPE))
		dialog = DIALOG_NONE;
}

void EDITOR::invoke_file_dialog(int listdirtypes, const char *title, const char *button_text,
	const char *basepath, const char *default_name,
	void (*func)(const char *filename, void *user), void *user)
{
	file_dialog_dirtypes = listdirtypes;
	file_dialog_title = title;
	file_dialog_button_text = button_text;
	file_dialog_func = func;
	file_dialog_user = user;
	file_dialog_filename[0] = 0;
	file_dialog_path[0] = 0;
	
	if(default_name)
		strncpy(file_dialog_filename, default_name, sizeof(file_dialog_filename));
	if(basepath)
		strncpy(file_dialog_path, basepath, sizeof(file_dialog_path));
		
	dialog = DIALOG_FILE;
}



void EDITOR::render_modebar(CUIRect view)
{
	CUIRect button;

	// mode buttons
	{
		view.VSplitLeft(40.0f, &button, &view);
		static int tile_button = 0;
		if(DoButton_ButtonM(&tile_button, "Layers", mode == MODE_LAYERS, &button, 0, "Switch to edit layers."))
			mode = MODE_LAYERS;

		view.VSplitLeft(40.0f, &button, &view);
		static int img_button = 0;
		if(DoButton_ButtonR(&img_button, "Images", mode == MODE_IMAGES, &button, 0, "Switch to manage images."))
			mode = MODE_IMAGES;
	}

	view.VSplitLeft(5.0f, 0, &view);
	
	// spacing
	//view.VSplitLeft(10.0f, 0, &view);
}

void EDITOR::render_statusbar(CUIRect view)
{
	CUIRect button;
	view.VSplitRight(60.0f, &view, &button);
	static int envelope_button = 0;
	if(DoButton_Editor(&envelope_button, "Envelopes", show_envelope_editor, &button, 0, "Toggles the envelope editor."))
		show_envelope_editor = (show_envelope_editor+1)%4;
	
	if(tooltip)
	{
		if(ui_got_context && ui_got_context == UI()->HotItem())
		{
			char buf[512];
			sprintf(buf, "%s Right click for context menu.", tooltip);
			UI()->DoLabel(&view, buf, 10.0f, -1, -1);
		}
		else
			UI()->DoLabel(&view, tooltip, 10.0f, -1, -1);
	}
}

void EDITOR::render_envelopeeditor(CUIRect view)
{
	if(selected_envelope < 0) selected_envelope = 0;
	if(selected_envelope >= map.envelopes.len()) selected_envelope--;

	ENVELOPE *envelope = 0;
	if(selected_envelope >= 0 && selected_envelope < map.envelopes.len())
		envelope = map.envelopes[selected_envelope];

	bool show_colorbar = false;
	if(envelope && envelope->channels == 4)
		show_colorbar = true;

	CUIRect toolbar, curvebar, colorbar;
	view.HSplitTop(15.0f, &toolbar, &view);
	view.HSplitTop(15.0f, &curvebar, &view);
	toolbar.Margin(2.0f, &toolbar);
	curvebar.Margin(2.0f, &curvebar);

	if(show_colorbar)
	{
		view.HSplitTop(20.0f, &colorbar, &view);
		colorbar.Margin(2.0f, &colorbar);
		render_background(colorbar, checker_texture, 16.0f, 1.0f);
	}

	render_background(view, checker_texture, 32.0f, 0.1f);

	// do the toolbar
	{
		CUIRect button;
		ENVELOPE *new_env = 0;
		
		toolbar.VSplitRight(50.0f, &toolbar, &button);
		static int new_4d_button = 0;
		if(DoButton_Editor(&new_4d_button, "Color+", 0, &button, 0, "Creates a new color envelope"))
			new_env = map.new_envelope(4);

		toolbar.VSplitRight(5.0f, &toolbar, &button);
		toolbar.VSplitRight(50.0f, &toolbar, &button);
		static int new_2d_button = 0;
		if(DoButton_Editor(&new_2d_button, "Pos.+", 0, &button, 0, "Creates a new pos envelope"))
			new_env = map.new_envelope(3);
		
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
		
		CUIRect shifter, inc, dec;
		toolbar.VSplitLeft(60.0f, &shifter, &toolbar);
		shifter.VSplitRight(15.0f, &shifter, &inc);
		shifter.VSplitLeft(15.0f, &dec, &shifter);
		char buf[512];
		sprintf(buf, "%d/%d", selected_envelope+1, map.envelopes.len());
		RenderTools()->DrawUIRect(&shifter, vec4(1,1,1,0.5f), 0, 0.0f);
		UI()->DoLabel(&shifter, buf, 10.0f, 0, -1);
		
		static int prev_button = 0;
		if(DoButton_ButtonDec(&prev_button, 0, 0, &dec, 0, "Previous Envelope"))
			selected_envelope--;
		
		static int next_button = 0;
		if(DoButton_ButtonInc(&next_button, 0, 0, &inc, 0, "Next Envelope"))
			selected_envelope++;
			
		if(envelope)
		{
			toolbar.VSplitLeft(15.0f, &button, &toolbar);
			toolbar.VSplitLeft(35.0f, &button, &toolbar);
			UI()->DoLabel(&button, "Name:", 10.0f, -1, -1);

			toolbar.VSplitLeft(80.0f, &button, &toolbar);
			
			static int name_box = 0;
			DoEditBox(&name_box, &button, envelope->name, sizeof(envelope->name), 10.0f);
		}
	}
	
	if(envelope)
	{
		static array<int> selection;
		static int envelope_editor_id = 0;
		static int active_channels = 0xf;
		
		if(envelope)
		{
			CUIRect button;	
			
			toolbar.VSplitLeft(15.0f, &button, &toolbar);

			static const char *names[4][4] = {
				{"X", "", "", ""},
				{"X", "Y", "", ""},
				{"X", "Y", "R", ""},
				{"R", "G", "B", "A"},
			};
			
			static int channel_buttons[4] = {0};
			int bit = 1;
			/*ui_draw_button_func draw_func;*/
			
			for(int i = 0; i < envelope->channels; i++, bit<<=1)
			{
				toolbar.VSplitLeft(15.0f, &button, &toolbar);
				
				/*if(i == 0) draw_func = draw_editor_button_l;
				else if(i == envelope->channels-1) draw_func = draw_editor_button_r;
				else draw_func = draw_editor_button_m;*/
				
				if(DoButton_Editor(&channel_buttons[i], names[envelope->channels-1][i], active_channels&bit, &button, 0, 0))
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
		
		if(UI()->MouseInside(&view))
			UI()->SetHotItem(&envelope_editor_id);
			
		if(UI()->HotItem() == &envelope_editor_id)
		{
			// do stuff
			if(envelope)
			{
				if(UI()->MouseButtonClicked(1))
				{
					// add point
					int time = (int)(((UI()->MouseX()-view.x)*timescale)*1000.0f);
					//float env_y = (UI()->MouseY()-view.y)/timescale;
					float channels[4];
					envelope->eval(time, channels);
					envelope->add_point(time,
						f2fx(channels[0]), f2fx(channels[1]),
						f2fx(channels[2]), f2fx(channels[3]));
				}
				
				tooltip = "Press right mouse button to create a new point";
			}
		}

		vec3 colors[] = {vec3(1,0.2f,0.2f), vec3(0.2f,1,0.2f), vec3(0.2f,0.2f,1), vec3(1,1,0.2f)};

		// render lines
		{
			UI()->ClipEnable(&view);
			Graphics()->TextureSet(-1);
			Graphics()->LinesBegin();
			for(int c = 0; c < envelope->channels; c++)
			{
				if(active_channels&(1<<c))
					Graphics()->SetColor(colors[c].r,colors[c].g,colors[c].b,1);
				else
					Graphics()->SetColor(colors[c].r*0.5f,colors[c].g*0.5f,colors[c].b*0.5f,1);
				
				float prev_x = 0;
				float results[4];
				envelope->eval(0.000001f, results);
				float prev_value = results[c];
				
				int steps = (int)((view.w/UI()->Screen()->w) * Graphics()->ScreenWidth());
				for(int i = 1; i <= steps; i++)
				{
					float a = i/(float)steps;
					envelope->eval(a*end_time, results);
					float v = results[c];
					v = (v-bottom)/(top-bottom);
					
					Graphics()->LinesDraw(view.x + prev_x*view.w, view.y+view.h - prev_value*view.h, view.x + a*view.w, view.y+view.h - v*view.h);
					prev_x = a;
					prev_value = v;
				}
			}
			Graphics()->LinesEnd();
			UI()->ClipDisable();
		}
		
		// render curve options
		{
			for(int i = 0; i < envelope->points.len()-1; i++)
			{
				float t0 = envelope->points[i].time/1000.0f/end_time;
				float t1 = envelope->points[i+1].time/1000.0f/end_time;

				//dbg_msg("", "%f", end_time);
				
				CUIRect v;
				v.x = curvebar.x + (t0+(t1-t0)*0.5f) * curvebar.w;
				v.y = curvebar.y;
				v.h = curvebar.h;
				v.w = curvebar.h;
				v.x -= v.w/2;
				void *id = &envelope->points[i].curvetype;
				const char *type_name[] = {
					"N", "L", "S", "F", "M"
					};
				
				if(DoButton_Editor(id, type_name[envelope->points[i].curvetype], 0, &v, 0, "Switch curve type"))
					envelope->points[i].curvetype = (envelope->points[i].curvetype+1)%NUM_CURVETYPES;
			}
		}
		
		// render colorbar
		if(show_colorbar)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
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

				Graphics()->SetColorVertex(0, r0, g0, b0, a0);
				Graphics()->SetColorVertex(1, r1, g1, b1, a1);
				Graphics()->SetColorVertex(2, r1, g1, b1, a1);
				Graphics()->SetColorVertex(3, r0, g0, b0, a0);

				float x0 = envelope->points[i].time/1000.0f/end_time;
//				float y0 = (fx2f(envelope->points[i].values[c])-bottom)/(top-bottom);
				float x1 = envelope->points[i+1].time/1000.0f/end_time;
				//float y1 = (fx2f(envelope->points[i+1].values[c])-bottom)/(top-bottom);
				CUIRect v;
				v.x = colorbar.x + x0*colorbar.w;
				v.y = colorbar.y;
				v.w = (x1-x0)*colorbar.w;
				v.h = colorbar.h;
				
				Graphics()->QuadsDrawTL(v.x, v.y, v.w, v.h);
			}
			Graphics()->QuadsEnd();
		}
		
		// render handles
		{
			static bool move = false;
			
			int current_value = 0, current_time = 0;
			
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			for(int c = 0; c < envelope->channels; c++)
			{
				if(!(active_channels&(1<<c)))
					continue;
				
				for(int i = 0; i < envelope->points.len(); i++)
				{
					float x0 = envelope->points[i].time/1000.0f/end_time;
					float y0 = (fx2f(envelope->points[i].values[c])-bottom)/(top-bottom);
					CUIRect final;
					final.x = view.x + x0*view.w;
					final.y = view.y+view.h - y0*view.h;
					final.x -= 2.0f;
					final.y -= 2.0f;
					final.w = 4.0f;
					final.h = 4.0f;
					
					void *id = &envelope->points[i].values[c];
					
					if(UI()->MouseInside(&final))
						UI()->SetHotItem(id);
						
					float colormod = 1.0f;

					if(UI()->ActiveItem() == id)
					{
						if(!UI()->MouseButton(0))
						{
							UI()->SetActiveItem(0);
							move = false;
						}
						else
						{
							envelope->points[i].values[c] -= f2fx(mouse_delta_y*valuescale);
							if(inp_key_pressed(KEY_LSHIFT) || inp_key_pressed(KEY_RSHIFT))
							{
								if(i != 0)
								{
									envelope->points[i].time += (int)((mouse_delta_x*timescale)*1000.0f);
									if(envelope->points[i].time < envelope->points[i-1].time)
										envelope->points[i].time = envelope->points[i-1].time + 1;
									if(i+1 != envelope->points.len() && envelope->points[i].time > envelope->points[i+1].time)
										envelope->points[i].time = envelope->points[i+1].time - 1;
								}
							}
						}
						
						colormod = 100.0f;
						Graphics()->SetColor(1,1,1,1);
					}
					else if(UI()->HotItem() == id)
					{
						if(UI()->MouseButton(0))
						{
							selection.clear();
							selection.add(i);
							UI()->SetActiveItem(id);
						}

						// remove point
						if(UI()->MouseButtonClicked(1))
							envelope->points.removebyindex(i);
							
						colormod = 100.0f;
						Graphics()->SetColor(1,0.75f,0.75f,1);
						tooltip = "Left mouse to drag. Hold shift to alter time point aswell. Right click to delete.";
					}

					if(UI()->ActiveItem() == id || UI()->HotItem() == id)
					{
						current_time = envelope->points[i].time;
						current_value = envelope->points[i].values[c];
					}
					
					Graphics()->SetColor(colors[c].r*colormod, colors[c].g*colormod, colors[c].b*colormod, 1.0f);
					Graphics()->QuadsDrawTL(final.x, final.y, final.w, final.h);
				}
			}
			Graphics()->QuadsEnd();

			char buf[512];
			sprintf(buf, "%.3f %.3f", current_time/1000.0f, fx2f(current_value));
			UI()->DoLabel(&toolbar, buf, 10.0f, 0, -1);
		}
	}
}

int EDITOR::popup_menu_file(EDITOR *pEditor, CUIRect view)
{
	static int new_map_button = 0;
	static int save_button = 0;
	static int save_as_button = 0;
	static int open_button = 0;
	static int append_button = 0;
	static int exit_button = 0;

	CUIRect slot;
	view.HSplitTop(2.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&new_map_button, "New", 0, &slot, 0, "Creates a new map"))
	{
		pEditor->reset();
		return 1;
	}

	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&open_button, "Open", 0, &slot, 0, "Opens a map for editing"))
	{
		pEditor->invoke_file_dialog(LISTDIRTYPE_ALL, "Open Map", "Open", "maps/", "", callback_open_map, pEditor);
		return 1;
	}

	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&append_button, "Append", 0, &slot, 0, "Opens a map and adds everything from that map to the current one"))
	{
		pEditor->invoke_file_dialog(LISTDIRTYPE_ALL, "Append Map", "Append", "maps/", "", callback_append_map, pEditor);
		return 1;
	}

	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&save_button, "Save (NOT IMPL)", 0, &slot, 0, "Saves the current map"))
	{
		return 1;
	}

	view.HSplitTop(2.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&save_as_button, "Save As", 0, &slot, 0, "Saves the current map under a new name"))
	{
		pEditor->invoke_file_dialog(LISTDIRTYPE_SAVE, "Save Map", "Save", "maps/", "", callback_save_map, pEditor);
		return 1;
	}
	
	view.HSplitTop(10.0f, &slot, &view);
	view.HSplitTop(12.0f, &slot, &view);
	if(pEditor->DoButton_MenuItem(&exit_button, "Exit", 0, &slot, 0, "Exits from the editor"))
	{
		config.cl_editor = 0;
		return 1;
	}	
		
	return 0;
}

void EDITOR::render_menubar(CUIRect menubar)
{
	static CUIRect file /*, view, help*/;

	menubar.VSplitLeft(60.0f, &file, &menubar);
	if(DoButton_Menu(&file, "File", 0, &file, 0, 0))
		ui_invoke_popup_menu(&file, 1, file.x, file.y+file.h-1.0f, 120, 150, popup_menu_file, this);
	
	/*
	menubar.VSplitLeft(5.0f, 0, &menubar);
	menubar.VSplitLeft(60.0f, &view, &menubar);
	if(do_editor_button(&view, "View", 0, &view, draw_editor_button_menu, 0, 0))
		(void)0;

	menubar.VSplitLeft(5.0f, 0, &menubar);
	menubar.VSplitLeft(60.0f, &help, &menubar);
	if(do_editor_button(&help, "Help", 0, &help, draw_editor_button_menu, 0, 0))
		(void)0;
		*/
}

void EDITOR::render()
{
	// basic start
	Graphics()->Clear(1.0f,0.0f,1.0f);
	CUIRect view = *UI()->Screen();
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
	
	// reset tip
	tooltip = 0;
	
	// render checker
	render_background(view, checker_texture, 32.0f, 1.0f);
	
	CUIRect menubar, modebar, toolbar, statusbar, envelope_editor, toolbox;
	
	if(gui_active)
	{
		
		view.HSplitTop(16.0f, &menubar, &view);
		view.VSplitLeft(80.0f, &toolbox, &view);
		view.HSplitTop(16.0f, &toolbar, &view);
		view.HSplitBottom(16.0f, &view, &statusbar);

		if(show_envelope_editor)
		{
			float size = 125.0f;
			if(show_envelope_editor == 2)
				size *= 2.0f;
			else if(show_envelope_editor == 3)
				size *= 3.0f;
			view.HSplitBottom(size, &view, &envelope_editor);
		}
	}
	
	//	a little hack for now
	if(mode == MODE_LAYERS)
		do_map_editor(view, toolbar);
	
	if(gui_active)
	{
		float brightness = 0.25f;
		render_background(menubar, background_texture, 128.0f, brightness*0);
		menubar.Margin(2.0f, &menubar);

		render_background(toolbox, background_texture, 128.0f, brightness);
		toolbox.Margin(2.0f, &toolbox);
		
		render_background(toolbar, background_texture, 128.0f, brightness);
		toolbar.Margin(2.0f, &toolbar);
		toolbar.VSplitLeft(150.0f, &modebar, &toolbar);

		render_background(statusbar, background_texture, 128.0f, brightness);
		statusbar.Margin(2.0f, &statusbar);
		
		// do the toolbar
		if(mode == MODE_LAYERS)
			do_toolbar(toolbar);
		
		if(show_envelope_editor)
		{
			render_background(envelope_editor, background_texture, 128.0f, brightness);
			envelope_editor.Margin(2.0f, &envelope_editor);
		}
	}
		
	
	if(mode == MODE_LAYERS)
		render_layers(toolbox, toolbar, view);
	else if(mode == MODE_IMAGES)
		render_images(toolbox, toolbar, view);

	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);

	if(gui_active)
	{
		render_menubar(menubar);
		
		render_modebar(modebar);
		if(show_envelope_editor)
			render_envelopeeditor(envelope_editor);
	}

	if(dialog == DIALOG_FILE)
	{
		static int null_ui_target = 0;
		UI()->SetHotItem(&null_ui_target);
		render_file_dialog();
	}
	
	
	ui_do_popup_menu();

	if(gui_active)
		render_statusbar(statusbar);

	//
	if(config.ed_showkeys)
	{
		Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
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
	
	if(show_mouse_pointer)
	{
		// render butt ugly mouse cursor
		float mx = UI()->MouseX();
		float my = UI()->MouseY();
		Graphics()->TextureSet(cursor_texture);
		Graphics()->QuadsBegin();
		if(ui_got_context == UI()->HotItem())
			Graphics()->SetColor(1,0,0,1);
		Graphics()->QuadsDrawTL(mx,my, 16.0f, 16.0f);
		Graphics()->QuadsEnd();
	}
	
}

void EDITOR::reset(bool create_default)
{
	map.clean();

	// create default layers
	if(create_default)
		map.create_default(entities_texture);
	
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

void EDITOR::Init(class IGraphics *pGraphics)
{
	m_pGraphics = pGraphics;
	
	checker_texture = Graphics()->LoadTexture("editor/checker.png", IMG_AUTO, 0);
	background_texture = Graphics()->LoadTexture("editor/background.png", IMG_AUTO, 0);
	cursor_texture = Graphics()->LoadTexture("editor/cursor.png", IMG_AUTO, 0);
	entities_texture = Graphics()->LoadTexture("editor/entities.png", IMG_AUTO, 0);
	
	tileset_picker.make_palette();
	tileset_picker.readonly = true;
	
	reset();
}

void EDITOR::UpdateAndRender()
{
	static int mouse_x = 0;
	static int mouse_y = 0;
	
	if(animate)
		animate_time = (time_get()-animate_start)/(float)time_freq();
	else
		animate_time = 0;
	ui_got_context = 0;

	// handle mouse movement
	float mx, my, mwx, mwy;
	int rx, ry;
	{
		inp_mouse_relative(&rx, &ry);
		mouse_delta_x = rx;
		mouse_delta_y = ry;
		
		if(!lock_mouse)
		{
			mouse_x += rx;
			mouse_y += ry;
		}
		
		if(mouse_x < 0) mouse_x = 0;
		if(mouse_y < 0) mouse_y = 0;
		if(mouse_x > UI()->Screen()->w) mouse_x = (int)UI()->Screen()->w;
		if(mouse_y > UI()->Screen()->h) mouse_y = (int)UI()->Screen()->h;

		// update the ui
		mx = mouse_x;
		my = mouse_y;
		mwx = 0;
		mwy = 0;
		
		// fix correct world x and y
		LAYERGROUP *g = get_selected_group();
		if(g)
		{
			float points[4];
			g->mapping(points);

			float world_width = points[2]-points[0];
			float world_height = points[3]-points[1];
			
			mwx = points[0] + world_width * (mouse_x/UI()->Screen()->w);
			mwy = points[1] + world_height * (mouse_y/UI()->Screen()->h);
			mouse_delta_wx = mouse_delta_x*(world_width / UI()->Screen()->w);
			mouse_delta_wy = mouse_delta_y*(world_height / UI()->Screen()->h);
		}
		
		int buttons = 0;
		if(inp_key_pressed(KEY_MOUSE_1)) buttons |= 1;
		if(inp_key_pressed(KEY_MOUSE_2)) buttons |= 2;
		if(inp_key_pressed(KEY_MOUSE_3)) buttons |= 4;
		
		UI()->Update(mx,my,mwx,mwy,buttons);
	}
	
	// toggle gui
	if(inp_key_down(KEY_TAB))
		gui_active = !gui_active;

	if(inp_key_down(KEY_F5))
		save("maps/debug_test2.map");

	if(inp_key_down(KEY_F6))
		load("maps/debug_test2.map");
	
	if(inp_key_down(KEY_F8))
		load("maps/debug_test.map");
	
	if(inp_key_down(KEY_F10))
		show_mouse_pointer = false;
	
	render();
	
	if(inp_key_down(KEY_F10))
	{
		Graphics()->TakeScreenshot();
		show_mouse_pointer = true;
	}
	
	inp_clear_events();
}

IEditor *CreateEditor() { return new EDITOR; }
