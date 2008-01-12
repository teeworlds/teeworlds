#include <game/g_vmath.h>
#include <game/g_protocol.h>
#include <game/g_game.h>

#include <game/client/gc_render.h>

// sound channels
enum
{
	CHN_GUI=0,
	CHN_MUSIC,
	CHN_WORLD,
	CHN_GLOBAL,
};

extern struct data_container *data;

extern vec2 mouse_pos;
extern vec2 local_character_pos;
extern vec2 local_target_pos;

// snap pointers
extern const obj_player_character *local_character;
extern const obj_player_character *local_prev_character;
extern const obj_player_info *local_info;
extern const obj_flag *flags[2];
extern const obj_game *gameobj;

// predicted players
extern player_core predicted_prev_player;
extern player_core predicted_player;

// input
extern int picked_up_weapon;
extern player_input input_data;
extern int input_target_lock;

// chat
enum
{
	CHATMODE_NONE=0,
	CHATMODE_ALL,
	CHATMODE_TEAM,
	CHATMODE_CONSOLE,
	CHATMODE_REMOTECONSOLE,
};

extern int chat_mode;
void chat_add_line(int client_id, int team, const char *line);
void chat_reset();

// client data
struct client_data
{
	char name[64];
	char skin_name[64];
	int skin_id;
	int skin_color;
	int team;
	int emoticon;
	int emoticon_start;
	player_core predicted;
	
	tee_render_info skin_info; // this is what the server reports
	tee_render_info render_info; // this is what we use
	
	void update_render_info();
};

extern client_data client_datas[MAX_CLIENTS];

// kill messages
struct killmsg
{
	int weapon;
	int victim;
	int killer;
	int mode_special; // for CTF, if the guy is carrying a flag for example
	int tick;
};

const int killmsg_max = 5;
extern killmsg killmsgs[killmsg_max];
extern int killmsg_current;

// various helpers
void snd_play_random(int chn, int setid, float vol, vec2 pos);
void process_events(int snaptype);
void clear_object_pointers();
void reset_projectile_particles();
void send_info(bool start);

void effect_air_jump(vec2 pos);
