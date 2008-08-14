#include <engine/e_server_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "projectile.hpp"


//////////////////////////////////////////////////
// projectile
//////////////////////////////////////////////////
PROJECTILE::PROJECTILE(int type, int owner, vec2 pos, vec2 dir, int span, ENTITY* powner,
	int damage, int flags, float force, int sound_impact, int weapon)
: ENTITY(NETOBJTYPE_PROJECTILE)
{
	this->type = type;
	this->pos = pos;
	this->direction = dir;
	this->lifespan = span;
	this->owner = owner;
	this->powner = powner;
	this->flags = flags;
	this->force = force;
	this->damage = damage;
	this->sound_impact = sound_impact;
	this->weapon = weapon;
	this->bounce = 0;
	this->start_tick = server_tick();
	game.world.insert_entity(this);
}

void PROJECTILE::reset()
{
	game.world.destroy_entity(this);
}

vec2 PROJECTILE::get_pos(float time)
{
	float curvature = 0;
	float speed = 0;
	if(type == WEAPON_GRENADE)
	{
		curvature = tuning.grenade_curvature;
		speed = tuning.grenade_speed;
	}
	else if(type == WEAPON_SHOTGUN)
	{
		curvature = tuning.shotgun_curvature;
		speed = tuning.shotgun_speed;
	}
	else if(type == WEAPON_GUN)
	{
		curvature = tuning.gun_curvature;
		speed = tuning.gun_speed;
	}
	
	return calc_pos(pos, direction, curvature, speed, time);
}


void PROJECTILE::tick()
{
	
	float pt = (server_tick()-start_tick-1)/(float)server_tickspeed();
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	vec2 prevpos = get_pos(pt);
	vec2 curpos = get_pos(ct);

	lifespan--;
	
	int collide = col_intersect_line(prevpos, curpos, &curpos);
	//int collide = col_check_point((int)curpos.x, (int)curpos.y);
	
	CHARACTER *targetchr = game.world.intersect_character(prevpos, curpos, 6.0f, curpos, powner);
	if(targetchr || collide || lifespan < 0)
	{
		if(lifespan >= 0 || weapon == WEAPON_GRENADE)
			game.create_sound(curpos, sound_impact);

		if(flags & PROJECTILE_FLAGS_EXPLODE)
			game.create_explosion(curpos, owner, weapon, false);
		else if(targetchr)
		{
			targetchr->take_damage(direction * max(0.001f, force), damage, owner, weapon);
		}

		game.world.destroy_entity(this);
	}
}

void PROJECTILE::fill_info(NETOBJ_PROJECTILE *proj)
{
	proj->x = (int)pos.x;
	proj->y = (int)pos.y;
	proj->vx = (int)(direction.x*100.0f);
	proj->vy = (int)(direction.y*100.0f);
	proj->start_tick = start_tick;
	proj->type = type;
}

void PROJECTILE::snap(int snapping_client)
{
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	
	if(distance(game.players[snapping_client].view_pos, get_pos(ct)) > 1000.0f)
		return;

	NETOBJ_PROJECTILE *proj = (NETOBJ_PROJECTILE *)snap_new_item(NETOBJTYPE_PROJECTILE, id, sizeof(NETOBJ_PROJECTILE));
	fill_info(proj);
}
