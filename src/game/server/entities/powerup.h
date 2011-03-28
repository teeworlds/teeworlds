/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_POWERUP_H
#define GAME_SERVER_ENTITIES_POWERUP_H

#include <game/server/entity.h>
#include <game/server/entities/pickup.h>

class CPowerup : public CPickup
{
public:
	CPowerup(CGameWorld *pGameWorld, int powerupType);

	virtual int OnPickup(CCharacter *pChr) = 0;
};

class CHeart : public CPowerup
{
public:
	CHeart(CGameWorld *pGameWorld);

	virtual int OnPickup(CCharacter *pChr);

	int m_Value;
};

class CArmor : public CPowerup
{
public:
	CArmor(CGameWorld *pGameWorld);

	virtual int OnPickup(CCharacter *pChr);

	int m_Value;
};

#endif
