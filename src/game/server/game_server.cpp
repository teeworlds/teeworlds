#include <stdlib.h>
#include <string.h>
#include "../game.h"
#include "data.h"

using namespace baselib;

// --------- DEBUG STUFF ---------
const bool debug_bots = true;

// --------- PHYSICS TWEAK! --------
const float ground_control_speed = 7.0f;
const float ground_control_accel = 2.0f;
const float ground_friction = 0.5f;
const float ground_jump_speed = 12.0f;
const float air_control_speed = 3.5f;
const float air_control_accel = 1.2f;
const float air_friction = 0.95f;
const float hook_length = 32*10.0f;
const float hook_fire_speed = 45.0f;
const float hook_drag_accel = 3.0f;
const float hook_drag_speed = 15.0f;
const float gravity = 0.5f;

class player* get_player(int index);
void create_damageind(vec2 p, vec2 dir, int amount);
void create_explosion(vec2 p, int owner = -1, bool bnodamage = false);
void create_smoke(vec2 p);
void create_sound(vec2 pos, int sound, int loopflags = 0);
class player* intersect_player(vec2 pos0, vec2 pos1, vec2& new_pos, class entity* notthis = 0);

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

//
class event_handler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*4;

	int types[MAX_EVENTS];  // TODO: remove some of these arrays
	int offsets[MAX_EVENTS];
	int sizes[MAX_EVENTS];
	char data[MAX_DATASIZE];
	
	int current_offset;
	int num_events;
public:
	event_handler()
	{
		clear();
	}
	
	void *create(int type, int size)
	{
		void *p = &data[current_offset];
		offsets[num_events] = current_offset;
		types[num_events] = type;
		sizes[num_events] = size;
		current_offset += size;
		num_events++;
		return p;
	}
	
	void clear()
	{
		num_events = 0;
		current_offset = 0;
	}
	
	void snap(int snapping_client)
	{
		for(int i = 0; i < num_events; i++)
		{
			void *d = snap_new_item(types[i], i, sizes[i]);
			mem_copy(d, &data[offsets[i]], sizes[i]);
		}
	}
};

static event_handler events;

// a basic entity
class entity
{
private:
	friend class game_world;
	friend class player;
	entity *prev_entity;
	entity *next_entity;

	entity *prev_type_entity;
	entity *next_type_entity;

	int index;
	static int current_id;
protected:
	int id;
	
public:
	float proximity_radius;
	unsigned flags;
	int objtype;
	vec2 pos;

	enum
	{
		FLAG_DESTROY=0x00000001,
		FLAG_ALIVE=0x00000002,
	};
	
	entity(int objtype)
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
	
	void set_flag(unsigned flag) { flags |= flag; }
	void clear_flag(unsigned flag) { flags &= ~flag; }
	
	virtual ~entity()
	{
	}

	virtual void destroy() { delete this; }
	virtual void tick() {}
	virtual void tick_defered() {}
		
	virtual void snap(int snapping_client) {}
		
	virtual bool take_damage(vec2 force, int dmg, int from) { return true; }
};

int entity::current_id = 1;

// game world. handles all entities
class game_world
{
public:
	enum
	{
		NUM_ENT_TYPES=10,
	};

	entity *first_entity;
	entity *first_entity_types[NUM_ENT_TYPES];
	
	game_world()
	{
		first_entity = 0x0;
		for(int i = 0; i < NUM_ENT_TYPES; i++)
			first_entity_types[i] = 0;
	}
	
	int find_entities(vec2 pos, float radius, entity **ents, int max)
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

	int find_entities(vec2 pos, float radius, entity **ents, int max, const int* types, int maxtypes)
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
	
