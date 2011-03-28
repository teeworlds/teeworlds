/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUPWEAPON_H
#define GAME_SERVER_ENTITIES_PICKUPWEAPON_H

#include <game/server/entity.h>
#include <game/server/entities/pickup.h>

class IPickupWeapon : public CPickup
{
public:
	IPickupWeapon(CGameWorld *pGameWorld, int weaponType);

	virtual int OnPickup(CCharacter *pChr);
	virtual void CreatePickupSound() {}

	int WeaponType() const { return m_Subtype;}

	int m_Ammo;
};

class CPickupNinja : public CPickup
{
public:
	CPickupNinja(CGameWorld *pGameWorld);
	virtual int OnPickup(CCharacter *pChr);
};

class CPickupLaser : public IPickupWeapon
{
public:
	CPickupLaser(CGameWorld *pGameWorld);
	virtual void CreatePickupSound();
};

class CPickupShotgun : public IPickupWeapon
{
public:
	CPickupShotgun(CGameWorld *pGameWorld);
	virtual void CreatePickupSound();
};

class CPickupGrenade : public IPickupWeapon
{
public:
	CPickupGrenade(CGameWorld *pGameWorld);
	virtual void CreatePickupSound();
};

#endif
