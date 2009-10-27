/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <base/system.h>
#include <base/math.hpp>
#include <base/vmath.hpp>

#include "menus.hpp"
#include "skins.hpp"

#include <engine/e_client_interface.h>
#include <engine/client/graphics.h>

#include <game/version.hpp>
#include <game/generated/g_protocol.hpp>

#include <game/generated/gc_data.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/lineinput.hpp>
#include <game/localization.hpp>
#include <mastersrv/mastersrv.h>

vec4 MENUS::gui_color;
vec4 MENUS::color_tabbar_inactive_outgame;
vec4 MENUS::color_tabbar_active_outgame;
vec4 MENUS::color_tabbar_inactive;
vec4 MENUS::color_tabbar_active;
vec4 MENUS::color_tabbar_inactive_ingame;
vec4 MENUS::color_tabbar_active_ingame;


float MENUS::button_height = 25.0f;
float MENUS::listheader_height = 17.0f;
float MENUS::fontmod_height = 0.8f;

INPUT_EVENT MENUS::inputevents[MAX_INPUTEVENTS];
int MENUS::num_inputevents;

inline float hue_to_rgb(float v1, float v2, float h)
{
   if(h < 0) h += 1;
   if(h > 1) h -= 1;
   if((6 * h) < 1) return v1 + ( v2 - v1 ) * 6 * h;
   if((2 * h) < 1) return v2;
   if((3 * h) < 2) return v1 + ( v2 - v1 ) * ((2.0f/3.0f) - h) * 6;
   return v1;
}

inline vec3 hsl_to_rgb(vec3 in)
{
	float v1, v2;
	vec3 out;

	if(in.s == 0)
	{
		out.r = in.l;
		out.g = in.l;
		out.b = in.l;
	}
	else
	{
		if(in.l < 0.5f) 
			v2 = in.l * (1 + in.s);
		else           
			v2 = (in.l+in.s) - (in.s*in.l);

		v1 = 2 * in.l - v2;

		out.r = hue_to_rgb(v1, v2, in.h + (1.0f/3.0f));
		out.g = hue_to_rgb(v1, v2, in.h);
		out.b = hue_to_rgb(v1, v2, in.h - (1.0f/3.0f));
	} 

	return out;
}


MENUS::MENUS()
{
	popup = POPUP_NONE;
	active_page = PAGE_INTERNET;
	game_page = PAGE_GAME;
	
	need_restart = false;
	need_sendinfo = false;
	menu_active = true;
	
	escape_pressed = false;
	enter_pressed = false;
	num_inputevents = 0;
	
	last_input = time_get();
}

vec4 MENUS::button_color_mul(const void *pID)
{
	if(UI()->ActiveItem() == pID)
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == pID)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

int MENUS::DoButton_BrowseIcon(int What, const CUIRect *pRect)
{
	Graphics()->TextureSet(data->images[IMAGE_BROWSEICONS].id);
	
	Graphics()->QuadsBegin();
	RenderTools()->select_sprite(What);
	Graphics()->QuadsDrawTL(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsEnd();
	
	return 0;
}


int MENUS::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f)*button_color_mul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, pRect->h*fontmod_height, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int MENUS::DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f)*button_color_mul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, pRect->h*fontmod_height, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int MENUS::DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners)
{
	vec4 ColorMod(1,1,1,1);
	
	if(Checked)
		RenderTools()->DrawUIRect(pRect, color_tabbar_active, Corners, 10.0f);
	else
		RenderTools()->DrawUIRect(pRect, color_tabbar_inactive, Corners, 10.0f);
	UI()->DoLabel(pRect, pText, pRect->h*fontmod_height, 0);
	
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}


