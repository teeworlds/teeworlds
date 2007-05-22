#include <stdlib.h>
#include <string.h>
#include "game.h"

using namespace baselib;

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
void create_healthmod(vec2 p, int amount);
void create_explosion(vec2 p, int owner = -1, bool bnodamage = false);
void create_smoke(vec2 p);
void create_sound(vec2 pos, int sound, int loopflags = 0);
class player* intersect_player(vec2 pos0, vec2 pos1, vec2& new_pos, class entity* notthis = 0);

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

// TODO: rewrite this smarter!
bool intersect_line(vec2 pos0, vec2 pos1, vec2 *out)
{
	float d = distance(pos0, pos1);
	
	for(float f = 0; f < d; f++)
	{
		float a = f/d;
		vec2 pos = mix(pos0, pos1, a);
		if(col_check_point(pos))
		{
			if(out)
				*out = pos;
			return true;
		}
	}
	if(out)
		*out = pos1;
	return false;
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
		num_events = 0;
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
/*
template<typename T, int SIZE>
class pool
{
	struct element
	{
		int next_free;
		T data;
	};
	
	element elements[SIZE];
	int first_free;
public:
	pool()
	{
		first_free = 0;
		for(int i = 0; i < SIZE; i++)
			elements[i].next_free = i+1;
		elements[SIZE-1].next_free = -1;
	}
};*/

// a basic entity
class entity
{
private:
	friend class game_world;
	friend class player;
	entity *prev_entity;
	entity *next_entity;
	int index;
	
public:
	vec2 pos;
	float proximity_radius;
	unsigned flags;
	int objtype;

	enum
	{
		FLAG_DESTROY=0x00000001,
	};

	entity(int objtype)
	{
		this->objtype = objtype;
		pos = vec2(0,0);
		flags = 0;
		proximity_radius = 0;
	}
	
	virtual ~entity()
	{
	}

	virtual void destroy() { delete this; }
	virtual void tick() {}
	virtual void snap(int snapping_client) {}
		
	virtual bool take_damage(vec2 force, int dmg, int from) { return true; }
};

class powerup : public entity
{
public:
	static const int phys_size = 14;
	enum
	{
		POWERUP_FLAG_HOOKABLE = 1 << 0,
	};
	vec2 vel;
	class player* playerhooked;
	int type;
	int id;
	int subtype; // weapon type for instance?
	int numitems; // number off powerup items
	int flags;
	int spawntick;
	powerup(int _type, int _subtype = 0, int _numitems = 0, int _flags = 0);

	static void spawnrandom(int _lifespan);

	void tick();

	void snap(int snapping_client);
};

// game world. handles all entities
class game_world
{
public:
	entity *first_entity;
	game_world()
	{
		first_entity = 0x0;
	}
	
	int find_entities(vec2 pos, float radius, entity **ents, int max)
	{
		int num = 0;
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
		{
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
		for(entity *ent = first_entity; ent; ent = ent->next_entity)
		{
			for (int i = 0; i < maxtypes; i++)
			{
				if (ent->objtype != types[i])
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
	}
	
	void destroy_entity(entity *ent)
	{
		ent->flags |= entity::FLAG_DESTROY;
		// call destroy
		//remove_entity(ent);
		//ent->destroy();
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

// projectile entity
class projectile : public entity
{
public:
	enum
	{
		PROJECTILE_FLAGS_EXPLODE = 1 << 0,
	};
	vec2 vel;
	entity* powner;
	int lifespan;
	int id;
	int owner;
	int type;
	int flags;
	int damage;
	int sound_impact;
	float force;
	
	projectile(int type, int owner, vec2 pos, vec2 vel, int span, entity* powner, int damage, int flags = 0, float force = 0.0f, int sound_impact = -1) :
		entity(OBJTYPE_PROJECTILE)
	{
		static int current_id = 0;
		this->id = current_id++;
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
		entity* targetplayer = (entity*)intersect_player(oldpos, pos, oldpos, powner);
		if(targetplayer || lifespan < 0 || col_check_point((int)pos.x, (int)pos.y))
		{
			if (lifespan >= 0)
				create_sound(pos, sound_impact);
			if (flags & PROJECTILE_FLAGS_EXPLODE)
			{
				create_explosion(oldpos, owner);
			}
			else if (targetplayer)
			{
				targetplayer->take_damage(normalize(vel) * force, damage, owner);
			}
			world.destroy_entity(this);
		}
	}
	
	void snap(int snapping_client)
	{
		obj_projectile *proj = (obj_projectile *)snap_new_item(OBJTYPE_PROJECTILE, id, sizeof(obj_projectile));
		proj->x = (int)pos.x;
		proj->y = (int)pos.y;
		proj->vx = (int)vel.x;
		proj->vy = (int)vel.y;
		proj->type = type;
	}
};

// player entity
class player : public entity
{
public:
	static const int phys_size = 28;
	enum
	{
		WEAPON_NEEDRELOAD		= 1 << 0,
		WEAPON_DROPONUNEQUIP	= 1 << 1,
		WEAPON_DRAWSAMMO		= 1 << 2,
		WEAPON_HASSECONDARY		= 1 << 3,
		WEAPON_ISACTIVE			= 1 << 4, // has the item
		WEAPON_AUTOFIRE			= 1 << 5,

		WEAPON_PROJECTILETYPE_GUN		= 0,
		WEAPON_PROJECTILETYPE_ROCKET	= 1,
		WEAPON_PROJECTILETYPE_SHOTGUN	= 2,

		// Gun


		// modifiers
		MODIFIER_HASACTIVATIONS			= 1 << 0,
		MODIFIER_TIMELIMITED			= 1 << 1,
		MODIFIER_ISACTIVE				= 1 << 2,
		MODIFIER_NEEDSACTIVATION		= 1 << 3,

		MODIFIER_RETURNFLAGS_OVERRIDEWEAPON		= 1 << 0,
		MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY	= 1 << 1,
		MODIFIER_RETURNFLAGS_OVERRIDEPOSITION	= 1 << 2,
		MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY	= 1 << 3,
	};
	class weapon
	{
	public:
		entity* hitobjects[10];
		int numobjectshit; // for melee, so we don't hit the same object more than once per bash
		int weapontype;
		int equiptime;
		int unequiptime;
		int numammo;
		int magsize;
		int nummagazines;
		int flags;
		int firetime;
		int reloadtime;
		int projectileclass;
		int damage;
		int sound_fire;
		int sound_equip;
		int sound_impact;
		int sound_impact_projectile;
		int visualtimeattack;
		float projectilevel;
		float projectilespan;
		float reach; // for melee
		float force;
		float recoilforce;
		float projoffsety;
		float projoffsetx;
		
		weapon()
		{
			weapontype = 0;
			numammo = 0;
			flags = 0;
			reloadtime = 0;
			projectileclass = 0;
			numobjectshit = 0;
			reach = 0.0f;
			force = 5.0f;
			damage = 1;
			sound_fire = -1;
			sound_equip = -1;
			sound_impact = -1;
			sound_impact_projectile = -1,
			visualtimeattack = 3;
			recoilforce = 0.0f;
			projoffsety = 0.0f;
			projoffsetx = 0.0f;
		}

		void setgun(int ammo = 10)
		{
			weapontype = WEAPON_TYPE_GUN;
			flags = 0;//WEAPON_DRAWSAMMO;
			numammo = ammo;
			projectileclass = WEAPON_PROJECTILETYPE_GUN;
			firetime = SERVER_TICK_SPEED/10;
			magsize = 0;
			projectilevel = 30.0f;
			projectilespan = 50.0f * 1.0f;
			sound_fire = SOUND_FIRE_GUN;
			sound_equip = SOUND_EQUIP_GUN;
			sound_impact_projectile = SOUND_IMPACT_PROJECTILE_GUN;
			projoffsety = -10.0f;
			projoffsetx = 24.0f;
		}

		void setrocket(int ammo = 10)
		{
			weapontype = WEAPON_TYPE_ROCKET;
			flags = WEAPON_DRAWSAMMO;
			numammo = ammo;
			projectileclass = WEAPON_PROJECTILETYPE_ROCKET;
			projectilevel = 15.0f;
			projectilespan = 50.0f * 5.0f;
			firetime = SERVER_TICK_SPEED * 4/5;
			magsize = 0;
			recoilforce = 5.0f;
			sound_fire = SOUND_FIRE_ROCKET;
			sound_equip = SOUND_EQUIP_ROCKET;
			sound_impact_projectile = SOUND_IMPACT_PROJECTILE_ROCKET;
			projoffsety = -17.0f;
			projoffsetx = 24.0f;
		}

		/*void setsniper(int ammo = 10)
		{
			weapontype = WEAPON_TYPE_SNIPER;
			flags = WEAPON_DRAWSAMMO | WEAPON_HASSECONDARY | WEAPON_NEEDRELOAD;
			numammo = ammo;
			projectileclass = WEAPON_PROJECTILETYPE_SNIPER;
			projectilevel = 30.0f;
			projectilespan = 50.0f * 5.0f;
			firetime = SERVER_TICK_SPEED;
			reloadtime = SERVER_TICK_SPEED/2;
			magsize = 2;
			recoilforce = 20.0f;
		}*/

		void setshotgun(int ammo = 10)
		{
			weapontype = WEAPON_TYPE_SHOTGUN;
			flags = WEAPON_DRAWSAMMO | WEAPON_NEEDRELOAD;
			numammo = ammo;
			projectileclass = WEAPON_PROJECTILETYPE_SHOTGUN;
			projectilevel = 30.0f;
			projectilespan = 50.0f * 5.0f;
			firetime = SERVER_TICK_SPEED/2;
			reloadtime = SERVER_TICK_SPEED/2;
			magsize = 2;
			damage = 3;
			recoilforce = 5.0f;
			sound_fire = SOUND_FIRE_SHOTGUN;
			sound_equip = SOUND_EQUIP_SHOTGUN;
			sound_impact_projectile = SOUND_IMPACT_PROJECTILE_SHOTGUN;
			projoffsety = -17.0f;
			projoffsetx = 24.0f;
		}

		void setmelee(int ammo = 10)
		{
			weapontype = WEAPON_TYPE_MELEE;
			flags = 0;//WEAPON_AUTOFIRE;
			numammo = ammo;
			projectileclass = -1;
			firetime = SERVER_TICK_SPEED/5;
			reloadtime = 0;
			magsize = 2;
			numobjectshit = 0;
			reach = 15.0f;
			damage = 1;
			sound_fire = SOUND_FIRE_MELEE;
			sound_equip = SOUND_EQUIP_MELEE;
			sound_impact = SOUND_PLAYER_IMPACT;
		}

		void settype()
		{
			switch(weapontype)
			{
			case WEAPON_TYPE_GUN:
				{
					setgun();
					break;
				}
			case WEAPON_TYPE_ROCKET:
				{
					setrocket();
					break;
				}
			/*case WEAPON_TYPE_SNIPER:
				{
					setsniper();
					break;
				}*/
			case WEAPON_TYPE_SHOTGUN:
				{
					setshotgun();
					break;
				}
			case WEAPON_TYPE_MELEE:
				{
					setmelee();
					break;
				}
			default:
				break;
			}
		}

		int activate(player* player)
		{
			// create sound event for fire
			int projectileflags = 0;
			create_sound(player->pos, sound_fire);

			switch (weapontype)
			{
			case WEAPON_TYPE_ROCKET:
				projectileflags |= projectile::PROJECTILE_FLAGS_EXPLODE;
			case WEAPON_TYPE_GUN:
			//case WEAPON_TYPE_SNIPER:
			case WEAPON_TYPE_SHOTGUN:
				{
					if (flags & WEAPON_DRAWSAMMO)
						numammo--;
					// Create projectile
					new projectile(projectileclass, player->client_id, player->pos+vec2(0,projoffsety)+player->direction*projoffsetx, player->direction*projectilevel, projectilespan, player, damage, projectileflags, force, sound_impact_projectile);
					// recoil force if any
					if (recoilforce > 0.0f)
					{
						vec2 dir(player->direction.x,0.5);
						if (dir.x == 0.0f)
							dir.x = 0.5f;
						else
							dir = normalize(dir);
						player->vel -= dir * recoilforce;
					}
					return firetime;
				}
			case WEAPON_TYPE_MELEE:
				{
					// Start bash sequence
					numobjectshit = 0;
					return firetime;
				}
			default:
				return 0;
			}
		}

		void update(player* owner, int fire_timeout)
		{
			switch(weapontype)
			{
			case WEAPON_TYPE_MELEE:
				{
					// No more melee
					if (fire_timeout <= 0)
						return;

					// only one that needs update (for now)
					// do selection for the weapon and bash anything in it
					// check if we hit anything along the way
					int type = OBJTYPE_PLAYER;
					entity *ents[64];
					vec2 dir = owner->pos + owner->direction * reach;
					float radius = length(dir * 0.5f);
					vec2 center = owner->pos + dir * 0.5f;
					int num = world.find_entities(center, radius, ents, 64, &type, 1);
					
					for (int i = 0; i < num; i++)
					{
						// Check if entity is a player
						if (ents[i] == owner)
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
						if (distance(ents[i]->pos, owner->pos) > (owner->phys_size * 2.0f))
							continue;
					
						// hit a player, give him damage and stuffs...
						// create sound for bash
						create_sound(ents[i]->pos, sound_impact);

						// set his velocity to fast upward (for now)
						create_smoke(ents[i]->pos);
						hitobjects[numobjectshit++] = ents[i];
						ents[i]->take_damage(vec2(0,10.0f), damage, owner->client_id);
						player* target = (player*)ents[i];
						vec2 dir;
						if (length(target->pos - owner->pos) > 0.0f)
							dir = normalize(target->pos - owner->pos);
						else
							dir = vec2(0,-1);
						target->vel += dir * 10.0f + vec2(0,-10.0f);
					}
					break;
				}
			default:
				break;
			}
		}
	};

	class modifier
	{
	public:
		vec2 activationdir;
		entity* hitobjects[10];
		int numobjectshit;
		float velocity;
		int modifiertype;
		int duration;
		int numactivations;
		int activationtime;
		int cooldown;
		int movetime;
		int visualtimeattack;
		int currentactivation;
		int currentmovetime;
		int currentcooldown;
		int damage;
		int flags;
		int sound_impact;
		int sound_activate;

		modifier()
		{
			modifiertype = 0;
			duration = 0;
			numobjectshit = 0;
			numactivations = 0;
			activationtime = 0;
			cooldown = 0;
			movetime = 0;
			currentactivation = 0;
			currentmovetime = 0;
			currentcooldown =0;
			damage = 0;
			flags = 0;
			activationdir = vec2(0.0f, 1.0f);
			velocity = 0.0f;
			visualtimeattack = 0;
			sound_impact = -1;
		}

		void setninja()
		{
			modifiertype = MODIFIER_TYPE_NINJA;
			duration = SERVER_TICK_SPEED * 15;
			numactivations = -1;
			movetime = SERVER_TICK_SPEED / 5;
			activationtime = SERVER_TICK_SPEED / 2;
			cooldown = SERVER_TICK_SPEED;
			currentactivation = 0;
			currentmovetime = 0;
			numobjectshit = 0;
			damage = 3;
			flags = MODIFIER_TIMELIMITED | MODIFIER_NEEDSACTIVATION;
			velocity = 50.0f;
			visualtimeattack = 3;
			sound_impact = SOUND_PLAYER_IMPACT_NINJA;
			sound_activate = SOUND_FIRE_NINJA;
		}

		void settimefield()
		{
			modifiertype = MODIFIER_TYPE_TIMEFIELD;
			duration = SERVER_TICK_SPEED * 10;
			numactivations = -1;
			activationtime = SERVER_TICK_SPEED;
			numobjectshit = 0;
			currentactivation = 0;
			flags = MODIFIER_TIMELIMITED;
			velocity = 0.0f;
		}

		void settype()
		{
			switch (modifiertype)
			{
			case MODIFIER_TYPE_NINJA:
				{
					setninja();
					break;
				}
			case MODIFIER_TYPE_TIMEFIELD:
				{
					settimefield();
					break;
				}
			default:
				break;
			}
		}

		int updateninja(player* player)
		{
			if (currentactivation <= 0)
				return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
			currentactivation--;
			currentmovetime--;
			
			if (currentmovetime == 0)
			{	
				// reset player velocity
				player->vel *= 0.2f;
				//return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
			}
			
			if (currentmovetime > 0)
			{
				// Set player velocity
				player->vel = activationdir * velocity;
				vec2 oldpos = player->pos;
				move_box(&player->pos, &player->vel, vec2(player->phys_size, player->phys_size), 0.0f);
				// reset velocity so the client doesn't predict stuff
				player->vel = vec2(0.0f,0.0f);
				if ((currentmovetime % 2) == 0)
				{
					create_smoke(player->pos);
				}
				
				// check if we hit anything along the way
				{
					int type = OBJTYPE_PLAYER;
					entity *ents[64];
					vec2 dir = player->pos - oldpos;
					float radius = length(dir * 0.5f);
					vec2 center = oldpos + dir * 0.5f;
					int num = world.find_entities(center, radius, ents, 64, &type, 1);
					
					for (int i = 0; i < num; i++)
					{
						// Check if entity is a player
						if (ents[i] == player)
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
						if (distance(ents[i]->pos, player->pos) > (player->phys_size * 2.0f))
							continue;
					
						// hit a player, give him damage and stuffs...
						create_sound(ents[i]->pos, sound_impact);
						// set his velocity to fast upward (for now)
						hitobjects[numobjectshit++] = ents[i];
						ents[i]->take_damage(vec2(0,10.0f), damage, player->client_id);
					}
				}
				return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON | MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY | MODIFIER_RETURNFLAGS_OVERRIDEPOSITION|MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY;
			}


			// move char, and check intersection from us to target
			return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON | MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY;
		}

		int activateninja(player* player)
		{
			// ok then, activate ninja
			activationdir = player->direction;
			currentactivation = activationtime;
			currentmovetime = movetime;
			currentcooldown = cooldown;
			// reset hit objects
			numobjectshit = 0;

			create_sound(player->pos, SOUND_FIRE_NINJA);

			return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
		}

		int activate(player* player)
		{
			if (flags & MODIFIER_NEEDSACTIVATION)
			{
				if (!numactivations)
					return 0;
				numactivations--;
			}

			switch (modifiertype)
			{
			case MODIFIER_TYPE_NINJA:
				{
					return activateninja(player);
				}
			/*case MODIFIER_TYPE_TIMEFIELD:
				{
					updatetimefield();
					break;
				}*/
			default:
				return 0;
			}
		}
		int update(player* player)
		{
			switch (modifiertype)
			{
			case MODIFIER_TYPE_NINJA:
				{
					return updateninja(player);
				}
			/*case MODIFIER_TYPE_TIMEFIELD:
				{
					updatetimefield();
					break;
				}*/
			default:
				return 0;
			}
		}
	};

	enum
	{
		PLAYER_FLAGS_ISRELOADING = 1 << 0,
		PLAYER_FLAGS_ISEQUIPPING = 1 << 1,
	};

	weapon lweapons[WEAPON_NUMWEAPONS];
	modifier modifiers[MODIFIER_NUMMODIFIERS];
	int iactiveweapon;
	int inextweapon;
	int equip_time;

	int client_id;
	int flags;

	char name[32];
	player_input previnput;
	player_input input;
	int tick_count;
	int damage_taken_tick;

	vec2 vel;
	vec2 direction;

	int jumped;
	int airjumped;

	//int firing;
	int hooking;

	int fire_timeout;
	int reload_timeout;

	int health;
	int armor;

	int score;

	// sounds
	int sound_player_jump;
	int sound_player_land;
	int sound_player_hurt_short;
	int sound_player_hurt_long;
	int sound_player_spawn;
	int sound_player_chain_loop;
	int sound_player_chain_impact;
	int sound_player_impact;
	int sound_player_impact_ninja;
	int sound_player_die;
	int sound_player_switchweapon;

	player* phookedplayer;
	powerup* phookedpowerup;
	int numhooked;
	vec2 hook_pos;
	vec2 hook_dir;

	player() :
		entity(OBJTYPE_PLAYER)
	{
		reset();
		
		//firing = 0;
		// setup weaponflags and stuff
		lweapons[WEAPON_TYPE_GUN].setgun();
		lweapons[WEAPON_TYPE_ROCKET].setrocket();
		//lweapons[WEAPON_TYPE_SNIPER].setsniper();
		lweapons[WEAPON_TYPE_SHOTGUN].setshotgun();
		lweapons[WEAPON_TYPE_MELEE].setmelee();

		modifiers[MODIFIER_TYPE_NINJA].setninja();
		modifiers[MODIFIER_TYPE_TIMEFIELD].settimefield();
		//modifiers[MODIFIER_TYPE_NINJA].flags |= MODIFIER_ISACTIVE;

		sound_player_jump = SOUND_PLAYER_JUMP;
		sound_player_hurt_short = SOUND_PLAYER_HURT_SHORT;
		sound_player_hurt_long = SOUND_PLAYER_HURT_LONG;
		sound_player_spawn = SOUND_PLAYER_SPAWN;
		sound_player_chain_loop = SOUND_PLAYER_CHAIN_LOOP;
		sound_player_chain_impact = SOUND_PLAYER_CHAIN_IMPACT;
		sound_player_impact = SOUND_PLAYER_IMPACT;
		sound_player_impact_ninja = SOUND_PLAYER_IMPACT_NINJA;
		sound_player_die = SOUND_PLAYER_DIE;
		sound_player_switchweapon = SOUND_PLAYER_SWITCHWEAPON;
		sound_player_land = SOUND_PLAYER_LAND;
	}
	
	void reset()
	{
		equip_time = 0;
		phookedplayer = 0;
		numhooked = 0;
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
		tick_count = 0;
		score = 0;
		flags = 0;		
	}
	
	virtual void destroy() { flags = 0; }
		
	void respawn()
	{
		health = PLAYER_MAXHEALTH;
		armor = 0;
		
		hooking = 0;
		phookedplayer = 0;
		phookedpowerup = 0;
		numhooked = 0;
		fire_timeout = 0;
		reload_timeout = 0;
		iactiveweapon = 0;
		inextweapon = -1;
		equip_time = 0;
		jumped = 0;
		airjumped = 0;
		mem_zero(&input, sizeof(input));
		vel = vec2(0.0f, 0.0f);
		
		int start, num;
		map_get_type(1, &start, &num);
		
		if(num)
		{
			mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + (rand()%num), NULL, NULL);
			pos = vec2(sp->x, sp->y);
		}
		else
			pos = vec2(100.0f, -60.0f);

		// reset active flags
		for (int i = 0; i < WEAPON_NUMWEAPONS; i++)
		{
			// reset and remove
			lweapons[i].settype();
			lweapons[i].flags &= ~WEAPON_ISACTIVE;
		}


		// TEMP REMOVE
		
		/*for (int i = 0; i < WEAPON_NUMWEAPONS; i++)
		{
			lweapons[i].settype();
			lweapons[i].flags |= WEAPON_ISACTIVE;
		}*/
		lweapons[WEAPON_TYPE_MELEE].flags |= WEAPON_ISACTIVE;
		// Add gun as default weapon
		iactiveweapon = WEAPON_TYPE_GUN;
		lweapons[WEAPON_TYPE_GUN].numammo = 10;
		lweapons[WEAPON_TYPE_GUN].flags |= WEAPON_ISACTIVE;

		create_sound(pos, sound_player_spawn);
	}
	
	bool is_grounded()
	{
		if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
			return true;
		if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
			return true;
		return false;
	}

	// Disable weapon activation if this returns true
	int handlemodifiers()
	{
		int returnflags = 0;
		for (int i = 0; i < MODIFIER_NUMMODIFIERS; i++)
		{
			if (modifiers[i].flags & MODIFIER_ISACTIVE)
			{
				modifiers[i].duration--;
				modifiers[i].currentcooldown--;

				// Check if it should activate
				if (modifiers[i].currentcooldown <= 0 && 
					(modifiers[i].flags & MODIFIER_NEEDSACTIVATION) &&
					input.fire && !(previnput.fire))
				{
					returnflags |= modifiers[i].activate(this);
				}

				returnflags |= modifiers[i].update(this);

				// remove active if timed out
				if (modifiers[i].duration <= 0 && modifiers[i].currentactivation <= 0)
					modifiers[i].flags &= ~MODIFIER_ISACTIVE;
			}
		}
		return returnflags;
	}

	void handleweapon()
	{
		// handle weapon
		if(input.fire && (!previnput.fire || lweapons[iactiveweapon].flags & WEAPON_AUTOFIRE) && 
			!(flags & PLAYER_FLAGS_ISEQUIPPING) && !reload_timeout)
		{
			if(fire_timeout == 0)
			{
				if (lweapons[iactiveweapon].numammo || !(lweapons[iactiveweapon].flags & WEAPON_DRAWSAMMO))
				{
					// Decrease ammo
					fire_timeout = lweapons[iactiveweapon].activate(this);
				}
				else if ((lweapons[iactiveweapon].flags & WEAPON_NEEDRELOAD) && lweapons[iactiveweapon].nummagazines)
				{
					// reload
					reload_timeout = lweapons[iactiveweapon].reloadtime;
					lweapons[iactiveweapon].nummagazines--;
					lweapons[iactiveweapon].numammo = lweapons[iactiveweapon].magsize;
				}
			}
		}

		// update active weapon
		lweapons[iactiveweapon].update(this, fire_timeout);
	}

	void handlehook()
	{
		// handle hook
		if(input.hook)
		{
			if(hooking == 0)
			{
				hooking = 1;
				hook_pos = pos;
				hook_dir = direction;
				// Sound
				create_sound(pos, sound_player_chain_loop, SOUND_LOOPFLAG_STARTLOOP);
			}
			else if(hooking == 1)
			{
				vec2 new_pos = hook_pos+hook_dir*hook_fire_speed;

				// Check against other players and powerups first
				player* targetplayer = 0;
				powerup* targetpowerup = 0;
				{
					static const int typelist[2] = { OBJTYPE_PLAYER, OBJTYPE_POWERUP};
					entity *ents[64];
					vec2 dir = new_pos - hook_pos;
					float radius = length(dir * 0.5f);
					vec2 center = hook_pos + dir * 0.5f;
					int num = world.find_entities(center, radius, ents, 64,typelist,2);
					
					for (int i = 0; i < num; i++)
					{
						// Check if entity is a player
						if (ents[i] == this || (targetplayer && targetpowerup))
							continue;

						if (!targetplayer && ents[i]->objtype == OBJTYPE_PLAYER)
						{
							// temp, set hook pos to our position
							if (((player*)ents[i])->phookedplayer != this)
								targetplayer = (player*)ents[i];
						}
						else if (!targetpowerup && ents[i]->objtype == OBJTYPE_POWERUP && 
							(((powerup*)ents[i])->flags & powerup::POWERUP_FLAG_HOOKABLE))
						{
							targetpowerup = (powerup*)ents[i];
						}
					}
				}

				//player* targetplayer = intersect_player(hook_pos, hook_pos, new_pos, this);
				if (targetplayer)
				{
					// So he can't move "normally"
					new_pos = targetplayer->pos;
					phookedplayer = targetplayer;
					targetplayer->numhooked++;
					hooking = 3;
					create_sound(pos, sound_player_chain_impact);

					// stop looping chain sound
					create_sound(pos, sound_player_chain_loop, SOUND_LOOPFLAG_STOPLOOP);
				}
				else if (targetpowerup)
				{
					new_pos = targetpowerup->pos;
					phookedpowerup = targetpowerup;
					phookedpowerup->playerhooked = this;
					hooking = 4;
					create_sound(pos, sound_player_chain_impact);

					// stop looping chain sound
					create_sound(pos, sound_player_chain_loop, SOUND_LOOPFLAG_STOPLOOP);
				}
				else if(intersect_line(hook_pos, new_pos, &new_pos))
				{
					hooking = 2;
					create_sound(pos, sound_player_chain_impact);

					// stop looping chain sound
					create_sound(pos, sound_player_chain_loop, SOUND_LOOPFLAG_STOPLOOP);
				}
				else if(distance(pos, new_pos) > hook_length)
				{
					hooking = -1;
					create_sound(pos, sound_player_chain_loop, SOUND_LOOPFLAG_STOPLOOP);
				}
				
				hook_pos = new_pos;
			}
			else if(hooking == 2)
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
			else if (hooking == 3)
			{
				// hmm, force the targetplayer towards us if possible, otherwise us towards them if they are already hooked
				if (phookedplayer)
				{
					if (phookedplayer->hooking > 1)
					{
						// Drag us towards target player
						vec2 hookvel = normalize(hook_pos-pos)*hook_drag_accel;
						if((hookvel.x < 0 && input.left) || (hookvel.x > 0 && input.right)) 
							hookvel.x *= 0.95f;
						else
							hookvel.x *= 0.75f;

						// Apply the velocity
						// the hook will boost it's power if the player wants to move
						// in that direction. otherwise it will dampen everything abit
						vec2 new_vel = vel+hookvel;
						
						// check if we are under the legal limit for the hook
						if(length(new_vel) < hook_drag_speed || length(new_vel) < length(vel))
							vel = new_vel; // no problem. apply
					}
					else
					{
						// Drag targetplayer towards us
						vec2 hookvel = normalize(pos-hook_pos)*hook_drag_accel;

						// Apply the velocity
						// the hook will boost it's power if the player wants to move
						// in that direction. otherwise it will dampen everything abit
						vec2 new_vel = phookedplayer->vel+hookvel;
						
						// check if we are under the legal limit for the hook
						if(length(new_vel) < hook_drag_speed || length(new_vel) < length(vel))
							phookedplayer->vel = new_vel; // no problem. apply
					}
					hook_pos = phookedplayer->pos;
					// if hooked player dies, release the hook
				}
				else
				{
					hooking = -1;
					phookedplayer = 0;
				}
			}
			else if (hooking == 4)
			{
				// Have a powerup, drag it towards us
				vec2 hookvel = normalize(pos-hook_pos)*hook_drag_accel;

				// Apply the velocity
				// the hook will boost it's power if the player wants to move
				// in that direction. otherwise it will dampen everything abit
				vec2 new_vel = phookedpowerup->vel+hookvel;
				
				// check if we are under the legal limit for the hook
				if(length(new_vel) < hook_drag_speed || length(new_vel) < length(vel))
					phookedpowerup->vel = new_vel; // no problem. apply
				hook_pos = phookedpowerup->pos;
			}
		}
		else
		{
			hooking = 0;
			hook_pos = pos;
			if (phookedplayer)
			{
				phookedplayer->numhooked--;
				phookedplayer = 0;
			}
		}
	}

	void getattackticks(int& curattack, int& attacklen, int& visualtimeattack)
	{
		// time left from current attack (if any)
		// first check modifiers (ninja...)
		for (int i = 0; i < MODIFIER_NUMMODIFIERS; i++)
		{
			if ((modifiers[i].flags & (MODIFIER_ISACTIVE | MODIFIER_NEEDSACTIVATION)) == (MODIFIER_ISACTIVE | MODIFIER_NEEDSACTIVATION))
			{
				curattack = modifiers[i].currentactivation;
				attacklen = modifiers[i].activationtime;
				visualtimeattack = modifiers[i].visualtimeattack;
				return;
			}
		}

		// otherwise current fire timeout
		curattack = fire_timeout;
		attacklen = lweapons[iactiveweapon].firetime;
		visualtimeattack = lweapons[iactiveweapon].visualtimeattack;
	}
	
	virtual void tick()
	{
		tick_count++;
		
		// fetch some info
		bool grounded = is_grounded();
		direction = get_direction(input.angle);
		
		// decrease reload timer
		if(fire_timeout)
			fire_timeout--;
		if (reload_timeout)
			reload_timeout--;

		// Switch weapons
		if (flags & PLAYER_FLAGS_ISEQUIPPING)
		{
			equip_time--;
			if (equip_time <= 0)
			{
				if (inextweapon >= 0)
				{
					equip_time = SERVER_TICK_SPEED * lweapons[inextweapon].equiptime;
					iactiveweapon = inextweapon;
					inextweapon = -1;

					// Send switch weapon event to client?
				}
				else
				{
					flags &= ~PLAYER_FLAGS_ISEQUIPPING;
				}
			}
		}
		else if (input.activeweapon && iactiveweapon != (input.activeweapon & ~0x80000000))
		{
			input.activeweapon &= ~0x80000000;
			// check which weapon to activate
			if (input.activeweapon >= 0 && input.activeweapon < WEAPON_NUMWEAPONS && 
				(lweapons[input.activeweapon].flags & WEAPON_ISACTIVE))
			{
				inextweapon = input.activeweapon;
				equip_time = SERVER_TICK_SPEED * lweapons[iactiveweapon].unequiptime;
				// unequip current
				flags |= PLAYER_FLAGS_ISEQUIPPING;

				create_sound(pos, sound_player_switchweapon);
			}
		}

		// don't do any weapon activations if modifier is currently overriding
		int modifierflags = handlemodifiers();
		if (!(modifierflags & MODIFIER_RETURNFLAGS_OVERRIDEWEAPON))
			handleweapon();

		handlehook();
		
		// handle movement
		if(grounded)
		{
			if (airjumped)
				create_sound(pos, SOUND_PLAYER_LAND);
			airjumped = 0;
		}
		
		float elast = 0.0f;

		if (!numhooked)
		{
			// I'm hooked by someone, so don't do any movement plz (temp)
			if (!(modifierflags & MODIFIER_RETURNFLAGS_OVERRIDEVELOCITY))
			{
				if(grounded)
				{
					// ground movement
					if(input.left)
					{
						if(vel.x > -ground_control_speed)
						{
							vel.x -= ground_control_accel; 
							if(vel.x < -ground_control_speed)
								vel.x = -ground_control_speed;
						}
					}
					else if(input.right)
					{
						if(vel.x < ground_control_speed)
						{
							vel.x += ground_control_accel;
							if(vel.x > ground_control_speed)
								vel.x = ground_control_speed;
						}
					}
					else
						vel.x *= ground_friction; // ground fiction
				}
				else
				{
					// air movement
					if(input.left)
					{
						if(vel.x > -air_control_speed)
							vel.x -= air_control_accel;
					}
					else if(input.right)
					{
						if(vel.x < air_control_speed)
							vel.x += air_control_accel;
					}
					else
						vel.x *= air_friction; // air fiction
				}
				
				if(input.jump)
				{
					if(jumped == 0)
					{
						if(grounded)
						{
							create_sound(pos, sound_player_jump);
							vel.y = -ground_jump_speed;
							jumped++;
						}
						/*
						else if(airjumped == 0)
						{
							vel.y = -12;
							airjumped++;
							jumped++;
						}*/
					}
					else if (!grounded)
					{
						airjumped++;
					}
				}
				else
					jumped = 0;
			}
				
			// meh, go through all players and stop their hook on me
			/*
			for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
			{
				if (ent && ent->objtype == OBJTYPE_PLAYER)
				{
					player *p = (player*)ent;
					if(p != this)
					{
						float d = distance(pos, p->pos);
						vec2 dir = normalize(pos - p->pos);
						if(d < phys_size*1.5f)
						{
							float a = phys_size*1.5f - d;
							vel = vel + dir*a;
						}
						
						if(p->phookedplayer == this)
						{
							if(d > phys_size*2.5f)
							{
								elast = 0.0f;
								vel = vel - dir*2.5f;
							}
						}
					}
				}
			}*/
			
			// gravity
			if (!(modifierflags & MODIFIER_RETURNFLAGS_OVERRIDEGRAVITY))
				vel.y += gravity;
		}
		
		if (!(modifierflags & MODIFIER_RETURNFLAGS_OVERRIDEPOSITION))
			move_box(&pos, &vel, vec2(phys_size, phys_size), elast);
	}
	
	void die()
	{
		create_sound(pos, sound_player_die);
		// release our hooked player
		if (phookedplayer)
		{
			phookedplayer->numhooked--;
			phookedplayer = 0;
			hooking = -1;
			numhooked = 0;
		}
		respawn();

		// meh, go through all players and stop their hook on me
		for(entity *ent = world.first_entity; ent; ent = ent->next_entity)
		{
			if (ent && ent->objtype == OBJTYPE_PLAYER)
			{
				player* p = (player*)ent;
				if (p->phookedplayer == this)
				{
					p->phookedplayer = 0;
					p->hooking = -1;
					//p->numhooked--;
				}
			}
		}
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
		/*
		int armordmg = (dmg+1)/2;
		int healthdmg = dmg-armordmg;
		if(armor < armordmg)
		{
			healthdmg += armordmg - armor;
			armor = 0;
		}
		else
			armor -= armordmg;
			
		health -= healthdmg;
		*/
		
		// create healthmod indicator
		create_healthmod(pos, dmg);
		
		damage_taken_tick = tick_count+50;
		
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
			create_sound(pos, sound_player_hurt_long);
		else
			create_sound(pos, sound_player_hurt_short);

		// spawn blood?

		return true;
	}

	virtual void snap(int snaping_client)
	{
		obj_player *player = (obj_player *)snap_new_item(OBJTYPE_PLAYER, client_id, sizeof(obj_player));

		client_info info;
		if(server_getclientinfo(client_id, &info))
			snap_encode_string(info.name, player->name, strlen(info.name), 32);
		
		player->x = (int)pos.x;
		player->y = (int)pos.y;
		player->vx = (int)vel.x;
		player->vy = (int)vel.y;
		player->emote = EMOTE_NORMAL;

		player->ammocount = lweapons[iactiveweapon].numammo;
		player->health = 0;
		player->armor = 0;
		player->local = 0;
		player->clientid = client_id;
		player->weapon = iactiveweapon;
		player->modifier = 0;
		for (int i = 0; i < MODIFIER_NUMMODIFIERS; i++)
		{
			// add active modifiers
			if (modifiers[i].flags & MODIFIER_ISACTIVE)
				player->modifier |= 1 << i;
		}
		// get current attack ticks
		getattackticks(player->attackticks, player->attacklen, player->visualtimeattack);
		

		if(client_id == snaping_client)
		{
			player->local = 1;
			player->health = health;
			player->armor = armor;
		}
		
		if(length(vel) > 15.0f)
			player->emote = EMOTE_HAPPY;
		
		if(damage_taken_tick > tick_count)
			player->emote = EMOTE_PAIN;
		
		if(player->emote == EMOTE_NORMAL)
		{
			if((tick_count%(50*5)) < 10)
				player->emote = EMOTE_BLINK;
		}
		
		player->hook_active = hooking>0?1:0;
		player->hook_x = (int)hook_pos.x;
		player->hook_y = (int)hook_pos.y;
			
		player->angle = input.angle;
		player->score = score;
	}
};

// POWERUP ///////////////////////

powerup::powerup(int _type, int _subtype, int _numitems, int _flags) : 
	entity(OBJTYPE_POWERUP)
{
	static int current_id = 0;
	playerhooked = 0;
	id = current_id++;
	vel = vec2(0.0f,0.0f);
	type = _type;
	subtype = _subtype;
	numitems = _numitems;
	flags = _flags;
	// set radius (so it can collide and be hooked and stuff)
	proximity_radius = phys_size;
	spawntick = -1;
	world.insert_entity(this);
}

void powerup::spawnrandom(int _lifespan)
{
	return;
	/*
	vec2 pos;
	int start, num;
	map_get_type(1, &start, &num);
	
	if(!num)
		return;
	
	mapres_spawnpoint *sp = (mapres_spawnpoint*)map_get_item(start + (rand()%num), NULL, NULL);
	pos = vec2(sp->x, sp->y);

	// Check if there already is a powerup at that location
	{
		int type = OBJTYPE_POWERUP;
		entity *ents[64];
		int num = world.find_entities(pos, 5.0f, ents, 64,&type,1);
		for (int i = 0; i < num; i++)
		{
			if (ents[i]->objtype == OBJTYPE_POWERUP)
			{
				// location busy
				return;
			}
		}
	}

	powerup* ppower = new powerup(rand() % POWERUP_TYPE_NUMPOWERUPS,_lifespan);
	ppower->pos = pos;
	if (ppower->type == POWERUP_TYPE_WEAPON)
	{
		ppower->subtype = rand() % WEAPON_NUMWEAPONS;
		ppower->numitems = 10;
	}
	else if (ppower->type == POWERUP_TYPE_HEALTH || ppower->type == POWERUP_TYPE_ARMOR)
	{
		ppower->numitems = rand() % 5;
	}
	ppower->flags |= POWERUP_FLAG_HOOKABLE;*/
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
	
	vec2 oldpos = pos;
	//vel.y += 0.25f;
	pos += vel;
	move_box(&pos, &vel, vec2(phys_size, phys_size), 0.0f);

	// Check if a player intersected us
	vec2 meh;
	player* pplayer = intersect_player(pos, pos + vec2(0,16), meh, 0);
	if (pplayer)
	{
		// player picked us up, is someone was hooking us, let them go
		if (playerhooked)
			playerhooked->hooking = -1;
		int respawntime = -1;
		switch (type)
		{
		case POWERUP_TYPE_HEALTH:
			{
				if(pplayer->health < PLAYER_MAXHEALTH)
				{
					pplayer->health = min(PLAYER_MAXHEALTH, pplayer->health + numitems);
					respawntime = 20;
				}
				break;
			}
		case POWERUP_TYPE_ARMOR:
			{
				if(pplayer->armor < PLAYER_MAXARMOR)
				{
					pplayer->armor = min(PLAYER_MAXARMOR, pplayer->armor + numitems);
					respawntime = 20;
				}
				break;
			}
		case POWERUP_TYPE_WEAPON:
			{
				if (pplayer->lweapons[subtype].flags & player::WEAPON_ISACTIVE)
				{
					// add ammo
					/*
					if (pplayer->lweapons[subtype].flags & player::WEAPON_NEEDRELOAD)
					{
						int numtoadd = min(numitems, pplayer->lweapons[subtype].magsize);
						pplayer->lweapons[subtype].numammo = min(10, pplayer->lweapons[subtype].numammo + numtoadd);
						pplayer->lweapons[subtype].nummagazines += (numitems - numtoadd) % pplayer->lweapons[subtype].magsize;
					}
					else*/
					if(pplayer->lweapons[subtype].numammo < 10)
					{
						respawntime = 20;
						pplayer->lweapons[subtype].numammo = min(10, pplayer->lweapons[subtype].numammo + numitems);
					}
				}
				else
				{
					pplayer->lweapons[subtype].settype();
					pplayer->lweapons[subtype].flags |= player::WEAPON_ISACTIVE;
					respawntime = 20;
				}
				break;
			}
		case POWERUP_TYPE_NINJA:
			{
				respawntime = 60;
				// reset and activate
				pplayer->modifiers[MODIFIER_TYPE_NINJA].settype();
				pplayer->modifiers[MODIFIER_TYPE_NINJA].flags |= player::MODIFIER_ISACTIVE;
				break;
			}
		//POWERUP_TYPE_TIMEFIELD		= 4,
		default:
			break;
		};
		
		if(respawntime >= 0)
			spawntick = server_tick() + server_tickspeed() * respawntime;
		//world.destroy_entity(this);
	}
}

void powerup::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	obj_powerup *powerup = (obj_powerup *)snap_new_item(OBJTYPE_POWERUP, id, sizeof(obj_powerup));
	powerup->x = (int)pos.x;
	powerup->y = (int)pos.y;
	powerup->vx = (int)vel.x;
	powerup->vy = (int)vel.y;
	powerup->type = type;
	powerup->subtype = subtype;
}

// POWERUP END ///////////////////////

static player players[MAX_CLIENTS];

player *get_player(int index)
{
	return &players[index];
}

void create_healthmod(vec2 p, int amount)
{
	ev_healthmod *ev = (ev_healthmod *)events.create(EVENT_HEALTHMOD, sizeof(ev_healthmod));
	ev->x = (int)p.x;
	ev->y = (int)p.y;
	ev->amount = amount;
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
			{
				ents[i]->take_damage(forcedir*dmg*2, (int)dmg, owner);/* && 
						ents[i]->objtype == OBJTYPE_PLAYER &&
						owner >= 0)
				{
					player *p = (player*)ents[i];
					if(p->client_id == owner)
						p->score--;
					else
						((player*)ents[owner])->score++;

				}*/
			}
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

player* intersect_player(vec2 pos0, vec2 pos1, vec2& new_pos, entity* notthis)
{
	// Find other players
	entity *ents[64];
	vec2 dir = pos1 - pos0;
	float radius = length(dir * 0.5f);
	vec2 center = pos0 + dir * 0.5f;
	int num = world.find_entities(center, radius, ents, 64);
	for (int i = 0; i < num; i++)
	{
		// Check if entity is a player
		if (ents[i] != notthis && ents[i]->objtype == OBJTYPE_PLAYER)
		{
			// temp, set hook pos to our position
			new_pos = ents[i]->pos;
			return (player*)ents[i];
		}
	}

	return 0;
}

// Server hooks
static int addtick = SERVER_TICK_SPEED * 5;
void mods_tick()
{
	// clear all events
	events.clear();
	world.tick();

	if (addtick <= 0)
	{
		powerup::spawnrandom(SERVER_TICK_SPEED * 5);
		addtick = SERVER_TICK_SPEED * 5;
	}
	addtick--;
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
	
}

void mods_client_drop(int client_id)
{
	players[client_id].client_id = -1;
	world.remove_entity(&players[client_id]);
}

void mods_init()
{
	col_init(32);

	int start, num;
	map_get_type(MAPRES_ITEM, &start, &num);
	
	for(int i = 0; i < num; i++)
	{
		mapres_item *it = (mapres_item *)map_get_item(start+i, 0, 0);
		
		int type = -1;
		int subtype = -1;
		int numitems = 1;
		
		switch(it->type)
		{
		case ITEM_WEAPON_GUN:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_GUN;
			break;
		case ITEM_WEAPON_SHOTGUN:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_SHOTGUN;
			numitems = 5;
			break;
		case ITEM_WEAPON_ROCKET:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_ROCKET;
			numitems = 5;
			break;
		/*case ITEM_WEAPON_SNIPER:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_ROCKET;
			break;*/
		case ITEM_WEAPON_HAMMER:
			type = POWERUP_TYPE_WEAPON;
			subtype = WEAPON_TYPE_MELEE;
			break;
		
		case ITEM_HEALTH_1:
			type = POWERUP_TYPE_HEALTH;
			numitems = 1;
			break;
		case ITEM_HEALTH_5:
			type = POWERUP_TYPE_HEALTH;
			numitems = 5;
			break;
		case ITEM_HEALTH_10:
			type = POWERUP_TYPE_HEALTH;
			numitems = 10;
			break;
		
		case ITEM_ARMOR_1:
			type = POWERUP_TYPE_ARMOR;
			numitems = 1;
			break;
		case ITEM_ARMOR_5:
			type = POWERUP_TYPE_ARMOR;
			numitems = 5;
			break;
		case ITEM_ARMOR_10:
			type = POWERUP_TYPE_ARMOR;
			numitems = 10;
			break;
		};
		
		powerup* ppower = new powerup(type, subtype, numitems);
		ppower->pos.x = it->x;
		ppower->pos.y = it->y;
	}
		
	
	/*
	powerup* ppower = new powerup(rand() % POWERUP_TYPE_NUMPOWERUPS,_lifespan);
	ppower->pos = pos;
	if (ppower->type == POWERUP_TYPE_WEAPON)
	{
		ppower->subtype = rand() % WEAPON_NUMWEAPONS;
		ppower->numitems = 10;
	}
	else if (ppower->type == POWERUP_TYPE_HEALTH || ppower->type == POWERUP_TYPE_ARMOR)
	{
		ppower->numitems = rand() % 5;
	}
	ppower->flags |= POWERUP_FLAG_HOOKABLE;
	*/
	
	//powerup::spawnrandom(SERVER_TICK_SPEED * 5);
}

void mods_shutdown() {}
void mods_presnap() {}
void mods_postsnap() {}
