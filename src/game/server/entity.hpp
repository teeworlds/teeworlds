#ifndef GAME_SERVER_ENTITY_H
#define GAME_SERVER_ENTITY_H

#include <new>
#include <base/vmath.hpp>

#define MACRO_ALLOC_HEAP() \
	public: \
	void *operator new(size_t size) \
	{ \
		void *p = mem_alloc(size, 1); \
		/*dbg_msg("", "++ %p %d", p, size);*/ \
		mem_zero(p, size); \
		return p; \
	} \
	void operator delete(void *p) \
	{ \
		/*dbg_msg("", "-- %p", p);*/ \
		mem_free(p); \
	} \
	private:

#define MACRO_ALLOC_POOL_ID() \
	public: \
	void *operator new(size_t size, int id); \
	void operator delete(void *p); \
	private:
	
#define MACRO_ALLOC_POOL_ID_IMPL(POOLTYPE, poolsize) \
	static char pool_data_##POOLTYPE[poolsize][sizeof(POOLTYPE)] = {{0}}; \
	static int pool_used_##POOLTYPE[poolsize] = {0}; \
	void *POOLTYPE::operator new(size_t size, int id) \
	{ \
		dbg_assert(sizeof(POOLTYPE) == size, "size error"); \
		dbg_assert(!pool_used_##POOLTYPE[id], "already used"); \
		/*dbg_msg("pool", "++ %s %d", #POOLTYPE, id);*/ \
		pool_used_##POOLTYPE[id] = 1; \
		mem_zero(pool_data_##POOLTYPE[id], size); \
		return pool_data_##POOLTYPE[id]; \
	} \
	void POOLTYPE::operator delete(void *p) \
	{ \
		int id = (POOLTYPE*)p - (POOLTYPE*)pool_data_##POOLTYPE; \
		dbg_assert(pool_used_##POOLTYPE[id], "not used"); \
		/*dbg_msg("pool", "-- %s %d", #POOLTYPE, id);*/ \
		pool_used_##POOLTYPE[id] = 0; \
		mem_zero(pool_data_##POOLTYPE[id], sizeof(POOLTYPE)); \
	}
	
/*
	Class: Entity
		Basic entity class.
*/
class ENTITY
{
	MACRO_ALLOC_HEAP()
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
		Function: networkclipped(int snapping_client)
			Performs a series of test to see if a client can see the
			entity.

		Arguments:
			snapping_client - ID of the client which snapshot is
				being generated. Could be -1 to create a complete
				snapshot of everything in the game for demo
				recording.
			
		Returns:
			Non-zero if the entity doesn't have to be in the snapshot.
	*/
	int networkclipped(int snapping_client);
	int networkclipped(int snapping_client, vec2 check_pos);
		

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