int MENUS::DoButton_SettingsTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, color_tabbar_active, CUI::CORNER_R, 10.0f);
	else
		RenderTools()->DrawUIRect(pRect, color_tabbar_inactive, CUI::CORNER_R, 10.0f);
	UI()->DoLabel(pRect, pText, pRect->h*fontmod_height, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int MENUS::DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
//void MENUS::ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), CUI::CORNER_T, 5.0f);
	CUIRect t;
	pRect->VSplitLeft(5.0f, 0, &t);
	UI()->DoLabel(&t, pText, pRect->h*fontmod_height, -1);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int MENUS::DoButton_ListRow(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	if(Checked)
	{
		CUIRect sr = *pRect;
		sr.Margin(1.5f, &sr);
		RenderTools()->DrawUIRect(&sr, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
	}
	UI()->DoLabel(pRect, pText, pRect->h*fontmod_height, -1);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int MENUS::DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect)
//void MENUS::ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra)
{
	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);
	
	c.Margin(2.0f, &c);
	RenderTools()->DrawUIRect(&c, vec4(1,1,1,0.25f)*button_color_mul(pID), CUI::CORNER_ALL, 3.0f);
	c.y += 2;
	UI()->DoLabel(&c, pBoxText, pRect->h*fontmod_height*0.6f, 0);
	UI()->DoLabel(&t, pText, pRect->h*fontmod_height*0.8f, -1);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int MENUS::DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButton_CheckBox_Common(pID, pText, Checked?"X":"", pRect);
}


int MENUS::DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	char buf[16];
	str_format(buf, sizeof(buf), "%d", Checked);
	return DoButton_CheckBox_Common(pID, pText, buf, pRect);
}

int MENUS::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, bool Hidden)
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

		for(int i = 0; i < num_inputevents; i++)
		{
			Len = strlen(pStr);
			LINEINPUT::manipulate(inputevents[i], pStr, StrSize, &Len, &AtIndex);
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

float MENUS::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	/* logic */
    float ReturnValue = Current;
    int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
		
		float min = pRect->y;
		float max = pRect->h-Handle.h;
		float cur = UI()->MouseY()-OffsetY;
		ReturnValue = (cur-min)/max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetY = UI()->MouseY()-Handle.y;
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.w = Rail.x-Slider.x;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
	Slider.x = Rail.x+Rail.w;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*button_color_mul(pID), CUI::CORNER_ALL, 2.5f);
	
    return ReturnValue;
}



float MENUS::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetX;
	pRect->VSplitLeft(33, &Handle, 0);

	Handle.x += (pRect->w-Handle.w)*Current;

	/* logic */
    float ReturnValue = Current;
    int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
		
		float min = pRect->x;
		float max = pRect->w-Handle.w;
		float cur = UI()->MouseX()-OffsetX;
		ReturnValue = (cur-min)/max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetX = UI()->MouseX()-Handle.x;
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.h = Rail.y-Slider.y;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_T, 2.5f);
	Slider.y = Rail.y+Rail.h;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_B, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*button_color_mul(pID), CUI::CORNER_ALL, 2.5f);
	
    return ReturnValue;
}

int MENUS::DoKeyReader(void *pID, const CUIRect *pRect, int Key)
{
	// process
	static void *pGrabbedID = 0;
	static bool MouseReleased = true;
	int Inside = UI()->MouseInside(pRect);
	int NewKey = Key;
	
	if(!UI()->MouseButton(0) && pGrabbedID == pID)
		MouseReleased = true;

	if(UI()->ActiveItem() == pID)
	{
		if(binder.got_key)
		{
			NewKey = binder.key.key;
			binder.got_key = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pID;
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0) && MouseReleased)
		{
			binder.take_key = true;
			binder.got_key = false;
			UI()->SetActiveItem(pID);
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// draw
	if (UI()->ActiveItem() == pID)
		DoButton_KeySelect(pID, "???", 0, pRect);
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pID, "", 0, pRect);
		else
			DoButton_KeySelect(pID, inp_key_name(Key), 0, pRect);
	}
	return NewKey;
}


