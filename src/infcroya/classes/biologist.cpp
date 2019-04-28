#include "biologist.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CBiologist::CBiologist()
{
	CSkin skin;
	skin.SetBodyColor(52, 156, 124);
	skin.SetMarkingName("twintri");
	skin.SetMarkingColor(40, 222, 227);
	skin.SetFeetColor(147, 4, 72);
	m_Skin = skin;
	m_InfectedClass = false;
}

CBiologist::~CBiologist()
{
}

const CSkin& CBiologist::GetSkin() const
{
	return m_Skin;
}

void CBiologist::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->SetInfected(m_InfectedClass);
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_LASER, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

int CBiologist::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

void CBiologist::OnWeaponFire(vec2 Direction, int Weapon)
{
}

bool CBiologist::IsInfectedClass() const
{
	return m_InfectedClass;
}
