#pragma once

#include <infcroya/skin.h>
#include <base/vmath.h>
#include <unordered_map>

class IClass {
private:
	std::string m_Name;
	CSkin m_Skin;
	bool m_Infected;
public:
	static std::unordered_map<int, class IClass*> classes;
	virtual ~IClass();

	virtual void InitialWeaponsHealth(class CCharacter* pChr) = 0;

	virtual void OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, class CCharacter* pChr) = 0;
	virtual void OnMouseWheelDown();
	virtual void OnMouseWheelUp();

	virtual void OnCharacterSpawn(class CCharacter* pChr);
	virtual int OnCharacterDeath(class CCharacter* pVictim, class CPlayer* pKiller, int Weapon);

	virtual const CSkin& GetSkin() const;
	virtual void SetSkin(CSkin skin);

	virtual bool IsInfectedClass() const;
	virtual void SetInfectedClass(bool Infected);

	virtual std::string GetName() const;
	virtual void SetName(std::string Name);
};

enum Class {
	HUMAN_CLASS_START = 0,
	DEFAULT,
	BIOLOGIST,
	ENGINEER,
	MEDIC,
	SOLDIER,
	SCIENTIST,
	MERCENARY,
	HUMAN_CLASS_END,

	ZOMBIE_CLASS_START,
	SMOKER,
	ZOMBIE_CLASS_END,
};