int MENUS::render_menubar(CUIRect r)
{
	CUIRect box = r;
	CUIRect button;
	
	int active_page = config.ui_page;
	int new_page = -1;
	
	if(client_state() != CLIENTSTATE_OFFLINE)
		active_page = game_page;
	
	if(client_state() == CLIENTSTATE_OFFLINE)
	{
		/* offline menus */
		if(0) // this is not done yet
		{
			box.VSplitLeft(90.0f, &button, &box);
			static int news_button=0;
			if (DoButton_MenuTab(&news_button, localize("News"), active_page==PAGE_NEWS, &button, 0))
				new_page = PAGE_NEWS;
			box.VSplitLeft(30.0f, 0, &box); 
		}

		box.VSplitLeft(100.0f, &button, &box);
		static int internet_button=0;
		if(DoButton_MenuTab(&internet_button, localize("Internet"), active_page==PAGE_INTERNET, &button, CUI::CORNER_TL))
		{
			client_serverbrowse_refresh(BROWSETYPE_INTERNET);
			new_page = PAGE_INTERNET;
		}

		//box.VSplitLeft(4.0f, 0, &box);
		box.VSplitLeft(80.0f, &button, &box);
		static int lan_button=0;
		if(DoButton_MenuTab(&lan_button, localize("LAN"), active_page==PAGE_LAN, &button, 0))
		{
			client_serverbrowse_refresh(BROWSETYPE_LAN);
			new_page = PAGE_LAN;
		}

		//box.VSplitLeft(4.0f, 0, &box);
		box.VSplitLeft(110.0f, &button, &box);
		static int favorites_button=0;
		if(DoButton_MenuTab(&favorites_button, localize("Favorites"), active_page==PAGE_FAVORITES, &button, CUI::CORNER_TR))
		{
			client_serverbrowse_refresh(BROWSETYPE_FAVORITES);
			new_page  = PAGE_FAVORITES;
		}
		
		box.VSplitLeft(4.0f*5, 0, &box);
		box.VSplitLeft(100.0f, &button, &box);
		static int demos_button=0;
		if(DoButton_MenuTab(&demos_button, localize("Demos"), active_page==PAGE_DEMOS, &button, 0))
		{
			demolist_populate();
			new_page  = PAGE_DEMOS;
		}		
	}
	else
	{
		/* online menus */
		box.VSplitLeft(90.0f, &button, &box);
		static int game_button=0;
		if(DoButton_MenuTab(&game_button, localize("Game"), active_page==PAGE_GAME, &button, 0))
			new_page = PAGE_GAME;

		box.VSplitLeft(4.0f, 0, &box);
		box.VSplitLeft(140.0f, &button, &box);
		static int server_info_button=0;
		if(DoButton_MenuTab(&server_info_button, localize("Server info"), active_page==PAGE_SERVER_INFO, &button, 0))
			new_page = PAGE_SERVER_INFO;

		box.VSplitLeft(4.0f, 0, &box);
		box.VSplitLeft(140.0f, &button, &box);
		static int callvote_button=0;
		if(DoButton_MenuTab(&callvote_button, localize("Call vote"), active_page==PAGE_CALLVOTE, &button, 0))
			new_page = PAGE_CALLVOTE;
			
		box.VSplitLeft(30.0f, 0, &box);
	}
		
	/*
	box.VSplitRight(110.0f, &box, &button);
	static int system_button=0;
	if (UI()->DoButton(&system_button, "System", config.ui_page==PAGE_SYSTEM, &button))
		config.ui_page = PAGE_SYSTEM;
		
	box.VSplitRight(30.0f, &box, 0);
	*/
	
	box.VSplitRight(90.0f, &box, &button);
	static int quit_button=0;
	if(DoButton_MenuTab(&quit_button, localize("Quit"), 0, &button, 0))
		popup = POPUP_QUIT;

	box.VSplitRight(10.0f, &box, &button);
	box.VSplitRight(130.0f, &box, &button);
	static int settings_button=0;
	if(DoButton_MenuTab(&settings_button, localize("Settings"), active_page==PAGE_SETTINGS, &button, 0))
		new_page = PAGE_SETTINGS;
	
	if(new_page != -1)
	{
		if(client_state() == CLIENTSTATE_OFFLINE)
			config.ui_page = new_page;
		else
			game_page = new_page;
	}
		
	return 0;
}

