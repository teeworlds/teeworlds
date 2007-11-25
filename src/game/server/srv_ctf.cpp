/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
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

	is_teamplay = true;
}

void gameobject_ctf::on_player_spawn(class player *p)
{
}

int gameobject_ctf::on_player_death(class player *victim, class player *killer, int weaponid)
{
	gameobject::on_player_death(victim, killer, weaponid);
	int had_flag = 0;
	
	// drop flags
	for(int fi = 0; fi < 2; fi++)
	{
		flag *f = flags[fi];
		if(f && f->carrying_player == killer)
			had_flag |= 2;
		if(f && f->carrying_player == victim)
		{
			f->carrying_player = 0;
			had_flag |= 1;
		}
	}
	
	return had_flag;
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
					teamscore[fi^1]++;
					for(int i = 0; i < 2; i++)
						flags[i]->reset();
				}
			}			
		}
		else
		{
			player *players[MAX_CLIENTS];
			int types[] = {OBJTYPE_PLAYER_CHARACTER};
			int num = world->find_entities(f->pos, 32.0f, (entity**)players, MAX_CLIENTS, types, 1);
			for(int i = 0; i < num; i++)
			{
				if(players[i]->team == f->team)
				{
					// return the flag
					if(!f->at_stand)
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

// Flag
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

void flag::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	obj_flag *flag = (obj_flag *)snap_new_item(OBJTYPE_FLAG, team, sizeof(obj_flag));
	flag->x = (int)pos.x;
	flag->y = (int)pos.y;
	flag->team = team;
	flag->carried_by = -1;
	
	if(at_stand)
		flag->carried_by = -2;
	else if(carrying_player)
		flag->carried_by = carrying_player->client_id;
}
