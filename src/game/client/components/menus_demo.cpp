
#include <base/math.hpp>

//#include <string.h> // strcmp, strlen, strncpy
//#include <stdlib.h> // atoi

#include <engine/e_client_interface.h>
#include <game/client/render.hpp>
#include <game/client/gameclient.hpp>

//#include <game/generated/g_protocol.hpp>
//#include <game/generated/gc_data.hpp>

#include <game/client/ui.hpp>
//#include <game/client/gameclient.hpp>
//#include <game/client/animstate.hpp>

#include "menus.hpp"

void MENUS::ui_draw_demoplayer_button(const void *id, const char *text, int checked, const RECT *r, const void *extra)
{
	ui_draw_rect(r, vec4(1,1,1, checked ? 0.10f : 0.5f)*button_color_mul(id), CORNER_ALL, 5.0f);
	ui_do_label(r, text, 14.0f, 0);
}

void MENUS::render_demoplayer(RECT main_view)
{
	const DEMOPLAYBACK_INFO *info = client_demoplayer_getinfo();
	
	const float seekbar_height = 15.0f;
	const float buttonbar_height = 20.0f;
	const float margins = 5.0f;
	float total_height;
	
	if(menu_active)
		total_height = seekbar_height+buttonbar_height+margins*3;
	else
		total_height = seekbar_height+margins*2;
	
	ui_hsplit_b(&main_view, total_height, 0, &main_view);
	ui_vsplit_l(&main_view, 250.0f, 0, &main_view);
	ui_vsplit_r(&main_view, 250.0f, &main_view, 0);
	
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_T, 10.0f);
		
	ui_margin(&main_view, 5.0f, &main_view);
	
	RECT seekbar, buttonbar;
	
	if(menu_active)
	{
		ui_hsplit_t(&main_view, seekbar_height, &seekbar, &buttonbar);
		ui_hsplit_t(&buttonbar, margins, 0, &buttonbar);
	}
	else
		seekbar = main_view;

	// do seekbar
	{
		static int seekbar_id = 0;
		void *id = &seekbar_id;
		char buffer[128];
		
		ui_draw_rect(&seekbar, vec4(0,0,0,0.5f), CORNER_ALL, 5.0f);
		
		int current_tick = info->current_tick - info->first_tick;
		int total_ticks = info->last_tick - info->first_tick;
		
		float amount = current_tick/(float)total_ticks;
		
		RECT filledbar = seekbar;
		filledbar.w = 10.0f + (filledbar.w-10.0f)*amount;
		
		ui_draw_rect(&filledbar, vec4(1,1,1,0.5f), CORNER_ALL, 5.0f);
		
		str_format(buffer, sizeof(buffer), "%d:%02d / %d:%02d",
			current_tick/SERVER_TICK_SPEED/60, (current_tick/SERVER_TICK_SPEED)%60,
			total_ticks/SERVER_TICK_SPEED/60, (total_ticks/SERVER_TICK_SPEED)%60);
		ui_do_label(&seekbar, buffer, seekbar.h*0.70f, 0);

		// do the logic
	    int inside = ui_mouse_inside(&seekbar);
			
		if(ui_active_item() == id)
		{
			if(!ui_mouse_button(0))
				ui_set_active_item(0);
			else
			{
				float amount = (ui_mouse_x()-seekbar.x)/(float)seekbar.w;
				if(amount > 0 && amount < 1.0f)
				{
					gameclient.on_reset();
					gameclient.suppress_events = true;
					client_demoplayer_setpos(amount);
					gameclient.suppress_events = false;
				}
			}
		}
		else if(ui_hot_item() == id)
		{
			if(ui_mouse_button(0))
				ui_set_active_item(id);
		}		
		
		if(inside)
			ui_set_hot_item(id);
	}	
	

	if(menu_active)
	{
		// do buttons
		RECT button;

		// pause button
		ui_vsplit_l(&buttonbar, buttonbar_height, &button, &buttonbar);
		static int pause_button = 0;
		if(ui_do_button(&pause_button, "| |", info->paused, &button, ui_draw_demoplayer_button, 0))
			client_demoplayer_setpause(!info->paused);
		
		// play button
		ui_vsplit_l(&buttonbar, margins, 0, &buttonbar);
		ui_vsplit_l(&buttonbar, buttonbar_height, &button, &buttonbar);
		static int play_button = 0;
		if(ui_do_button(&play_button, ">", !info->paused, &button, ui_draw_demoplayer_button, 0))
		{
			client_demoplayer_setpause(0);
			client_demoplayer_setspeed(1.0f);
		}

		// slowdown
		ui_vsplit_l(&buttonbar, margins, 0, &buttonbar);
		ui_vsplit_l(&buttonbar, buttonbar_height, &button, &buttonbar);
		static int slowdown_button = 0;
		if(ui_do_button(&slowdown_button, "<<", 0, &button, ui_draw_demoplayer_button, 0))
		{
			if(info->speed > 4.0f) client_demoplayer_setspeed(4.0f);
			else if(info->speed > 2.0f) client_demoplayer_setspeed(2.0f);
			else if(info->speed > 1.0f) client_demoplayer_setspeed(1.0f);
			else if(info->speed > 0.5f) client_demoplayer_setspeed(0.5f);
			else client_demoplayer_setspeed(0.05f);
		}
		
		// fastforward
		ui_vsplit_l(&buttonbar, margins, 0, &buttonbar);
		ui_vsplit_l(&buttonbar, buttonbar_height, &button, &buttonbar);
		static int fastforward_button = 0;
		if(ui_do_button(&fastforward_button, ">>", 0, &button, ui_draw_demoplayer_button, 0))
		{
			if(info->speed < 0.5f) client_demoplayer_setspeed(0.5f);
			else if(info->speed < 1.0f) client_demoplayer_setspeed(1.0f);
			else if(info->speed < 2.0f) client_demoplayer_setspeed(2.0f);
			else if(info->speed < 4.0f) client_demoplayer_setspeed(4.0f);
			else client_demoplayer_setspeed(8.0f);
		}

		// speed meter
		ui_vsplit_l(&buttonbar, margins*3, 0, &buttonbar);
		char buffer[64];
		if(info->speed >= 1.0f)
			str_format(buffer, sizeof(buffer), "x%.0f", info->speed);
		else
			str_format(buffer, sizeof(buffer), "x%.1f", info->speed);
		ui_do_label(&buttonbar, buffer, button.h*0.7f, -1);

		// close button
		ui_vsplit_r(&buttonbar, buttonbar_height*3, &buttonbar, &button);
		static int exit_button = 0;
		if(ui_do_button(&exit_button, localize("Close"), 0, &button, ui_draw_demoplayer_button, 0))
			client_disconnect();
	}
}