void MENUS::render_loading(float percent)
{
	static int64 last_load_render = 0;

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	if(time_get()-last_load_render < time_freq()/60)
		return;
		
	last_load_render = time_get();
	
	// need up date this here to get correct
	vec3 rgb = hsl_to_rgb(vec3(config.ui_color_hue/255.0f, config.ui_color_sat/255.0f, config.ui_color_lht/255.0f));
	gui_color = vec4(rgb.r, rgb.g, rgb.b, config.ui_color_alpha/255.0f);
	
    CUIRect screen = *UI()->Screen();
	Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);
	
	render_background();

	float tw;

	float w = 700;
	float h = 200;
	float x = screen.w/2-w/2;
	float y = screen.h/2-h/2;

	Graphics()->BlendNormal();

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.50f);
	RenderTools()->draw_round_rect(x, y, w, h, 40.0f);
	Graphics()->QuadsEnd();


	const char *caption = localize("Loading");

	tw = gfx_text_width(0, 48.0f, caption, -1);
	CUIRect r;
	r.x = x;
	r.y = y+20;
	r.w = w;
	r.h = h;
	UI()->DoLabel(&r, caption, 48.0f, 0, -1);

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,0.75f);
	RenderTools()->draw_round_rect(x+40, y+h-75, (w-80)*percent, 25, 5.0f);
	Graphics()->QuadsEnd();

	gfx_swap();
}

void MENUS::render_news(CUIRect main_view)
{
	RenderTools()->DrawUIRect(&main_view, color_tabbar_active, CUI::CORNER_ALL, 10.0f);
}

void MENUS::on_init()
{

	/*
	array<string> my_strings;
	array<string>::range r2;
	my_strings.add("4");
	my_strings.add("6");
	my_strings.add("1");
	my_strings.add("3");
	my_strings.add("7");
	my_strings.add("5");
	my_strings.add("2");

	for(array<string>::range r = my_strings.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%s", r.front().cstr());
		
	sort(my_strings.all());
	
	dbg_msg("", "after:");
	for(array<string>::range r = my_strings.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%s", r.front().cstr());
		
	
	array<int> myarray;
	myarray.add(4);
	myarray.add(6);
	myarray.add(1);
	myarray.add(3);
	myarray.add(7);
	myarray.add(5);
	myarray.add(2);

	for(array<int>::range r = myarray.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%d", r.front());
		
	sort(myarray.all());
	sort_verify(myarray.all());
	
	dbg_msg("", "after:");
	for(array<int>::range r = myarray.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%d", r.front());
	
	exit(-1);
	// */
	
	if(config.cl_show_welcome)
		popup = POPUP_FIRST_LAUNCH;
	config.cl_show_welcome = 0;
}

void MENUS::popup_message(const char *topic, const char *body, const char *button)
{
	str_copy(message_topic, topic, sizeof(message_topic));
	str_copy(message_body, body, sizeof(message_body));
	str_copy(message_button, button, sizeof(message_button));
	popup = POPUP_MESSAGE;
}


