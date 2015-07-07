/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITY_H
#define GAME_SERVER_ENTITY_H

#include <base/vmath.h>

#include "alloc.h"
#include "gameworld.h"

/*
	Class: Entity
		Basic entity class.
*/
class CEntity
{
	MACRO_ALLOC_HEAP()

private:
	/* Identity */
	class CGameWorld *m_pGameWorld;
	int m_ID;
	int m_EntType;
	float m_ProximityRadius;

	/* State */
	CEntity *m_pPrevEntity;
	CEntity *m_pNextEntity;
	bool m_MarkedForDestroy;

protected:
	/* State */
	vec2 m_Pos;

public:
	/* Constructor */
	CEntity(CGameWorld *pGameWorld, int EntType, vec2 Pos, int ProximityRadius=0);

	/* Destructor */
	virtual ~CEntity();

	/* Objects */
	class CGameWorld *GameWorld()		{ return m_pGameWorld; }
	class CGameContext *GameServer()	{ return m_pGameWorld->GameServer(); }
	class IServer *Server()				{ return m_pGameWorld->Server(); }

	/* Getters */
	int GetID() const					{ return m_ID; }
	int GetEntType() const				{ return m_EntType; }
	float GetProximityRadius() const	{ return m_ProximityRadius; }
	CEntity *GetPrevEntity()			{ return m_pPrevEntity; }
	CEntity *GetNextEntity()			{ return m_pNextEntity; }
	bool IsMarkedForDestroy() const		{ return m_MarkedForDestroy; }
	vec2 GetPos() const					{ return m_Pos; }

	/* Setters */
	void SetPrevEntity(CEntity *pEnt)	{ m_pPrevEntity = pEnt; }
	void SetNextEntity(CEntity *pEnt)	{ m_pNextEntity = pEnt; }

	/* Functions */
	void MarkForDestroy();

	/*
		Function: Destroy
			Called when the entity has been removed from the world.
	*/
	virtual void Destroy() { }

	/*
		Function: Reset
			Called when the game resets the map. Puts the entity
			back to its starting state or perhaps destroys it.
	*/
	virtual void Reset() {}

	/*
		Function: Tick
			Called to progress the entity to the next tick. Updates
			and moves the entity to its new state and position.
	*/
	virtual void Tick() {}

	/*
		Function: TickDefered
			Called after all entities Tick() function has been called.
	*/
	virtual void TickDefered() {}

	/*
		Function: TickPaused
			Called when the game is paused, to freeze the state and position of the entity.
	*/
	virtual void TickPaused() {}

	/*
		Function: Snap
			Called when a new snapshot is being generated for a specific
			client.

		Arguments:
			SnappingClient - ID of the client which snapshot is
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
			SnappingClient - ID of the client which snapshot is
				being generated. Could be -1 to create a complete
				snapshot of everything in the game for demo
				recording.

		Returns:
			Non-zero if the entity doesn't have to be in the snapshot.
	*/
	bool NetworkClipped(int SnappingClient);
	bool NetworkClipped(int SnappingClient, vec2 CheckPos);
	bool NetworkClipped(int SnappingClient, vec2 CheckLinePoint1, vec2 CheckLinePoint2);

	bool GameLayerClipped(vec2 CheckPos);
};

#endif
