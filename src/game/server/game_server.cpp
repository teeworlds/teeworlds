#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <engine/config.h>
#include "../version.h"
#include "game_server.h"
#include "srv_common.h"
#include "srv_ctf.h"
#include "srv_tdm.h"
#include "srv_dm.h"

data_container *data = 0x0;

class player* get_player(int index);
void create_damageind(vec2 p, float angle_mod, int amount);
void create_explosion(vec2 p, int owner, int weapon, bool bnodamage);
void create_smoke(vec2 p);
void create_spawn(vec2 p);
void create_death(vec2 p);
void create_sound(vec2 pos, int sound, int loopflags = 0);
void create_targetted_sound(vec2 pos, int sound, int target, int loopflags = 0);
class player *intersect_player(vec2 pos0, vec2 pos1, vec2 &new_pos, class entity *notthis = 0);

game_world *world;

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
event_handler::event_handler()
{
	clear();
}

void *event_handler::create(int type, int size, int target)
{
	if(num_events == MAX_EVENTS)
		return 0;
	if(current_offset+size >= MAX_DATASIZE)
		return 0;

	void *p = &data[current_offset];
	offsets[num_events] = current_offset;
	types[num_events] = type;
	sizes[num_events] = size;
	targets[num_events] = target;
	current_offset += size;
	num_events++;
	return p;
}

void event_handler::clear()
{
	num_events = 0;
	current_offset = 0;
}

void event_handler::snap(int snapping_client)
{
	for(int i = 0; i < num_events; i++)
	{
		if (targets[i] == -1 || targets[i] == snapping_client)
		{
			void *d = snap_new_item(types[i], i, sizes[i]);
			mem_copy(d, &data[offsets[i]], sizes[i]);
		}
	}
}

event_handler events;

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
entity::entity(int objtype)
{
	this->objtype = objtype;
	pos = vec2(0,0);
	flags = FLAG_PHYSICS;
	proximity_radius = 0;
	
	id = snap_new_id();
	
	next_entity = 0;
	prev_entity = 0;
	prev_type_entity = 0;
	next_type_entity = 0;
}

entity::~entity()
{
	snap_free_id(id);
}

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
game_world::game_world()
{
	paused = false;
	reset_requested = false;
	first_entity = 0x0;
	for(int i = 0; i < NUM_ENT_TYPES; i++)
		first_entity_types[i] = 0;
}

int game_world::find_entities(vec2 pos, float radius, entity **ents, int max)
{
	int num = 0;
	for(entity *ent = first_entity; ent; ent = ent->next_entity)
	{
		if(!(ent->flags&entity::FLAG_PHYSICS))
			continue;
			
		if(distance(ent->pos, pos) < radius+ent->proximity_radius)
		{
			ents[num] = ent;
			num++;
			if(num == max)
				break;
		}
	}
	
	return num;
}

int game_world::find_entities(vec2 pos, float radius, entity **ents, int max, const int* types, int maxtypes)
{
	int num = 0;
	for(int t = 0; t < maxtypes; t++)
	{
		for(entity *ent = first_entity_types[types[t]]; ent; ent = ent->next_type_entity)
		{
			if(!(ent->flags&entity::FLAG_PHYSICS))
				continue;
			
			if(distance(ent->pos, pos) < radius+ent->proximity_radius)
			{
				ents[num] = ent;
				num++;
				if(num == max)
					break;
			}
		}
	}
	
	return num;
}

void game_world::insert_entity(entity *ent)
{
	entity *cur = first_entity;
	while(cur)
	{
		dbg_assert(cur != ent, "err");
		cur = cur->next_entity;
	}
	
	// insert it
	if(first_entity)
		first_entity->prev_entity = ent;
	ent->next_entity = first_entity;
	ent->prev_entity = 0x0;
	first_entity = ent;

	// into typelist aswell
	if(first_entity_types[ent->objtype])
		first_entity_types[ent->objtype]->prev_type_entity = ent;
	ent->next_type_entity = first_entity_types[ent->objtype];
	ent->prev_type_entity = 0x0;
	first_entity_types[ent->objtype] = ent;
}

void game_world::destroy_entity(entity *ent)
{
	ent->set_flag(entity::FLAG_DESTROY);
}

void game_world::remove_entity(entity *ent)
{
	// not in the list
	if(!ent->next_entity && !ent->prev_entity && first_entity != ent)
		return;
	
	// remove
	if(ent->prev_entity)
		ent->prev_entity->next_entity = ent->next_entity;
	else
		first_entity = ent->next_entity;
	if(ent->next_entity)
		ent->next_entity->prev_entity = ent->prev_entity;

	if(ent->prev_type_entity)
		ent->prev_type_entity->next_type_entity = ent->next_type_entity;
	else
		first_entity_types[ent->objtype] = ent->next_type_entity;
	if(ent->next_type_entity)
		ent->next_type_entity->prev_type_entity = ent->prev_type_entity;
		
	ent->next_entity = 0;
	ent->prev_entity = 0;
	ent->next_type_entity = 0;
	ent->prev_type_entity = 0;
}

//
void game_world::snap(int snapping_client)
{
	for(entity *ent = first_entity; ent; ent = ent->next_entity)
		ent->snap(snapping_client);
}

void game_world::reset()
{
	// reset all entities
	for(entity *ent = first_entity; ent; ent = ent->next_entity)
		ent->reset();
	remove_entities();
	
	for(entity *ent = first_entity; ent; ent = ent->next_entity)
		ent->post_reset();
	remove_entities();
	
	reset_requested = false;
}

