/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

class CPickup : public CEntity
{
private:
	/* Identity */
	int m_Type;
	int m_PickupSound;
	int m_RespawnSound;

	/* State */
	int m_SpawnTick;

protected:
	/* Functions */
	virtual bool OnPickup(class CCharacter *pChar) = 0;

public:
	/* Constants */
	static int const ms_PhysSize = 14;

	/* Constructor */
	CPickup(CGameWorld *pGameWorld, int Type, int PickupSound, int RespawnSound, vec2 Pos);

	/* Getters */
	int GetType() const			{ return m_Type; }
	int GetPickupSound() const	{ return m_PickupSound; }
	int GetRespawnSound() const	{ return m_RespawnSound; }
	bool IsSpawned() const		{ return m_SpawnTick < 0; }

	/* CEntity functions */
	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
};

#endif
