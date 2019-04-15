#include "default.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CDefault::CDefault()
{
	m_BodyColor = HSLtoInt(27, 111, 116);
	m_MarkingColor = HSLtoInt(0, 0, 0);
	m_FeetColor = HSLtoInt(28, 135, 62);
}

CDefault::~CDefault()
{
}

int CDefault::GetBodyColor() const
{
	return m_BodyColor;
}

int CDefault::GetMarkingColor() const
{
	return m_MarkingColor;
}

int CDefault::GetFeetColor() const
{
	return m_FeetColor;
}

const char* CDefault::GetMarkingName() const
{
	return m_MarkingName;
}

void CDefault::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
}
