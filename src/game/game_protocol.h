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
	OBJTYPE_PLAYER,
	OBJTYPE_PROJECTILE,
	OBJTYPE_POWERUP,
	OBJTYPE_FLAG,
	EVENT_EXPLOSION,
	EVENT_DAMAGEINDICATION,
	EVENT_SOUND,
	EVENT_SMOKE,
	EVENT_SPAWN,
	EVENT_DEATH,
};

enum
{
	MSG_NULL=0,
	MSG_SAY,
	MSG_CHAT,
	MSG_SETNAME,
	MSG_KILLMSG,
	MSG_SWITCHTEAM,
	MSG_JOIN,
	MSG_QUIT,
	MSG_EMOTICON,
	MSG_CHANGENAME,
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
	int activeweapon;
	int state;
	
	int next_weapon;
	int prev_weapon;
};


struct ev_explosion
{
	int x, y;
};

struct ev_spawn
{
	int x, y;
};

struct ev_death
{
	int x, y;
};

struct ev_sound
{
	int x, y;
	int sound; // if (0x80000000 flag is set -> looping) if (0x40000000 is set -> stop looping
};

struct ev_damageind
{
	int x, y;
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
	int x, y;
	int vx, vy; // should be an angle instead
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
	int local_carry;
};


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

struct obj_player : public obj_player_core
{
	int local;
	int clientid;
	int state;

	int health;
	int armor;
	int ammocount;
	
	int weapon; // current active weapon

	int attacktick; // num attack ticks left of current attack
	
	int score;
	int latency;
	int latency_flux;
	int emote;
	
	int team;
};
