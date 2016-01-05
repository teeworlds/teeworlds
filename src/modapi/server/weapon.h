#ifndef MODAPI_SERVER_WEAPON_H
#define MODAPI_SERVER_WEAPON_H

#include <base/vmath.h>
#include <modapi/weapon.h>

class CModAPI_Weapon
{
private:
	int m_ID;
	class CCharacter* m_pCharacter;
	
public:
	CModAPI_Weapon(int ID, class CCharacter* pCharacter);

	int GetID() const;
	class CCharacter* Character();
	class CPlayer* Player();
	class CGameWorld* GameWorld();
	class CGameContext* GameServer();
	class IServer* Server();
	const class IServer* Server() const;
	
	virtual int GetAmmoType() const = 0;
	virtual int GetMaxAmmo() const = 0;
	virtual int GetAmmo() const = 0;
	virtual bool AddAmmo(int Ammo) = 0;
	virtual bool IsAutomatic() const = 0;
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPaused(bool IsActive);
	virtual bool TickPreFire(bool IsActive) = 0;
	virtual bool OnFire(vec2 Direction) = 0;
	virtual void OnActivation();
	
	virtual void Snap(int SnappingClient, class CNetObj_Character* pChar);
};

class CModAPI_Weapon_Hammer : public CModAPI_Weapon
{
protected:
	int m_ReloadTimer;

public:
	CModAPI_Weapon_Hammer(class CCharacter* pCharacter);
	
	virtual int GetMaxAmmo() const;
	virtual int GetAmmoType() const;
	virtual int GetAmmo() const;
	virtual bool AddAmmo(int Ammo);
	virtual bool IsAutomatic() const;
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPreFire(bool IsActive);
	virtual bool OnFire(vec2 Direction);
};

class CModAPI_Weapon_GenericGun07 : public CModAPI_Weapon
{
protected:
	int m_ReloadTimer;
	int m_LastNoAmmoSound;
	int m_AmmoRegenStart;
	int m_Ammo;
	
public:
	CModAPI_Weapon_GenericGun07(int WID, class CCharacter* pCharacter, int Ammo);
	
	virtual int GetMaxAmmo() const;
	virtual int GetAmmoType() const;
	virtual int GetAmmo() const;
	virtual bool AddAmmo(int Ammo);
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPaused(bool IsActive);
	virtual bool TickPreFire(bool IsActive);
	virtual bool OnFire(vec2 Direction);
	virtual void OnActivation();

	virtual void Snap(int SnappingClient, class CNetObj_Character* pChar);
	
	virtual void CreateProjectiles(vec2 Pos, vec2 Direction) = 0;
};

class CModAPI_Weapon_Gun : public CModAPI_Weapon_GenericGun07
{
public:
	CModAPI_Weapon_Gun(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const;
	virtual void CreateProjectiles(vec2 Pos, vec2 Direction);
};

class CModAPI_Weapon_Shotgun : public CModAPI_Weapon_GenericGun07
{
public:
	CModAPI_Weapon_Shotgun(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const;
	virtual void CreateProjectiles(vec2 Pos, vec2 Direction);
};

class CModAPI_Weapon_Grenade : public CModAPI_Weapon_GenericGun07
{
public:
	CModAPI_Weapon_Grenade(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const;
	virtual void CreateProjectiles(vec2 Pos, vec2 Direction);
};

class CModAPI_Weapon_Laser : public CModAPI_Weapon_GenericGun07
{
public:
	CModAPI_Weapon_Laser(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const;
	virtual void CreateProjectiles(vec2 Pos, vec2 Direction);
};

class CModAPI_Weapon_Ninja : public CModAPI_Weapon
{
protected:
	int m_ReloadTimer;
	
	class CEntity* m_apHitObjects[10];
	int m_NumObjectsHit;
	
	vec2 m_ActivationDir;
	int m_ActivationTick;
	int m_CurrentMoveTime;
	int m_OldVelAmount;

public:
	CModAPI_Weapon_Ninja(class CCharacter* pCharacter);
	
	virtual int GetMaxAmmo() const;
	virtual int GetAmmoType() const;
	virtual int GetAmmo() const;
	virtual bool AddAmmo(int Ammo);
	virtual bool IsAutomatic() const;
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPaused(bool IsActive);
	virtual bool TickPreFire(bool IsActive);
	virtual bool OnFire(vec2 Direction);
	
	virtual void Snap(int SnappingClient, class CNetObj_Character* pChar);
};

#endif
