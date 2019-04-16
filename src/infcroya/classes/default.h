#pragma once

#include "class.h"

class CDefault : public IClass {
private:
	CSkin m_Skin;
	bool m_InfectedClass;
public:
	CDefault();
	~CDefault() override;
	CSkin& GetSkin();

	void OnCharacterSpawn(CCharacter* pChr) override;
	int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) override;

	bool IsInfectedClass() const override;
};