	void insert_entity(entity *ent)
	{
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
	
	void destroy_entity(entity *ent)
	{
		ent->set_flag(entity::FLAG_DESTROY);
	}
	
	void remove_entity(entity *ent)
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
	void snap(int snapping_client)
	{
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
			ent->snap(snapping_client);
	}
	
	void tick()
	{
		// update all objects
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick();
			
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick_defered();
		
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
};

static game_world world;

// TODO: move to seperate file
class powerup : public entity
{
public:
	static const int phys_size = 14;
	
	int type;
	int subtype; // weapon type for instance?
	int spawntick;
	powerup(int _type, int _subtype = 0);
	
	virtual void tick();
	virtual void snap(int snapping_client);
};

// projectile entity
class projectile : public entity
{
public:
	enum
	{
		PROJECTILE_FLAGS_EXPLODE = 1 << 0,
	};
	
	vec2 vel;
	entity *powner; // this is nasty, could be removed when client quits
	int lifespan;
	int owner;
	int type;
	int flags;
	int damage;
	int sound_impact;
	float force;
	
	projectile(int type, int owner, vec2 pos, vec2 vel, int span, entity* powner, int damage, int flags = 0, float force = 0.0f, int sound_impact = -1)
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
		world.insert_entity(this);
	}

	void tick()
	{
		vec2 oldpos = pos;
		vel.y += 0.25f;
		pos += vel;
		lifespan--;
		
		// check player intersection as well
		entity *targetplayer = (entity*)intersect_player(oldpos, pos, oldpos, powner);
		if(targetplayer || lifespan < 0 || col_check_point((int)pos.x, (int)pos.y))
		{
			if (lifespan >= 0)
				create_sound(pos, sound_impact);
				
			if (flags & PROJECTILE_FLAGS_EXPLODE)
				create_explosion(oldpos, owner);
			else if (targetplayer)
				targetplayer->take_damage(normalize(vel) * max(0.001f, force), damage, owner);
				
			world.destroy_entity(this);
		}
	}
	
	void snap(int snapping_client)
	{
		obj_projectile *proj = (obj_projectile *)snap_new_item(OBJTYPE_PROJECTILE, id, sizeof(obj_projectile));
		proj->x = (int)pos.x;
		proj->y = (int)pos.y;
		proj->vx = (int)vel.x; // TODO: should be an angle
		proj->vy = (int)vel.y;
		proj->type = type;
	}
};

// player entity
// TODO: move to separate file
class player : public entity
{
public:
	static const int phys_size = 28;
	
	enum // what are these?
	{
		WEAPON_PROJECTILETYPE_GUN		= 0,
		WEAPON_PROJECTILETYPE_ROCKET	= 1,
		WEAPON_PROJECTILETYPE_SHOTGUN	= 2,
	};
	
	// weapon info
	struct weaponstat
	{
		bool got;
		int ammo;
	} weapons[NUM_WEAPONS];
	int active_weapon;
	int reload_timer;
	int attack_tick;
	
	// we need a defered position so we can handle the physics correctly
	vec2 defered_pos;
	vec2 vel;
	vec2 direction;

	//
	int client_id;
	char name[64];

	// input	
	player_input previnput;
	player_input input;
	int jumped;
	
	int damage_taken_tick;

	int health;
	int armor;

	int score;
	
	bool dead;
	int die_tick;

	// hooking stuff
	enum
	{
		HOOK_RETRACTED=-1,
		HOOK_IDLE=0,
		HOOK_FLYING,
		HOOK_GRABBED
	};
	
	int hook_state; 
	player *hooked_player;
	vec2 hook_pos;
	vec2 hook_dir;

	//
	player()
	: entity(OBJTYPE_PLAYER)
	{
		reset();
	}
	
	void reset()
	{
		//equip_time = 0;
		
		release_hooked();
		release_hooks();
		
		proximity_radius = phys_size;
		name[0] = 'n';
		name[1] = 'o';
		name[2] = 'o';
		name[3] = 'b';
		name[4] = 0;
		
		pos = vec2(100.0f, 0.0f);
		vel = vec2(0.0f, 0.0f);
		direction = vec2(0.0f, 1.0f);
		client_id = -1;
		score = 0;
		dead = true;
		die_tick = 0;
	}
	
	virtual void destroy() {  }
		
	void respawn()
	{
		health = PLAYER_MAXHEALTH;
		armor = 0;
		jumped = 0;
		dead = false;
		set_flag(entity::FLAG_ALIVE);
		
		mem_zero(&input, sizeof(input));
		vel = vec2(0.0f, 0.0f);
		
		// get spawn point
		int start, num;
		map_get_type(1, &start, &num);
		
		if(num)
		{
			mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + (rand()%num), NULL, NULL);
			pos = vec2((float)sp->x, (float)sp->y);
		}
		else
			pos = vec2(100.0f, -60.0f);
		defered_pos = pos;
			
		// init weapons
		mem_zero(&weapons, sizeof(weapons));
		weapons[WEAPON_HAMMER].got = true;
		weapons[WEAPON_HAMMER].ammo = -1;
		weapons[WEAPON_GUN].got = true;
		weapons[WEAPON_GUN].ammo = 10;
		active_weapon = WEAPON_GUN;
		reload_timer = 0;
		
		create_sound(pos, SOUND_PLAYER_SPAWN);
	}
	
	bool is_grounded()
	{
		if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
			return true;
		if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
			return true;
		return false;
	}

	// releases the hooked player
	void release_hooked()
	{
		hook_state = HOOK_IDLE;
		hooked_player = 0x0;
	}

	// release all hooks to this player	
	void release_hooks()
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
	
