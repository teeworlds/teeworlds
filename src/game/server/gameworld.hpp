#ifndef GAME_SERVER_GAMEWORLD_H
#define GAME_SERVER_GAMEWORLD_H

#include <game/g_game.hpp>

class ENTITY;
class CHARACTER;

/*
	Class: Game World
		Tracks all entities in the game. Propagates tick and
		snap calls to all entities.
*/
class GAMEWORLD
{
	void reset();
	void remove_entities();

	enum
	{
		NUM_ENT_TYPES=10, // TODO: are more exact value perhaps? :)
	};

	// TODO: two lists seams kinda not good, shouldn't be needed
	ENTITY *first_entity;
	ENTITY *first_entity_types[NUM_ENT_TYPES];

public:
	bool reset_requested;
	bool paused;
	WORLD_CORE core;
	
	GAMEWORLD();
	~GAMEWORLD();
	
	ENTITY *find_first() { return first_entity; }
	ENTITY *find_first(int type);
	
	/*
		Function: find_entities
			Finds entities close to a position and returns them in a list.
			
		Arguments:
			pos - Position.
			radius - How close the entities have to be.
			ents - Pointer to a list that should be filled with the pointers
				to the entities.
			max - Number of entities that fits into the ents array.
			type - Type of the entities to find. -1 for all types.
			
		Returns:
			Number of entities found and added to the ents array.
	*/
	int find_entities(vec2 pos, float radius, ENTITY **ents, int max, int type = -1);
	
	/*
		Function: interserct_character
			Finds the closest character that intersects the line.
			
		Arguments:
			pos0 - Start position
			pos2 - End position
			radius - How for from the line the character is allowed to be.
			new_pos - Intersection position
			notthis - Entity to ignore intersecting with
			
		Returns:
			Returns a pointer to the closest hit or NULL of there is no intersection.
	*/
	class CHARACTER *intersect_character(vec2 pos0, vec2 pos1, float radius, vec2 &new_pos, class ENTITY *notthis = 0);
	
	/*
		Function: closest_character
			Finds the closest character to a specific point.
			
		Arguments:
			pos - The center position.
			radius - How far off the character is allowed to be
			notthis - Entity to ignore
			
		Returns:
			Returns a pointer to the closest character or NULL if no character is close enough.
	*/
	class CHARACTER *closest_character(vec2 pos, float radius, ENTITY *notthis);

	/*
		Function: insert_entity
			Adds an entity to the world.
			
		Arguments:
			entity - Entity to add
	*/
	void insert_entity(ENTITY *entity);

	/*
		Function: remove_entity
			Removes an entity from the world.
			
		Arguments:
			entity - Entity to remove
	*/
	void remove_entity(ENTITY *entity);

	/*
		Function: destroy_entity
			Destroys an entity in the world.
			
		Arguments:
			entity - Entity to destroy
	*/
	void destroy_entity(ENTITY *entity);
	
	/*
		Function: snap
			Calls snap on all the entities in the world to create
			the snapshot.
			
		Arguments:
			snapping_client - ID of the client which snapshot
			is being created.
	*/
	void snap(int snapping_client);
	
	/*
		Function: tick
			Calls tick on all the entities in the world to progress
			the world to the next tick.
		
	*/
	void tick();
};

#endif
