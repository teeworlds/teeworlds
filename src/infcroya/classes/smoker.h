#pragma once

#include "class.h"

class CSmoker : public IClass {
private:
	CSkin m_Skin;
	bool m_InfectedClass;
public:
	CSmoker();
	~CSmoker() override;
	CSkin& GetSkin() override;

	void OnCharacterSpawn(CCharacter* pChr) override;
	int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) override;

	bool IsInfectedClass() const override;
};