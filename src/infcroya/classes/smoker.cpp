#include "smoker.h"
#include "base/system.h"
#include <game/server/entities/character.h>

CSmoker::CSmoker()
{
	m_BodyColor = HSLtoInt(58, 200, 79);
	m_MarkingColor = HSLtoInt(0, 0, 0);
	m_FeetColor = HSLtoInt(0, 79, 70);
	char SkinName[24];
	str_format(SkinName, sizeof(SkinName), "cammostripes");
	str_copy(m_MarkingName, SkinName, 24);
}

CSmoker::~CSmoker()
{
}

int CSmoker::GetBodyColor() const
{
	return m_BodyColor;
}

int CSmoker::GetMarkingColor() const
{
	return m_MarkingColor;
}

int CSmoker::GetFeetColor() const
{
	return m_FeetColor;
}

const char* CSmoker::GetMarkingName() const
{
	return m_MarkingName;
}

void CSmoker::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
}
