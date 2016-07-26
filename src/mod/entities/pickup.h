/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <modapi/server/entity.h>

const int PickupPhysSize = 14;

class CPickup : public CModAPI_Entity
{
public:
	CPickup(CGameWorld *pGameWorld, int Type, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap06(int Snapshot, int SnappingClient);
	virtual void Snap07(int Snapshot, int SnappingClient);

private:
	int m_Type;
	int m_SpawnTick;
};

#endif
