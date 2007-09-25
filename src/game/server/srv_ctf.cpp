#include "srv_common.h"
#include "srv_ctf.h"

gameobject_ctf::gameobject_ctf()
{
	// fetch flagstands
	for(int i = 0; i < 2; i++)
	{
		mapres_flagstand *stand;
		stand = (mapres_flagstand *)map_find_item(MAPRES_FLAGSTAND_RED+i, 0);
		if(stand)
		{
			flag *f = new flag(i);
			f->stand_pos = vec2(stand->x, stand->y);
			f->pos = f->stand_pos;
			flags[i] = f;
			//dbg_msg("game", "flag at %f,%f", f->pos.x, f->pos.y);
		}
		else
		{
			// report massive failure
		}
	}
}

void gameobject_ctf::on_player_spawn(class player *p)
{
}

void gameobject_ctf::on_player_death(class player *victim, class player *killer, int weapon)
{
	// drop flags
	for(int fi = 0; fi < 2; fi++)
	{
		flag *f = flags[fi];
		if(f && f->carrying_player == victim)
			f->carrying_player = 0;
	}
}

void gameobject_ctf::tick()
{
	gameobject::tick();
	
	// do flags
	for(int fi = 0; fi < 2; fi++)
	{
		flag *f = flags[fi];
		
		//
		if(f->carrying_player)
		{
			// update flag position
			f->pos = f->carrying_player->pos;
			
			if(flags[fi^1]->at_stand)
			{
				if(distance(f->pos, flags[fi^1]->pos) < 24)
				{
					// CAPTURE! \o/
					for(int i = 0; i < 2; i++)
						flags[i]->reset();
				}
			}			
		}
		else
		{
			player *players[MAX_CLIENTS];
			int types[] = {OBJTYPE_PLAYER};
			int num = world->find_entities(f->pos, 32.0f, (entity**)players, MAX_CLIENTS, types, 1);
			for(int i = 0; i < num; i++)
			{
				if(players[i]->team == f->team)
				{
					// return the flag
					f->reset();
				}
				else
				{
					// take the flag
					f->at_stand = 0;
					f->carrying_player = players[i];
					break;
				}
			}
			
			if(!f->carrying_player)
			{
				f->vel.y += gravity;
				move_box(&f->pos, &f->vel, vec2(f->phys_size, f->phys_size), 0.5f);
			}
		}
	}
}

//////////////////////////////////////////////////
// FLAG
//////////////////////////////////////////////////
flag::flag(int _team)
: entity(OBJTYPE_FLAG)
{
	team = _team;
	proximity_radius = phys_size;
	carrying_player = 0x0;
	
	reset();
	
	// TODO: should this be done here?
	world->insert_entity(this);
}

void flag::reset()
{
	carrying_player = 0;
	at_stand = 1;
	pos = stand_pos;
	spawntick = -1;
}

void flag::tick()
{
}

bool flag::is_grounded()
{
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	return false;
}

void flag::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	obj_flag *flag = (obj_flag *)snap_new_item(OBJTYPE_FLAG, id, sizeof(obj_flag));
	flag->x = (int)pos.x;
	flag->y = (int)pos.y;
	flag->team = team;
	
	if(carrying_player && carrying_player->client_id == snapping_client)
		flag->local_carry = 1;
	else
		flag->local_carry = 0;
}
// FLAG END ///////////////////////
