#pragma once

#include <infcroya/skin.h>

class IClass {
public:
	virtual ~IClass() {};
	virtual CSkin& GetSkin() = 0;

	virtual void OnCharacterSpawn(class CCharacter* pChr) = 0;
	virtual int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) = 0;

	virtual bool IsInfectedClass() const = 0;
};