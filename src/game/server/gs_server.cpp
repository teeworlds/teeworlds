/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <engine/e_config.h>
#include "../g_version.h"
#include "gs_common.h"
#include "gs_game_ctf.h"
#include "gs_game_tdm.h"
#include "gs_game_dm.h"

data_container *data = 0x0;

class player* get_player(int index);
void create_damageind(vec2 p, float angle_mod, int amount);
void create_explosion(vec2 p, int owner, int weapon, bool bnodamage);
void create_smoke(vec2 p);
void create_spawn(vec2 p);
void create_death(vec2 p);
void create_sound(vec2 pos, int sound, int mask=-1);
class player *intersect_player(vec2 pos0, vec2 pos1, vec2 &new_pos, class entity *notthis = 0);

game_world *world;

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
event_handler::event_handler()
{
	clear();
}

void *event_handler::create(int type, int size, int mask)
{
	if(num_events == MAX_EVENTS)
		return 0;
	if(current_offset+size >= MAX_DATASIZE)
		return 0;

	void *p = &data[current_offset];
	offsets[num_events] = current_offset;
	types[num_events] = type;
	sizes[num_events] = size;
	client_masks[num_events] = mask;
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
		if(cmask_is_set(client_masks[i], snapping_client))
		{
			ev_common *ev = (ev_common *)&data[offsets[i]];
			if(distance(players[snapping_client].pos, vec2(ev->x, ev->y)) < 1500.0f)
			{
				void *d = snap_new_item(types[i], i, sizes[i]);
				mem_copy(d, &data[offsets[i]], sizes[i]);
			}
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
		static PERFORMACE_INFO scopes[OBJTYPE_FLAG+1] =
		{
			{"null", 0},
			{"game", 0},
			{"player_info", 0},
			{"player_character", 0},
			{"projectile", 0},
			{"powerup", 0},
			{"flag", 0}
		};

		static PERFORMACE_INFO scopes_def[OBJTYPE_FLAG+1] =
		{
			{"null", 0},
			{"game", 0},
			{"player_info", 0},
			{"player_character", 0},
			{"projectile", 0},
			{"powerup", 0},
			{"flag", 0}
		};
				
		static PERFORMACE_INFO tick_scope = {"tick", 0};
		perf_start(&tick_scope);
		
		// update all objects
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
		{
			if(ent->objtype >= 0 && ent->objtype < OBJTYPE_FLAG)
				perf_start(&scopes[ent->objtype]);
			ent->tick();
			if(ent->objtype >= 0 && ent->objtype < OBJTYPE_FLAG)
				perf_end();
		}
		
		perf_end();

		static PERFORMACE_INFO deftick_scope = {"tick_defered", 0};
		perf_start(&deftick_scope);
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
		{
			if(ent->objtype >= 0 && ent->objtype < OBJTYPE_FLAG)
				perf_start(&scopes_def[ent->objtype]);
			ent->tick_defered();
			if(ent->objtype >= 0 && ent->objtype < OBJTYPE_FLAG)
				perf_end();
		}
		perf_end();
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
	this->vel = vel * SERVER_TICK_SPEED; // TODO: remove this
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
	world->insert_entity(this);
}

void projectile::reset()
{
	world->destroy_entity(this);
}

void projectile::tick()
{
	float gravity = -400;
	if(type != WEAPON_ROCKET)
		gravity = -100;
	
	float pt = (server_tick()-start_tick-1)/(float)SERVER_TICK_SPEED;
	float ct = (server_tick()-start_tick)/(float)SERVER_TICK_SPEED;
	vec2 prevpos = calc_pos(pos, vel, gravity, pt);
	vec2 curpos = calc_pos(pos, vel, gravity, ct);

	lifespan--;
	
	int collide = col_check_point((int)curpos.x, (int)curpos.y);
	
	vec2 new_pos;
	entity *targetplayer = (entity*)intersect_player(prevpos, curpos, new_pos, powner);
	
	if(targetplayer || collide || lifespan < 0 )
	{
		if (lifespan >= 0 || weapon == WEAPON_ROCKET)
			create_sound(pos, sound_impact);

		if (flags & PROJECTILE_FLAGS_EXPLODE)
			create_explosion(prevpos, owner, weapon, false);
		else if (targetplayer)
		{
			targetplayer->take_damage(normalize(vel) * max(0.001f, force), damage, owner, weapon);
		}

		world->destroy_entity(this);
	}
}

void projectile::snap(int snapping_client)
{
	float ct = (server_tick()-start_tick)/(float)SERVER_TICK_SPEED;
	vec2 curpos = calc_pos(pos, vel, -7.5f*SERVER_TICK_SPEED, ct);

	if(distance(players[snapping_client].pos, curpos) > 1000.0f)
		return;

	obj_projectile *proj = (obj_projectile *)snap_new_item(OBJTYPE_PROJECTILE, id, sizeof(obj_projectile));
	proj->x = (int)pos.x;
	proj->y = (int)pos.y;
	proj->vx = (int)vel.x;
	proj->vy = (int)vel.y;
	proj->start_tick = start_tick;
	proj->type = type;
}

//////////////////////////////////////////////////
// player
//////////////////////////////////////////////////
// TODO: move to separate file
player::player()
: entity(OBJTYPE_PLAYER_CHARACTER)
{
	init();
}

void player::init()
{
	proximity_radius = phys_size;
	client_id = -1;
	team = -1; // -1 == spectator
	extrapowerflags = 0;

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
	clear_flag(entity::FLAG_PHYSICS);
	spawning = false;
	die_tick = 0;
	damage_taken = 0;
	state = STATE_UNKNOWN;

	mem_zero(&input, sizeof(input));
	mem_zero(&previnput, sizeof(previnput));
	num_inputs = 0;

	last_action = -1;

	emote_stop = 0;
	damage_taken_tick = 0;
	attack_tick = 0;
	numobjectshit = 0;
	ninja_activationtick = 0;
	sniper_chargetick = -1;
	currentmovetime = 0;
	
	active_weapon = WEAPON_GUN;
	last_weapon = WEAPON_HAMMER;
	wanted_weapon = WEAPON_GUN;
}

void player::destroy() {  }

void player::set_weapon(int w)
{
	last_weapon = active_weapon;
	active_weapon = w;
	if(active_weapon < 0 || active_weapon >= NUM_WEAPONS)
		active_weapon = 0;
}

void player::respawn()
{
	spawning = true;
}


void player::set_team(int new_team)
{
	team = new_team;
	die(client_id, -1);

	dbg_msg("game", "cid=%d team=%d", client_id, team);

	if(team == -1)
		clear_flag(FLAG_PHYSICS);
	else
		set_flag(FLAG_PHYSICS);
}


bool try_spawntype(int t, vec2 *outpos)
{
	// get spawn point
	int start, num;
	map_get_type(t, &start, &num);
	if(!num)
		return false;

	int id = rand()%num;
	mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + id, NULL, NULL);
	*outpos = vec2((float)sp->x, (float)sp->y);
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
	int types[] = {OBJTYPE_PLAYER_CHARACTER};
	int num_ents = world->find_entities(spawnpos, 64, ents, 2, types, 1);
	for(int i = 0; i < num_ents; i++)
	{
		if(ents[i] != this)
			return;
	}
	
	spawning = false;
	pos = spawnpos;

	core.pos = pos;
	core.vel = vec2(0,0);
	core.hooked_player = -1;

	health = 10;
	armor = 0;
	jumped = 0;
	
	ninja_activationtick = 0;
	sniper_chargetick = -1;
	currentcooldown = 0;
	
	dead = false;
	set_flag(entity::FLAG_PHYSICS);
	state = STATE_PLAYING;

	core.hook_state = HOOK_IDLE;

	mem_zero(&input, sizeof(input));

	// init weapons
	mem_zero(&weapons, sizeof(weapons));
	weapons[WEAPON_HAMMER].got = true;
	weapons[WEAPON_HAMMER].ammo = -1;
	weapons[WEAPON_GUN].got = true;
	weapons[WEAPON_GUN].ammo = data->weapons[WEAPON_GUN].maxammo;

	//weapons[WEAPON_SNIPER].got = true;
	//weapons[WEAPON_SNIPER].ammo = data->weapons[WEAPON_SNIPER].maxammo;
	active_weapon = WEAPON_GUN;
	last_weapon = WEAPON_HAMMER;
	wanted_weapon = WEAPON_GUN;

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

struct input_count
{
	int presses;
	int releases;
};

static input_count count_input(int prev, int cur)
{
	input_count c = {0,0};
	prev &= INPUT_STATE_MASK;
	cur &= INPUT_STATE_MASK;
	int i = prev;
	while(i != cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.presses++;
		else
			c.releases++;
	}

	return c;
}


int player::handle_ninja()
{
	vec2 direction = normalize(vec2(input.target_x, input.target_y));

	if ((server_tick() - ninja_activationtick) > (data->weapons[WEAPON_NINJA].duration * server_tickspeed() / 1000))
	{
		// time's up, return
		weapons[WEAPON_NINJA].got = false;
		active_weapon = last_weapon;
		set_weapon(active_weapon);
		return 0;
	}

	// Check if it should activate
	if (count_input(previnput.fire, input.fire).presses && (server_tick() > currentcooldown))
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
			int type = OBJTYPE_PLAYER_CHARACTER;
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
				if(numobjectshit < 10)
					hitobjects[numobjectshit++] = ents[i];
				ents[i]->take_damage(vec2(0,10.0f), data->weapons[WEAPON_NINJA].meleedamage, client_id,WEAPON_NINJA);
			}
		}
		return MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION | MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY;
	}

	return 0;
}