	void handle_weapons()
	{
		// check reload timer
		if(reload_timer)
		{
			reload_timer--;
			return;
		}

		// switch weapon if wanted		
		if(input.activeweapon >= 0 && input.activeweapon < NUM_WEAPONS && weapons[input.activeweapon].got)
			active_weapon = input.activeweapon;

		if(input.fire)
		{
			if(reload_timer == 0)
			{
				// fire!
				if(weapons[active_weapon].ammo)
				{
					switch(active_weapon)
					{
						case WEAPON_HAMMER:
							break;

						case WEAPON_GUN:
							new projectile(WEAPON_PROJECTILETYPE_GUN,
								client_id,
								pos+vec2(0,0),
								direction*30.0f,
								100,
								this,
								1, 0, 0, -1);
							break;
						case WEAPON_ROCKET:
							new projectile(WEAPON_PROJECTILETYPE_ROCKET,
								client_id,
								pos+vec2(0,0),
								direction*15.0f,
								100,
								this,
								1, projectile::PROJECTILE_FLAGS_EXPLODE, 0, -1);						
							break;
						case WEAPON_SHOTGUN:
							for(int i = 0; i < 3; i++)
							{
								new projectile(WEAPON_PROJECTILETYPE_SHOTGUN,
									client_id,
									pos+vec2(0,0),
									direction*(20.0f+(i+1)*2.0f),
									100,
									this,
									1, 0, 0, -1);
							}
							break;
					}
					
					weapons[active_weapon].ammo--;
				}
				else
				{
					// click!!! click
				}
				
				attack_tick = server_tick();
				reload_timer = 10; // make this variable depending on weapon
			}
		}
	}
	
	virtual void tick()
	{
		// TODO: rework the input to be more robust
		// TODO: remove this tick count, it feels weird
		if(dead)
		{
			if(server_tick()-die_tick >= server_tickspeed()*3) // auto respawn after 3 sec
				respawn();
			if(input.fire && server_tick()-die_tick >= server_tickspeed()/2) // auto respawn after 0.5 sec
				respawn();
			return;
		}
		
		// fetch some info
		bool grounded = is_grounded();
		direction = get_direction(input.angle);
		
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
						if(p != this && distance(p->pos, new_pos) < p->phys_size)
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
					create_sound(pos, SOUND_HOOK_ATTACH);
			}
		}
		else
		{
			release_hooked();
			hook_pos = pos;
		}
			
		if(hook_state == HOOK_GRABBED)
		{
			if(hooked_player)
				hook_pos = hooked_player->pos;

			float d = distance(pos, hook_pos);
			vec2 dir = normalize(pos - hook_pos);		
			if(d > 10.0f) // TODO: fix tweakable variable
			{
				float accel = hook_drag_accel * (d/hook_length);
				vel.x = saturated_add(-hook_drag_speed, hook_drag_speed, vel.x, -accel*dir.x*0.75f);
				vel.y = saturated_add(-hook_drag_speed, hook_drag_speed, vel.y, -accel*dir.y);
			}
		}
			
		// fix influence of other players, collision + hook
		// TODO: loop thru players only
		for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
		{
			if(ent && ent->objtype == OBJTYPE_PLAYER)
			{
				player *p = (player*)ent;
				if(p == this)
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
		handle_weapons();
		
		// add gravity
		vel.y += gravity;
		
		// do the move
		defered_pos = pos;
		move_box(&defered_pos, &vel, vec2(phys_size, phys_size), 0);
		return;
	}
	
	virtual void tick_defered()
	{
		// apply the new position
		pos = defered_pos;
	}
	
	void die()
	{
		create_sound(pos, SOUND_PLAYER_DIE);
		
		release_hooked();
		release_hooks();
		
		// TODO: insert timer here
		dead = true;
		die_tick = server_tick();
		clear_flag(entity::FLAG_ALIVE);
	}
	
	virtual bool take_damage(vec2 force, int dmg, int from)
	{
		vel += force;

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
		
		// create healthmod indicator
		create_damageind(pos, normalize(force), dmg);
		
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
			
			die();
			return false;
		}

		if (dmg > 2)
			create_sound(pos, SOUND_PLAYER_PAIN_LONG);
		else
			create_sound(pos, SOUND_PLAYER_PAIN_SHORT);

		// spawn blood?
		return true;
	}

	virtual void snap(int snaping_client)
	{
		obj_player *player = (obj_player *)snap_new_item(OBJTYPE_PLAYER, client_id, sizeof(obj_player));

		player->x = (int)pos.x;
		player->y = (int)pos.y;
		player->vx = (int)vel.x;
		player->vy = (int)vel.y;
		player->emote = EMOTE_NORMAL;

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
			
		player->angle = input.angle;
		player->score = score;
	}
};

