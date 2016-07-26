#ifndef MOD_WEAPON_GUN_H
#define MOD_WEAPON_GUN_H

#include <base/vmath.h>
#include <modapi/server/weapon.h>

class CMod_Weapon_Gun : public CModAPI_Weapon_GenericGun07
{
public:
	CMod_Weapon_Gun(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const { return false; }
	virtual void CreateProjectile(vec2 Pos, vec2 Direction);
};

#endif