int player::handle_sniper()
{
	/*
	struct input_count button = count_input(previnput.fire, input.fire);
	
	bool must_release = false;
	int current_load = (server_tick()-sniper_chargetick) / server_tickspeed() + 1;
	
	if(input.fire&1)
	{
		if(sniper_chargetick == -1)
		{
			// start charge
			sniper_chargetick = server_tick();
		}
		else
		{
			if(current_load > weapons[WEAPON_SNIPER].ammo+3)
				must_release = true;
		}
	}

	if(button.releases || must_release)
	{
		vec2 direction = normalize(vec2(input.target_x, input.target_y));
		
		// released
		sniper_chargetick = -1;
		
		if(current_load > weapons[WEAPON_SNIPER].ammo)
			current_load = weapons[WEAPON_SNIPER].ammo;
			
		weapons[WEAPON_SNIPER].ammo -= current_load;

		new projectile(projectile::WEAPON_PROJECTILETYPE_SNIPER,
						client_id, pos+vec2(0,0), direction*50.0f,
						100, this, current_load, 0, 0, -1, WEAPON_SNIPER);
		create_sound(pos, SOUND_SNIPER_FIRE);

	}
	*/

	/*
	if(button.releases)
	{
		vec2 direction = normalize(vec2(input.target_x, input.target_y));
		// Check if we were charging, if so fire
		if (weapons[WEAPON_SNIPER].weaponstage >= WEAPONSTAGE_SNIPER_CHARGING)
		{
			new projectile(projectile::WEAPON_PROJECTILETYPE_SNIPER,
							client_id, pos+vec2(0,0), direction*50.0f,
							100 + weapons[WEAPON_SNIPER].weaponstage * 20,this, weapons[WEAPON_SNIPER].weaponstage, 0, 0, -1, WEAPON_SNIPER);
			create_sound(pos, SOUND_SNIPER_FIRE);
		}
		// Add blowback
		core.vel = -direction * 10.0f * weapons[WEAPON_SNIPER].weaponstage;

		// update ammo and stuff
		weapons[WEAPON_SNIPER].ammo = max(0,weapons[WEAPON_SNIPER].ammo - weapons[WEAPON_SNIPER].weaponstage);
		weapons[WEAPON_SNIPER].weaponstage = WEAPONSTAGE_SNIPER_NEUTRAL;
		weapons[WEAPON_SNIPER].chargetick = 0;
	}
	else if (input.fire & 1)
	{
		// Charge!! (if we are on the ground)
		if (is_grounded() && weapons[WEAPON_SNIPER].ammo > 0)
		{
			if (!weapons[WEAPON_SNIPER].chargetick)
			{
				weapons[WEAPON_SNIPER].chargetick = server_tick();
				dbg_msg("game", "Chargetick='%d:'", server_tick());
			}
			if ((server_tick() - weapons[WEAPON_SNIPER].chargetick) > server_tickspeed() * data->weapons[active_weapon].chargetime)
			{
				if (weapons[WEAPON_SNIPER].ammo > weapons[WEAPON_SNIPER].weaponstage)
				{
					weapons[WEAPON_SNIPER].weaponstage++;
					weapons[WEAPON_SNIPER].chargetick = server_tick();
				}
				else if ((server_tick() - weapons[WEAPON_SNIPER].chargetick) > server_tickspeed() * data->weapons[active_weapon].overchargetime)
				{
					// Ooopsie, weapon exploded
					create_explosion(pos, client_id, WEAPON_SNIPER, false);
					create_sound(pos, SOUND_ROCKET_EXPLODE);
					// remove this weapon and change weapon to gun
					weapons[WEAPON_SNIPER].got = false;
					weapons[WEAPON_SNIPER].ammo = 0;
					last_weapon = active_weapon;
					active_weapon = WEAPON_GUN;
					return 0;
				}
			}

			// While charging, don't move
			return 0;
		}
		else if (weapons[WEAPON_SNIPER].weaponstage)
			weapons[WEAPON_SNIPER].weaponstage = WEAPONSTAGE_SNIPER_NEUTRAL;
	}*/
	return 0;
}

