#include <engine/system.h>
#include <game/math.h>
#include <math.h>
#include "../engine/interface.h"
#include "mapres_col.h"

#include "game_protocol.h"

inline vec2 get_direction(int angle)
{
	float a = angle/256.0f;
	return vec2(cosf(a), sinf(a));
}

inline float get_angle(vec2 dir)
{
	float a = atan(dir.y/dir.x);
	if(dir.x < 0)
		a = a+pi;
	return a;
}

#define LERP(a,b,t) (a + (b-a) * t)
#define min(a, b) ( a > b ? b : a)
#define max(a, b) ( a > b ? a : b)

inline bool col_check_point(float x, float y) { return col_check_point((int)x, (int)y) != 0; }
inline bool col_check_point(vec2 p) { return col_check_point(p.x, p.y); }

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
	MAPRES_SPAWNPOINT_RED=2,
	MAPRES_SPAWNPOINT_BLUE=3,
	MAPRES_FLAGSTAND_RED=4,
	MAPRES_FLAGSTAND_BLUE=5,
	
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
