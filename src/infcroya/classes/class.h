#pragma once

#include <infcroya/skin.h>
#include <base/vmath.h>

class IClass {
public:
	virtual ~IClass() {};
	virtual const CSkin& GetSkin() const = 0;

	virtual void OnCharacterSpawn(class CCharacter* pChr) = 0;
	virtual int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon) = 0;

	virtual void OnWeaponFire(vec2 Direction, int Weapon) = 0;

	virtual bool IsInfectedClass() const = 0;
};

enum Class {
	DEFAULT = 0,
	BIOLOGIST,
	SMOKER,
};