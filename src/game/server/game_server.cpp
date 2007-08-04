#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <engine/config.h>
#include "../game.h"
#include "data.h"
#include "game_server.h"

data_container *data = 0x0;

using namespace baselib;

// --------- DEBUG STUFF ---------
const int debug_bots = 0;

// --------- PHYSICS TWEAK! --------
const float ground_control_speed = 7.0f;
const float ground_control_accel = 2.0f;
const float ground_friction = 0.5f;
const float ground_jump_speed = 13.5f;
const float air_control_speed = 3.5f;
const float air_control_accel = 1.2f;
const float air_friction = 0.95f;
const float hook_length = 34*10.0f;
const float hook_fire_speed = 45.0f;
const float hook_drag_accel = 3.0f;
const float hook_drag_speed = 15.0f;
const float gravity = 0.5f;

class player* get_player(int index);
void create_damageind(vec2 p, vec2 dir, int amount);
void create_explosion(vec2 p, int owner, int weapon, bool bnodamage);
void create_smoke(vec2 p);
void create_spawn(vec2 p);
void create_death(vec2 p);
void create_sound(vec2 pos, int sound, int loopflags = 0);
void create_targetted_sound(vec2 pos, int sound, int target, int loopflags = 0);
class player *intersect_player(vec2 pos0, vec2 pos1, vec2 &new_pos, class entity *notthis = 0);

template<typename T>
T saturated_add(T min, T max, T current, T modifier)
{
	if(modifier < 0)
	{
		if(current < min)
			return current;
		current += modifier;
		if(current < min)
			current = min;
		return current;
	}
	else
	{
		if(current > max)
			return current;
		current += modifier;
		if(current > max)
			current = max;
		return current;
	}
}

// TODO: rewrite this smarter!
void move_box(vec2 *inout_pos, vec2 *inout_vel, vec2 size, float elasticity)
{
	// do the move
	vec2 pos = *inout_pos;
	vec2 vel = *inout_vel;
	
	float distance = length(vel);
	int max = (int)distance;
	
	vec2 offsets[4] = {	vec2(-size.x/2, -size.y/2), vec2( size.x/2, -size.y/2),
						vec2(-size.x/2,  size.y/2), vec2( size.x/2,  size.y/2)};
	
	if(distance > 0.00001f)
	{
		vec2 old_pos = pos;
		for(int i = 0; i <= max; i++)
		{
			float amount = i/(float)max;
			if(max == 0)
				amount = 0;
			
			vec2 new_pos = pos + vel*amount; // TODO: this row is not nice

			for(int p = 0; p < 4; p++)
			{
				vec2 np = new_pos+offsets[p];
				vec2 op = old_pos+offsets[p];
				if(col_check_point(np))
				{
					int affected = 0;
					if(col_check_point(np.x, op.y))
					{
						vel.x = -vel.x*elasticity;
						pos.x = old_pos.x;
						new_pos.x = old_pos.x;
						affected++;
					}

					if(col_check_point(op.x, np.y))
					{
						vel.y = -vel.y*elasticity;
						pos.y = old_pos.y;
						new_pos.y = old_pos.y;
						affected++;
					}
					
					if(!affected)
					{
						new_pos = old_pos;
						pos = old_pos;
						vel *= -elasticity;
					}
				}
			}
			
			old_pos = new_pos;
		}
		
		pos = old_pos;
	}
	
	*inout_pos = pos;
	*inout_vel = vel;
}

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
event_handler::event_handler()
{
	clear();
}

void *event_handler::create(int type, int size, int target)
{
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
	flags = FLAG_ALIVE;
	proximity_radius = 0;
	
	current_id++;
	id = current_id;
	
	next_entity = 0;
	prev_entity = 0;
	prev_type_entity = 0;
	next_type_entity = 0;
}

entity::~entity()
{
}

int entity::current_id = 1;

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
		if(!(ent->flags&entity::FLAG_ALIVE))
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
			if(!(ent->flags&entity::FLAG_ALIVE))
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

game_world world;

