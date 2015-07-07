/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEWORLD_H
#define GAME_SERVER_GAMEWORLD_H

#include <game/gamecore.h>
#include "entitylist.h"

enum
{
	ENTTYPE_PROJECTILE = 0,
	ENTTYPE_LASER,
	ENTTYPE_PICKUP,
	ENTTYPE_FLAG,
	ENTTYPE_CHARACTER,
	NUM_ENTTYPES
};

class CGameWorld
{
private:
	/* Identity */
	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

	/* State */
	CEntityList m_aLists[NUM_ENTTYPES];
	CEntityList m_DestroyList;

	CWorldCore m_Core;
	bool m_Resetting;
	bool m_Paused;

	/* Functions */
	void Reset();
	void CleanupEntities();

	CEntity *GetFirst(int Type);
	int FindEntities(int Type, vec2 Pos, float Radius, CEntity **ppEnts, int MaxEnts);
	CEntity *IntersectEntity(int Type, vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, CEntity *pNotThis = 0);
	CEntity *ClosestEntity(int Type, vec2 Pos, float Radius, CEntity *pNotThis = 0);

public:
	/* Constructor */
	CGameWorld();

	/* Objects */
	class CGameContext *GameServer()	{ return m_pGameServer; }
	class IServer *Server()				{ return m_pServer; }

	/* Getters */
	CWorldCore *GetCore()				{ return &m_Core; }
	bool IsResetting() const			{ return m_Resetting; }
	bool IsPaused() const				{ return m_Paused; }

	/* Setters */
	void SetResetting(bool Resetting)	{ m_Resetting = Resetting; }
	void SetPaused(bool Paused)			{ m_Paused = Paused; }

	/* Functions */
	void SetGameServer(CGameContext *pGameServer);

	void InsertEntity(CEntity *pEntity);
	void DestroyEntity(CEntity *pEntity);

	void Tick();
	void Snap(int SnappingClient);
	void PostSnap();

	template <class T> T *GetFirst()
	{
		return static_cast<T *>(GetFirst(T::ms_EntType));
	}

	template <class T> int FindEntities(vec2 Pos, float Radius, T **ppEnts, int MaxEnts)
	{
		return FindEntities(T::ms_EntType, Pos, Radius, (CEntity **) ppEnts, MaxEnts);
	}

	template <class T> T *IntersectEntity(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, T *pNotThis = 0)
	{
		return static_cast<T *>(IntersectEntity(T::ms_EntType, Pos0, Pos1, Radius, NewPos, pNotThis));
	}

	template <class T> T *ClosestEntity(vec2 Pos, float Radius, T *pNotThis = 0)
	{
		return static_cast<T *>(ClosestEntity(T::ms_EntType, Pos, Radius, pNotThis));
	}
};

#endif
