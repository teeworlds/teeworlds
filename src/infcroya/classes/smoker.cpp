#include "smoker.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <limits>

CSmoker::CSmoker()
{
	CSkin skin;
	skin.SetBodyColor(58, 200, 79);
	skin.SetMarkingColor(0, 0, 0);
	skin.SetFeetColor(0, 79, 70);
	skin.SetMarkingName("cammostripes");
	m_Skin = skin;
	m_InfectedClass = true;
}

CSmoker::~CSmoker()
{
}

CSkin& CSmoker::GetSkin()
{
	return m_Skin;
}

void CSmoker::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->SetNormalEmote(EMOTE_ANGRY);
}

int CSmoker::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

bool CSmoker::IsInfectedClass() const
{
	return m_InfectedClass;
}