void game_world::remove_entities()
{
	// destroy objects marked for destruction
	entity *ent = first_entity;
	while(ent)
	{
		entity *next = ent->next_entity;
		if(ent->flags&entity::FLAG_DESTROY)
		{
			remove_entity(ent);
			ent->destroy();
		}
		ent = next;
	}
}

void game_world::tick()
{
	if(reset_requested)
		reset();
	
	if(!paused)
	{
		// update all objects
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick();
			
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick_defered();
	}
	
	remove_entities();
}

//////////////////////////////////////////////////
// projectile
//////////////////////////////////////////////////
projectile::projectile(int type, int owner, vec2 pos, vec2 vel, int span, entity* powner,
	int damage, int flags, float force, int sound_impact, int weapon)
: entity(OBJTYPE_PROJECTILE)
{
	this->type = type;
	this->pos = pos;
	this->vel = vel;
	this->lifespan = span;
	this->owner = owner;
	this->powner = powner;
	this->flags = flags;
	this->force = force;
	this->damage = damage;
	this->sound_impact = sound_impact;
	this->weapon = weapon;
	this->bounce = 0;
	world->insert_entity(this);
}

void projectile::reset()
{
	world->destroy_entity(this);
}

void projectile::tick()
{
	vec2 oldpos = pos;
	
	int collide = 0;
	if(bounce)
	{
		int numbounces;
		vel.y += 0.25f;
		move_point(&pos, &vel, 0.25f, &numbounces);
		bounce -= numbounces;
	}
	else
	{
		vel.y += 0.25f;
		pos += vel;
		collide = col_check_point((int)pos.x, (int)pos.y);
	}
	
	lifespan--;
	
	// check player intersection as well
	vec2 new_pos;
	entity *targetplayer = (entity*)intersect_player(oldpos, pos, new_pos, powner);
	
	if(targetplayer || lifespan < 0 || collide || bounce < 0)
	{
		if (lifespan >= 0 || weapon == WEAPON_ROCKET)
			create_sound(pos, sound_impact);
			
		if (flags & PROJECTILE_FLAGS_EXPLODE)
			create_explosion(oldpos, owner, weapon, false);
		else if (targetplayer)
		{
			targetplayer->take_damage(normalize(vel) * max(0.001f, force), damage, owner, weapon);
		}
			
		world->destroy_entity(this);
	}
}

void projectile::snap(int snapping_client)
{
	obj_projectile *proj = (obj_projectile *)snap_new_item(OBJTYPE_PROJECTILE, id, sizeof(obj_projectile));
	proj->x = (int)pos.x;
	proj->y = (int)pos.y;
	proj->vx = (int)vel.x; // TODO: should be an angle
	proj->vy = (int)vel.y;
	proj->type = type;
}

//////////////////////////////////////////////////
// player
//////////////////////////////////////////////////
// TODO: move to separate file
player::player()
: entity(OBJTYPE_PLAYER)
{
	init();
}

void player::init()
{
	proximity_radius = phys_size;
	name[0] = 'n';
	name[1] = 'o';
	name[2] = 'o';
	name[3] = 'b';
	name[4] = 0;
	client_id = -1;
	team = -1; // -1 == spectator
	extrapowerflags = 0;
	ninjaactivationtick = 0;

	latency_accum = 0;
	latency_accum_min = 0;
	latency_accum_max = 0;
	latency_avg = 0;
	latency_min = 0;
	latency_max = 0;
	
	reset();
}

void player::reset()
{
	pos = vec2(0.0f, 0.0f);
	core.vel = vec2(0.0f, 0.0f);
	//direction = vec2(0.0f, 1.0f);
	score = 0;
	dead = true;
	spawning = false;
	die_tick = 0;
	damage_taken = 0;
	state = STATE_UNKNOWN;
	
	mem_zero(&input, sizeof(input));
	mem_zero(&previnput, sizeof(previnput));
	
	last_action = -1;
	
	emote_stop = 0;
	damage_taken_tick = 0;
	attack_tick = 0;
}

void player::destroy() {  }

void player::set_weapon(int w)
{
	last_weapon = active_weapon;
	active_weapon = w;
}

	
void player::respawn()
{
	spawning = true;
}


void player::set_team(int new_team)
{
	team = new_team;
	die(client_id, -1);
	score--;
	
	dbg_msg("game", "cid=%d team=%d", client_id, team);
	
	if(team == -1)
		clear_flag(FLAG_PHYSICS);
	else
		set_flag(FLAG_PHYSICS);
}


bool try_spawntype(int t, vec2 *pos)
{
	// get spawn point
	int start, num;
	map_get_type(t, &start, &num);
	if(!num)
		return false;
		
	mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + (rand()%num), NULL, NULL);
	*pos = vec2((float)sp->x, (float)sp->y);
	return true;
}


