#ifndef GAME_SERVER_ENTITY_H
#define GAME_SERVER_ENTITY_H

#include <base/vmath.hpp>

/*
	Class: Entity
		Basic entity class.
*/
class ENTITY
{
private:
	friend class GAMEWORLD; // thy these?
	ENTITY *prev_entity;
	ENTITY *next_entity;

	ENTITY *prev_type_entity;
	ENTITY *next_type_entity;
protected:
	bool marked_for_destroy;
	int id;
	int objtype;
public:
	
	ENTITY(int objtype);
	virtual ~ENTITY();
	
	ENTITY *typenext() { return next_type_entity; }
	ENTITY *typeprev() { return prev_type_entity; }

	/*
		Function: destroy
		Destorys the entity.
	*/
	virtual void destroy() { delete this; }
		
	/*
		Function: reset
		Called when the game resets the map. Puts the entity
		back to it's starting state or perhaps destroys it.
	*/
	virtual void reset() {}
	
	/*
		Function: tick
		Called progress the entity to the next tick. Updates
		and moves the entity to it's new state and position.
	*/
	virtual void tick() {}

	/*
		Function: tick_defered
		Called after all entities tick() function has been called.
	*/
	virtual void tick_defered() {}
	
	/*
		Function: snap
		Called when a new snapshot is being generated for a specific
		client.
		
		Arguments:
			snapping_client - ID of the client which snapshot is
				being generated. Could be -1 to create a complete
				snapshot of everything in the game for demo
				recording.
	*/
	virtual void snap(int snapping_client) {}

	/*
		Variable: proximity_radius
		Contains the physical size of the entity.
	*/
	float proximity_radius;
	
	/*
		Variable: pos
		Contains the current posititon of the entity.
	*/
	vec2 pos;
};

#endif
