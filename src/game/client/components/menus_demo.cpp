
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

int MENUS::DoButton_DemoPlayer(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, Checked ? 0.10f : 0.5f)*button_color_mul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, 14.0f, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

void MENUS::render_demoplayer(CUIRect main_view)
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
	
	main_view.HSplitBottom(total_height, 0, &main_view);
	main_view.VSplitLeft(250.0f, 0, &main_view);
	main_view.VSplitRight(250.0f, &main_view, 0);
	
	RenderTools()->DrawUIRect(&main_view, color_tabbar_active, CUI::CORNER_T, 10.0f);
		
	main_view.Margin(5.0f, &main_view);
	
	CUIRect seekbar, buttonbar;
	
	if(menu_active)
	{
		main_view.HSplitTop(seekbar_height, &seekbar, &buttonbar);
		buttonbar.HSplitTop(margins, 0, &buttonbar);
	}
	else
		seekbar = main_view;

	// do seekbar
	{
		static int seekbar_id = 0;
		void *id = &seekbar_id;
		char buffer[128];
		
		RenderTools()->DrawUIRect(&seekbar, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);
		
		int current_tick = info->current_tick - info->first_tick;
		int total_ticks = info->last_tick - info->first_tick;
		
		float amount = current_tick/(float)total_ticks;
		
		CUIRect filledbar = seekbar;
		filledbar.w = 10.0f + (filledbar.w-10.0f)*amount;
		
		RenderTools()->DrawUIRect(&filledbar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
		
		str_format(buffer, sizeof(buffer), "%d:%02d / %d:%02d",
			current_tick/SERVER_TICK_SPEED/60, (current_tick/SERVER_TICK_SPEED)%60,
			total_ticks/SERVER_TICK_SPEED/60, (total_ticks/SERVER_TICK_SPEED)%60);
		UI()->DoLabel(&seekbar, buffer, seekbar.h*0.70f, 0);

		// do the logic
	    int inside = UI()->MouseInside(&seekbar);
			
		if(UI()->ActiveItem() == id)
		{
			if(!UI()->MouseButton(0))
				UI()->SetActiveItem(0);
			else
			{
				float amount = (UI()->MouseX()-seekbar.x)/(float)seekbar.w;
				if(amount > 0 && amount < 1.0f)
				{
					gameclient.on_reset();
					gameclient.suppress_events = true;
					client_demoplayer_setpos(amount);
					gameclient.suppress_events = false;
				}
			}
		}
		else if(UI()->HotItem() == id)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(id);
		}		
		
		if(inside)
			UI()->SetHotItem(id);
	}	
	

	if(menu_active)
	{
		// do buttons
		CUIRect button;

		// pause button
		buttonbar.VSplitLeft(buttonbar_height, &button, &buttonbar);
		static int pause_button = 0;
		if(DoButton_DemoPlayer(&pause_button, "| |", info->paused, &button))
			client_demoplayer_setpause(!info->paused);
		
		// play button
		buttonbar.VSplitLeft(margins, 0, &buttonbar);
		buttonbar.VSplitLeft(buttonbar_height, &button, &buttonbar);
		static int play_button = 0;
		if(DoButton_DemoPlayer(&play_button, ">", !info->paused, &button))
		{
			client_demoplayer_setpause(0);
			client_demoplayer_setspeed(1.0f);
		}

		// slowdown
		buttonbar.VSplitLeft(margins, 0, &buttonbar);
		buttonbar.VSplitLeft(buttonbar_height, &button, &buttonbar);
		static int slowdown_button = 0;
		if(DoButton_DemoPlayer(&slowdown_button, "<<", 0, &button))
		{
			if(info->speed > 4.0f) client_demoplayer_setspeed(4.0f);
			else if(info->speed > 2.0f) client_demoplayer_setspeed(2.0f);
			else if(info->speed > 1.0f) client_demoplayer_setspeed(1.0f);
			else if(info->speed > 0.5f) client_demoplayer_setspeed(0.5f);
			else client_demoplayer_setspeed(0.05f);
		}
		
		// fastforward
		buttonbar.VSplitLeft(margins, 0, &buttonbar);
		buttonbar.VSplitLeft(buttonbar_height, &button, &buttonbar);
		static int fastforward_button = 0;
		if(DoButton_DemoPlayer(&fastforward_button, ">>", 0, &button))
		{
			if(info->speed < 0.5f) client_demoplayer_setspeed(0.5f);
			else if(info->speed < 1.0f) client_demoplayer_setspeed(1.0f);
			else if(info->speed < 2.0f) client_demoplayer_setspeed(2.0f);
			else if(info->speed < 4.0f) client_demoplayer_setspeed(4.0f);
			else client_demoplayer_setspeed(8.0f);
		}

		// speed meter
		buttonbar.VSplitLeft(margins*3, 0, &buttonbar);
		char buffer[64];
		if(info->speed >= 1.0f)
			str_format(buffer, sizeof(buffer), "x%.0f", info->speed);
		else
			str_format(buffer, sizeof(buffer), "x%.1f", info->speed);
		UI()->DoLabel(&buttonbar, buffer, button.h*0.7f, -1);

		// close button
		buttonbar.VSplitRight(buttonbar_height*3, &buttonbar, &button);
		static int exit_button = 0;
		if(DoButton_DemoPlayer(&exit_button, localize("Close"), 0, &button))
			client_disconnect();
	}
}

