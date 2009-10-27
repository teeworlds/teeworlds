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
	
	vec4 button_color_mul(const void *pID);


	int DoButton_DemoPlayer(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_SettingsTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoButton_BrowseIcon(int Checked, const CUIRect *pRect);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_ListRow(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	
	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, bool Hidden=false);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	int DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(void *pID, const CUIRect *pRect, int Key);

	//static int ui_do_key_reader(void *id, const CUIRect *rect, int key);
	void ui_do_getbuttons(int start, int stop, CUIRect view);

	struct LISTBOXITEM
	{
		int visible;
		int selected;
		CUIRect rect;
		CUIRect hitrect;
	};
	
	void ui_do_listbox_start(void *id, const CUIRect *rect, float row_height, const char *title, int num_items, int selected_index);
	LISTBOXITEM ui_do_listbox_nextitem(void *id);
	static LISTBOXITEM ui_do_listbox_nextrow();
	int ui_do_listbox_end();
	
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
	//void render_background();
	//void render_loading(float percent);
	int render_menubar(CUIRect r);
	void render_news(CUIRect main_view);
	
	// found in menus_demo.cpp
	void render_demoplayer(CUIRect main_view);
	void render_demolist(CUIRect main_view);
	
	// found in menus_ingame.cpp
	void render_game(CUIRect main_view);
	void render_serverinfo(CUIRect main_view);
	void render_servercontrol(CUIRect main_view);
	void render_servercontrol_kick(CUIRect main_view);
	void render_servercontrol_server(CUIRect main_view);
	
	// found in menus_browser.cpp
	int selected_index;
	void render_serverbrowser_serverlist(CUIRect view);
	void render_serverbrowser_serverdetail(CUIRect view);
	void render_serverbrowser_filters(CUIRect view);
	void render_serverbrowser(CUIRect main_view);
	
	// found in menus_settings.cpp
	void render_settings_general(CUIRect main_view);
	void render_settings_player(CUIRect main_view);
	void render_settings_controls(CUIRect main_view);
	void render_settings_graphics(CUIRect main_view);
	void render_settings_sound(CUIRect main_view);
	void render_settings(CUIRect main_view);
	
	void set_active(bool active);
public:
	void render_background();


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