static RECT listbox_originalview;
static RECT listbox_view;
static float listbox_rowheight;
static int listbox_itemindex;
static int listbox_selected_index;
static int listbox_new_selected;

void MENUS::ui_do_listbox_start(void *id, const RECT *rect, float row_height, const char *title, int num_items, int selected_index)
{
	RECT scroll, row;
	RECT view = *rect;
	RECT header, footer;
	
	// draw header
	ui_hsplit_t(&view, listheader_height, &header, &view);
	ui_draw_rect(&header, vec4(1,1,1,0.25f), CORNER_T, 5.0f); 
	ui_do_label(&header, title, header.h*fontmod_height, 0);

	// draw footers
	ui_hsplit_b(&view, listheader_height, &view, &footer);
	ui_draw_rect(&footer, vec4(1,1,1,0.25f), CORNER_B, 5.0f); 
	ui_vsplit_l(&footer, 10.0f, 0, &footer);

	// background
	ui_draw_rect(&view, vec4(0,0,0,0.15f), 0, 0);

	// prepare the scroll
	ui_vsplit_r(&view, 15, &view, &scroll);

	// setup the variables	
	listbox_originalview = view;
	listbox_selected_index = selected_index;
	listbox_new_selected = selected_index;
	listbox_itemindex = 0;
	listbox_rowheight = row_height;
	//int num_servers = client_serverbrowse_sorted_num();


	// do the scrollbar
	ui_hsplit_t(&view, listbox_rowheight, &row, 0);
	
	int num_viewable = (int)(listbox_originalview.h/row.h) + 1;
	int num = num_items-num_viewable+1;
	if(num < 0)
		num = 0;
		
	static float scrollvalue = 0;
	ui_hmargin(&scroll, 5.0f, &scroll);
	scrollvalue = ui_do_scrollbar_v(id, &scroll, scrollvalue);

	int start = (int)(num*scrollvalue);
	if(start < 0)
		start = 0;
	
	// the list
	listbox_view = listbox_originalview;
	ui_vmargin(&listbox_view, 5.0f, &listbox_view);
	ui_clip_enable(&listbox_view);
	listbox_view.y -= scrollvalue*num*row.h;	
}

MENUS::LISTBOXITEM MENUS::ui_do_listbox_nextitem(void *id)
{
	RECT row;
	LISTBOXITEM item = {0};
	ui_hsplit_t(&listbox_view, listbox_rowheight /*-2.0f*/, &row, &listbox_view);
	//ui_hsplit_t(&listbox_view, 2.0f, 0, &listbox_view);

	RECT select_hit_box = row;

	item.visible = 1;
	if(listbox_selected_index == listbox_itemindex)
		item.selected = 1;
	
	// make sure that only those in view can be selected
	if(row.y+row.h > listbox_originalview.y)
	{
		
		if(select_hit_box.y < listbox_originalview.y) // clip the selection
		{
			select_hit_box.h -= listbox_originalview.y-select_hit_box.y;
			select_hit_box.y = listbox_originalview.y;
		}
		
		if(ui_do_button(id, "", listbox_selected_index==listbox_itemindex, &select_hit_box, 0, 0))
			listbox_new_selected = listbox_itemindex;
	}
	else
		item.visible = 0;
	
	item.rect = row;
	
	// check if we need to do more
	if(row.y > listbox_originalview.y+listbox_originalview.h)
		item.visible = 0;

	if(listbox_selected_index==listbox_itemindex)
	{
		//selected_index = i;
		RECT r = row;
		ui_margin(&r, 1.5f, &r);
		ui_draw_rect(&r, vec4(1,1,1,0.5f), CORNER_ALL, 4.0f);
	}	

	listbox_itemindex++;

	ui_vmargin(&item.rect, 5.0f, &item.rect);

	return item;
}