//////////////////////////////////////////////////
// game object
//////////////////////////////////////////////////
gameobject::gameobject()
: entity(OBJTYPE_GAME)
{
	gametype = GAMETYPE_DM;
	game_over_tick = -1;
	sudden_death = 0;
	round_start_tick = server_tick();
}

void gameobject::endround()
{
	world.paused = true;
	game_over_tick = server_tick();
	sudden_death = 0;
}

void gameobject::resetgame()
{
	world.reset_requested = true;
}

void gameobject::startround()
{
	resetgame();
	
	round_start_tick = server_tick();
	sudden_death = 0;
	game_over_tick = -1;
	world.paused = false;
}

void gameobject::post_reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1)
			players[i].respawn();
	}
}

void gameobject::tick_dm()
{
	if(game_over_tick == -1)
	{
		// game is running
		
		// gather some stats
		int topscore = 0;
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
			{
				if(players[i].score > topscore)
				{
					topscore = players[i].score;
					topscore_count = 1;
				}
				else if(players[i].score == topscore)
					topscore_count++;
			}
		}
		
		// check score win condition
		if((config.scorelimit > 0 && topscore >= config.scorelimit) ||
			(config.timelimit > 0 && (server_tick()-round_start_tick) >= config.timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
	else
	{
		// game over.. wait for restart
		if(server_tick() > game_over_tick+server_tickspeed()*10)
			startround();
	}
}

void gameobject::tick_tdm()
{
	if(game_over_tick == -1)
	{
		// game is running
		
		// gather some stats
		int totalscore[2] = {0,0};
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
				totalscore[players[i].team] += players[i].score;
		}
		if (totalscore[0] >= config.scorelimit)
			topscore_count++;
		if (totalscore[1] >= config.scorelimit)
			topscore_count++;
		
		// check score win condition
		if((config.scorelimit > 0 && (totalscore[0] >= config.scorelimit || totalscore[1] >= config.scorelimit)) ||
			(config.timelimit > 0 && (server_tick()-round_start_tick) >= config.timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
	else
	{
		// game over.. wait for restart
		if(server_tick() > game_over_tick+server_tickspeed()*10)
			startround();
	}
}

void gameobject::tick()
{
	switch(gametype)
	{
	case GAMETYPE_TDM:
		{
			tick_tdm();
			break;
		}
	default:
		{
			tick_dm();
			break;
		}
	}
}

void gameobject::snap(int snapping_client)
{
	obj_game *game = (obj_game *)snap_new_item(OBJTYPE_GAME, id, sizeof(obj_game));
	game->paused = world.paused;
	game->game_over = game_over_tick==-1?0:1;
	game->sudden_death = sudden_death;
	
	game->score_limit = config.scorelimit;
	game->time_limit = config.timelimit;
	game->round_start_tick = round_start_tick;
	game->gametype = gametype;
}

int gameobject::getteam(int notthisid)
{
	int numplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1 && players[i].client_id != notthisid)
		{
			numplayers[players[i].team]++;
		}
	}

	return numplayers[0] > numplayers[1] ? 1 : 0;
}

gameobject gameobj;

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
	world.insert_entity(this);
}

void projectile::reset()
{
	world.destroy_entity(this);
}

void projectile::tick()
{
	vec2 oldpos = pos;
	vel.y += 0.25f;
	pos += vel;
	lifespan--;
	
	// check player intersection as well
	vec2 new_pos;
	entity *targetplayer = (entity*)intersect_player(oldpos, pos, new_pos, powner);
	if(targetplayer || lifespan < 0 || col_check_point((int)pos.x, (int)pos.y))
	{
		if (lifespan >= 0 || weapon == WEAPON_ROCKET)
			create_sound(pos, sound_impact);
			
		if (flags & PROJECTILE_FLAGS_EXPLODE)
			create_explosion(oldpos, owner, weapon, false);
		else if (targetplayer)
		{
			targetplayer->take_damage(normalize(vel) * max(0.001f, force), damage, owner, weapon);
				
			create_targetted_sound(pos, SOUND_HIT, owner);
		}
			
		world.destroy_entity(this);
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
// projectile_backpackrocket
//////////////////////////////////////////////////
projectile_backpackrocket::projectile_backpackrocket(baselib::vec2 pos, baselib::vec2 target, int owner, entity* powner)
: projectile(WEAPON_PROJECTILETYPE_ROCKET, owner, pos, vec2(0,0), 100, powner, 0, 0, 0, -1, WEAPON_ROCKET_BACKPACK)
{
	stage = 0;
	start_tick = server_tick();
	vel = vec2(0,-10.0f);
	this->target = target;
	start = pos;
	midpoint = pos;
	midpoint.y = target.y;
	direction = normalize(target-midpoint);
	deply_ticks = (int)( distance(start, midpoint)/(float)server_tickspeed() * 5.0f );
	dbg_msg("rocket_bp", "%f %d", distance(start, midpoint), deply_ticks);
}

void projectile_backpackrocket::tick()
{
	lifespan--;
	if(!lifespan)
		world.destroy_entity(this);
		
	vec2 oldpos = pos;
		
	if(stage == 0)
	{
		float time = (server_tick()-start_tick)/(float)(deply_ticks);
		if(midpoint.y > start.y)
			pos.y = mix(start.y, midpoint.y, 1-sinf((1-time)*pi/2));
		else
			pos.y = mix(start.y, midpoint.y, sinf(time*pi/2));

		float a = (server_tick()-start_tick)/(float)server_tickspeed()*pi*7.5f;
		vel.x = sinf(a)*30.0f;
		vel.y = cosf(a)*30.0f;
		
		if(server_tick() > start_tick+deply_ticks)
		{
			pos = midpoint;
			direction = normalize(target-pos);
			vel = vec2(0,0);
			stage = 1;
		}
	}
	else if(stage == 1)
	{
		vel += direction*1.5f;
		vel.x = clamp(vel.x, -20.0f, 20.0f);
		pos += vel;
	}

	// check player intersection as well
	vec2 new_pos;
	entity *targetplayer = (entity*)intersect_player(oldpos, pos, new_pos, powner);
	if(targetplayer || lifespan < 0 || col_check_point((int)pos.x, (int)pos.y))
	{
		if (lifespan >= 0)
			create_sound(pos, sound_impact);
			
		create_explosion(oldpos, owner, weapon, false);
			
		world.destroy_entity(this);
	}	
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
	team = 0;
	extrapowerflags = 0;
	ninjaactivationtick = 0;
	reset();
}

void player::reset()
{
	release_hooked();
	release_hooks();
	
	pos = vec2(0.0f, 0.0f);
	vel = vec2(0.0f, 0.0f);
	direction = vec2(0.0f, 1.0f);
	score = 0;
	dead = true;
	spawning = false;
	die_tick = 0;
	state = STATE_UNKNOWN;
}

void player::destroy() {  }
	
void player::respawn()
{
	spawning = true;
}

void player::try_respawn()
{
	// get spawn point
	int start, num;
	map_get_type(1, &start, &num);

	vec2 spawnpos = vec2(100.0f, -60.0f);
	if(num)
	{
		mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + (rand()%num), NULL, NULL);
		spawnpos = vec2((float)sp->x, (float)sp->y);
	}
	
	// check if the position is occupado
	entity *ents[2] = {0};
	int types[] = {OBJTYPE_PLAYER};
	int num_ents = world.find_entities(spawnpos, 64, ents, 2, types, 1);
	for(int i = 0; i < num_ents; i++)
	{
		if(ents[i] != this)
			return;
	}
	
	spawning = false;
	pos = spawnpos;
	defered_pos = pos;
	

	health = data->playerinfo[gameobj.gametype].maxhealth;
	armor = 0;
	jumped = 0;
	dead = false;
	set_flag(entity::FLAG_ALIVE);
	state = STATE_PLAYING;
	
	mem_zero(&input, sizeof(input));
	vel = vec2(0.0f, 0.0f);
		
	// init weapons
	mem_zero(&weapons, sizeof(weapons));
	weapons[WEAPON_HAMMER].got = true;
	weapons[WEAPON_HAMMER].ammo = -1;
	weapons[WEAPON_GUN].got = true;
	weapons[WEAPON_GUN].ammo = data->weapons[active_weapon].maxammo;
	
	active_weapon = WEAPON_GUN;
	reload_timer = 0;
	
	// Create sound and spawn effects
	create_sound(pos, SOUND_PLAYER_SPAWN);
	create_spawn(pos);
}

bool player::is_grounded()
{
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	return false;
}

// releases the hooked player
void player::release_hooked()
{
	hook_state = HOOK_RETRACTED;
	hooked_player = 0x0;
}

// release all hooks to this player	
void player::release_hooks()
{
	// TODO: loop thru players only
	for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
	{
		if(ent && ent->objtype == OBJTYPE_PLAYER)
		{
			player *p = (player*)ent;
			if(p->hooked_player == this)
				p->release_hooked();
		}
	}
}

int player::handle_ninja()
{
	if ((server_tick() - ninjaactivationtick) > (data->weapons[WEAPON_NINJA].duration * server_tickspeed() / 1000))
	{
		// time's up, return
		active_weapon = WEAPON_GUN;
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
		release_hooked();
		release_hooks();
	}

	currentmovetime--;
	
	if (currentmovetime == 0)
	{	
		// reset player velocity
		vel *= 0.2f;
		//return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
	}
	
	if (currentmovetime > 0)
	{
		// Set player velocity
		vel = activationdir * data->weapons[WEAPON_NINJA].velocity;
		vec2 oldpos = pos;
		move_box(&defered_pos, &vel, vec2(phys_size, phys_size), 0.0f);
		// reset velocity so the client doesn't predict stuff
		vel = vec2(0.0f,0.0f);
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
			int num = world.find_entities(center, radius, ents, 64, &type, 1);
			
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
				ents[i]->take_damage(vec2(0,10.0f), data->weapons[WEAPON_NINJA].meleedamage, client_id,-1);
			}
		}
		return MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION | MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY;
	}
	return 0;
}

