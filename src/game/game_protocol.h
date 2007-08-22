// NOTE: Be very careful when editing this file as it will change the network version

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
};

struct obj_player
{
	int local;
	int clientid;
	int state;

	int health;
	int armor;
	int ammocount;
	
	int x, y;
	int vx, vy;
	int angle;
	
	int weapon; // current active weapon

	int attacktick; // num attack ticks left of current attack
	
	int score;
	int latency;
	int latency_flux;
	int emote;
	
	int hook_active;
	int hook_x, hook_y;
	int team;
};
