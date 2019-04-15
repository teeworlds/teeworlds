#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include "classes/class.h"

CroyaPlayer::CroyaPlayer(int ClientID, CPlayer* pPlayer)
{
	m_ClientID = ClientID;
	m_pPlayer = pPlayer;
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
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[0] = m_pClass->GetBodyColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[1] = m_pClass->GetMarkingColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[4] = m_pClass->GetFeetColor();
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1]), 
		"%s", m_pClass->GetMarkingName()
		);
}

void CroyaPlayer::SetCharacter(CCharacter* pCharacter)
{
	m_pCharacter = pCharacter;
}

void CroyaPlayer::OnCharacterSpawn()
{
	m_pClass->OnCharacterSpawn(m_pCharacter);
}