int player::handle_weapons()
{
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
	if(input.activeweapon >= 0 && input.activeweapon < NUM_WEAPONS && weapons[input.activeweapon].got && 
		data->weapons[active_weapon].duration <= 0)
	{
		if (active_weapon != input.activeweapon)
			create_sound(pos, SOUND_WEAPON_SWITCH);

		active_weapon = input.activeweapon;
		
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
						new projectile(projectile::WEAPON_PROJECTILETYPE_GUN,
							client_id,
							pos+vec2(0,0),
							direction*30.0f,
							100,
							this,
							1, 0, 0, -1, WEAPON_GUN);
						create_sound(pos, SOUND_GUN_FIRE);
						break;
					case WEAPON_ROCKET:
						new projectile(projectile::WEAPON_PROJECTILETYPE_ROCKET,
							client_id,
							pos+vec2(0,0),
							direction*15.0f,
							100,
							this,
							1, projectile::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_ROCKET_EXPLODE, WEAPON_ROCKET);
						create_sound(pos, SOUND_ROCKET_FIRE);
						break;
					case WEAPON_SHOTGUN:
					{
						int shotspread = min(2, weapons[active_weapon].ammo);
						weapons[active_weapon].ammo -= shotspread - 1; // one will be taken later
						for(int i = -shotspread; i <= shotspread; i++)
						{
							float a = get_angle(direction);
							a += i*0.075f;
							new projectile(projectile::WEAPON_PROJECTILETYPE_SHOTGUN,
								client_id,
								pos+vec2(0,0),
								vec2(cosf(a), sinf(a))*25.0f,
								//vec2(cosf(a), sinf(a))*20.0f,
								server_tickspeed()/3,
								this,
								2, 0, 0, -1, WEAPON_SHOTGUN);
						}
						create_sound(pos, SOUND_SHOTGUN_FIRE);
						break;
					}
					case WEAPON_ROCKET_BACKPACK:
						new projectile_backpackrocket(
							pos+vec2(0,0),
							pos+vec2(input.target_x,input.target_y),
							client_id,
							this);
						break;						
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
		int num = world.find_entities(center, radius, ents, 64, &type, 1);
		
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

			// set his velocity to fast upward (for now)
			create_smoke(ents[i]->pos);
			create_sound(pos, SOUND_HAMMER_HIT);
			hitobjects[numobjectshit++] = ents[i];
			ents[i]->take_damage(vec2(0,10.0f), data->weapons[active_weapon].meleedamage, client_id, active_weapon);
			player* target = (player*)ents[i];
			vec2 dir;
			if (length(target->pos - pos) > 0.0f)
				dir = normalize(target->pos - pos);
			else
				dir = vec2(0,-1);
			target->vel += dir * 10.0f + vec2(0,-10.0f);
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
		client_info info;
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
	
	if(spawning)
		try_respawn();

	// TODO: rework the input to be more robust
	// TODO: remove this tick count, it feels weird
	if(dead)
	{
		if(server_tick()-die_tick >= server_tickspeed()*5) // auto respawn after 3 sec
			respawn();
		if(input.fire && server_tick()-die_tick >= server_tickspeed()/2) // auto respawn after 0.5 sec
			respawn();
		return;
	}
	
	// fetch some info
	bool grounded = is_grounded();
	direction = normalize(vec2(input.target_x, input.target_y));
	
	float max_speed = grounded ? ground_control_speed : air_control_speed;
	float accel = grounded ? ground_control_accel : air_control_accel;
	float friction = grounded ? ground_friction : air_friction;
	
	// handle movement
	if(input.left)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, -accel);
	if(input.right)
		vel.x = saturated_add(-max_speed, max_speed, vel.x, accel);
		
	if(!input.left && !input.right)
		vel.x *= friction;
	
	// handle jumping
	if(input.jump)
	{
		if(!jumped && grounded)
		{
			create_sound(pos, SOUND_PLAYER_JUMP);
			vel.y = -ground_jump_speed;
			jumped++;
		}
	}
	else
		jumped = 0;
		
	// do hook
	if(input.hook)
	{
		if(hook_state == HOOK_IDLE)
		{
			hook_state = HOOK_FLYING;
			hook_pos = pos;
			hook_dir = direction;
			hook_tick = -1;
		}
		else if(hook_state == HOOK_FLYING)
		{
			vec2 new_pos = hook_pos+hook_dir*hook_fire_speed;

			// Check against other players first
			for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
			{
				if(ent && ent->objtype == OBJTYPE_PLAYER)
				{
					player *p = (player*)ent;
					if(p != this && !p->dead && distance(p->pos, new_pos) < p->phys_size)
					{
						hook_state = HOOK_GRABBED;
						hooked_player = p;
						break;
					}
				}
			}
			
			if(hook_state == HOOK_FLYING)
			{
				// check against ground
				if(col_intersect_line(hook_pos, new_pos, &new_pos))
				{
					hook_state = HOOK_GRABBED;
					hook_pos = new_pos;	
				}
				else if(distance(pos, new_pos) > hook_length)
				{
					hook_state = HOOK_RETRACTED;
				}
				else
					hook_pos = new_pos;
			}
			
			if(hook_state == HOOK_GRABBED)
			{
				create_sound(pos, SOUND_HOOK_ATTACH);
				hook_tick = server_tick();
			}
		}
	}
	else
	{
		release_hooked();
		hook_state = HOOK_IDLE;
		hook_pos = pos;
	}
		
	if(hook_state == HOOK_GRABBED)
	{
		if(hooked_player)
		{
			hook_pos = hooked_player->pos;
			
			// keep players hooked for a max of 1.5sec
			if(server_tick() > hook_tick+(server_tickspeed()*3)/2)
				release_hooked();
		}
			
		/*if(hooked_player)
			hook_pos = hooked_player->pos;

		float d = distance(pos, hook_pos);
		vec2 dir = normalize(pos - hook_pos);		
		if(d > 10.0f) // TODO: fix tweakable variable
		{
			float accel = hook_drag_accel * (d/hook_length);
			vel.x = saturated_add(-hook_drag_speed, hook_drag_speed, vel.x, -accel*dir.x*0.75f);
			vel.y = saturated_add(-hook_drag_speed, hook_drag_speed, vel.y, -accel*dir.y);
		}*/
		
		// Old version feels much better (to me atleast)
		if(distance(hook_pos, pos) > 46.0f)
		{
			vec2 hookvel = normalize(hook_pos-pos)*hook_drag_accel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(hookvel.y > 0)
				hookvel.y *= 0.3f;
			
			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((hookvel.x < 0 && input.left) || (hookvel.x > 0 && input.right)) 
				hookvel.x *= 0.95f;
			else
				hookvel.x *= 0.75f;
			
			vec2 new_vel = vel+hookvel;
			
			// check if we are under the legal limit for the hook
			if(length(new_vel) < hook_drag_speed || length(new_vel) < length(vel))
				vel = new_vel; // no problem. apply
		}
	}
		
	// fix influence of other players, collision + hook
	// TODO: loop thru players only
	for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
	{
		if(ent && ent->objtype == OBJTYPE_PLAYER)
		{
			player *p = (player*)ent;
			if(p == this || !(p->flags&FLAG_ALIVE))
				continue; // make sure that we don't nudge our self
			
			// handle player <-> player collision
			float d = distance(pos, p->pos);
			vec2 dir = normalize(pos - p->pos);
			if(d < phys_size*1.25f)
			{
				float a = phys_size*1.25f - d;
				vel = vel + dir*a;
			}
			
			// handle hook influence
			if(p->hooked_player == this)
			{
				if(d > phys_size*1.50f) // TODO: fix tweakable variable
				{
					float accel = hook_drag_accel * (d/hook_length);
					vel.x = saturated_add(-hook_drag_speed, hook_drag_speed, vel.x, -accel*dir.x);
					vel.y = saturated_add(-hook_drag_speed, hook_drag_speed, vel.y, -accel*dir.y);
				}
			}
		}
	}
	
	// handle weapons
	int retflags = handle_weapons();
	if (!(retflags & (MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION)))
	{
		// add gravity
		if (!(retflags & MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY))
			vel.y += gravity;
	
		// do the move
		defered_pos = pos;
		move_box(&defered_pos, &vel, vec2(phys_size, phys_size), 0);
	}

	state = input.state;
	
	// Previnput
	previnput = input;
	return;
}

