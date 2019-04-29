#pragma once

#include "class.h"

class CDefault : public IClass {
public:
	CDefault();
	~CDefault() override;

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};