int player::handle_weapons()
{
	vec2 direction = normalize(vec2(input.target_x, input.target_y));

	if(config.dbg_stress)
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

	// select weapon
	int next = count_input(previnput.next_weapon, input.next_weapon).presses;
	int prev = count_input(previnput.prev_weapon, input.prev_weapon).presses;
	
	if(next < 128) // make sure we only try sane stuff
	{
		while(next) // next weapon selection
		{
			wanted_weapon = (wanted_weapon+1)%NUM_WEAPONS;
			if(weapons[wanted_weapon].got)
				next--;
		}
	}

	if(prev < 128) // make sure we only try sane stuff
	{
		while(prev) // prev weapon selection
		{
			wanted_weapon = (wanted_weapon-1)<0?NUM_WEAPONS-1:wanted_weapon-1;
			if(weapons[wanted_weapon].got)
				prev--;
		}
	}

	if(input.wanted_weapon) // direct weapon selection
		wanted_weapon = input.wanted_weapon-1;

	if(wanted_weapon < 0 || wanted_weapon >= NUM_WEAPONS)
		wanted_weapon = 0;

	// switch weapon if wanted
	if(data->weapons[active_weapon].duration <= 0)
	{
		if(wanted_weapon != -1 && wanted_weapon != active_weapon && wanted_weapon >= 0 && wanted_weapon < NUM_WEAPONS && weapons[wanted_weapon].got)
		{
			if(active_weapon != wanted_weapon)
				create_sound(pos, SOUND_WEAPON_SWITCH);

			set_weapon(wanted_weapon);
		}
	}

	// don't update other weapons while sniper is active
	/*
	if (active_weapon == WEAPON_SNIPER)
		return handle_sniper();
	*/

	if(count_input(previnput.fire, input.fire).presses) //previnput.fire != input.fire && (input.fire&1))
	{
		if(reload_timer == 0)
		{
			// fire!
			if(weapons[active_weapon].ammo)
			{
				switch(active_weapon)
				{
					case WEAPON_HAMMER:
					{
						// reset objects hit
						numobjectshit = 0;
						create_sound(pos, SOUND_HAMMER_FIRE);
						break;
					}

					case WEAPON_GUN:
					{
						new projectile(WEAPON_GUN,
							client_id,
							pos+vec2(0,0),
							direction*30.0f,
							100,
							this,
							1, 0, 0, -1, WEAPON_GUN);
						create_sound(pos, SOUND_GUN_FIRE);
						break;
					}
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
							float spreading[] = {-0.12f, -0.05f, 0, 0.05f, 0.12f};
							float a = get_angle(direction);
							float v = 1.0f-fabs(i/(float)shotspread);
							a += spreading[i+2];
							new projectile(WEAPON_SHOTGUN,
								client_id,
								pos+vec2(0,0),
								vec2(cosf(a), sinf(a))*(30.0f + 15.0f*v),
								//vec2(cosf(a), sinf(a))*20.0f,
								(int)(server_tickspeed()*0.3f),
								this,
								1, 0, 0, -1, WEAPON_SHOTGUN);
						}
						create_sound(pos, SOUND_SHOTGUN_FIRE);
						break;
					}
					
					case WEAPON_SNIPER:
					{
						new projectile(WEAPON_SNIPER,
							client_id,
							pos+vec2(0,0),
							direction*300.0f,
							100,
							this,
							1, 0, 0, -1, WEAPON_SNIPER);
						create_sound(pos, SOUND_SNIPER_FIRE);						
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
		int type = OBJTYPE_PLAYER_CHARACTER;
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
			if(numobjectshit < 10)
				hitobjects[numobjectshit++] = ents[i];
			ents[i]->take_damage(vec2(0,-1.0f), data->weapons[active_weapon].meleedamage, client_id, active_weapon);
			player* target = (player*)ents[i];
			vec2 dir;
			if (length(target->pos - pos) > 0.0f)
				dir = normalize(target->pos - pos);
			else
				dir = vec2(0,-1);
				
			target->core.vel += normalize(dir + vec2(0,-1.1f)) * 10.0f;
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
	// check if we have enough input
	// this is to prevent initial weird clicks
	if(num_inputs < 2)
		previnput = input;

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

	// enable / disable physics
	if(team == -1 || dead)
		world->core.players[client_id] = 0;
	else
		world->core.players[client_id] = &core;

	// spectator
	if(team == -1)
		return;

	if(spawning)
		try_respawn();

	// TODO: rework the input to be more robust
	if(dead)
	{
		if(server_tick()-die_tick >= server_tickspeed()*5) // auto respawn after 3 sec
			respawn();
		if((input.fire&1) && server_tick()-die_tick >= server_tickspeed()/2) // auto respawn after 0.5 sec
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

	state = input.state;

	// Previnput
	previnput = input;
	return;
}

void player::tick_defered()
{
	if(!dead)
	{
		vec2 start_pos = core.pos;
		vec2 start_vel = core.vel;
		bool stuck_before = test_box(core.pos, vec2(28.0f, 28.0f));
		
		// TODO: this should be moved into the g_game
		// but not done to preserve the nethash
		if(length(core.vel) > 150.0f)
		{
			dbg_msg("server", "insane move! clamping (%f,%f) %f", core.vel.x, core.vel.y, length(core.vel));
			core.vel = normalize(core.vel) * 150.0f;
		}
		
		core.move();
		bool stuck_after_move = test_box(core.pos, vec2(28.0f, 28.0f));
		core.quantize();
		bool stuck_after_quant = test_box(core.pos, vec2(28.0f, 28.0f));
		pos = core.pos;
		
		if(!stuck_before && (stuck_after_move || stuck_after_quant))
		{
			dbg_msg("player", "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x", 
				stuck_before,
				stuck_after_move,
				stuck_after_quant,
				start_pos.x, start_pos.y,
				start_vel.x, start_vel.y,
				*((unsigned *)&start_pos.x), *((unsigned *)&start_pos.y),
				*((unsigned *)&start_vel.x), *((unsigned *)&start_vel.y));
		}

		int events = core.triggered_events;
		int mask = cmask_all_except_one(client_id);
		
		if(events&COREEVENT_GROUND_JUMP) create_sound(pos, SOUND_PLAYER_JUMP, mask);
		if(events&COREEVENT_AIR_JUMP)
		{
			create_sound(pos, SOUND_PLAYER_AIRJUMP, mask);
			ev_common *c = (ev_common *)::events.create(EVENT_AIR_JUMP, sizeof(ev_common), mask);
			if(c)
			{
				c->x = (int)pos.x;
				c->y = (int)pos.y;
			}
		}
		
		//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
		if(events&COREEVENT_HOOK_ATTACH_PLAYER) create_sound(pos, SOUND_HOOK_ATTACH_PLAYER, mask);
		if(events&COREEVENT_HOOK_ATTACH_GROUND) create_sound(pos, SOUND_HOOK_ATTACH_GROUND, mask);
		//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
		
	}
	
	if(team == -1)
	{
		pos.x = input.target_x;
		pos.y = input.target_y;
	}
}

void player::die(int killer, int weapon)
{
	int mode_special = gameobj->on_player_death(this, get_player(killer), weapon);

	dbg_msg("game", "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		killer, server_clientname(killer),
		client_id, server_clientname(client_id), weapon, mode_special);

	// send the kill message
	msg_pack_start(MSG_KILLMSG, MSGFLAG_VITAL);
	msg_pack_int(killer);
	msg_pack_int(client_id);
	msg_pack_int(weapon);
	msg_pack_int(mode_special);
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
	
	if(gameobj->is_friendly_fire(client_id, from) && !config.sv_teamdamage)
		return false;

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
		create_sound(get_player(from)->pos, SOUND_HIT, cmask_one(from));

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
	if(1)
	{
		obj_player_info *info = (obj_player_info *)snap_new_item(OBJTYPE_PLAYER_INFO, client_id, sizeof(obj_player_info));

		info->latency = latency_avg;
		info->latency_flux = latency_max-latency_min;
		info->local = 0;
		info->clientid = client_id;
		info->score = score;
		info->team = team;

		if(client_id == snaping_client)
			info->local = 1;
	}

	if(health > 0 && distance(players[snaping_client].pos, pos) < 1000.0f)
	{
		obj_player_character *character = (obj_player_character *)snap_new_item(OBJTYPE_PLAYER_CHARACTER, client_id, sizeof(obj_player_character));

		core.write(character);

		// this is to make sure that players that are just standing still
		// isn't sent. this is because the physics keep bouncing between
		// 0-128 when just standing.
		// TODO: fix the physics so this isn't needed
		if(snaping_client != client_id && abs(character->vy) < 256.0f)
			character->vy = 0;

		if (emote_stop < server_tick())
		{
			emote_type = EMOTE_NORMAL;
			emote_stop = -1;
		}

		character->emote = emote_type;

		character->ammocount = weapons[active_weapon].ammo;
		character->health = 0;
		character->armor = 0;
		character->weapon = active_weapon;
		character->weaponstage = weapons[active_weapon].weaponstage;
		character->attacktick = attack_tick;

		if(client_id == snaping_client)
		{
			character->health = health;
			character->armor = armor;
		}

		if(dead)
			character->health = -1;

		//if(length(vel) > 15.0f)
		//	player->emote = EMOTE_HAPPY;

		//if(damage_taken_tick+50 > server_tick())
		//	player->emote = EMOTE_PAIN;

		if (character->emote == EMOTE_NORMAL)
		{
			if(250 - ((server_tick() - last_action)%(250)) < 5)
				character->emote = EMOTE_BLINK;
		}

		character->state = state;
	}
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


void send_weapon_pickup(int cid, int weapon);

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
				create_sound(pos, SOUND_WEAPON_SPAWN);
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
				create_sound(pos, SOUND_PICKUP_HEALTH);
				pplayer->health = min(10, pplayer->health + data->powerupinfo[type].amount);
				respawntime = data->powerupinfo[type].respawntime;
			}
			break;
		case POWERUP_ARMOR:
			if(pplayer->armor < 10)
			{
				create_sound(pos, SOUND_PICKUP_ARMOR);
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

                    send_weapon_pickup(pplayer->client_id, subtype);
				}
			}
			break;
		case POWERUP_NINJA:
			{
				// activate ninja on target player
				pplayer->ninja_activationtick = server_tick();
				pplayer->weapons[WEAPON_NINJA].got = true;
				pplayer->last_weapon = pplayer->active_weapon;
				pplayer->active_weapon = WEAPON_NINJA;
				respawntime = data->powerupinfo[type].respawntime;
				create_sound(pos, SOUND_PICKUP_NINJA);

				// loop through all players, setting their emotes
				entity *ents[64];
				const int types[] = {OBJTYPE_PLAYER_CHARACTER};
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
			dbg_msg("game", "pickup player='%d:%s' item=%d/%d",
				pplayer->client_id, server_clientname(pplayer->client_id), type, subtype);
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

void create_sound(vec2 pos, int sound, int mask)
{
	if (sound < 0)
		return;

	// create a sound
	ev_sound *ev = (ev_sound *)events.create(EVENT_SOUND_WORLD, sizeof(ev_sound), mask);
	if(ev)
	{
		ev->x = (int)pos.x;
		ev->y = (int)pos.y;
		ev->sound = sound;
	}
}

void create_sound_global(int sound, int target)
{
	if (sound < 0)
		return;

	msg_pack_start(MSG_SOUND_GLOBAL, MSGFLAG_VITAL);
	msg_pack_int(sound);
	server_send_msg(target);
}

// TODO: should be more general
player* intersect_player(vec2 pos0, vec2 pos1, vec2& new_pos, entity* notthis)
{
	// Find other players
	entity *ents[64];
	vec2 dir = pos1 - pos0;
	float radius = length(dir * 0.5f);
	vec2 center = pos0 + dir * 0.5f;
	const int types[] = {OBJTYPE_PLAYER_CHARACTER};
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


void send_chat(int cid, int team, const char *msg)
{
	if(cid >= 0 && cid < MAX_CLIENTS)
		dbg_msg("chat", "%d:%d:%s: %s", cid, team, server_clientname(cid), msg);
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


// Server hooks
void mods_tick()
{
	// clear all events
	events.clear();
	
	world->tick();

	if(world->paused) // make sure that the game object always updates
		gameobj->tick();

	if(config.sv_restart)
	{
		if(config.sv_restart > 1)
			gameobj->do_warmup(config.sv_restart);
		else
			gameobj->startround();

		config.sv_restart = 0;
	}

	if(config.sv_msg[0] != 0)
	{
		send_chat(-1, 0, config.sv_msg);
		config.sv_msg[0] = 0;
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
		players[client_id].num_inputs++;
	}
}

void send_info(int who, int to_who)
{
	msg_pack_start(MSG_SETINFO, MSGFLAG_VITAL);
	msg_pack_int(who);
	msg_pack_string(server_clientname(who), 64);
	msg_pack_string(players[who].skin_name, 64);
	msg_pack_int(players[who].use_custom_color);
	msg_pack_int(players[who].color_body);
	msg_pack_int(players[who].color_feet);
	msg_pack_end();
	server_send_msg(to_who);
}

void send_emoticon(int cid, int emoticon)
{
	msg_pack_start(MSG_EMOTICON, MSGFLAG_VITAL);
	msg_pack_int(cid);
	msg_pack_int(emoticon % 16);
	msg_pack_end();
	server_send_msg(-1);
}

void send_weapon_pickup(int cid, int weapon)
{
	msg_pack_start(MSG_WEAPON_PICKUP, MSGFLAG_VITAL);
	msg_pack_int(weapon);
	msg_pack_end();
	server_send_msg(cid);
}

void mods_client_enter(int client_id)
{
	world->insert_entity(&players[client_id]);
	players[client_id].respawn();
	dbg_msg("game", "join player='%d:%s'", client_id, server_clientname(client_id));
	
	char buf[512];
	sprintf(buf, "%s has joined the game", server_clientname(client_id));
	send_chat(-1, -1, buf);	
}

void mods_connected(int client_id)
{
	players[client_id].init();
	players[client_id].client_id = client_id;

	//dbg_msg("game", "connected player='%d:%s'", client_id, server_clientname(client_id));

	// Check which team the player should be on
	if(gameobj->gametype == GAMETYPE_DM)
		players[client_id].team = 0;
	else
		players[client_id].team = gameobj->getteam(client_id);
}

void mods_client_drop(int client_id)
{
	char buf[512];
	sprintf(buf, "%s has left the game", server_clientname(client_id));
	send_chat(-1, -1, buf);

	dbg_msg("game", "leave player='%d:%s'", client_id, server_clientname(client_id));

	gameobj->on_player_death(&players[client_id], 0, -1);
	world->remove_entity(&players[client_id]);
	world->core.players[client_id] = 0x0;
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
		gameobj->on_player_info_change(&players[client_id]);
		
		// send all info to this client
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
				send_info(i, -1);
		}		
	}
	else if (msg == MSG_CHANGEINFO || msg == MSG_STARTINFO)
	{
		const char *name = msg_unpack_string();
		const char *skin_name = msg_unpack_string();
		players[client_id].use_custom_color = msg_unpack_int();
		players[client_id].color_body = msg_unpack_int();
		players[client_id].color_feet = msg_unpack_int();

		// check for invalid chars
		const char *p = name;
		while (*p)
		{
			if(*p < 32)
				return;
			p++;
		}

		//
		if(msg == MSG_CHANGEINFO && strcmp(name, server_clientname(client_id)) != 0)
		{
			char msg[256];
			sprintf(msg, "*** %s changed name to %s", server_clientname(client_id), name);
			send_chat(-1, -1, msg);
		}

		//send_set_name(client_id, players[client_id].name, name);
		strncpy(players[client_id].skin_name, skin_name, 64);
		server_setclientname(client_id, name);
		
		gameobj->on_player_info_change(&players[client_id]);
		
		if(msg == MSG_STARTINFO)
		{
			// send all info to this client
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(players[i].client_id != -1)
					send_info(i, client_id);
			}
			
			msg_pack_start(MSG_READY_TO_ENTER, MSGFLAG_VITAL);
			msg_pack_end();
			server_send_msg(client_id);			
		}
		
		send_info(client_id, -1);
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
	if(strcmp(config.sv_gametype, "ctf") == 0)
		gameobj = new gameobject_ctf;
	else if(strcmp(config.sv_gametype, "tdm") == 0)
		gameobj = new gameobject_tdm;
	else
		gameobj = new gameobject_dm;

	// setup core world
	for(int i = 0; i < MAX_CLIENTS; i++)
		players[i].core.world = &world->core;

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
			if(config.sv_powerups)
			{
				type = POWERUP_NINJA;
				subtype = WEAPON_NINJA;
			}
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

		for(int i = 0; i < config.dbg_bots ; i++)
		{
			mods_connected(MAX_CLIENTS-i-1);
			mods_client_enter(MAX_CLIENTS-i-1);
			if(gameobj->gametype != GAMETYPE_DM)
				players[MAX_CLIENTS-i-1].team = i&1;
		}
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
