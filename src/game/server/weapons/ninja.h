/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_WEAPONS_NINJA_H
#define GAME_SERVER_WEAPONS_NINJA_H

#include <engine/shared/protocol.h>

#include <game/server/weapon.h>

class CWeaponNinja : public CWeapon
{
private:
	class CEntity *m_apHitObjects[MAX_CLIENTS];
	int m_NumHitObjects;

	vec2 m_ActivationDir;
	int m_ActivationTick;
	int m_CurrentMoveTime;
	int m_OldVelAmount;

	bool AlreadyHit(CEntity *pEntity);

protected:
	/* CWeapon functions */
	virtual void OnFire(vec2 Direction, vec2 ProjStartPos);
	virtual void OnTick();
	virtual void OnTickPaused();

public:
	/* Constructor */
	CWeaponNinja(class CCharacter *pOwner, int Ammo);

	/* CWeapon functions */
	virtual int SnapAmmo();
};

#endif
