
#include "gameworld.hpp"
#include "entity.hpp"
#include "gamecontext.hpp"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
GAMEWORLD::GAMEWORLD()
{
	paused = false;
	reset_requested = false;
	first_entity = 0x0;
	for(int i = 0; i < NUM_ENT_TYPES; i++)
		first_entity_types[i] = 0;
}

GAMEWORLD::~GAMEWORLD()
{
	// delete all entities
	while(first_entity)
		delete first_entity;
}

ENTITY *GAMEWORLD::find_first(int type)
{
	return first_entity_types[type];
}


int GAMEWORLD::find_entities(vec2 pos, float radius, ENTITY **ents, int max, int type)
{
	int num = 0;
	for(ENTITY *ent = (type<0) ? first_entity : first_entity_types[type];
		ent; ent = (type<0) ? ent->next_entity : ent->next_type_entity)
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

void GAMEWORLD::insert_entity(ENTITY *ent)
{
	ENTITY *cur = first_entity;
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

void GAMEWORLD::destroy_entity(ENTITY *ent)
{
	ent->marked_for_destroy = true;
}

void GAMEWORLD::remove_entity(ENTITY *ent)
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
void GAMEWORLD::snap(int snapping_client)
{
	for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
		ent->snap(snapping_client);
}

void GAMEWORLD::reset()
{
	// reset all entities
	for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
		ent->reset();
	remove_entities();

	game.controller->post_reset();
	remove_entities();

	reset_requested = false;
}

void GAMEWORLD::remove_entities()
{
	// destroy objects marked for destruction
	ENTITY *ent = first_entity;
	while(ent)
	{
		ENTITY *next = ent->next_entity;
		if(ent->marked_for_destroy)
		{
			remove_entity(ent);
			ent->destroy();
		}
		ent = next;
	}
}

void GAMEWORLD::tick()
{
	if(reset_requested)
		reset();

	if(!paused)
	{
		if(game.controller->is_force_balanced())
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick();
		
		for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick_defered();
	}

	remove_entities();
}


// TODO: should be more general
CHARACTER *GAMEWORLD::intersect_character(vec2 pos0, vec2 pos1, float radius, vec2& new_pos, ENTITY *notthis)
{
	// Find other players
	float closest_len = distance(pos0, pos1) * 100.0f;
	vec2 line_dir = normalize(pos1-pos0);
	CHARACTER *closest = 0;

	CHARACTER *p = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; p; p = (CHARACTER *)p->typenext())
 	{
		if(p == notthis)
			continue;
			
		vec2 intersect_pos = closest_point_on_line(pos0, pos1, p->pos);
		float len = distance(p->pos, intersect_pos);
		if(len < CHARACTER::phys_size+radius)
		{
			if(len < closest_len)
			{
				new_pos = intersect_pos;
				closest_len = len;
				closest = p;
			}
		}
	}
	
	return closest;
}


CHARACTER *GAMEWORLD::closest_character(vec2 pos, float radius, ENTITY *notthis)
{
	// Find other players
	float closest_range = radius*2;
	CHARACTER *closest = 0;
		
	CHARACTER *p = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; p; p = (CHARACTER *)p->typenext())
 	{
		if(p == notthis)
			continue;
			
		float len = distance(pos, p->pos);
		if(len < CHARACTER::phys_size+radius)
		{
			if(len < closest_range)
			{
				closest_range = len;
				closest = p;
			}
		}
	}
	
	return closest;
}
