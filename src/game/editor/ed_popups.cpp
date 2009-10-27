#include <stdio.h>
#include <engine/client/graphics.h>
#include "ed_editor.hpp"


// popup menu handling
static struct
{
	CUIRect rect;
	void *id;
	int (*func)(EDITOR *pEditor, CUIRect rect);
	int is_menu;
	void *extra;
} ui_popups[8];

static int ui_num_popups = 0;

void EDITOR::ui_invoke_popup_menu(void *id, int flags, float x, float y, float w, float h, int (*func)(EDITOR *pEditor, CUIRect rect), void *extra)
{
	dbg_msg("", "invoked");
	ui_popups[ui_num_popups].id = id;
	ui_popups[ui_num_popups].is_menu = flags;
	ui_popups[ui_num_popups].rect.x = x;
	ui_popups[ui_num_popups].rect.y = y;
	ui_popups[ui_num_popups].rect.w = w;
	ui_popups[ui_num_popups].rect.h = h;
	ui_popups[ui_num_popups].func = func;
	ui_popups[ui_num_popups].extra = extra;
	ui_num_popups++;
}

void EDITOR::ui_do_popup_menu()
{
	for(int i = 0; i < ui_num_popups; i++)
	{
		bool inside = UI()->MouseInside(&ui_popups[i].rect);
		UI()->SetHotItem(&ui_popups[i].id);
		
		if(UI()->ActiveItem() == &ui_popups[i].id)
		{
			if(!UI()->MouseButton(0))
			{
				if(!inside)
					ui_num_popups--;
				UI()->SetActiveItem(0);
			}
		}
		else if(UI()->HotItem() == &ui_popups[i].id)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(&ui_popups[i].id);
		}
		
		int corners = CUI::CORNER_ALL;
		if(ui_popups[i].is_menu)
			corners = CUI::CORNER_R|CUI::CORNER_B;
		
		CUIRect r = ui_popups[i].rect;
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), corners, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), corners, 3.0f);
		r.Margin(4.0f, &r);
		
		if(ui_popups[i].func(this, r))
			ui_num_popups--;
			
		if(inp_key_down(KEY_ESCAPE))
			ui_num_popups--;
	}
}


