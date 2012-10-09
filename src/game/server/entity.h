/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITY_H
#define GAME_SERVER_ENTITY_H

#include <base/vmath.h>

#include <game/server/gameworld.h>

#include "alloc.h"

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

	virtual void PostSnap() {}

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
