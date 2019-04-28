#include "smoker.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CSmoker::CSmoker()
{
	CSkin skin;
	skin.SetBodyColor(58, 200, 79);
	skin.SetMarkingName("cammostripes");
	skin.SetMarkingColor(0, 0, 0);
	skin.SetFeetColor(0, 79, 70);
	m_Skin = skin;
	m_InfectedClass = true;
}

CSmoker::~CSmoker()
{
}

const CSkin& CSmoker::GetSkin() const
{
	return m_Skin;
}

void CSmoker::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->SetInfected(m_InfectedClass);
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->SetNormalEmote(EMOTE_ANGRY);
}

int CSmoker::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

void CSmoker::OnWeaponFire(vec2 Direction, int Weapon)
{
}

bool CSmoker::IsInfectedClass() const
{
	return m_InfectedClass;
}
