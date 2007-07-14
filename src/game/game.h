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

inline bool col_check_point(float x, float y) { return col_check_point((int)x, (int)y) != 0; }
inline bool col_check_point(baselib::vec2 p) { return col_check_point(p.x, p.y); }

// Network stuff

enum
{
	OBJTYPE_NULL=0,
	OBJTYPE_PLAYER,
	OBJTYPE_PROJECTILE,
	OBJTYPE_POWERUP,
	EVENT_EXPLOSION,
	EVENT_DAMAGEINDICATION,
	EVENT_SOUND,
	EVENT_SMOKE,
};

enum
{
	MSG_NULL=0,
	MSG_SAY,
	MSG_CHAT,
	MSG_SETNAME,
	MSG_KILLMSG,
};

enum
{
	EMOTE_NORMAL=0,
	EMOTE_BLINK,
	EMOTE_WINK,
	EMOTE_PAIN,
	EMOTE_HAPPY,
};

struct player_input
{
	int left;
	int right;
	int angle;
	int jump;
	int fire;
	int hook;
	int blink;
	int activeweapon;
};


struct ev_explosion
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

struct obj_player
{
	//int name[8];
	int local;
	int clientid;

	int health;
	int armor;
	int ammocount;
	
	int x, y;
	int vx, vy;
	int angle;
	
	// current active weapon
	int weapon;
	// current active modifier

	// num attack ticks left of current attack
	int attacktick;
	
	int score;
	int emote;
	
	int hook_active;
	int hook_x, hook_y;
};

enum
{
	WEAPON_TYPE_GUN			= 0,
	WEAPON_TYPE_ROCKET		= 1,
	WEAPON_TYPE_SHOTGUN		= 2,
	WEAPON_TYPE_MELEE		= 3,
	WEAPON_NUMWEAPONS,
	//WEAPON_TYPE_SNIPER		= 2,

	POWERUP_TYPE_HEALTH			= 0,
	POWERUP_TYPE_ARMOR			= 1,
	POWERUP_TYPE_WEAPON			= 2,
	POWERUP_TYPE_NINJA			= 3,
	POWERUP_TYPE_TIMEFIELD		= 4,
	POWERUP_TYPE_NUMPOWERUPS,

	PLAYER_MAXHEALTH			= 10,
	PLAYER_MAXARMOR				= 10,

	MODIFIER_TYPE_NINJA			= 0,
	MODIFIER_TYPE_TIMEFIELD		= 1,
	MODIFIER_NUMMODIFIERS,
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
	ITEM_HEALTH_1 =0x00020001,
	ITEM_HEALTH_5 =0x00020005,
	ITEM_HEALTH_10=0x00020010,
	ITEM_ARMOR_1=0x00030001,
	ITEM_ARMOR_5=0x00030005,
	ITEM_ARMOR_10=0x00030010,
	ITEM_NINJA=0x00040001,
};
