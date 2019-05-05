/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>
#include <game/server/entity.h>

#ifndef GAME_SERVER_ENTITIES_GROWINGEXP_H
#define GAME_SERVER_ENTITIES_GROWINGEXP_H

enum
{
	GROWINGEXPLOSIONEFFECT_FREEZE_INFECTED=0,
	GROWINGEXPLOSIONEFFECT_POISON_INFECTED,
	GROWINGEXPLOSIONEFFECT_ELECTRIC_INFECTED,
	GROWINGEXPLOSIONEFFECT_LOVE_INFECTED,
	GROWINGEXPLOSIONEFFECT_BOOM_INFECTED,
	GROWINGEXPLOSIONEFFECT_HEAL_HUMANS,
	GROWINGEXPLOSIONEFFECT_MERC_INFECTED,
};

class CGrowingExplosion : public CEntity
{
public:
	CGrowingExplosion(CGameWorld *pGameWorld, vec2 Pos, vec2 Dir, int Owner, int Radius, int ExplosionEffect);
	virtual ~CGrowingExplosion();
	
	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();

	int GetOwner() const;

private:
	int m_MaxGrowing;
	int m_GrowingMap_Length;
	int m_GrowingMap_Size;
	
	int m_Owner;
	vec2 m_SeedPos;
	int m_SeedX;
	int m_SeedY;
	int m_StartTick;
	int* m_pGrowingMap;
	vec2* m_pGrowingMapVec;
	int m_ExplosionEffect;
	bool m_Hit[MAX_CLIENTS];
};

#endif