void player::tick_defered()
{
	// apply the new position
	pos = defered_pos;
}

void player::die(int killer, int weapon)
{
	// send the kill message
	msg_pack_start(MSG_KILLMSG, MSGFLAG_VITAL);
	msg_pack_int(killer);
	msg_pack_int(client_id);
	msg_pack_int(weapon);
	msg_pack_end();
	server_send_msg(-1);
	
	// a nice sound
	create_sound(pos, SOUND_PLAYER_DIE);
	
	// release all hooks
	release_hooked();
	release_hooks();
	
	// set dead state
	dead = true;
	die_tick = server_tick();
	clear_flag(entity::FLAG_ALIVE);
	create_death(pos);
}

bool player::take_damage(vec2 force, int dmg, int from, int weapon)
{
	vel += force;

	// create healthmod indicator
	create_damageind(pos, normalize(force), dmg);

	if (gameobj.gametype == GAMETYPE_TDM && from >= 0 && players[from].team == team)
		return false;

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
	
	damage_taken_tick = server_tick()+50;
	
	// check for death
	if(health <= 0)
	{
		// apply score
		if(from != -1)
		{
			if(from == client_id)
				score--;
			else
			{
				player *p = get_player(from);
				p->score++;
			}
		}
		
		die(from, weapon);
		return false;
	}

	if (dmg > 2)
		create_sound(pos, SOUND_PLAYER_PAIN_LONG);
	else
		create_sound(pos, SOUND_PLAYER_PAIN_SHORT);

	// spawn blood?
	return true;
}

