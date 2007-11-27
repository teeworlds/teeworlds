/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
// NOTE: Be very careful when editing this file as it will change the network version

// --------- PHYSICS TWEAK! --------
const float ground_control_speed = 7.0f;
const float ground_control_accel = 2.0f;
const float ground_friction = 0.5f;
const float ground_jump_speed = 13.5f;
const float air_control_speed = 5.0f;
const float air_control_accel = 1.5f;
const float air_friction = 0.95f;
const float hook_length = 34*10.0f;
const float hook_fire_speed = 45.0f;
const float hook_drag_accel = 3.0f;
const float hook_drag_speed = 15.0f;
const float gravity = 0.5f;
const float wall_friction = 0.80f;
const float wall_jump_speed_up = ground_jump_speed*0.8f;
const float wall_jump_speed_out = ground_jump_speed*0.8f;

// Network stuff
enum
{
	OBJTYPE_NULL=0,
	OBJTYPE_GAME,
	OBJTYPE_PLAYER_INFO,
	OBJTYPE_PLAYER_CHARACTER, // use this if you are searching for the player entity
	OBJTYPE_PROJECTILE,
	OBJTYPE_POWERUP,
	OBJTYPE_FLAG,
	EVENT_EXPLOSION,
	EVENT_DAMAGEINDICATION,
	EVENT_SOUND_WORLD,
	EVENT_SMOKE,
	EVENT_SPAWN,
	EVENT_DEATH,
};

enum
{
	MSG_NULL=0,
	MSG_SAY, // client -> server
	MSG_CHAT, // server -> client
	MSG_SETINFO, // server -> client - contains name, skin and color info
	MSG_KILLMSG, // server -> client
	MSG_SETTEAM,
	MSG_JOIN,
	MSG_QUIT,
	MSG_EMOTICON,
	MSG_STARTINFO, // client -> server
	MSG_CHANGEINFO, // client -> server
	MSG_READY_TO_ENTER, // server -> client
    MSG_WEAPON_PICKUP,
    MSG_SOUND_GLOBAL,
};

enum
{
	EMOTE_NORMAL=0,
	EMOTE_PAIN,
	EMOTE_HAPPY,
	EMOTE_SURPRISE,
	EMOTE_ANGRY,
	EMOTE_BLINK,
};

enum
{
	INPUT_STATE_MASK=0x1f,
};

enum
{
	STATE_UNKNOWN=0,
	STATE_PLAYING,
	STATE_IN_MENU,
	STATE_CHATTING,

	GAMETYPE_DM=0,
	GAMETYPE_TDM,
	GAMETYPE_CTF,
};

struct player_input
{
	int left;
	int right;

	int target_x;
	int target_y;

	int jump;
	int fire;
	int hook;
	int blink;
	int state;

	int wanted_weapon;
	int next_weapon;
	int prev_weapon;
};

struct ev_common
{
	int x, y;
};

struct ev_explosion : public ev_common
{
};

struct ev_spawn : public ev_common
{
};

struct ev_death : public ev_common
{
};

struct ev_sound : public ev_common
{
	int sound;
};

struct ev_damageind : public ev_common
{
	int angle;
};

struct obj_game
{
	int round_start_tick;
	int game_over;
	int sudden_death;
	int paused;

	int score_limit;
	int time_limit;
	int gametype;

	int warmup;

	int teamscore[2];
};

struct obj_projectile
{
	int type;
	int tick;
	int x, y;
	int vx, vy; // should be an angle instead
	int start_tick;
};

struct obj_powerup
{
	int x, y;
	int type; // why do we need two types?
	int subtype;
};

struct obj_flag
{
	int x, y;
	int team;
	int carried_by; // is set if the local player has the flag
};

// core object needed for physics
struct obj_player_core
{
	int x, y;
	int vx, vy;
	int angle;

	int hooked_player;
	int hook_state;
	int hook_x, hook_y;
	int hook_dx, hook_dy;
};

// info about the player that is only needed when it's on screen
struct obj_player_character : public obj_player_core
{
	int state;

	int health;
	int armor;
	int ammocount;
	int weaponstage;

	int weapon; // current active weapon

	int emote;

	int attacktick; // num attack ticks left of current attack
};

// information about the player that is always needed
struct obj_player_info
{
	int local;
	int clientid;

	int team;
	int score;
	int latency;
	int latency_flux;
};
