/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_MERCENARY_BOMB_H
#define GAME_SERVER_ENTITIES_MERCENARY_BOMB_H

#include <game/server/entity.h>
#include <base/tl/array.h>

class CMercenaryBomb : public CEntity
{
public:
	enum
	{
		NUM_SIDE = 12,
		NUM_HINT = 12,
		NUM_IDS = NUM_SIDE + NUM_HINT,
	};
	
public:
	CMercenaryBomb(CGameWorld *pGameWorld, vec2 Pos, int Owner);
	~CMercenaryBomb();

	virtual void Snap(int SnappingClient);
	virtual void Tick();
	virtual void Reset();
	void Explode();
	void IncreaseDamage();
	bool ReadyToExplode();

private:
	int m_IDs[NUM_IDS];
	
public:
	int m_LoadingTick;
	int m_Owner;
	int m_Damage;
};

#endif