void player::try_respawn()
{
	vec2 spawnpos = vec2(100.0f, -60.0f);
	
	// get spawn point
	if(gameobj->gametype == GAMETYPE_CTF)
	{
		// try first try own team spawn, then normal spawn and then enemy
		if(!try_spawntype(MAPRES_SPAWNPOINT_RED+(team&1), &spawnpos))
		{
			if(!try_spawntype(MAPRES_SPAWNPOINT, &spawnpos))
				try_spawntype(MAPRES_SPAWNPOINT_RED+((team+1)&1), &spawnpos);
		}
	}
	else
	{
		if(!try_spawntype(MAPRES_SPAWNPOINT, &spawnpos))
			try_spawntype(MAPRES_SPAWNPOINT_RED+(rand()&1), &spawnpos);
	}
		
	// check if the position is occupado
	entity *ents[2] = {0};
	int types[] = {OBJTYPE_PLAYER};
	int num_ents = world->find_entities(spawnpos, 64, ents, 2, types, 1);
	for(int i = 0; i < num_ents; i++)
	{
		if(ents[i] != this)
			return;
	}
	
	spawning = false;
	pos = spawnpos;
	
	core.pos = pos;
	core.hooked_player = -1;
	

	health = 10;
	armor = 0;
	jumped = 0;
	dead = false;
	set_flag(entity::FLAG_PHYSICS);
	state = STATE_PLAYING;
	
	core.hook_state = HOOK_IDLE;
	
	mem_zero(&input, sizeof(input));
	core.vel = vec2(0.0f, 0.0f);
		
	// init weapons
	mem_zero(&weapons, sizeof(weapons));
	weapons[WEAPON_HAMMER].got = true;
	weapons[WEAPON_HAMMER].ammo = -1;
	weapons[WEAPON_GUN].got = true;
	weapons[WEAPON_GUN].ammo = data->weapons[WEAPON_GUN].maxammo;
	
	active_weapon = WEAPON_GUN;
	last_weapon = WEAPON_HAMMER;
	
	reload_timer = 0;

	// Create sound and spawn effects
	create_sound(pos, SOUND_PLAYER_SPAWN);
	create_spawn(pos);
	
	gameobj->on_player_spawn(this);
}

bool player::is_grounded()
{
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	return false;
}