int MENUS::render()
{
    CUIRect screen = *UI()->Screen();
	Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);

	static bool first = true;
	if(first)
	{
		if(config.ui_page == PAGE_INTERNET)
			client_serverbrowse_refresh(0);
		else if(config.ui_page == PAGE_LAN)
			client_serverbrowse_refresh(1);
		first = false;
	}
	
	if(client_state() == CLIENTSTATE_ONLINE)
	{
		color_tabbar_inactive = color_tabbar_inactive_ingame;
		color_tabbar_active = color_tabbar_active_ingame;
	}
	else
	{
		render_background();
		color_tabbar_inactive = color_tabbar_inactive_outgame;
		color_tabbar_active = color_tabbar_active_outgame;
	}
	
	CUIRect tab_bar;
	CUIRect main_view;

	// some margin around the screen
	screen.Margin(10.0f, &screen);
	
	if(popup == POPUP_NONE)
	{
		// do tab bar
		screen.HSplitTop(24.0f, &tab_bar, &main_view);
		tab_bar.VMargin(20.0f, &tab_bar);
		render_menubar(tab_bar);
		
		// news is not implemented yet
		if(config.ui_page <= PAGE_NEWS || config.ui_page > PAGE_SETTINGS || (client_state() == CLIENTSTATE_OFFLINE && config.ui_page >= PAGE_GAME && config.ui_page <= PAGE_CALLVOTE))
		{
			client_serverbrowse_refresh(BROWSETYPE_INTERNET);
			config.ui_page = PAGE_INTERNET;
		}
		
		// render current page
		if(client_state() != CLIENTSTATE_OFFLINE)
		{
			if(game_page == PAGE_GAME)
				render_game(main_view);
			else if(game_page == PAGE_SERVER_INFO)
				render_serverinfo(main_view);
			else if(game_page == PAGE_CALLVOTE)
				render_servercontrol(main_view);
			else if(game_page == PAGE_SETTINGS)
				render_settings(main_view);
		}
		else if(config.ui_page == PAGE_NEWS)
			render_news(main_view);
		else if(config.ui_page == PAGE_INTERNET)
			render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_LAN)
			render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_DEMOS)
			render_demolist(main_view);
		else if(config.ui_page == PAGE_FAVORITES)
			render_serverbrowser(main_view);
		else if(config.ui_page == PAGE_SETTINGS)
			render_settings(main_view);
	}
	else
	{
		// make sure that other windows doesn't do anything funnay!
		//UI()->SetHotItem(0);
		//UI()->SetActiveItem(0);
		char buf[128];
		const char *title = "";
		const char *extra_text = "";
		const char *button_text = "";
		int extra_align = 0;
		
		if(popup == POPUP_MESSAGE)
		{
			title = message_topic;
			extra_text = message_body;
			button_text = message_button;
		}
		else if(popup == POPUP_CONNECTING)
		{
			title = localize("Connecting to");
			extra_text = config.ui_server_address;  // TODO: query the client about the address
			button_text = localize("Abort");
			if(client_mapdownload_totalsize() > 0)
			{
				title = localize("Downloading map");
				str_format(buf, sizeof(buf), "%d/%d KiB", client_mapdownload_amount()/1024, client_mapdownload_totalsize()/1024);
				extra_text = buf;
			}
		}
		else if(popup == POPUP_DISCONNECTED)
		{
			title = localize("Disconnected");
			extra_text = client_error_string();
			button_text = localize("Ok");
			extra_align = -1;
		}
		else if(popup == POPUP_PURE)
		{
			title = localize("Disconnected");
			extra_text = localize("The server is running a non-standard tuning on a pure game type.");
			button_text = localize("Ok");
			extra_align = -1;
		}
		else if(popup == POPUP_PASSWORD)
		{
			title = localize("Password Incorrect");
			extra_text = client_error_string();
			button_text = localize("Try again");
		}
		else if(popup == POPUP_QUIT)
		{
			title = localize("Quit");
			extra_text = localize("Are you sure that you want to quit?");
		}
		else if(popup == POPUP_FIRST_LAUNCH)
		{
			title = localize("Welcome to Teeworlds");
			extra_text = localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server.");
			button_text = localize("Ok");
			extra_align = -1;
		}
		
		CUIRect box, part;
		box = screen;
		box.VMargin(150.0f, &box);
		box.HMargin(150.0f, &box);
		
		// render the box
		RenderTools()->DrawUIRect(&box, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);
		 
		box.HSplitTop(20.f, &part, &box);
		box.HSplitTop(24.f, &part, &box);
		UI()->DoLabel(&part, title, 24.f, 0);
		box.HSplitTop(20.f, &part, &box);
		box.HSplitTop(24.f, &part, &box);
		part.VMargin(20.f, &part);
		
		if(extra_align == -1)
			UI()->DoLabel(&part, extra_text, 20.f, -1, (int)part.w);
		else
			UI()->DoLabel(&part, extra_text, 20.f, 0, -1);

		if(popup == POPUP_QUIT)
		{
			CUIRect yes, no;
			box.HSplitBottom(20.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			part.VMargin(80.0f, &part);
			
			part.VSplitMid(&no, &yes);
			
			yes.VMargin(20.0f, &yes);
			no.VMargin(20.0f, &no);

			static int button_abort = 0;
			if(DoButton_Menu(&button_abort, localize("No"), 0, &no) || escape_pressed)
				popup = POPUP_NONE;

			static int button_tryagain = 0;
			if(DoButton_Menu(&button_tryagain, localize("Yes"), 0, &yes) || enter_pressed)
				client_quit();
		}
		else if(popup == POPUP_PASSWORD)
		{
			CUIRect label, textbox, tryagain, abort;
			
			box.HSplitBottom(20.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			part.VMargin(80.0f, &part);
			
			part.VSplitMid(&abort, &tryagain);
			
			tryagain.VMargin(20.0f, &tryagain);
			abort.VMargin(20.0f, &abort);
			
			static int button_abort = 0;
			if(DoButton_Menu(&button_abort, localize("Abort"), 0, &abort) || escape_pressed)
				popup = POPUP_NONE;

			static int button_tryagain = 0;
			if(DoButton_Menu(&button_tryagain, localize("Try again"), 0, &tryagain) || enter_pressed)
			{
				client_connect(config.ui_server_address);
			}
			
			box.HSplitBottom(60.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			
			part.VSplitLeft(60.0f, 0, &label);
			label.VSplitLeft(100.0f, 0, &textbox);
			textbox.VSplitLeft(20.0f, 0, &textbox);
			textbox.VSplitRight(60.0f, &textbox, 0);
			UI()->DoLabel(&label, localize("Password"), 20, -1);
			DoEditBox(&config.password, &textbox, config.password, sizeof(config.password), 14.0f, true);
		}
		else if(popup == POPUP_FIRST_LAUNCH)
		{
			CUIRect label, textbox;
			
			box.HSplitBottom(20.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			part.VMargin(80.0f, &part);
			
			static int enter_button = 0;
			if(DoButton_Menu(&enter_button, localize("Enter"), 0, &part) || enter_pressed)
				popup = POPUP_NONE;
			
			box.HSplitBottom(40.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			
			part.VSplitLeft(60.0f, 0, &label);
			label.VSplitLeft(100.0f, 0, &textbox);
			textbox.VSplitLeft(20.0f, 0, &textbox);
			textbox.VSplitRight(60.0f, &textbox, 0);
			UI()->DoLabel(&label, localize("Nickname"), 20, -1);
			DoEditBox(&config.player_name, &textbox, config.player_name, sizeof(config.player_name), 14.0f);
		}
		else
		{
			box.HSplitBottom(20.f, &box, &part);
			box.HSplitBottom(24.f, &box, &part);
			part.VMargin(120.0f, &part);

			static int button = 0;
			if(DoButton_Menu(&button, button_text, 0, &part) || escape_pressed || enter_pressed)
			{
				if(popup == POPUP_CONNECTING)
					client_disconnect();
				popup = POPUP_NONE;
			}
		}
	}
	
	return 0;
}


void MENUS::set_active(bool active)
{
	menu_active = active;
	if(!menu_active && need_sendinfo)
	{
		gameclient.send_info(false);
		need_sendinfo = false;
	}
}

void MENUS::on_reset()
{
}

bool MENUS::on_mousemove(float x, float y)
{
	last_input = time_get();
	
	if(!menu_active)
		return false;
		
	mouse_pos.x += x;
	mouse_pos.y += y;
	if(mouse_pos.x < 0) mouse_pos.x = 0;
	if(mouse_pos.y < 0) mouse_pos.y = 0;
	if(mouse_pos.x > Graphics()->ScreenWidth()) mouse_pos.x = Graphics()->ScreenWidth();
	if(mouse_pos.y > Graphics()->ScreenHeight()) mouse_pos.y = Graphics()->ScreenHeight();
	
	return true;
}

bool MENUS::on_input(INPUT_EVENT e)
{
	last_input = time_get();
	
	// special handle esc and enter for popup purposes
	if(e.flags&INPFLAG_PRESS)
	{
		if(e.key == KEY_ESCAPE)
		{
			escape_pressed = true;
			set_active(!is_active());
			return true;
		}
	}
		
	if(is_active())
	{
		// special for popups
		if(e.flags&INPFLAG_PRESS && e.key == KEY_RETURN)
			enter_pressed = true;
		
		if(num_inputevents < MAX_INPUTEVENTS)
			inputevents[num_inputevents++] = e;
		return true;
	}
	return false;
}

void MENUS::on_statechange(int new_state, int old_state)
{
	if(new_state == CLIENTSTATE_OFFLINE)
	{
		popup = POPUP_NONE;
		if(client_error_string() && client_error_string()[0] != 0)
		{
			if(strstr(client_error_string(), "password"))
			{
				popup = POPUP_PASSWORD;
				UI()->SetHotItem(&config.password);
				UI()->SetActiveItem(&config.password);
			}
			else
				popup = POPUP_DISCONNECTED;
		}	}
	else if(new_state == CLIENTSTATE_LOADING)
	{
		popup = POPUP_CONNECTING;
		client_serverinfo_request();
	}
	else if(new_state == CLIENTSTATE_CONNECTING)
		popup = POPUP_CONNECTING;
	else if (new_state == CLIENTSTATE_ONLINE || new_state == CLIENTSTATE_DEMOPLAYBACK)
	{
		popup = POPUP_NONE;
		set_active(false);
	}
}

extern "C" void font_debug_render();

void MENUS::on_render()
{
	/*
	// text rendering test stuff
	render_background();

	TEXT_CURSOR cursor;
	gfx_text_set_cursor(&cursor, 10, 10, 20, TEXTFLAG_RENDER);
	gfx_text_ex(&cursor, "ようこそ - ガイド", -1);

	gfx_text_set_cursor(&cursor, 10, 30, 15, TEXTFLAG_RENDER);
	gfx_text_ex(&cursor, "ようこそ - ガイド", -1);
	
	//Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->QuadsDrawTL(60, 60, 5000, 5000);
	Graphics()->QuadsEnd();
	return;*/
	
	if(client_state() != CLIENTSTATE_ONLINE && client_state() != CLIENTSTATE_DEMOPLAYBACK)
		set_active(true);

	if(client_state() == CLIENTSTATE_DEMOPLAYBACK)
	{
		CUIRect screen = *UI()->Screen();
		Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);
		render_demoplayer(screen);
	}
	
	if(client_state() == CLIENTSTATE_ONLINE && gameclient.servermode == gameclient.SERVERMODE_PUREMOD)
	{
		client_disconnect();
		set_active(true);
		popup = POPUP_PURE;
	}
	
	if(!is_active())
	{
		escape_pressed = false;
		enter_pressed = false;
		num_inputevents = 0;
		return;
	}
	
	// update colors
	vec3 rgb = hsl_to_rgb(vec3(config.ui_color_hue/255.0f, config.ui_color_sat/255.0f, config.ui_color_lht/255.0f));
	gui_color = vec4(rgb.r, rgb.g, rgb.b, config.ui_color_alpha/255.0f);

	color_tabbar_inactive_outgame = vec4(0,0,0,0.25f);
	color_tabbar_active_outgame = vec4(0,0,0,0.5f);

	float color_ingame_scale_i = 0.5f;
	float color_ingame_scale_a = 0.2f;
	color_tabbar_inactive_ingame = vec4(
		gui_color.r*color_ingame_scale_i,
		gui_color.g*color_ingame_scale_i,
		gui_color.b*color_ingame_scale_i,
		gui_color.a*0.8f);
	
	color_tabbar_active_ingame = vec4(
		gui_color.r*color_ingame_scale_a,
		gui_color.g*color_ingame_scale_a,
		gui_color.b*color_ingame_scale_a,
		gui_color.a);
    
	// update the ui
	CUIRect *screen = UI()->Screen();
	float mx = (mouse_pos.x/(float)Graphics()->ScreenWidth())*screen->w;
	float my = (mouse_pos.y/(float)Graphics()->ScreenHeight())*screen->h;
		
	int buttons = 0;
	if(inp_key_pressed(KEY_MOUSE_1)) buttons |= 1;
	if(inp_key_pressed(KEY_MOUSE_2)) buttons |= 2;
	if(inp_key_pressed(KEY_MOUSE_3)) buttons |= 4;
		
	UI()->Update(mx,my,mx*3.0f,my*3.0f,buttons);
    
	// render
	if(client_state() != CLIENTSTATE_DEMOPLAYBACK)
		render();

	// render cursor
	Graphics()->TextureSet(data->images[IMAGE_CURSOR].id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	Graphics()->QuadsDrawTL(mx,my,24,24);
	Graphics()->QuadsEnd();

	// render debug information
	if(config.debug)
	{
		CUIRect screen = *UI()->Screen();
		Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);

		char buf[512];
		str_format(buf, sizeof(buf), "%p %p %p", UI()->HotItem(), UI()->ActiveItem(), UI()->LastActiveItem());
		TEXT_CURSOR cursor;
		gfx_text_set_cursor(&cursor, 10, 10, 10, TEXTFLAG_RENDER);
		gfx_text_ex(&cursor, buf, -1);
	}

	escape_pressed = false;
	enter_pressed = false;
	num_inputevents = 0;
}

static int texture_blob = -1;

void MENUS::render_background()
{
	//Graphics()->Clear(1,1,1);
	//render_sunrays(0,0);
	if(texture_blob == -1)
		texture_blob = Graphics()->LoadTexture("blob.png", IMG_AUTO, 0);


	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	CUIRect s = *UI()->Screen();

	// render background color
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		//vec4 bottom(gui_color.r*0.3f, gui_color.g*0.3f, gui_color.b*0.3f, 1.0f);
		//vec4 bottom(0, 0, 0, 1.0f);
		vec4 bottom(gui_color.r, gui_color.g, gui_color.b, 1.0f);
		vec4 top(gui_color.r, gui_color.g, gui_color.b, 1.0f);
		Graphics()->SetColorVertex(0, top.r, top.g, top.b, top.a);
		Graphics()->SetColorVertex(1, top.r, top.g, top.b, top.a);
		Graphics()->SetColorVertex(2, bottom.r, bottom.g, bottom.b, bottom.a);
		Graphics()->SetColorVertex(3, bottom.r, bottom.g, bottom.b, bottom.a);
		Graphics()->QuadsDrawTL(0, 0, sw, sh);
	Graphics()->QuadsEnd();
	
	// render the tiles
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		float size = 15.0f;
		float offset_time = fmod(client_localtime()*0.15f, 2.0f);
		for(int y = -2; y < (int)(sw/size); y++)
			for(int x = -2; x < (int)(sh/size); x++)
			{
				Graphics()->SetColor(0,0,0,0.045f);
				Graphics()->QuadsDrawTL((x-offset_time)*size*2+(y&1)*size, (y+offset_time)*size, size, size);
			}
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(texture_blob);
	Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		Graphics()->QuadsDrawTL(-100, -100, sw+200, sh+200);
	Graphics()->QuadsEnd();

	// restore screen	
    {CUIRect screen = *UI()->Screen();
	Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);}	
}