void player::snap(int snaping_client)
{
	obj_player *player = (obj_player *)snap_new_item(OBJTYPE_PLAYER, client_id, sizeof(obj_player));

	player->x = (int)pos.x;
	player->y = (int)pos.y;
	player->vx = (int)vel.x;
	player->vy = (int)vel.y;
	player->emote = EMOTE_NORMAL;

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
	
	if(length(vel) > 15.0f)
		player->emote = EMOTE_HAPPY;
	
	if(damage_taken_tick > server_tick())
		player->emote = EMOTE_PAIN;
	
	if(player->emote == EMOTE_NORMAL)
	{
		if((server_tick()%(50*5)) < 10)
			player->emote = EMOTE_BLINK;
	}
	
	player->hook_active = hook_state>0?1:0;
	player->hook_x = (int)hook_pos.x;
	player->hook_y = (int)hook_pos.y;

	float a = atan((float)input.target_y/(float)input.target_x);
	if(input.target_x < 0)
		a = a+pi;
		
	player->angle = (int)(a*256.0f);
	
	player->score = score;
	player->team = team;

	player->state = state;
}

player players[MAX_CLIENTS];

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
	world.insert_entity(this);
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
			if(pplayer->health < data->playerinfo[gameobj.gametype].maxhealth)
			{
				create_sound(pos, SOUND_PICKUP_HEALTH, 0);
				pplayer->health = min((int)data->playerinfo[gameobj.gametype].maxhealth, pplayer->health + data->powerupinfo[type].amount);
				respawntime = data->powerupinfo[type].respawntime;
			}
			break;
		case POWERUP_ARMOR:
			if(pplayer->armor < data->playerinfo[gameobj.gametype].maxarmor)
			{
				create_sound(pos, SOUND_PICKUP_ARMOR, 0);
				pplayer->armor = min((int)data->playerinfo[gameobj.gametype].maxarmor, pplayer->armor + data->powerupinfo[type].amount);
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
				pplayer->active_weapon = WEAPON_NINJA;
				respawntime = data->powerupinfo[type].respawntime;
				create_sound(pos, SOUND_PICKUP_NINJA);
				break;
			}
		default:
			break;
		};
		
		if(respawntime >= 0)
			spawntick = server_tick() + server_tickspeed() * respawntime;
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
	world.insert_entity(this);
}