int player::handle_ninja()
{
	vec2 direction = normalize(vec2(input.target_x, input.target_y));
	
	if ((server_tick() - ninjaactivationtick) > (data->weapons[WEAPON_NINJA].duration * server_tickspeed() / 1000))
	{
		// time's up, return
		active_weapon = last_weapon;
		return 0;
	}
	
	// Check if it should activate
	if ((input.fire && !(previnput.fire)) && (server_tick() > currentcooldown))
	{
		// ok then, activate ninja
		attack_tick = server_tick();
		activationdir = direction;
		currentmovetime = data->weapons[WEAPON_NINJA].movetime * server_tickspeed() / 1000;
		currentcooldown = data->weapons[WEAPON_NINJA].firedelay * server_tickspeed() / 1000 + server_tick();
		// reset hit objects
		numobjectshit = 0;

		create_sound(pos, SOUND_NINJA_FIRE);
		
		// release all hooks when ninja is activated
		//release_hooked();
		//release_hooks();
	}

	currentmovetime--;
	
	if (currentmovetime == 0)
	{	
		// reset player velocity
		core.vel *= 0.2f;
		//return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
	}
	
	if (currentmovetime > 0)
	{
		// Set player velocity
		core.vel = activationdir * data->weapons[WEAPON_NINJA].velocity;
		vec2 oldpos = pos;
		move_box(&core.pos, &core.vel, vec2(phys_size, phys_size), 0.0f);
		// reset velocity so the client doesn't predict stuff
		core.vel = vec2(0.0f,0.0f);
		if ((currentmovetime % 2) == 0)
		{
			create_smoke(pos);
		}
		
		// check if we hit anything along the way
		{
			int type = OBJTYPE_PLAYER;
			entity *ents[64];
			vec2 dir = pos - oldpos;
			float radius = phys_size * 2.0f; //length(dir * 0.5f);
			vec2 center = oldpos + dir * 0.5f;
			int num = world->find_entities(center, radius, ents, 64, &type, 1);
			
			for (int i = 0; i < num; i++)
			{
				// Check if entity is a player
				if (ents[i] == this)
					continue;
				// make sure we haven't hit this object before
				bool balreadyhit = false;
				for (int j = 0; j < numobjectshit; j++)
				{
					if (hitobjects[j] == ents[i])
						balreadyhit = true;
				}
				if (balreadyhit)
					continue;

				// check so we are sufficiently close
				if (distance(ents[i]->pos, pos) > (phys_size * 2.0f))
					continue;
			
				// hit a player, give him damage and stuffs...
				create_sound(ents[i]->pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				hitobjects[numobjectshit++] = ents[i];
				ents[i]->take_damage(vec2(0,10.0f), data->weapons[WEAPON_NINJA].meleedamage, client_id,WEAPON_NINJA);
			}
		}
		return MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION | MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY;
	}
	return 0;
}

int player::handle_weapons()
{
	vec2 direction = normalize(vec2(input.target_x, input.target_y));
	
	if(config.stress)
	{
		for(int i = 0; i < NUM_WEAPONS; i++)
		{
			weapons[i].got = true;
			weapons[i].ammo = 10;
		}
		
		if(reload_timer) // twice as fast reload
			reload_timer--;
	}
	
	// check reload timer
	if(reload_timer)
	{
		reload_timer--;
		return 0;
	}
	if (active_weapon == WEAPON_NINJA)
	{
		// don't update other weapons while ninja is active
		return handle_ninja();
	}

	// switch weapon if wanted		
	if(input.activeweapon && data->weapons[active_weapon].duration <= 0)
	{
		int new_weapon = active_weapon;
		if(input.activeweapon > 0) // straight selection
			new_weapon = input.activeweapon-1;
		else if(input.activeweapon == -1 && !previnput.activeweapon) // next weapon
		{
			do
				new_weapon = (new_weapon+1)%NUM_WEAPONS;
			while(!weapons[new_weapon].got);
		}
		else if(input.activeweapon == -2 && !previnput.activeweapon)
		{
			do
				new_weapon = (new_weapon-1)<0?NUM_WEAPONS-1:new_weapon-1;
			while(!weapons[new_weapon].got);
		}
			
		if(new_weapon >= 0 && new_weapon < NUM_WEAPONS && weapons[new_weapon].got)
		{
			if(active_weapon != new_weapon)
				create_sound(pos, SOUND_WEAPON_SWITCH);

			last_weapon = active_weapon;
			active_weapon = new_weapon;
		}
	}
	
	if(!previnput.fire && input.fire)
	{
		if(reload_timer == 0)
		{
			// fire!
			if(weapons[active_weapon].ammo)
			{
				switch(active_weapon)
				{
					case WEAPON_HAMMER:
						// reset objects hit
						numobjectshit = 0;
						create_sound(pos, SOUND_HAMMER_FIRE);
						break;

					case WEAPON_GUN:
						new projectile(WEAPON_GUN,
							client_id,
							pos+vec2(0,0),
							direction*30.0f,
							100,
							this,
							1, 0, 0, -1, WEAPON_GUN);
						create_sound(pos, SOUND_GUN_FIRE);
						break;
					case WEAPON_ROCKET:
					{
						new projectile(WEAPON_ROCKET,
							client_id,
							pos+vec2(0,0),
							direction*15.0f,
							100,
							this,
							1, projectile::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_ROCKET_EXPLODE, WEAPON_ROCKET);
						create_sound(pos, SOUND_ROCKET_FIRE);
						break;
					}
					case WEAPON_SHOTGUN:
					{
						int shotspread = 2;
						for(int i = -shotspread; i <= shotspread; i++)
						{
							float a = get_angle(direction);
							a += i*0.08f;
							new projectile(WEAPON_SHOTGUN,
								client_id,
								pos+vec2(0,0),
								vec2(cosf(a), sinf(a))*25.0f,
								//vec2(cosf(a), sinf(a))*20.0f,
								(int)(server_tickspeed()*0.4f),
								this,
								1, 0, 0, -1, WEAPON_SHOTGUN);
						}
						create_sound(pos, SOUND_SHOTGUN_FIRE);
						break;
					}	
				}
				
				weapons[active_weapon].ammo--;
				attack_tick = server_tick();
				reload_timer = data->weapons[active_weapon].firedelay * server_tickspeed() / 1000;
			}
			else
			{
				create_sound(pos, SOUND_WEAPON_NOAMMO);
			}
		}
	}
	// Update weapons
	if (active_weapon == WEAPON_HAMMER && reload_timer > 0)
	{
		// Handle collisions
		// only one that needs update (for now)
		// do selection for the weapon and bash anything in it
		// check if we hit anything along the way
		int type = OBJTYPE_PLAYER;
		entity *ents[64];
		vec2 lookdir(direction.x > 0.0f ? 1.0f : -1.0f, 0.0f);
		vec2 dir = lookdir * data->weapons[active_weapon].meleereach;
		float radius = length(dir * 0.5f);
		vec2 center = pos + dir * 0.5f;
		int num = world->find_entities(center, radius, ents, 64, &type, 1);
		
		for (int i = 0; i < num; i++)
		{
			// Check if entity is a player
			if (ents[i] == this)
				continue;
			// make sure we haven't hit this object before
			bool balreadyhit = false;
			for (int j = 0; j < numobjectshit; j++)
			{
				if (hitobjects[j] == ents[i])
					balreadyhit = true;
			}
			if (balreadyhit)
				continue;

			// check so we are sufficiently close
			if (distance(ents[i]->pos, pos) > (phys_size * 2.0f))
				continue;
		
			// hit a player, give him damage and stuffs...
			// create sound for bash
			//create_sound(ents[i]->pos, sound_impact);
			vec2 fdir = normalize(ents[i]->pos- pos);

			// set his velocity to fast upward (for now)
			create_smoke(ents[i]->pos);
			create_sound(pos, SOUND_HAMMER_HIT);
			hitobjects[numobjectshit++] = ents[i];
			ents[i]->take_damage(vec2(0,-1.0f), data->weapons[active_weapon].meleedamage, client_id, active_weapon);
			player* target = (player*)ents[i];
			vec2 dir;
			if (length(target->pos - pos) > 0.0f)
				dir = normalize(target->pos - pos);
			else
				dir = vec2(0,-1);
			target->core.vel += dir * 25.0f + vec2(0,-5.0f);
		}
	}
	if (data->weapons[active_weapon].ammoregentime)
	{
		// If equipped and not active, regen ammo?
		if (reload_timer <= 0)
		{
			if (weapons[active_weapon].ammoregenstart < 0)
				weapons[active_weapon].ammoregenstart = server_tick();

			if ((server_tick() - weapons[active_weapon].ammoregenstart) >= data->weapons[active_weapon].ammoregentime * server_tickspeed() / 1000)
			{
				// Add some ammo
				weapons[active_weapon].ammo = min(weapons[active_weapon].ammo + 1, data->weapons[active_weapon].maxammo);
				weapons[active_weapon].ammoregenstart = -1;
			}
		}
		else
		{
			weapons[active_weapon].ammoregenstart = -1;
		}
	}
	return 0;
}

void player::tick()
{
	// do latency stuff
	{
		CLIENT_INFO info;
		if(server_getclientinfo(client_id, &info))
		{
			latency_accum += info.latency;
			latency_accum_max = max(latency_accum_max, info.latency);
			latency_accum_min = min(latency_accum_min, info.latency);
		}
			
		if(server_tick()%server_tickspeed() == 0)
		{
			latency_avg = latency_accum/server_tickspeed();
			latency_max = latency_accum_max;
			latency_min = latency_accum_min;
			latency_accum = 0;
			latency_accum_min = 1000;
			latency_accum_max = 0;
		}
	}
	
	if(team == -1)
	{
		// spectator
		return;
	}
	
	
	if(spawning)
		try_respawn();

	// TODO: rework the input to be more robust
	if(dead)
	{
		if(server_tick()-die_tick >= server_tickspeed()*5) // auto respawn after 3 sec
			respawn();
		if(input.fire && server_tick()-die_tick >= server_tickspeed()/2) // auto respawn after 0.5 sec
			respawn();
		return;
	}
	
	//player_core core;
	//core.pos = pos;
	//core.jumped = jumped;
	core.input = input;
	core.tick();
	
	
	// handle weapons
	handle_weapons();
	/*
	if (!(retflags & (MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION)))
	{
		// add gravity
		//if (!(retflags & MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY))
			//vel.y += gravity;
	
		// do the move
		defered_pos = pos;
		move_box(&core.pos, &vel, vec2(phys_size, phys_size), 0);
	}*/

	//defered_pos = core.pos;
	//jumped = core.jumped;
	
	state = input.state;
	
	// Previnput
	previnput = input;
	return;
}

void player::tick_defered()
{
	core.move();
	core.quantize();
	pos = core.pos;
	
	// apply the new position
	//pos = defered_pos;
}

void player::die(int killer, int weapon)
{
	gameobj->on_player_death(this, get_player(killer), weapon);
	
	dbg_msg("game", "kill killer='%d:%s' victim='%d:%s' weapon=%d", killer, players[killer].name, client_id, name, weapon);
	
	// send the kill message
	msg_pack_start(MSG_KILLMSG, MSGFLAG_VITAL);
	msg_pack_int(killer);
	msg_pack_int(client_id);
	msg_pack_int(weapon);
	msg_pack_end();
	server_send_msg(-1);
	
	// a nice sound
	create_sound(pos, SOUND_PLAYER_DIE);
	
	// set dead state
	dead = true;
	die_tick = server_tick();
	clear_flag(FLAG_PHYSICS);
	create_death(pos);
}

bool player::take_damage(vec2 force, int dmg, int from, int weapon)
{
	core.vel += force;

	// player only inflicts half damage on self	
	if(from == client_id)
		dmg = max(1, dmg/2);

	// CTF and TDM (TODO: check for FF)
	//if (gameobj->gametype != GAMETYPE_DM && from >= 0 && players[from].team == team)
		//return false;

	damage_taken++;

	// create healthmod indicator
	if(server_tick() < damage_taken_tick+25)
	{
		// make sure that the damage indicators doesn't group together
		create_damageind(pos, damage_taken*0.25f, dmg);
	}
	else
	{
		damage_taken = 0;
		create_damageind(pos, 0, dmg);
	}

	if(armor)
	{
		armor -= 1;
		dmg--;
	}
	
	if(dmg > armor)
	{
		dmg -= armor;
		armor = 0;
		health -= dmg;
	}
	else
		armor -= dmg;
	
	damage_taken_tick = server_tick();
	
	// do damage hit sound
	if(from >= 0)
		create_targetted_sound(get_player(from)->pos, SOUND_HIT, from);
			
	// check for death
	if(health <= 0)
	{
		die(from, weapon);

		// set attacker's face to happy (taunt!)
		if (from >= 0 && from != client_id)
		{
			player *p = get_player(from);

			p->emote_type = EMOTE_HAPPY;
			p->emote_stop = server_tick() + server_tickspeed(); 
		}
	
		return false;
	}

	if (dmg > 2)
		create_sound(pos, SOUND_PLAYER_PAIN_LONG);
	else
		create_sound(pos, SOUND_PLAYER_PAIN_SHORT);

	emote_type = EMOTE_PAIN;
	emote_stop = server_tick() + 500 * server_tickspeed() / 1000;

	// spawn blood?
	return true;
}

void player::snap(int snaping_client)
{
	obj_player *player = (obj_player *)snap_new_item(OBJTYPE_PLAYER, client_id, sizeof(obj_player));

	core.write(player);
	
	if(snaping_client != client_id)
	{
		player->vx = 0; // make sure that we don't send these to clients who don't need them
		player->vy = 0;
		player->hook_dx = 0;
		player->hook_dy = 0;
	}

	if (emote_stop < server_tick())
	{
		emote_type = EMOTE_NORMAL;
		emote_stop = -1;
	}

	player->emote = emote_type;

	player->latency = latency_avg;
	player->latency_flux = latency_max-latency_min;

	player->ammocount = weapons[active_weapon].ammo;
	player->health = 0;
	player->armor = 0;
	player->local = 0;
	player->clientid = client_id;
	player->weapon = active_weapon;
	player->attacktick = attack_tick;
	
	if(client_id == snaping_client)
	{
		player->local = 1;
		player->health = health;
		player->armor = armor;
	}
	
	if(dead)
		player->health = -1;
	
	//if(length(vel) > 15.0f)
	//	player->emote = EMOTE_HAPPY;
	
	//if(damage_taken_tick+50 > server_tick())
	//	player->emote = EMOTE_PAIN;
	
	if (player->emote == EMOTE_NORMAL)
	{
		if(250 - ((server_tick() - last_action)%(250)) < 5)
			player->emote = EMOTE_BLINK;
	}
	
	player->score = score;
	player->team = team;

	player->state = state;
}

player *players;

//////////////////////////////////////////////////
// powerup
//////////////////////////////////////////////////
powerup::powerup(int _type, int _subtype)
: entity(OBJTYPE_POWERUP)
{
	type = _type;
	subtype = _subtype;
	proximity_radius = phys_size;
	
	reset();
	
	// TODO: should this be done here?
	world->insert_entity(this);
}

void powerup::reset()
{
	if (data->powerupinfo[type].startspawntime > 0)
		spawntick = server_tick() + server_tickspeed() * data->powerupinfo[type].startspawntime;
	else
		spawntick = -1;
}

void powerup::tick()
{
	// wait for respawn
	if(spawntick > 0)
	{
		if(server_tick() > spawntick)
		{
			// respawn
			spawntick = -1;
			
			if(type == POWERUP_WEAPON)
				create_sound(pos, SOUND_WEAPON_SPAWN, 0);
		}
		else
			return;
	}
	// Check if a player intersected us
	vec2 meh;
	player* pplayer = intersect_player(pos, pos + vec2(0,16), meh, 0);
	if (pplayer)
	{
		// player picked us up, is someone was hooking us, let them go
		int respawntime = -1;
		switch (type)
		{
		case POWERUP_HEALTH:
			if(pplayer->health < 10)
			{
				create_sound(pos, SOUND_PICKUP_HEALTH, 0);
				pplayer->health = min(10, pplayer->health + data->powerupinfo[type].amount);
				respawntime = data->powerupinfo[type].respawntime;
			}
			break;
		case POWERUP_ARMOR:
			if(pplayer->armor < 10)
			{
				create_sound(pos, SOUND_PICKUP_ARMOR, 0);
				pplayer->armor = min(10, pplayer->armor + data->powerupinfo[type].amount);
				respawntime = data->powerupinfo[type].respawntime;
			}
			break;
				
		case POWERUP_WEAPON:
			if(subtype >= 0 && subtype < NUM_WEAPONS)
			{
				if(pplayer->weapons[subtype].ammo < 10 || !pplayer->weapons[subtype].got)
				{
					pplayer->weapons[subtype].got = true;
					pplayer->weapons[subtype].ammo = min(10, pplayer->weapons[subtype].ammo + data->powerupinfo[type].amount);
					respawntime = data->powerupinfo[type].respawntime;
					
					// TODO: data compiler should take care of stuff like this
					if(subtype == WEAPON_ROCKET)
						create_sound(pos, SOUND_PICKUP_ROCKET);
					else if(subtype == WEAPON_SHOTGUN)
						create_sound(pos, SOUND_PICKUP_SHOTGUN);
				}
			}
			break;
		case POWERUP_NINJA:
			{
				// activate ninja on target player
				pplayer->ninjaactivationtick = server_tick();
				pplayer->weapons[WEAPON_NINJA].got = true;
				pplayer->last_weapon = pplayer->active_weapon;
				pplayer->active_weapon = WEAPON_NINJA;
				respawntime = data->powerupinfo[type].respawntime;
				create_sound(pos, SOUND_PICKUP_NINJA);

				// loop through all players, setting their emotes
				entity *ents[64];
				const int types[] = {OBJTYPE_PLAYER};
				int num = world->find_entities(vec2(0, 0), 1000000, ents, 64, types, 1);
				for (int i = 0; i < num; i++)
				{
					player *p = (player *)ents[i];
					if (p != pplayer)
					{
						p->emote_type = EMOTE_SURPRISE;
						p->emote_stop = server_tick() + server_tickspeed();
					}
				}

				pplayer->emote_type = EMOTE_ANGRY;
				pplayer->emote_stop = server_tick() + 1200 * server_tickspeed() / 1000;

				break;
			}
		default:
			break;
		};
		
		if(respawntime >= 0)
		{
			dbg_msg("game", "pickup player='%d:%s' item=%d/%d", pplayer->client_id, pplayer->name, type, subtype);
			spawntick = server_tick() + server_tickspeed() * respawntime;
		}
	}
}

void powerup::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	obj_powerup *up = (obj_powerup *)snap_new_item(OBJTYPE_POWERUP, id, sizeof(obj_powerup));
	up->x = (int)pos.x;
	up->y = (int)pos.y;
	up->type = type; // TODO: two diffrent types? what gives?
	up->subtype = subtype;
}

