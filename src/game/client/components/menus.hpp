#include <base/vmath.hpp>

#include <game/client/component.hpp>
#include <game/client/ui.hpp>

class MENUS : public COMPONENT
{	
	static vec4 gui_color;
	static vec4 color_tabbar_inactive_outgame;
	static vec4 color_tabbar_active_outgame;
	static vec4 color_tabbar_inactive_ingame;
	static vec4 color_tabbar_active_ingame;
	static vec4 color_tabbar_inactive;
	static vec4 color_tabbar_active;
	
	static vec4 button_color_mul(const void *id);

	static void ui_draw_browse_icon(int what, const RECT *r);
	static void ui_draw_menu_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_grid_header(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_list_row(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const RECT *r);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const RECT *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const RECT *r, const void *extra);

	static int ui_do_edit_box(void *id, const RECT *rect, char *str, int str_size, float font_size, bool hidden=false);
	static float ui_do_scrollbar_v(const void *id, const RECT *rect, float current);
	static float ui_do_scrollbar_h(const void *id, const RECT *rect, float current);

	static int ui_do_key_reader(void *id, const RECT *rect, int key);


	enum
	{
		POPUP_NONE=0,
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_DISCONNECTED,
		POPUP_PASSWORD,
		POPUP_QUIT, 
	};

	enum
	{
		PAGE_NEWS=0,
		PAGE_GAME,
		PAGE_SERVER_INFO,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_FAVORITES,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
	};

	int game_page;
	int popup;
	int active_page;
	bool menu_active;
	vec2 mouse_pos;
	
	int64 last_input;

	// TODO: this is a bit ugly but.. well.. yeah	
	enum { MAX_INPUTEVENTS = 32 };
	static INPUT_EVENT inputevents[MAX_INPUTEVENTS];
	static int num_inputevents;
	
	// for graphic settings
	bool need_restart;

	// found in menus.cpp
	int render();
	void render_background();
	//void render_loading(float percent);
	int render_menubar(RECT r);
	void render_news(RECT main_view);
	void render_game(RECT main_view);
	void render_serverinfo(RECT main_view);
	
	// found in menus_browser.cpp
	int selected_index;
	void render_serverbrowser_serverlist(RECT view);
	void render_serverbrowser_serverdetail(RECT view);
	void render_serverbrowser_filters(RECT view);
	void render_serverbrowser(RECT main_view);
	
	// found in menus_settings.cpp
	void render_settings_player(RECT main_view);
	void render_settings_controls(RECT main_view);
	void render_settings_graphics(RECT main_view);
	void render_settings_sound(RECT main_view);
	void render_settings(RECT main_view);
	
public:
	MENUS();

	void render_loading(float percent);

	bool is_active() const { return menu_active; }

	void init();

	virtual void on_statechange(int new_state, int old_state);
	virtual void on_reset();
	virtual void on_render();
	virtual bool on_input(INPUT_EVENT e);
	virtual bool on_mousemove(float x, float y);
};
