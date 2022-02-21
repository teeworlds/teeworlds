/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CPickup : public CEntity
{
public:
	CPickup(CGameWorld *pGameWorld, int Type, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void DeallocateHarpoon();
	virtual void AllocateHarpoon(CHarpoon* pHarpoon);
	virtual bool IsValidForHarpoon(CHarpoon* pHarpoon);
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_OriginalPos;
	int m_Type;
	int m_SpawnTick;
	vec2 m_Vel;

	int m_AmountOfHarpoons;
	//CHarpoon* m_apHarpoons[16];
	int m_DisturbedTick; //When this reaches 0, the disturbed pickup will return to m_OriginalPos
};

#endif