void flag::reset()
{
	spawntick = -1;
}

void flag::tick()
{
	// wait for respawn
	if(spawntick > 0)
	{
		if(server_tick() > spawntick)
			spawntick = -1;
		else
			return;
	}

	// Check if a player intersected us
	vec2 meh;
	player* pplayer = intersect_player(pos, pos + vel, meh, 0);
	if (pplayer)
	{
		if (!carrying_player)
			carrying_player = pplayer;

		// TODO: something..?
	}

	if (carrying_player)
	{
		if (carrying_player->dead)
			carrying_player = 0x0;
		else
		{
			vel = carrying_player->pos - pos;
			pos = carrying_player->pos;
		}
	}
	
	if (!carrying_player)
	{
		vel.y += 0.25f;
		vec2 new_pos = pos + vel;

		col_intersect_line(pos, new_pos, &new_pos);

		pos = new_pos;

		if (is_grounded())
			vel.x = vel.y = 0;
	}
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
}
// FLAG END ///////////////////////

player *get_player(int index)
{
	return &players[index];
}

void create_damageind(vec2 p, vec2 dir, int amount)
{
	float a = 3 * 3.14159f / 2;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(amount+2));
		ev_damageind *ev = (ev_damageind *)events.create(EVENT_DAMAGEINDICATION, sizeof(ev_damageind));
		ev->x = (int)p.x;
		ev->y = (int)p.y;
		ev->angle = (int)(f*256.0f);
	}
}

