#include "class.h"
#include <game/server/entities/character.h>

IClass::~IClass()
{
}

void IClass::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->SetInfected(IsInfectedClass());
	pChr->ResetWeaponsHealth();
	InitialWeaponsHealth(pChr);
}

int IClass::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	return 0;
}

const CSkin& IClass::GetSkin() const
{
	return m_Skin;
}

void IClass::SetSkin(CSkin skin)
{
	m_Skin = skin;
}

bool IClass::IsInfectedClass() const
{
	return m_Infected;
}

void IClass::SetInfectedClass(bool Infected)
{
	m_Infected = Infected;
}

std::string IClass::GetName() const
{
	return m_Name;
}

void IClass::SetName(std::string Name)
{
	m_Name = Name;
}