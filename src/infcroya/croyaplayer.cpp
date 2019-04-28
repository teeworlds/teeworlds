#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <infcroya/classes/class.h>
#include <infcroya/entities/biologist-mine.h>

CroyaPlayer::CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer)
{
	m_ClientID = ClientID;
	m_pPlayer = pPlayer;
	m_pGameServer = pGameServer;
	m_pCharacter = nullptr;
	m_Infected = false;
}

CroyaPlayer::~CroyaPlayer()
{
}

IClass* CroyaPlayer::GetClass()
{
	return m_pClass;
}

void CroyaPlayer::SetClass(IClass* pClass)
{
	for (int& each : m_pPlayer->m_TeeInfos.m_aUseCustomColors) {
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

	if (m_pClass->IsInfectedClass()) {
		m_Infected = true;
	}
	else {
		m_Infected = false;
	}

	for (CPlayer* each : m_pGameServer->m_apPlayers) {
		if (each) {
			m_pGameServer->SendSkinChange(m_pPlayer->GetCID(), each->GetCID());
		}
	}
}

void CroyaPlayer::SetCharacter(CCharacter* pCharacter)
{
	m_pCharacter = pCharacter;
}

void CroyaPlayer::OnCharacterSpawn(CCharacter* pChr)
{
	m_pClass->OnCharacterSpawn(pChr);
	m_pCharacter->SetCroyaPlayer(this);
}

void CroyaPlayer::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	m_pClass->OnCharacterDeath(pVictim, pKiller, Weapon);
	m_pCharacter = nullptr;
}

void CroyaPlayer::OnWeaponLaser(vec2 Direction)
{
	CGameWorld* pGameWorld = m_pCharacter->GameWorld();
	for (CBiologistMine* pMine = (CBiologistMine*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_BIOLOGIST_MINE); pMine; pMine = (CBiologistMine*)pMine->TypeNext())
	{
		if (pMine->m_Owner != m_pPlayer->GetCID()) continue;
		m_pGameServer->m_World.DestroyEntity(pMine);
	}

	vec2 To = m_pCharacter->GetPos() + Direction * 400.0f;
	if (m_pGameServer->Collision()->IntersectLine(m_pCharacter->GetPos(), To, 0x0, &To))
	{
		new CBiologistMine(pGameWorld, m_pCharacter->GetPos(), To, m_pPlayer->GetCID());
		m_pGameServer->CreateSound(m_pCharacter->GetPos(), SOUND_LASER_FIRE);
		m_pCharacter->m_aWeapons[WEAPON_LASER].m_Ammo = 0;
	}
}

bool CroyaPlayer::IsHuman() const
{
	return !m_Infected;
}

bool CroyaPlayer::IsZombie() const
{
	return m_Infected;
}