void create_explosion(vec2 p, int owner, int weapon, bool bnodamage)
{
	// create the event
	ev_explosion *ev = (ev_explosion *)events.create(EVENT_EXPLOSION, sizeof(ev_explosion));
	ev->x = (int)p.x;
	ev->y = (int)p.y;
	
	if (!bnodamage)
	{
		// deal damage
		entity *ents[64];
		const float radius = 128.0f;
		const float innerradius = 42.0f;
		int num = world.find_entities(p, radius, ents, 64);
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
				ents[i]->take_damage(forcedir*dmg*2, (int)dmg/2, owner, weapon);
		}
	}
}

void create_smoke(vec2 p)
{
	// create the event
	ev_explosion *ev = (ev_explosion *)events.create(EVENT_SMOKE, sizeof(ev_explosion));
	ev->x = (int)p.x;
	ev->y = (int)p.y;
}

void create_spawn(vec2 p)
{
	// create the event
	ev_spawn *ev = (ev_spawn *)events.create(EVENT_SPAWN, sizeof(ev_spawn));
	ev->x = (int)p.x;
	ev->y = (int)p.y;
}

void create_death(vec2 p)
{
	// create the event
	ev_death *ev = (ev_death *)events.create(EVENT_DEATH, sizeof(ev_death));
	ev->x = (int)p.x;
	ev->y = (int)p.y;
}

