#ifndef MOD_WEAPON_HAMMER_H
#define MOD_WEAPON_HAMMER_H

#include <base/vmath.h>
#include <modapi/server/weapon.h>

class CMod_Weapon_Hammer : public CModAPI_Weapon
{
protected:
	int m_ReloadTimer;

public:
	CMod_Weapon_Hammer(class CCharacter* pCharacter);
	
	virtual int GetMaxAmmo() const { return -1; }
	virtual int GetAmmoType() const { return MODAPI_AMMOTYPE_INFINITE; }
	virtual int GetAmmo() const { return -1; }
	virtual bool AddAmmo(int Ammo) { return false; }
	virtual bool IsAutomatic() const { return false; }
	virtual bool IsWeaponSwitchLocked() const;
	
	virtual bool TickPreFire(bool IsActive);
	virtual bool OnFire(vec2 Direction);
};

#endif
