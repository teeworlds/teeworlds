/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITY_H
#define GAME_SERVER_ENTITY_H

#include <new>
#include <base/vmath.h>
#include <game/server/gameworld.h>

#define MACRO_ALLOC_HEAP() \
	public: \
	void *operator new(size_t Size) \
	{ \
		void *p = mem_alloc(Size, 1); \
		/*dbg_msg("", "++ %p %d", p, size);*/ \
		mem_zero(p, Size); \
		return p; \
	} \
	void operator delete(void *pPtr) \
	{ \
		/*dbg_msg("", "-- %p", p);*/ \
		mem_free(pPtr); \
	} \
	private:

#define MACRO_ALLOC_POOL_ID() \
	public: \
	void *operator new(size_t Size, int id); \
	void operator delete(void *p); \
	private:

#define MACRO_ALLOC_POOL_ID_IMPL(POOLTYPE, PoolSize) \
	static char ms_PoolData##POOLTYPE[PoolSize][sizeof(POOLTYPE)] = {{0}}; \
	static int ms_PoolUsed##POOLTYPE[PoolSize] = {0}; \
	void *POOLTYPE::operator new(size_t Size, int id) \
	{ \
		dbg_assert(sizeof(POOLTYPE) == Size, "size error"); \
		dbg_assert(!ms_PoolUsed##POOLTYPE[id], "already used"); \
		/*dbg_msg("pool", "++ %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 1; \
		mem_zero(ms_PoolData##POOLTYPE[id], Size); \
		return ms_PoolData##POOLTYPE[id]; \
	} \
	void POOLTYPE::operator delete(void *p) \
	{ \
		int id = (POOLTYPE*)p - (POOLTYPE*)ms_PoolData##POOLTYPE; \
		dbg_assert(ms_PoolUsed##POOLTYPE[id], "not used"); \
		/*dbg_msg("pool", "-- %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 0; \
		mem_zero(ms_PoolData##POOLTYPE[id], sizeof(POOLTYPE)); \
	}

/*
	Class: Entity
		Basic entity class.
*/
class CEntity
{
	MACRO_ALLOC_HEAP()

	friend class CGameWorld;	// entity list handling
	CEntity *m_pPrevTypeEntity;
	CEntity *m_pNextTypeEntity;

	class CGameWorld *m_pGameWorld;
protected:
	bool m_MarkedForDestroy;
	int m_ID;
	int m_ObjType;
public:
	CEntity(CGameWorld *pGameWorld, int Objtype);
	virtual ~CEntity();

	class CGameWorld *GameWorld() { return m_pGameWorld; }
	class CGameContext *GameServer() { return GameWorld()->GameServer(); }
	class IServer *Server() { return GameWorld()->Server(); }


	CEntity *TypeNext() { return m_pNextTypeEntity; }
	CEntity *TypePrev() { return m_pPrevTypeEntity; }

	/*
		Function: destroy
			Destorys the entity.
	*/
	virtual void Destroy() { delete this; }

	/*
		Function: reset
			Called when the game resets the map. Puts the entity
			back to it's starting state or perhaps destroys it.
	*/
	virtual void Reset() {}

	/*
		Function: tick
			Called progress the entity to the next tick. Updates
			and moves the entity to it's new state and position.
	*/
	virtual void Tick() {}

	/*
		Function: tick_defered
			Called after all entities tick() function has been called.
	*/
	virtual void TickDefered() {}

	/*
		Function: TickPaused
			Called when the game is paused, to freeze the state and position of the entity.
	*/
	virtual void TickPaused() {}

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
	virtual void Snap(int SnappingClient) {}

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
	int NetworkClipped(int SnappingClient);
	int NetworkClipped(int SnappingClient, vec2 CheckPos);

	bool GameLayerClipped(vec2 CheckPos);

	/*
		Variable: proximity_radius
			Contains the physical size of the entity.
	*/
	float m_ProximityRadius;

	/*
		Variable: pos
			Contains the current posititon of the entity.
	*/
	vec2 m_Pos;
};

#endif
