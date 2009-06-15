#include <base/vmath.hpp>
#include <base/tl/sorted_array.hpp>

#include <game/client/component.hpp>
#include <game/client/ui.hpp>


// compnent to fetch keypresses, override all other input
class MENUS_KEYBINDER : public COMPONENT
{
public:
	bool take_key;
	bool got_key;
	INPUT_EVENT key;
	MENUS_KEYBINDER();
	virtual bool on_input(INPUT_EVENT e);
};

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


	static void ui_draw_demoplayer_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);

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
	static int ui_do_edit_box(void *id, const RECT *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	static float ui_do_scrollbar_v(const void *id, const RECT *rect, float current);
	static float ui_do_scrollbar_h(const void *id, const RECT *rect, float current);

	static int ui_do_key_reader(void *id, const RECT *rect, int key);
	static void ui_do_getbuttons(int start, int stop, RECT view);

	struct LISTBOXITEM
	{
		int visible;
		int selected;
		RECT rect;
	};
	
	static void ui_do_listbox_start(void *id, const RECT *rect, float row_height, const char *title, int num_items, int selected_index);
	static LISTBOXITEM ui_do_listbox_nextitem(void *id);
	static int ui_do_listbox_end();
	
	//static void demolist_listdir_callback(const char *name, int is_dir, void *user);
	//static void demolist_list_callback(const RECT *rect, int index, void *user);

	enum
	{
		POPUP_NONE=0,
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_MESSAGE,
		POPUP_DISCONNECTED,
		POPUP_PURE,
		POPUP_PASSWORD,
		POPUP_QUIT, 
	};

	enum
	{
		PAGE_NEWS=1,
		PAGE_GAME,
		PAGE_SERVER_INFO,
		PAGE_CALLVOTE,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_FAVORITES,
		PAGE_DEMOS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
	};

	int game_page;
	int popup;
	int active_page;
	bool menu_active;
	vec2 mouse_pos;
	
	int64 last_input;
	
	//
	char message_topic[512];
	char message_body[512];
	char message_button[512];
	
	void popup_message(const char *topic, const char *body, const char *button);

	// TODO: this is a bit ugly but.. well.. yeah	
	enum { MAX_INPUTEVENTS = 32 };
	static INPUT_EVENT inputevents[MAX_INPUTEVENTS];
	static int num_inputevents;
	
	// some settings
	static float button_height;
	static float listheader_height;
	static float fontmod_height;
	
	// for graphic settings
	bool need_restart;
	bool need_sendinfo;
	
	//
	bool escape_pressed;
	bool enter_pressed;
	
	// for call vote
	int callvote_selectedoption;
	int callvote_selectedplayer;
	
	// demo
	struct DEMOITEM
	{
		char filename[512];
		char name[256];
		
		bool operator<(const DEMOITEM &other) { return str_comp(name, other.name) < 0; } 
	};
	
	sorted_array<DEMOITEM> demos;
		
	void demolist_populate();
	static void demolist_count_callback(const char *name, int is_dir, void *user);
	static void demolist_fetch_callback(const char *name, int is_dir, void *user);

	// found in menus.cpp
	int render();
	void render_background();
	//void render_loading(float percent);
	int render_menubar(RECT r);
	void render_news(RECT main_view);
	
	// found in menus_demo.cpp
	void render_demoplayer(RECT main_view);
	void render_demolist(RECT main_view);
	
	// found in menus_ingame.cpp
	void render_game(RECT main_view);
	void render_serverinfo(RECT main_view);
	void render_servercontrol(RECT main_view);
	void render_servercontrol_kick(RECT main_view);
	void render_servercontrol_server(RECT main_view);
	
	// found in menus_browser.cpp
	int selected_index;
	void render_serverbrowser_serverlist(RECT view);
	void render_serverbrowser_serverdetail(RECT view);
	void render_serverbrowser_filters(RECT view);
	void render_serverbrowser(RECT main_view);
	
	// found in menus_settings.cpp
	void render_settings_general(RECT main_view);
	void render_settings_player(RECT main_view);
	void render_settings_controls(RECT main_view);
	void render_settings_graphics(RECT main_view);
	void render_settings_sound(RECT main_view);
	void render_settings(RECT main_view);
	
	void set_active(bool active);
public:
	static MENUS_KEYBINDER binder;
	
	MENUS();

	void render_loading(float percent);

	bool is_active() const { return menu_active; }

	virtual void on_init();

	virtual void on_statechange(int new_state, int old_state);
	virtual void on_reset();
	virtual void on_render();
	virtual bool on_input(INPUT_EVENT e);
	virtual bool on_mousemove(float x, float y);
};
