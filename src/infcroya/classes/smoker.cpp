#include "smoker.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CSmoker::CSmoker() : IClass()
{
	CSkin skin;
	skin.SetBodyColor(58, 200, 79);
	skin.SetMarkingName("cammostripes");
	skin.SetMarkingColor(0, 0, 0);
	skin.SetFeetColor(0, 79, 70);
	SetSkin(skin);
	SetInfectedClass(true);
}

CSmoker::~CSmoker()
{
}

void CSmoker::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->SetNormalEmote(EMOTE_ANGRY);
}

void CSmoker::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	switch (Weapon) {
	case WEAPON_HAMMER:

		break;
	}
}