// POWERUP END ///////////////////////

player *get_player(int index)
{
	return &players[index];
}

void create_damageind(vec2 p, float angle, int amount)
{
	float a = 3 * 3.14159f / 2 + angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(amount+2));
		ev_damageind *ev = (ev_damageind *)events.create(EVENT_DAMAGEINDICATION, sizeof(ev_damageind));
		if(ev)
		{
			ev->x = (int)p.x;
			ev->y = (int)p.y;
			ev->angle = (int)(f*256.0f);
		}
	}
}

void create_explosion(vec2 p, int owner, int weapon, bool bnodamage)
{
	// create the event
	ev_explosion *ev = (ev_explosion *)events.create(EVENT_EXPLOSION, sizeof(ev_explosion));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
	
	if (!bnodamage)
	{
		// deal damage
		entity *ents[64];
		const float radius = 128.0f;
		const float innerradius = 42.0f;
		int num = world->find_entities(p, radius, ents, 64);
		for(int i = 0; i < num; i++)
		{
			vec2 diff = ents[i]->pos - p;
			vec2 forcedir(0,1);
			float l = length(diff);
			if(l)
				forcedir = normalize(diff);
			l = 1-clamp((l-innerradius)/(radius-innerradius), 0.0f, 1.0f);
			float dmg = 6 * l;
			if((int)dmg)
				ents[i]->take_damage(forcedir*dmg*2, (int)dmg, owner, weapon);
		}
	}
}