static CUIRect listbox_originalview;
static CUIRect listbox_view;
static float listbox_rowheight;
static int listbox_itemindex;
static int listbox_selected_index;
static int listbox_new_selected;
static int listbox_doneevents;
static int listbox_numitems;

void MENUS::ui_do_listbox_start(void *id, const CUIRect *rect, float row_height, const char *title, int num_items, int selected_index)
{
	CUIRect scroll, row;
	CUIRect view = *rect;
	CUIRect header, footer;
	
	// draw header
	view.HSplitTop(listheader_height, &header, &view);
	RenderTools()->DrawUIRect(&header, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f); 
	UI()->DoLabel(&header, title, header.h*fontmod_height, 0);

	// draw footers
	view.HSplitBottom(listheader_height, &view, &footer);
	RenderTools()->DrawUIRect(&footer, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f); 
	footer.VSplitLeft(10.0f, 0, &footer);

	// background
	RenderTools()->DrawUIRect(&view, vec4(0,0,0,0.15f), 0, 0);

	// prepare the scroll
	view.VSplitRight(15, &view, &scroll);

	// setup the variables	
	listbox_originalview = view;
	listbox_selected_index = selected_index;
	listbox_new_selected = selected_index;
	listbox_itemindex = 0;
	listbox_rowheight = row_height;
	listbox_numitems = num_items;
	listbox_doneevents = 0;
	//int num_servers = client_serverbrowse_sorted_num();


	// do the scrollbar
	view.HSplitTop(listbox_rowheight, &row, 0);
	
	int num_viewable = (int)(listbox_originalview.h/row.h) + 1;
	int num = num_items-num_viewable+1;
	if(num < 0)
		num = 0;
		
	static float scrollvalue = 0;
	scroll.HMargin(5.0f, &scroll);
	scrollvalue = DoScrollbarV(id, &scroll, scrollvalue);

	int start = (int)(num*scrollvalue);
	if(start < 0)
		start = 0;
	
	// the list
	listbox_view = listbox_originalview;
	listbox_view.VMargin(5.0f, &listbox_view);
	UI()->ClipEnable(&listbox_view);
	listbox_view.y -= scrollvalue*num*row.h;	
}

MENUS::LISTBOXITEM MENUS::ui_do_listbox_nextrow()
{
	LISTBOXITEM item = {0};
	listbox_view.HSplitTop(listbox_rowheight /*-2.0f*/, &item.rect, &listbox_view);
	item.visible = 1;
	//item.rect = row;
	
	item.hitrect = item.rect;
	
	//CUIRect select_hit_box = item.rect;

	if(listbox_selected_index == listbox_itemindex)
		item.selected = 1;
	
	// make sure that only those in view can be selected
	if(item.rect.y+item.rect.h > listbox_originalview.y)
	{
		
		if(item.hitrect.y < item.hitrect.y) // clip the selection
		{
			item.hitrect.h -= listbox_originalview.y-item.hitrect.y;
			item.hitrect.y = listbox_originalview.y;
		}
		
	}
	else
		item.visible = 0;

	// check if we need to do more
	if(item.rect.y > listbox_originalview.y+listbox_originalview.h)
		item.visible = 0;
		
	listbox_itemindex++;
	return item;
}

