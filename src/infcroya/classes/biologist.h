#pragma once

#include "class.h"

class CBiologist : public IClass {
public:
	CBiologist();
	~CBiologist() override;

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};