void create_smoke(vec2 p)
{
	// create the event
	ev_explosion *ev = (ev_explosion *)events.create(EVENT_SMOKE, sizeof(ev_explosion));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}

void create_spawn(vec2 p)
{
	// create the event
	ev_spawn *ev = (ev_spawn *)events.create(EVENT_SPAWN, sizeof(ev_spawn));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}

void create_death(vec2 p)
{
	// create the event
	ev_death *ev = (ev_death *)events.create(EVENT_DEATH, sizeof(ev_death));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}

void create_targetted_sound(vec2 pos, int sound, int target, int loopingflags)
{
	if (sound < 0)
		return;

	// create a sound
	ev_sound *ev = (ev_sound *)events.create(EVENT_SOUND, sizeof(ev_sound), target);
	if(ev)
	{
		ev->x = (int)pos.x;
		ev->y = (int)pos.y;
		ev->sound = sound | loopingflags;
	}
}

void create_sound(vec2 pos, int sound, int loopingflags)
{
	create_targetted_sound(pos, sound, -1, loopingflags);
}

// TODO: should be more general
player* intersect_player(vec2 pos0, vec2 pos1, vec2& new_pos, entity* notthis)
{
	// Find other players
	entity *ents[64];
	vec2 dir = pos1 - pos0;
	float radius = length(dir * 0.5f);
	vec2 center = pos0 + dir * 0.5f;
	const int types[] = {OBJTYPE_PLAYER};
	int num = world->find_entities(center, radius, ents, 64, types, 1);
	for (int i = 0; i < num; i++)
	{
		// Check if entity is a player
		if (ents[i] != notthis)
		{
			new_pos = ents[i]->pos;
			return (player*)ents[i];
		}
	}

	return 0;
}

