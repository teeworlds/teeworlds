#pragma once

#include "class.h"

class CMedic : public IClass {
public:
	CMedic();
	~CMedic() override;

	void InitialWeaponsHealth(class CCharacter* pChr) override;

	void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) override;
};