void create_targetted_sound(vec2 pos, int sound, int target, int loopingflags)
{
	if (sound < 0)
		return;

	// create a sound
	ev_sound *ev = (ev_sound *)events.create(EVENT_SOUND, sizeof(ev_sound), target);
	ev->x = (int)pos.x;
	ev->y = (int)pos.y;
	ev->sound = sound | loopingflags;
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
	int num = world.find_entities(center, radius, ents, 64, types, 1);
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
	world.tick();
	
	if(world.paused) // make sure that the game object always updates
		gameobj.tick();

	if(debug_bots)
	{
		static int count = 0;
		if(count >= 0)
		{
			count++;
			if(count == 10)
			{
				for(int i = 0; i < debug_bots ; i++)
				{
					mods_client_enter(MAX_CLIENTS-i-1);
					strcpy(players[MAX_CLIENTS-i-1].name, "(bot)");
				}
				count = -1;
			}
		}
	}
}

void mods_snap(int client_id)
{
	world.snap(client_id);
	events.snap(client_id);
}

void mods_client_input(int client_id, void *input)
{
	if(!world.paused)
	{
		//players[client_id].previnput = players[client_id].input;
		players[client_id].input = *(player_input*)input;
	}
}

void send_chat_all(int cid, const char *msg)
{
	msg_pack_start(MSG_CHAT, MSGFLAG_VITAL);
	msg_pack_int(cid);
	msg_pack_string(msg, 512);
	msg_pack_end();
	server_send_msg(-1);
}

void mods_client_enter(int client_id)
{
	players[client_id].init();
	players[client_id].client_id = client_id;
	world.insert_entity(&players[client_id]);
	players[client_id].respawn();
	
	
	client_info info; // fetch login name
	if(server_getclientinfo(client_id, &info))
	{
		strcpy(players[client_id].name, info.name);
	}
	else
		strcpy(players[client_id].name, "(bot)");

	if (gameobj.gametype == GAMETYPE_TDM)
	{
		// Check which team the player should be on
		players[client_id].team = gameobj.getteam(client_id);
	}
	
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
	send_chat_all(-1, buf);
}

void mods_client_drop(int client_id)
{
	char buf[512];
	sprintf(buf, "%s has left the game", players[client_id].name);
	send_chat_all(-1, buf);
	
	dbg_msg("mods", "client drop %d", client_id);
	world.remove_entity(&players[client_id]);
	players[client_id].client_id = -1;
}

void mods_message(int msg, int client_id)
{
	if(msg == MSG_SAY)
	{
		send_chat_all(client_id, msg_unpack_string());
	}
	else if (msg == MSG_SWITCHTEAM)
	{
		// Switch team on given client and kill/respawn him
		players[client_id].team = !players[client_id].team;
		players[client_id].die(client_id, -1);
		players[client_id].score--;
	}
}

extern unsigned char internal_data[];

void mods_init()
{
	data = load_data_from_memory(internal_data);
	col_init(32);

	int start, num;
	map_get_type(MAPRES_ITEM, &start, &num);
	
	// TODO: this is way more complicated then it should be
	for(int i = 0; i < num; i++)
	{
		mapres_item *it = (mapres_item *)map_get_item(start+i, 0, 0);
		
		int type = -1;
		int subtype = -1;
		
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
			 // perhaps we can get rid of it. seams like a stupid thing to have
			powerup *ppower = new powerup(type, subtype);
			ppower->pos = vec2(it->x, it->y);
		}
	}
	
	world.insert_entity(&gameobj);
}

void mods_shutdown() {}
void mods_presnap() {}
void mods_postsnap() {}