// Server hooks
void mods_tick()
{
	// clear all events
	events.clear();
	world->tick();
	
	if(world->paused) // make sure that the game object always updates
		gameobj->tick();
	
	if(config.restart)
	{
		if(config.restart > 1)
			gameobj->do_warmup(config.restart);
		else
			gameobj->startround();
			
		config.restart = 0;
	}
}

void mods_snap(int client_id)
{
	world->snap(client_id);
	events.snap(client_id);
}

void mods_client_input(int client_id, void *input)
{
	if(!world->paused)
	{
		if (memcmp(&players[client_id].input, input, sizeof(player_input)) != 0)
			players[client_id].last_action = server_tick();

		//players[client_id].previnput = players[client_id].input;
		players[client_id].input = *(player_input*)input;
	}
}

void send_chat(int cid, int team, const char *msg)
{
	if(cid >= 0 && cid < MAX_CLIENTS)
		dbg_msg("chat", "%d:%d:%s: %s", cid, team, players[cid].name, msg);
	else
		dbg_msg("chat", "*** %s", msg);
	
	if(team == -1)
	{
		msg_pack_start(MSG_CHAT, MSGFLAG_VITAL);
		msg_pack_int(cid);
		msg_pack_int(0);
		msg_pack_string(msg, 512);
		msg_pack_end();
		server_send_msg(-1);
	}
	else
	{
		msg_pack_start(MSG_CHAT, MSGFLAG_VITAL);
		msg_pack_int(cid);
		msg_pack_int(1);
		msg_pack_string(msg, 512);
		msg_pack_end();
				
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1 && players[i].team == team)
				server_send_msg(i);
		}
	}
}

void send_set_name(int cid, const char *old_name, const char *new_name)
{
	msg_pack_start(MSG_SETNAME, MSGFLAG_VITAL);
	msg_pack_int(cid);
	msg_pack_string(new_name, 64);
	msg_pack_end();
	server_send_msg(-1);

	char msg[256];
	sprintf(msg, "*** %s changed name to %s", old_name, new_name);
	send_chat(-1, -1, msg);
}

void send_emoticon(int cid, int emoticon)
{
	msg_pack_start(MSG_EMOTICON, MSGFLAG_VITAL);
	msg_pack_int(cid);
	msg_pack_int(emoticon % 16);
	msg_pack_end();
	server_send_msg(-1);
}

