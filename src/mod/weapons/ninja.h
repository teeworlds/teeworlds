#ifndef MOD_WEAPON_NINJA_H
#define MOD_WEAPON_NINJA_H

#include <base/vmath.h>
#include <modapi/server/weapon.h>

class CMod_Weapon_Ninja : public CModAPI_Weapon
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
	CMod_Weapon_Ninja(class CCharacter* pCharacter);
	
	virtual int GetMaxAmmo() const;
	virtual int GetAmmoType() const { return MODAPI_AMMOTYPE_TIME; }
	virtual int GetAmmo() const;
	virtual bool AddAmmo(int Ammo) { return true; }
	virtual bool IsAutomatic() const { return false; }
	virtual bool IsWeaponSwitchLocked() const { return true; }
	
	virtual bool TickPaused(bool IsActive);
	virtual bool TickPreFire(bool IsActiveWeapon);
	virtual bool OnFire(vec2 Direction);
	
	virtual void Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pChar);
	virtual void Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pChar);
	virtual void Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pChar);
};

#endif