// POWERUP ///////////////////////

powerup::powerup(int _type, int _subtype)
: entity(OBJTYPE_POWERUP)
{
	//static int current_id = 0;
	type = _type;
	subtype = _subtype;
	// set radius (so it can collide and be hooked and stuff)
	proximity_radius = phys_size;
	spawntick = -1;
	
	// TODO: should this be done here?
	world.insert_entity(this);
}

void powerup::tick()
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
	player* pplayer = intersect_player(pos, pos + vec2(0,16), meh, 0);
	if (pplayer)
	{
		// player picked us up, is someone was hooking us, let them go
		int respawntime = -1;
		switch (type)
		{
		case POWERUP_TYPE_HEALTH:
			if(pplayer->health < PLAYER_MAXHEALTH)
			{
				pplayer->health = min((int)PLAYER_MAXHEALTH, pplayer->health + 1);
				respawntime = 20;
			}
			break;
		case POWERUP_TYPE_ARMOR:
			if(pplayer->armor < PLAYER_MAXARMOR)
			{
				pplayer->armor = min((int)PLAYER_MAXARMOR, pplayer->armor + 1);
				respawntime = 20;
			}
			break;
				
		case POWERUP_TYPE_WEAPON:
			if(subtype >= 0 && subtype < NUM_WEAPONS)
			{
				if(pplayer->weapons[subtype].ammo < 10 || !pplayer->weapons[subtype].got)
				{
					pplayer->weapons[subtype].got = true;
					pplayer->weapons[subtype].ammo = min(10, pplayer->weapons[subtype].ammo + 5);
					respawntime = 20;
				}
			}
			break;
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

static player players[MAX_CLIENTS];

player *get_player(int index)
{
	return &players[index];
}

void create_damageind(vec2 p, vec2 dir, int amount)
{
	float a = get_angle(dir);
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

void create_explosion(vec2 p, int owner, bool bnodamage)
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
		int num = world.find_entities(p, radius, ents, 64);
		for(int i = 0; i < num; i++)
		{
			vec2 diff = ents[i]->pos - p;
			vec2 forcedir(0,1);
			if (length(diff))
				forcedir = normalize(diff);
			float l = length(diff);
			float dmg = 5 * (1 - (l/radius));
			if((int)dmg)
				ents[i]->take_damage(forcedir*dmg*2, (int)dmg, owner);
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

void create_sound(vec2 pos, int sound, int loopingflags)
{
	if (sound < 0)
		return;

	// create a sound
	ev_sound *ev = (ev_sound *)events.create(EVENT_SOUND, sizeof(ev_sound));
	ev->x = (int)pos.x;
	ev->y = (int)pos.y;
	ev->sound = sound | loopingflags;
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

	if(debug_bots)
	{
		static int count = 0;
		if(count >= 0)
		{
			count++;
			if(count == 10)
			{
				for(int i = 0; i < 1; i++)
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
	players[client_id].previnput = players[client_id].input;
	players[client_id].input = *(player_input*)input;
}

void mods_client_enter(int client_id)
{
	players[client_id].reset();
	players[client_id].client_id = client_id;
	players[client_id].respawn();
	world.insert_entity(&players[client_id]);
	
	client_info info; // fetch login name
	if(server_getclientinfo(client_id, &info))
		strcpy(players[client_id].name, info.name);
	else
		strcpy(players[client_id].name, "(bot)");
	
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
}

void mods_client_drop(int client_id)
{
	players[client_id].client_id = -1;
	world.remove_entity(&players[client_id]);
}

void mods_message(int msg, int client_id)
{
	if(msg == MSG_SAY)
	{
		msg_pack_start(MSG_CHAT, MSGFLAG_VITAL);
		msg_pack_int(client_id);
		msg_pack_string(msg_unpack_string(), 512);
		msg_pack_end();
		server_send_msg(-1);
	}
}

void mods_init()
{
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
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_GUN;
			break;
		case ITEM_WEAPON_SHOTGUN:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_SHOTGUN;
			break;
		case ITEM_WEAPON_ROCKET:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_ROCKET;
			break;
		case ITEM_WEAPON_HAMMER:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_MELEE;
			break;
		
		case ITEM_HEALTH_1:
			type = POWERUP_TYPE_HEALTH;
			break;
		
		case ITEM_ARMOR_1:
			type = POWERUP_TYPE_ARMOR;
			break;
		};
		
		powerup *ppower = new powerup(type, subtype);
		ppower->pos = vec2(it->x, it->y);
	}
}

void mods_shutdown() {}
void mods_presnap() {}
void mods_postsnap() {}
