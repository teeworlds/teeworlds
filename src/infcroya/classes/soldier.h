#pragma once

#include "class.h"

class CSoldier : public IClass {
public:
	CSoldier();

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};