void mods_client_enter(int client_id)
{
	players[client_id].init();
	players[client_id].client_id = client_id;
	world->insert_entity(&players[client_id]);
	players[client_id].respawn();
	
	CLIENT_INFO info; // fetch login name
	if(server_getclientinfo(client_id, &info))
	{
		strcpy(players[client_id].name, info.name);
	}
	else
		strcpy(players[client_id].name, "(bot)");


	dbg_msg("game", "join player='%d:%s'", client_id, players[client_id].name);

	// Check which team the player should be on
	if(gameobj->gametype == GAMETYPE_DM)
		players[client_id].team = 0;
	else
		players[client_id].team = gameobj->getteam(client_id);

	//	
	msg_pack_start(MSG_SETNAME, MSGFLAG_VITAL);
	msg_pack_int(client_id);
	msg_pack_string(players[client_id].name, 64);
	msg_pack_end();
	server_send_msg(-1);
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[client_id].client_id != -1)
		{
			msg_pack_start(MSG_SETNAME, MSGFLAG_VITAL);
			msg_pack_int(i);
			msg_pack_string(players[i].name, 64);
			msg_pack_end();
			server_send_msg(client_id);
		}
	}

	char buf[512];
	sprintf(buf, "%s has joined the game", players[client_id].name);
	send_chat(-1, -1, buf);
}

void mods_client_drop(int client_id)
{
	char buf[512];
	sprintf(buf, "%s has left the game", players[client_id].name);
	send_chat(-1, -1, buf);

	dbg_msg("game", "leave player='%d:%s'", client_id, players[client_id].name);
	
	gameobj->on_player_death(&players[client_id], 0, -1);
	world->remove_entity(&players[client_id]);
	players[client_id].client_id = -1;
}

void mods_message(int msg, int client_id)
{
	if(msg == MSG_SAY)
	{
		int team = msg_unpack_int();
		const char *text = msg_unpack_string();
		if(team)
			team = players[client_id].team;
		else
			team = -1;
		send_chat(client_id, team, text);
	}
	else if (msg == MSG_SETTEAM)
	{
		// Switch team on given client and kill/respawn him
		players[client_id].set_team(msg_unpack_int());
	}
	else if (msg == MSG_CHANGENAME)
	{
		const char *name = msg_unpack_string();

		// check for invalid chars
		const char *p = name;
		while (*p)
			if (*p++ < 32)
				return;

		send_set_name(client_id, players[client_id].name, name);
		strcpy(players[client_id].name, name);
	}
	else if (msg == MSG_EMOTICON)
	{
		int emoteicon = msg_unpack_int();
		send_emoticon(client_id, emoteicon % 16);
	}
}

extern unsigned char internal_data[];

void mods_init()
{
	if(!data) /* only load once */
		data = load_data_from_memory(internal_data);
		
	col_init(32);

	world = new game_world;
	players = new player[MAX_CLIENTS];
	
	// select gametype
	if(strcmp(config.gametype, "ctf") == 0)
		gameobj = new gameobject_ctf;
	else if(strcmp(config.gametype, "tdm") == 0)
		gameobj = new gameobject_tdm;
	else
		gameobj = new gameobject_dm;
	
	// setup core world	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		players[i].core.world = &world->core;
		world->core.players[i] = &players[i].core;
	}

	//
	int start, num;
	map_get_type(MAPRES_ITEM, &start, &num);
	
	// TODO: this is way more complicated then it should be
	for(int i = 0; i < num; i++)
	{
		mapres_item *it = (mapres_item *)map_get_item(start+i, 0, 0);
		
		int type = -1;
		int subtype = 0;
		
		switch(it->type)
		{
		case ITEM_WEAPON_GUN:
			type = POWERUP_WEAPON;
			subtype = WEAPON_GUN;
			break;
		case ITEM_WEAPON_SHOTGUN:
			type = POWERUP_WEAPON;
			subtype = WEAPON_SHOTGUN;
			break;
		case ITEM_WEAPON_ROCKET:
			type = POWERUP_WEAPON;
			subtype = WEAPON_ROCKET;
			break;
		case ITEM_WEAPON_HAMMER:
			type = POWERUP_WEAPON;
			subtype = WEAPON_HAMMER;
			break;
		
		case ITEM_HEALTH:
			type = POWERUP_HEALTH;
			break;
		
		case ITEM_ARMOR:
			type = POWERUP_ARMOR;
			break;

		case ITEM_NINJA:
			type = POWERUP_NINJA;
			subtype = WEAPON_NINJA;
			break;
		};
		
		if(type != -1)
		{
			 // LOL, the only new in the entire game code
			 // perhaps we can get rid of it. seems like a stupid thing to have
			powerup *ppower = new powerup(type, subtype);
			ppower->pos = vec2(it->x, it->y);
		}
	}
	
	if(gameobj->gametype == GAMETYPE_CTF)
	{
	}
	
	world->insert_entity(gameobj);
	

	if(config.dbg_bots)
	{
		/*
		static int count = 0;
		if(count >= 0)
		{
			count++;
			if(count == 10)
			{*/
				for(int i = 0; i < config.dbg_bots ; i++)
				{
					mods_client_enter(MAX_CLIENTS-i-1);
					strcpy(players[MAX_CLIENTS-i-1].name, "(bot)");
					if(gameobj->gametype != GAMETYPE_DM)
						players[MAX_CLIENTS-i-1].team = i&1;
				}
				/*
				count = -1;
			}
		}*/
	}	
}

void mods_shutdown()
{
	delete [] players;
	delete gameobj;
	delete world;
	gameobj = 0;
	players = 0;
	world = 0;
}
	
void mods_presnap() {}
void mods_postsnap() {}

extern "C" const char *mods_net_version() { return TEEWARS_NETVERSION; }
