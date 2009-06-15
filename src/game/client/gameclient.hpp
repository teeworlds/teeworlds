
#include <base/vmath.hpp>
#include <engine/e_console.h>
#include <game/gamecore.hpp>
#include "render.hpp"

class GAMECLIENT
{
	class STACK
	{
	public:
		enum
		{
			MAX_COMPONENTS = 64,
		};
	
		STACK();
		void add(class COMPONENT *component);
		
		class COMPONENT *components[MAX_COMPONENTS];
		int num;
	};
	
	STACK all;
	STACK input;
	
	
	void dispatch_input();
	void process_events();
	void update_local_character_pos();

	int predicted_tick;
	int last_new_predicted_tick;

	static void con_team(void *result, void *user_data);
	static void con_kill(void *result, void *user_data);
	
	static void conchain_special_infoupdate(void *result, void *user_data, CONSOLE_CALLBACK cb, void *cbuser);
	
public:
	bool suppress_events;
	bool new_tick;
	bool new_predicted_tick;

	// TODO: move this
	TUNING_PARAMS tuning;
	
	enum
	{
		SERVERMODE_PURE=0,
		SERVERMODE_MOD,
		SERVERMODE_PUREMOD,
	};
	int servermode;

	vec2 local_character_pos;

	// predicted players
	CHARACTER_CORE predicted_prev_char;
	CHARACTER_CORE predicted_char;

	// snap pointers
	struct SNAPSTATE
	{
		const NETOBJ_CHARACTER *local_character;
		const NETOBJ_CHARACTER *local_prev_character;
		const NETOBJ_PLAYER_INFO *local_info;
		const NETOBJ_FLAG *flags[2];
		const NETOBJ_GAME *gameobj;

		const NETOBJ_PLAYER_INFO *player_infos[MAX_CLIENTS];
		const NETOBJ_PLAYER_INFO *info_by_score[MAX_CLIENTS];
		
		int local_cid;
		int num_players;
		int team_size[2];
		bool spectate;
		
		//
		struct CHARACTERINFO
		{
			bool active;
			
			// snapshots
			NETOBJ_CHARACTER prev;
			NETOBJ_CHARACTER cur;
			
			// interpolated position
			vec2 position;
		};
		
		CHARACTERINFO characters[MAX_CLIENTS];
	};

	SNAPSTATE snap;
	
	// client data
	struct CLIENT_DATA
	{
		int use_custom_color;
		int color_body;
		int color_feet;
		
		char name[64];
		char skin_name[64];
		int skin_id;
		int skin_color;
		int team;
		int emoticon;
		int emoticon_start;
		CHARACTER_CORE predicted;
		
		TEE_RENDER_INFO skin_info; // this is what the server reports
		TEE_RENDER_INFO render_info; // this is what we use
		
		float angle;
		
		void update_render_info();
	};

	CLIENT_DATA clients[MAX_CLIENTS];
	
	void on_reset();

	// hooks
	void on_connected();
	void on_render();
	void on_init();
	void on_save();
	void on_console_init();
	void on_statechange(int new_state, int old_state);
	void on_message(int msgtype);
	void on_snapshot();
	void on_predict();
	int on_snapinput(int *data);

	// actions
	// TODO: move these
	void send_switch_team(int team);
	void send_info(bool start);
	void send_kill(int client_id);
	
	// pointers to all systems
	class CONSOLE *console;
	class BINDS *binds;
	class PARTICLES *particles;
	class MENUS *menus;
	class SKINS *skins;
	class FLOW *flow;
	class CHAT *chat;
	class DAMAGEIND *damageind;
	class CAMERA *camera;
	class CONTROLS *controls;
	class EFFECTS *effects;
	class SOUNDS *sounds;
	class MOTD *motd;
	class MAPIMAGES *mapimages;
	class VOTING *voting;
};

extern GAMECLIENT gameclient;

extern const char *localize(const char *str);