int EDITOR::popup_group(EDITOR *pEditor, CUIRect view)
{
	// remove group button
	CUIRect button;
	view.HSplitBottom(12.0f, &view, &button);
	static int delete_button = 0;
	
	// don't allow deletion of game group
	if(pEditor->map.game_group != pEditor->get_selected_group() &&
		pEditor->DoButton_Editor(&delete_button, "Delete Group", 0, &button, 0, "Delete group"))
	{
		pEditor->map.delete_group(pEditor->selected_group);
		return 1;
	}

	// new tile layer
	view.HSplitBottom(10.0f, &view, &button);
	view.HSplitBottom(12.0f, &view, &button);
	static int new_quad_layer_button = 0;
	if(pEditor->DoButton_Editor(&new_quad_layer_button, "Add Quads Layer", 0, &button, 0, "Creates a new quad layer"))
	{
		LAYER *l = new LAYER_QUADS;
		pEditor->map.groups[pEditor->selected_group]->add_layer(l);
		pEditor->selected_layer = pEditor->map.groups[pEditor->selected_group]->layers.len()-1;
		return 1;
	}

	// new quad layer
	view.HSplitBottom(5.0f, &view, &button);
	view.HSplitBottom(12.0f, &view, &button);
	static int new_tile_layer_button = 0;
	if(pEditor->DoButton_Editor(&new_tile_layer_button, "Add Tile Layer", 0, &button, 0, "Creates a new tile layer"))
	{
		LAYER *l = new LAYER_TILES(50, 50);
		pEditor->map.groups[pEditor->selected_group]->add_layer(l);
		pEditor->selected_layer = pEditor->map.groups[pEditor->selected_group]->layers.len()-1;
		return 1;
	}
	
	enum
	{
		PROP_ORDER=0,
		PROP_POS_X,
		PROP_POS_Y,
		PROP_PARA_X,
		PROP_PARA_Y,
		PROP_USE_CLIPPING,
		PROP_CLIP_X,
		PROP_CLIP_Y,
		PROP_CLIP_W,
		PROP_CLIP_H,
		NUM_PROPS,
	};
	
	PROPERTY props[] = {
		{"Order", pEditor->selected_group, PROPTYPE_INT_STEP, 0, pEditor->map.groups.len()-1},
		{"Pos X", -pEditor->map.groups[pEditor->selected_group]->offset_x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", -pEditor->map.groups[pEditor->selected_group]->offset_y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para X", pEditor->map.groups[pEditor->selected_group]->parallax_x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para Y", pEditor->map.groups[pEditor->selected_group]->parallax_y, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{"Use Clipping", pEditor->map.groups[pEditor->selected_group]->use_clipping, PROPTYPE_BOOL, 0, 1},
		{"Clip X", pEditor->map.groups[pEditor->selected_group]->clip_x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip Y", pEditor->map.groups[pEditor->selected_group]->clip_y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip W", pEditor->map.groups[pEditor->selected_group]->clip_w, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip H", pEditor->map.groups[pEditor->selected_group]->clip_h, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{0},
	};
	
	static int ids[NUM_PROPS] = {0};
	int new_val = 0;
	
	// cut the properties that isn't needed
	if(pEditor->get_selected_group()->game_group)
		props[PROP_POS_X].name = 0;
		
	int prop = pEditor->do_properties(&view, props, ids, &new_val);
	if(prop == PROP_ORDER)
		pEditor->selected_group = pEditor->map.swap_groups(pEditor->selected_group, new_val);
		
	// these can not be changed on the game group
	if(!pEditor->get_selected_group()->game_group)
	{
		if(prop == PROP_PARA_X) pEditor->map.groups[pEditor->selected_group]->parallax_x = new_val;
		else if(prop == PROP_PARA_Y) pEditor->map.groups[pEditor->selected_group]->parallax_y = new_val;
		else if(prop == PROP_POS_X) pEditor->map.groups[pEditor->selected_group]->offset_x = -new_val;
		else if(prop == PROP_POS_Y) pEditor->map.groups[pEditor->selected_group]->offset_y = -new_val;
		else if(prop == PROP_USE_CLIPPING) pEditor->map.groups[pEditor->selected_group]->use_clipping = new_val;
		else if(prop == PROP_CLIP_X) pEditor->map.groups[pEditor->selected_group]->clip_x = new_val;
		else if(prop == PROP_CLIP_Y) pEditor->map.groups[pEditor->selected_group]->clip_y = new_val;
		else if(prop == PROP_CLIP_W) pEditor->map.groups[pEditor->selected_group]->clip_w = new_val;
		else if(prop == PROP_CLIP_H) pEditor->map.groups[pEditor->selected_group]->clip_h = new_val;
	}
	
	return 0;
}

int EDITOR::popup_layer(EDITOR *pEditor, CUIRect view)
{
	// remove layer button
	CUIRect button;
	view.HSplitBottom(12.0f, &view, &button);
	static int delete_button = 0;
	
	// don't allow deletion of game layer
	if(pEditor->map.game_layer != pEditor->get_selected_layer(0) &&
		pEditor->DoButton_Editor(&delete_button, "Delete Layer", 0, &button, 0, "Deletes the layer"))
	{
		pEditor->map.groups[pEditor->selected_group]->delete_layer(pEditor->selected_layer);
		return 1;
	}

	view.HSplitBottom(10.0f, &view, 0);
	
	LAYERGROUP *current_group = pEditor->map.groups[pEditor->selected_group];
	LAYER *current_layer = pEditor->get_selected_layer(0);
	
	enum
	{
		PROP_GROUP=0,
		PROP_ORDER,
		PROP_HQ,
		NUM_PROPS,
	};
	
	PROPERTY props[] = {
		{"Group", pEditor->selected_group, PROPTYPE_INT_STEP, 0, pEditor->map.groups.len()-1},
		{"Order", pEditor->selected_layer, PROPTYPE_INT_STEP, 0, current_group->layers.len()},
		{"Detail", current_layer->flags&LAYERFLAG_DETAIL, PROPTYPE_BOOL, 0, 1},
		{0},
	};
	
	static int ids[NUM_PROPS] = {0};
	int new_val = 0;
	int prop = pEditor->do_properties(&view, props, ids, &new_val);		
	
	if(prop == PROP_ORDER)
		pEditor->selected_layer = current_group->swap_layers(pEditor->selected_layer, new_val);
	else if(prop == PROP_GROUP && current_layer->type != LAYERTYPE_GAME)
	{
		if(new_val >= 0 && new_val < pEditor->map.groups.len())
		{
			current_group->layers.remove(current_layer);
			pEditor->map.groups[new_val]->layers.add(current_layer);
			pEditor->selected_group = new_val;
			pEditor->selected_layer = pEditor->map.groups[new_val]->layers.len()-1;
		}
	}
	else if(prop == PROP_HQ)
	{
		current_layer->flags &= ~LAYERFLAG_DETAIL;
		if(new_val)
			current_layer->flags |= LAYERFLAG_DETAIL;
	}
		
	return current_layer->render_properties(&view);
}

int EDITOR::popup_quad(EDITOR *pEditor, CUIRect view)
{
	QUAD *quad = pEditor->get_selected_quad();

	CUIRect button;
	
	// delete button
	view.HSplitBottom(12.0f, &view, &button);
	static int delete_button = 0;
	if(pEditor->DoButton_Editor(&delete_button, "Delete", 0, &button, 0, "Deletes the current quad"))
	{
		LAYER_QUADS *layer = (LAYER_QUADS *)pEditor->get_selected_layer_type(0, LAYERTYPE_QUADS);
		if(layer)
		{
			layer->quads.removebyindex(pEditor->selected_quad);
			pEditor->selected_quad--;
		}
		return 1;
	}

	// square button
	view.HSplitBottom(10.0f, &view, &button);
	view.HSplitBottom(12.0f, &view, &button);
	static int sq_button = 0;
	if(pEditor->DoButton_Editor(&sq_button, "Square", 0, &button, 0, "Squares the current quad"))
	{
		int top = quad->points[0].y;
		int left = quad->points[0].x;
		int bottom = quad->points[0].y;
		int right = quad->points[0].x;
		
		for(int k = 1; k < 4; k++)
		{
			if(quad->points[k].y < top) top = quad->points[k].y;
			if(quad->points[k].x < left) left = quad->points[k].x;
			if(quad->points[k].y > bottom) bottom = quad->points[k].y;
			if(quad->points[k].x > right) right = quad->points[k].x;
		}
		
		quad->points[0].x = left; quad->points[0].y = top;
		quad->points[1].x = right; quad->points[1].y = top;
		quad->points[2].x = left; quad->points[2].y = bottom;
		quad->points[3].x = right; quad->points[3].y = bottom;
		return 1;
	}


	enum
	{
		PROP_POS_ENV=0,
		PROP_POS_ENV_OFFSET,
		PROP_COLOR_ENV,
		PROP_COLOR_ENV_OFFSET,
		NUM_PROPS,
	};
	
	PROPERTY props[] = {
		{"Pos. Env", quad->pos_env, PROPTYPE_INT_STEP, -1, pEditor->map.envelopes.len()},
		{"Pos. TO", quad->pos_env_offset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Color Env", quad->color_env, PROPTYPE_INT_STEP, -1, pEditor->map.envelopes.len()},
		{"Color TO", quad->color_env_offset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		
		{0},
	};
	
	static int ids[NUM_PROPS] = {0};
	int new_val = 0;
	int prop = pEditor->do_properties(&view, props, ids, &new_val);		
	
	if(prop == PROP_POS_ENV) quad->pos_env = clamp(new_val, -1, pEditor->map.envelopes.len()-1);
	if(prop == PROP_POS_ENV_OFFSET) quad->pos_env_offset = new_val;
	if(prop == PROP_COLOR_ENV) quad->color_env = clamp(new_val, -1, pEditor->map.envelopes.len()-1);
	if(prop == PROP_COLOR_ENV_OFFSET) quad->color_env_offset = new_val;
	
	return 0;
}

int EDITOR::popup_point(EDITOR *pEditor, CUIRect view)
{
	QUAD *quad = pEditor->get_selected_quad();
	
	enum
	{
		PROP_COLOR=0,
		NUM_PROPS,
	};
	
	int color = 0;

	for(int v = 0; v < 4; v++)
	{
		if(pEditor->selected_points&(1<<v))
		{
			color = 0;
			color |= quad->colors[v].r<<24;
			color |= quad->colors[v].g<<16;
			color |= quad->colors[v].b<<8;
			color |= quad->colors[v].a;
		}
	}
	
	
	PROPERTY props[] = {
		{"Color", color, PROPTYPE_COLOR, -1, pEditor->map.envelopes.len()},
		{0},
	};
	
	static int ids[NUM_PROPS] = {0};
	int new_val = 0;
	int prop = pEditor->do_properties(&view, props, ids, &new_val);		
	if(prop == PROP_COLOR)
	{
		for(int v = 0; v < 4; v++)
		{
			if(pEditor->selected_points&(1<<v))
			{
				color = 0;
				quad->colors[v].r = (new_val>>24)&0xff;
				quad->colors[v].g = (new_val>>16)&0xff;
				quad->colors[v].b = (new_val>>8)&0xff;
				quad->colors[v].a = new_val&0xff;
			}
		}
	}
	
	return 0;	
}



static int select_image_selected = -100;
static int select_image_current = -100;

int EDITOR::popup_select_image(EDITOR *pEditor, CUIRect view)
{
	CUIRect buttonbar, imageview;
	view.VSplitLeft(80.0f, &buttonbar, &view);
	view.Margin(10.0f, &imageview);
	
	int show_image = select_image_current;
	
	for(int i = -1; i < pEditor->map.images.len(); i++)
	{
		CUIRect button;
		buttonbar.HSplitTop(12.0f, &button, &buttonbar);
		buttonbar.HSplitTop(2.0f, 0, &buttonbar);
		
		if(pEditor->UI()->MouseInside(&button))
			show_image = i;
			
		if(i == -1)
		{
			if(pEditor->DoButton_MenuItem(&pEditor->map.images[i], "None", i==select_image_current, &button))
				select_image_selected = -1;
		}
		else
		{
			if(pEditor->DoButton_MenuItem(&pEditor->map.images[i], pEditor->map.images[i]->name, i==select_image_current, &button))
				select_image_selected = i;
		}
	}
	
	if(show_image >= 0 && show_image < pEditor->map.images.len())
		pEditor->Graphics()->TextureSet(pEditor->map.images[show_image]->tex_id);
	else
		pEditor->Graphics()->TextureSet(-1);
	pEditor->Graphics()->QuadsBegin();
	pEditor->Graphics()->QuadsDrawTL(imageview.x, imageview.y, imageview.w, imageview.h);
	pEditor->Graphics()->QuadsEnd();

	return 0;
}

void EDITOR::popup_select_image_invoke(int current, float x, float y)
{
	static int select_image_popup_id = 0;
	select_image_selected = -100;
	select_image_current = current;
	ui_invoke_popup_menu(&select_image_popup_id, 0, x, y, 400, 300, popup_select_image);
}

int EDITOR::popup_select_image_result()
{
	if(select_image_selected == -100)
		return -100;
		
	select_image_current = select_image_selected;
	select_image_selected = -100;
	return select_image_current;
}





