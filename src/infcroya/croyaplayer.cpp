#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <infcroya/classes/class.h>
#include <infcroya/entities/biologist-mine.h>

CroyaPlayer::CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer, std::unordered_map<int, IClass*> Classes)
{
	m_ClientID = ClientID;
	m_pPlayer = pPlayer;
	m_pGameServer = pGameServer;
	m_pCharacter = nullptr;
	m_Infected = false;
	m_Classes = Classes;
}

CroyaPlayer::~CroyaPlayer()
{
}

int CroyaPlayer::GetClassNum()
{
	int ClassNum;
	for (auto& c : m_Classes) {
		if (m_pClass == c.second)
			ClassNum = c.first;
	}
	return ClassNum;
}

void CroyaPlayer::SetClassNum(int Class)
{
	SetClass(m_Classes[Class]);
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
	if (m_pCharacter) {
		m_pCharacter->SetInfected(m_pClass->IsInfectedClass());
		m_pCharacter->ResetWeaponsHealth();
		m_pClass->InitialWeaponsHealth(m_pCharacter);
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

void CroyaPlayer::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon)
{
	m_pClass->OnWeaponFire(Direction, ProjStartPos, Weapon, m_pCharacter);
}

bool CroyaPlayer::IsHuman() const
{
	return !m_Infected;
}

bool CroyaPlayer::IsZombie() const
{
	return m_Infected;
}

std::unordered_map<int, class IClass*>& CroyaPlayer::GetClasses()
{
	return m_Classes;
}

void CroyaPlayer::SetNextHumanClass()
{
	int NextClass = GetClassNum() + 1;
	int FirstClass = Class::HUMAN_CLASS_START + 1;
	bool NotInRange = !(NextClass > HUMAN_CLASS_START && NextClass < HUMAN_CLASS_END);

	if (NextClass == Class::HUMAN_CLASS_END || NotInRange)
		NextClass = FirstClass;
	SetClassNum(NextClass);
}

void CroyaPlayer::SetPrevHumanClass()
{
	int PrevClass = GetClassNum() - 1;
	int LastClass = Class::HUMAN_CLASS_END - 1;
	bool NotInRange = !(PrevClass > HUMAN_CLASS_START && PrevClass < HUMAN_CLASS_END);

	if (PrevClass == Class::HUMAN_CLASS_START || NotInRange)
		PrevClass = LastClass;
	SetClassNum(PrevClass);
}
