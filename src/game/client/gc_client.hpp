#include <base/vmath.hpp>

#include <game/generated/g_protocol.hpp>
#include <game/g_game.hpp>

#include <game/client/gc_render.hpp>

// sound channels
enum
{
	CHN_GUI=0,
	CHN_MUSIC,
	CHN_WORLD,
	CHN_GLOBAL,
};

//extern struct data_container *data;

extern vec2 mouse_pos;
extern vec2 local_character_pos;
extern vec2 local_target_pos;

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
	int num_players;
};

extern SNAPSTATE netobjects;

/*
extern const NETOBJ_PLAYER_CHARACTER *local_character;
extern const NETOBJ_PLAYER_CHARACTER *local_prev_character;
extern const NETOBJ_PLAYER_INFO *local_info;
extern const NETOBJ_FLAG *flags[2];
extern const NETOBJ_GAME *gameobj;
* */

extern TUNING_PARAMS tuning;

// predicted players
extern CHARACTER_CORE predicted_prev_char;
extern CHARACTER_CORE predicted_char;

// input
extern NETOBJ_PLAYER_INPUT input_data;
extern int input_direction_left;
extern int input_direction_right;

// debug
extern int64 debug_firedelay;

// extra projs
enum
{
	MAX_EXTRA_PROJECTILES=32,
};

extern NETOBJ_PROJECTILE extraproj_projectiles[MAX_EXTRA_PROJECTILES];
extern int extraproj_num;

void extraproj_reset();

// chat
enum
{
	CHATMODE_NONE=0,
	CHATMODE_ALL,
	CHATMODE_TEAM,
};

extern int chat_mode;
void chat_add_line(int client_id, int team, const char *line);
void chat_reset();
bool chat_input_handle(INPUT_EVENT e, void *user_data);

// broadcasts
extern char broadcast_text[1024];
extern int64 broadcast_time;

// motd
extern int64 server_motd_time;
extern char server_motd[900]; // FUGLY

// line input helter
class line_input
{
	char str[256];
	unsigned len;
	unsigned cursor_pos;
public:
	class callback
	{
	public:
		virtual ~callback() {}
		virtual bool event(INPUT_EVENT e) = 0;
	};

	line_input();
	void clear();
	void process_input(INPUT_EVENT e);
	void set(const char *string);
	const char *get_string() const { return str; }
	int get_length() const { return len; }
	unsigned cursor_offset() const { return cursor_pos; }
};

class INPUT_STACK_HANDLER
{
public:
	typedef bool (*CALLBACK)(INPUT_EVENT e, void *user);
	
	INPUT_STACK_HANDLER();
	void add_handler(CALLBACK cb, void *user_data);
	void dispatch_input();
	
private:
	enum
	{
		MAX_HANDLERS=16
	};
	
	CALLBACK handlers[MAX_HANDLERS];
	void *user_data[MAX_HANDLERS];
	int num_handlers;
};

extern INPUT_STACK_HANDLER input_stack;


extern int emoticon_selector_active; // TODO: ugly
extern int scoreboard_active; // TODO: ugly

// client data
struct CLIENT_DATA
{
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

extern CLIENT_DATA client_datas[MAX_CLIENTS];

// kill messages
struct KILLMSG
{
	int weapon;
	int victim;
	int killer;
	int mode_special; // for CTF, if the guy is carrying a flag for example
	int tick;
};

const int killmsg_max = 5;
extern KILLMSG killmsgs[killmsg_max];
extern int killmsg_current;

//
void send_switch_team(int team);

// various helpers
void snd_play_random(int chn, int setid, float vol, vec2 pos);
void process_events(int snaptype);
void clear_object_pointers();
void reset_projectile_particles();
void send_info(bool start);
void send_emoticon(int emoticon);

void chat_say(int team, const char *line);
void chat_enable_mode(int team);

inline vec2 random_dir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }


// effects
void effects_update();

void effect_bullettrail(vec2 pos);
void effect_smoketrail(vec2 pos, vec2 vel);
void effect_skidtrail(vec2 pos, vec2 vel);
void effect_explosion(vec2 pos);
void effect_air_jump(vec2 pos);
void effect_damage_indicator(vec2 pos, vec2 dir);
void effect_playerspawn(vec2 pos);
void effect_playerdeath(vec2 pos, int cid);
void effect_powerupshine(vec2 pos, vec2 size);

// particles
struct PARTICLE
{
	void set_default()
	{
		vel = vec2(0,0);
		life_span = 0;
		start_size = 32;
		end_size = 32;
		rot = 0;
		rotspeed = 0;
		gravity = 0;
		friction = 0;
		flow_affected = 1.0f;
		color = vec4(1,1,1,1);
	}
	
	vec2 pos;
	vec2 vel;

	int spr;

	float flow_affected;

	float life_span;
	
	float start_size;
	float end_size;

	float rot;
	float rotspeed;

	float gravity;
	float friction;

	vec4 color;
	
	// set by the particle system
	float life;
	int prev_part;
	int next_part;
};

enum
{
	PARTGROUP_PROJECTILE_TRAIL=0,
	PARTGROUP_EXPLOSIONS,
	PARTGROUP_GENERAL,
	NUM_PARTGROUPS
};

void particle_add(int group, PARTICLE *part);
void particle_render(int group);
void particle_update(float time_passed);
void particle_reset();

// flow grid
vec2 flow_get(vec2 pos);
void flow_add(vec2 pos, vec2 vel, float size);
void flow_dbg_render();
void flow_init();
void flow_update();

//
void binds_default();
void binds_save();
void binds_set(int keyid, const char *str);
const char *binds_get(int keyid);

