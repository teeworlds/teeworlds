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

CSkin& CDefault::GetSkin()
{
	return m_Skin;
}

void CDefault::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

int CDefault::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

bool CDefault::IsInfectedClass() const
{
	return m_InfectedClass;
}
