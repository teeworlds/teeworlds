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
	virtual ~CModAPI_Weapon() {}

	int GetID() const { return m_ID; }
	class CCharacter* Character();
	class CPlayer* Player();
	class CGameWorld* GameWorld();
	class CGameContext* GameServer();
	class IServer* Server();
	int WorldID();
	const class IServer* Server() const;
	
	virtual int GetAmmoType() const = 0;
	virtual int GetMaxAmmo() const = 0;
	virtual int GetAmmo() const = 0;
	virtual bool AddAmmo(int Ammo) = 0;
	virtual bool IsAutomatic() const = 0;
	virtual bool IsWeaponSwitchLocked() const { return false; }
	
	virtual bool TickPaused(bool IsActive) { return !IsActive; }
	virtual bool TickPreFire(bool IsActive) = 0;
	virtual bool OnFire(vec2 Direction) = 0;
	virtual void OnActivation() {}
	
	virtual void Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pChar) = 0;
	virtual void Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pChar) = 0;
	virtual void Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pChar) = 0;
};

class CModAPI_Weapon_GenericGun07 : public CModAPI_Weapon
{
protected:
	int m_TW07ID;
	int m_ReloadTimer;
	int m_LastNoAmmoSound;
	int m_AmmoRegenStart;
	int m_Ammo;
	
public:
	CModAPI_Weapon_GenericGun07(int WID, int TW07ID, class CCharacter* pCharacter, int Ammo);
	
	virtual int GetMaxAmmo() const;
	virtual int GetAmmoType() const { return MODAPI_AMMOTYPE_INTEGER; }
	virtual int GetAmmo() const;
	virtual bool AddAmmo(int Ammo);
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPaused(bool IsActive);
	virtual bool TickPreFire(bool IsActive);
	virtual bool OnFire(vec2 Direction);
	virtual void OnActivation();

	virtual void Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pChar);
	virtual void Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pChar);
	virtual void Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pChar);
	
	virtual void CreateProjectile(vec2 Pos, vec2 Direction) = 0;
};

#endif