int MENUS::ui_do_listbox_end()
{
	ui_clip_disable();
	return listbox_new_selected;
}

/*
void MENUS::demolist_listdir_callback(const char *name, int is_dir, void *user)
{

	(*count)++;
	LISTBOXITEM item = ui_do_listbox_nextitem((void*)(10+*count));
	if(item.visible)
		ui_do_label(&item.rect, name, item.rect.h*fontmod_height, -1);
}


	DEMOITEM *demos;
	int num_demos;
	*/
	
/*void MENUS::demolist_count_callback(const char *name, int is_dir, void *user)
{
	if(is_dir || name[0] == '.')
		return;
	(*(int *)user)++;
}*/

struct FETCH_CALLBACKINFO
{
	MENUS *self;
	const char *prefix;
	int count;
};

void MENUS::demolist_fetch_callback(const char *name, int is_dir, void *user)
{
	if(is_dir || name[0] == '.')
		return;
			
	FETCH_CALLBACKINFO *info = (FETCH_CALLBACKINFO *)user;
	
	DEMOITEM item;
	str_format(item.filename, sizeof(item.filename), "%s/%s", info->prefix, name);
	str_copy(item.name, name, sizeof(item.name));
	info->self->demos.add(item);
	
	/*
	if(info->count == info->self->num_demos)
		return;
	
	info->count++;
	*/
}


void MENUS::demolist_populate()
{
	demos.clear();
	
	/*if(demos)
		mem_free(demos);
	demos = 0;
	num_demos = 0;*/
	
	char buf[512];
	str_format(buf, sizeof(buf), "%s/demos", client_user_directory());
	//fs_listdir(buf, demolist_count_callback, &num_demos);
	//fs_listdir("demos", demolist_count_callback, &num_demos);
	
	//demos = (DEMOITEM *)mem_alloc(sizeof(DEMOITEM)*num_demos, 1);
	//mem_zero(demos, sizeof(DEMOITEM)*num_demos);

	FETCH_CALLBACKINFO info = {this, buf, 0};
	fs_listdir(buf, demolist_fetch_callback, &info);
	info.prefix = "demos";
	fs_listdir("demos", demolist_fetch_callback, &info);
}


void MENUS::render_demolist(RECT main_view)
{
	static int inited = 0;
	if(!inited)
		demolist_populate();
	inited = 1;
	
	// render background
	ui_draw_rect(&main_view, color_tabbar_active, CORNER_ALL, 10.0f);
	ui_margin(&main_view, 10.0f, &main_view);
	
	RECT buttonbar;
	ui_hsplit_b(&main_view, button_height+5.0f, &main_view, &buttonbar);
	ui_hsplit_t(&buttonbar, 5.0f, 0, &buttonbar);
	
	static int selected_item = -1;
	static int demolist_id = 0;
	
	ui_do_listbox_start(&demolist_id, &main_view, 17.0f, localize("Demos"), demos.size(), selected_item);
	//for(int i = 0; i < num_demos; i++)
	for(sorted_array<DEMOITEM>::range r = demos.all(); !r.empty(); r.pop_front())
	{
		LISTBOXITEM item = ui_do_listbox_nextitem((void*)(&r.front()));
		if(item.visible)
			ui_do_label(&item.rect, r.front().name, item.rect.h*fontmod_height, -1);
	}
	selected_item = ui_do_listbox_end();
	
	RECT refresh_rect, play_rect;
	ui_vsplit_r(&buttonbar, 250.0f, &buttonbar, &refresh_rect);
	ui_vsplit_r(&refresh_rect, 130.0f, &refresh_rect, &play_rect);
	ui_vsplit_r(&play_rect, 120.0f, 0x0, &play_rect);
	
	static int refresh_button = 0;
	if(ui_do_button(&refresh_button, localize("Refresh"), 0, &refresh_rect, ui_draw_menu_button, 0))
	{
		demolist_populate();
	}	
	
	static int play_button = 0;
	if(ui_do_button(&play_button, localize("Play"), 0, &play_rect, ui_draw_menu_button, 0))
	{
		if(selected_item >= 0 && selected_item < demos.size())
		{
			const char *error = client_demoplayer_play(demos[selected_item].filename);
			if(error)
				popup_message(localize("Error"), error, localize("Ok"));
		}
	}
	
}

