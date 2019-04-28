#include "default.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CDefault::CDefault()
{
	m_Skin = CSkin();
	m_InfectedClass = false;
}

CDefault::~CDefault()
{
}

const CSkin& CDefault::GetSkin() const
{
	return m_Skin;
}

void CDefault::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->SetInfected(m_InfectedClass);
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_LASER, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

int CDefault::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

void CDefault::OnWeaponFire(vec2 Direction, int Weapon)
{
}

bool CDefault::IsInfectedClass() const
{
	return m_InfectedClass;
}
