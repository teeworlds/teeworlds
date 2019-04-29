#include "default.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CDefault::CDefault()
{
	SetSkin(CSkin());
	SetInfectedClass(false);
}

CDefault::~CDefault()
{
}

void CDefault::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

void CDefault::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
}