MENUS::LISTBOXITEM MENUS::ui_do_listbox_nextitem(void *id)
{
	int this_itemindex = listbox_itemindex;
	
	LISTBOXITEM item = ui_do_listbox_nextrow();

	if(UI()->DoButtonLogic(id, "", listbox_selected_index == listbox_itemindex, &item.hitrect))
		listbox_new_selected = listbox_itemindex;
	
	//CUIRect row;
	//LISTBOXITEM item = {0};
	//listbox_view.HSplitTop(listbox_rowheight /*-2.0f*/, &row, &listbox_view);
	//listbox_view.HSplitTop(2.0f, 0, &listbox_view);
	/*
	CUIRect select_hit_box = row;

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
		
		if(UI()->DoButton(id, "", listbox_selected_index==listbox_itemindex, &select_hit_box, 0, 0))
			listbox_new_selected = listbox_itemindex;
	}
	else
		item.visible = 0;
	
	item.rect = row;
	*/
	

	if(listbox_selected_index == this_itemindex)
	{
		if(!listbox_doneevents)
		{
			listbox_doneevents = 1;
			
			for(int i = 0; i < num_inputevents; i++)
			{
				if(inputevents[i].flags&INPFLAG_PRESS)
				{
					if(inputevents[i].key == KEY_DOWN) listbox_new_selected++;
					if(inputevents[i].key == KEY_UP) listbox_new_selected--;
				}
			}

			if(listbox_new_selected >= listbox_numitems)
				listbox_new_selected = listbox_numitems-1;
			if(listbox_new_selected < 0)
				listbox_new_selected = 0;
		}
		
		//selected_index = i;
		CUIRect r = item.rect;
		r.Margin(1.5f, &r);
		RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
	}	

	//listbox_itemindex++;
	return item;
}

int MENUS::ui_do_listbox_end()
{
	UI()->ClipDisable();
	return listbox_new_selected;
}

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
}

void MENUS::demolist_populate()
{
	demos.clear();
	
	char buf[512];
	str_format(buf, sizeof(buf), "%s/demos", client_user_directory());

	FETCH_CALLBACKINFO info = {this, buf, 0};
	fs_listdir(buf, demolist_fetch_callback, &info);
	info.prefix = "demos";
	fs_listdir("demos", demolist_fetch_callback, &info);
}


void MENUS::render_demolist(CUIRect main_view)
{
	static int inited = 0;
	if(!inited)
		demolist_populate();
	inited = 1;
	
	// render background
	RenderTools()->DrawUIRect(&main_view, color_tabbar_active, CUI::CORNER_ALL, 10.0f);
	main_view.Margin(10.0f, &main_view);
	
	CUIRect buttonbar;
	main_view.HSplitBottom(button_height+5.0f, &main_view, &buttonbar);
	buttonbar.HSplitTop(5.0f, 0, &buttonbar);
	
	static int selected_item = -1;
	static int demolist_id = 0;
	
	ui_do_listbox_start(&demolist_id, &main_view, 17.0f, localize("Demos"), demos.size(), selected_item);
	//for(int i = 0; i < num_demos; i++)
	for(sorted_array<DEMOITEM>::range r = demos.all(); !r.empty(); r.pop_front())
	{
		LISTBOXITEM item = ui_do_listbox_nextitem((void*)(&r.front()));
		if(item.visible)
			UI()->DoLabel(&item.rect, r.front().name, item.rect.h*fontmod_height, -1);
	}
	selected_item = ui_do_listbox_end();
	
	CUIRect refresh_rect, play_rect;
	buttonbar.VSplitRight(250.0f, &buttonbar, &refresh_rect);
	refresh_rect.VSplitRight(130.0f, &refresh_rect, &play_rect);
	play_rect.VSplitRight(120.0f, 0x0, &play_rect);
	
	static int refresh_button = 0;
	if(DoButton_Menu(&refresh_button, localize("Refresh"), 0, &refresh_rect))
	{
		demolist_populate();
	}	
	
	static int play_button = 0;
	if(DoButton_Menu(&play_button, localize("Play"), 0, &play_rect))
	{
		if(selected_item >= 0 && selected_item < demos.size())
		{
			const char *error = client_demoplayer_play(demos[selected_item].filename);
			if(error)
				popup_message(localize("Error"), error, localize("Ok"));
		}
	}
	
}

