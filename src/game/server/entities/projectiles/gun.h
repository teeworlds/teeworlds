/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILES_GUN_H
#define GAME_SERVER_ENTITIES_PROJECTILES_GUN_H

#include <game/server/entities/projectile.h>

class CProjectileGun : public CProjectile
{
protected:
	/* CProjectile functions */
	virtual void OnLifeOver(vec2 Pos);
	virtual bool OnCharacterHit(vec2 Pos, class CCharacter *pChar);
	virtual bool OnGroundHit(vec2 Pos);

public:
	/* Constructor */
	CProjectileGun(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos);

	/* CProjectile getters */
	virtual float GetCurvature();
	virtual float GetSpeed();
	virtual float GetLifeSpan();
};

#endif
