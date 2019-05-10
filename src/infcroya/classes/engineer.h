#pragma once

#include "class.h"

class CEngineer : public IClass {
public:
	CEngineer();

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};