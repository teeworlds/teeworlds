/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "laser.hpp"

//////////////////////////////////////////////////
// laser
//////////////////////////////////////////////////
LASER::LASER(vec2 pos, vec2 direction, float start_energy, CHARACTER *owner)
: ENTITY(NETOBJTYPE_LASER)
{
	this->pos = pos;
	this->owner = owner;
	energy = start_energy;
	dir = direction;
	bounces = 0;
	do_bounce();
	
	game.world.insert_entity(this);
}


bool LASER::hit_character(vec2 from, vec2 to)
{
	vec2 at;
	CHARACTER *hit = game.world.intersect_character(pos, to, 0.0f, at, owner);
	if(!hit)
		return false;

	this->from = from;
	pos = at;
	energy = -1;		
	hit->take_damage(vec2(0,0), tuning.laser_damage, owner->player->client_id, WEAPON_RIFLE);
	return true;
}

void LASER::do_bounce()
{
	eval_tick = server_tick();
	
	if(energy < 0)
	{
		//dbg_msg("laser", "%d removed", server_tick());
		game.world.destroy_entity(this);
		return;
	}
	
	vec2 to = pos + dir*energy;
	
	if(col_intersect_line(pos, to, &to))
	{
		if(!hit_character(pos, to))
		{
			// intersected
			from = pos;
			pos = to - dir*2;
			vec2 temp_pos = pos;
			vec2 temp_dir = dir*4.0f;
			
			move_point(&temp_pos, &temp_dir, 1.0f, 0);
			pos = temp_pos;
			dir = normalize(temp_dir);
			
			energy -= distance(from, pos) + tuning.laser_bounce_cost;
			bounces++;
			
			if(bounces > tuning.laser_bounce_num)
				energy = -1;
				
			game.create_sound(pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!hit_character(pos, to))
		{
			from = pos;
			pos = to;
			energy = -1;
		}
	}
		
	//dbg_msg("laser", "%d done %f %f %f %f", server_tick(), from.x, from.y, pos.x, pos.y);
}
	
void LASER::reset()
{
	game.world.destroy_entity(this);
}

void LASER::tick()
{
	if(server_tick() > eval_tick+(server_tickspeed()*tuning.laser_bounce_delay)/1000.0f)
	{
		do_bounce();
	}

}

void LASER::snap(int snapping_client)
{
	if(networkclipped(snapping_client))
		return;

	NETOBJ_LASER *obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
	obj->x = (int)pos.x;
	obj->y = (int)pos.y;
	obj->from_x = (int)from.x;
	obj->from_y = (int)from.y;
	obj->start_tick = eval_tick;
}
