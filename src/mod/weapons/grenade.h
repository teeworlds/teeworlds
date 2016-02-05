#ifndef MOD_WEAPON_GRENADE_H
#define MOD_WEAPON_GRENADE_H

#include <base/vmath.h>
#include <modapi/server/weapon.h>

class CMod_Weapon_Grenade : public CModAPI_Weapon_GenericGun07
{
public:
	CMod_Weapon_Grenade(class CCharacter* pCharacter, int Ammo);
	
	virtual bool IsAutomatic() const { return true; }
	virtual void CreateProjectile(vec2 Pos, vec2 Direction);
};

#endif
