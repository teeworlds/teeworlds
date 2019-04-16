#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include "classes/class.h"

CroyaPlayer::CroyaPlayer(int ClientID, CPlayer* pPlayer)
{
	m_ClientID = ClientID;
	m_pPlayer = pPlayer;
	m_pCharacter = nullptr;
	m_Infected = false;
}

CroyaPlayer::~CroyaPlayer()
{
}

void CroyaPlayer::SetClass(IClass* pClass)
{
	for (auto& each : m_pPlayer->m_TeeInfos.m_aUseCustomColors) {
		each = 1;
	}
	m_pClass = pClass;
	auto& skin = m_pClass->GetSkin();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[0] = skin.GetBodyColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[1] = skin.GetMarkingColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[4] = skin.GetFeetColor();
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1]), 
		"%s", skin.GetMarkingName()
		);
	if (m_pClass->IsInfectedClass())
		m_Infected = true;
}

void CroyaPlayer::SetCharacter(CCharacter* pCharacter)
{
	m_pCharacter = pCharacter;
}

void CroyaPlayer::OnCharacterSpawn()
{
	m_pClass->OnCharacterSpawn(m_pCharacter);
}

bool CroyaPlayer::IsHuman() const
{
	return !m_Infected;
}

bool CroyaPlayer::IsZombie() const
{
	return m_Infected;
}
