/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_WEAPON_H
#define GAME_SERVER_ENTITIES_WEAPON_H

#include <game/server/entity.h>
#include <game/server/entities/pickup.h>
#include "character.h"

class IWeapon 
{
	MACRO_ALLOC_HEAP()

public:
	IWeapon(CGameWorld *pGameWorld,CCharacter *pCarrier, int weaponType);
  
	int WeaponType() const {return m_Type;}

	virtual int FireWeapon(vec2 StartPos, vec2 Direction);
	virtual bool CanFire();
	virtual void RegenAmmo(int ReloadTimer);
	virtual bool GiveWeapon(int Ammo);
	virtual void Update(){}
	virtual void ConsumeAmmo();

	virtual CGameWorld *GameWorld() {return m_pGameWorld;}
	virtual CGameContext *GameServer(){return GameWorld()->GameServer();}
	virtual IServer *Server(){return GameWorld()->Server();}


	//attributes public for practicity
	int m_Type;
	int m_AmmoRegenStart;
	int m_AmmoRegenTime;
	int m_AmmoRegenValue;

	int m_FireDelay;

	int m_LifeTime;

	int m_Ammo;
	int m_AmmoCost;
	int m_Maxammo;
	int m_Damage;
	bool m_Explosive;
	float m_Force;
	float m_SpeedFactor;

	int m_SoundImpact;
	int m_SoundFire;

	bool m_FullAuto;
	bool m_Got;

	CGameWorld *m_pGameWorld;
	CCharacter *m_pCarrier;
};

class CGun : public IWeapon
{
public:
	CGun(CGameWorld *pGameWorld, CCharacter *pCarrier);
};

class CHammer : public IWeapon
{
public:
	CHammer(CGameWorld *pGameWorld, CCharacter *pCarrier);
	virtual int FireWeapon(vec2 StartPos, vec2 Direction);
};

class CNinja : public IWeapon
{
public:
	CNinja(CGameWorld *pGameWorld, CCharacter *pCarrier);

	virtual void Update();
	virtual int FireWeapon(vec2 StartPos, vec2 Direction);
	virtual bool GiveWeapon(int Ammo);

	//private ?
	vec2 m_ActivationDir;
	int m_ActivationTick;
	int m_CurrentMoveTime;

	int m_NumObjectsHit;
	CEntity *m_apHitObjects[10];

	int m_MoveTime;
	int m_Duration;
	float m_Velocity;
};

class CLaserRifle : public IWeapon
{
public:
	CLaserRifle(CGameWorld *pGameWorld, CCharacter *pCarrier);
	virtual int FireWeapon(vec2 StartPos, vec2 Direction);

	int m_Reach;
};

class CShotgun : public IWeapon
{
public:
	CShotgun(CGameWorld *pGameWorld, CCharacter *pCarrier);
	virtual int FireWeapon(vec2 StartPos, vec2 Direction);

	int m_ShotSpread;
	float m_ShotgunSpeeddiff;
};

class CGrenadeLauncher : public IWeapon
{
public:
	CGrenadeLauncher(CGameWorld *pGameWorld, CCharacter *pCarrier);
};

#endif
