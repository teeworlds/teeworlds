#pragma once

#include "class.h"

class CSmoker : public IClass {
public:
	CSmoker();
	~CSmoker() override;

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};