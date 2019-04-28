#pragma once

#include "class.h"

class CBiologist : public IClass {
private:
	CSkin m_Skin;
	bool m_InfectedClass;
public:
	CBiologist();
	~CBiologist() override;
	const CSkin& GetSkin() const override;

	void OnCharacterSpawn(CCharacter* pChr) override;
	int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) override;

	void OnWeaponFire(vec2 Direction, int Weapon) override;

	bool IsInfectedClass() const override;
};