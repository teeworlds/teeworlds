#include <baselib/system.h>
#include <baselib/math.h>
#include <math.h>
#include "../engine/interface.h"
#include "mapres_col.h"

inline baselib::vec2 get_direction(int angle)
{
	float a = angle/256.0f;
	return baselib::vec2(cosf(a), sinf(a));
}

inline float get_angle(baselib::vec2 dir)
{
	float a = atan(dir.y/dir.x);
	if(dir.x < 0)
		a = a+baselib::pi;
	return a;
}

#define LERP(a,b,t) (a + (b-a) * t)
#define min(a, b) ( a > b ? b : a)
#define max(a, b) ( a > b ? a : b)

inline bool col_check_point(float x, float y) { return col_check_point((int)x, (int)y) != 0; }
inline bool col_check_point(baselib::vec2 p) { return col_check_point(p.x, p.y); }

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


struct mapres_spawnpoint
{
	int x, y;
	int type;
};

struct mapres_item
{
	int x, y;
	int type;
};

enum
{
	MAPRES_SPAWNPOINT=1,
	MAPRES_ITEM=2,
	
	ITEM_NULL=0,
	ITEM_WEAPON_GUN=0x00010001,
	ITEM_WEAPON_SHOTGUN=0x00010002,
	ITEM_WEAPON_ROCKET=0x00010003,
	ITEM_WEAPON_SNIPER=0x00010004,
	ITEM_WEAPON_HAMMER=0x00010005,
	ITEM_HEALTH =0x00020001,
	ITEM_ARMOR=0x00030001,
	ITEM_NINJA=0x00040001,
};
