#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <infcroya/classes/class.h>
#include <infcroya/entities/biologist-mine.h>
#include <infcroya/localization/localization.h>
#include <game/server/gamemodes/mod.h>

CroyaPlayer::CroyaPlayer(int ClientID, CPlayer* pPlayer, CGameContext* pGameServer, CGameControllerMOD* pGameController, std::unordered_map<int, IClass*> Classes)
{
	m_ClientID = ClientID;
	m_pPlayer = pPlayer;
	m_pPlayer->SetCroyaPlayer(this);
	m_pGameServer = pGameServer;
	m_pGameController = pGameController;
	m_pCharacter = nullptr;
	m_Infected = false;
	m_HookProtected = true;
	m_Classes = Classes;
	m_Language = "english";
}

CroyaPlayer::~CroyaPlayer()
{
}

int CroyaPlayer::GetClassNum()
{
	int ClassNum;
	for (const auto& c : m_Classes) {
		if (m_pClass == c.second)
			ClassNum = c.first;
	}
	return ClassNum;
}

void CroyaPlayer::SetClassNum(int Class, bool DrawPurpleThing)
{
	SetClass(m_Classes[Class], DrawPurpleThing);
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
	if (IsHuman())
		TurnIntoRandomZombie();
	m_pCharacter = nullptr;
}

void CroyaPlayer::OnKill(int Victim)
{
	int64 Mask = CmaskOne(m_ClientID);
	m_pGameServer->CreateSound(m_pPlayer->m_ViewPos, SOUND_CTF_GRAB_PL, Mask);
	if (IsZombie()) {
		m_pPlayer->m_Score += 3;
	}
	else {
		m_pPlayer->m_Score++;
	}
}

void CroyaPlayer::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon)
{
	m_pClass->OnWeaponFire(Direction, ProjStartPos, Weapon, m_pCharacter);
}

void CroyaPlayer::OnButtonF3()
{
	SetHookProtected(!IsHookProtected());
}

void CroyaPlayer::OnMouseWheelDown()
{
	TurnIntoNextHumanClass();
	m_pClass->OnMouseWheelDown();
}

void CroyaPlayer::OnMouseWheelUp()
{
	TurnIntoPrevHumanClass();
	m_pClass->OnMouseWheelUp();
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

void CroyaPlayer::TurnIntoNextHumanClass()
{
	int NextClass = GetClassNum() + 1;
	int FirstClass = Class::HUMAN_CLASS_START + 1;
	bool NotInRange = !(NextClass > HUMAN_CLASS_START && NextClass < HUMAN_CLASS_END);

	if (NextClass == Class::HUMAN_CLASS_END || NotInRange)
		NextClass = FirstClass;
	SetClassNum(NextClass);
}

void CroyaPlayer::TurnIntoPrevHumanClass()
{
	int PrevClass = GetClassNum() - 1;
	int LastClass = Class::HUMAN_CLASS_END - 1;
	bool NotInRange = !(PrevClass > HUMAN_CLASS_START && PrevClass < HUMAN_CLASS_END);

	if (PrevClass == Class::HUMAN_CLASS_START || NotInRange)
		PrevClass = LastClass;
	SetClassNum(PrevClass);
}

void CroyaPlayer::TurnIntoRandomZombie()
{
	int RandomZombieClass = random_int() % (Class::ZOMBIE_CLASS_END - Class::ZOMBIE_CLASS_START - 1) + Class::ZOMBIE_CLASS_START + 1;
	SetClassNum(RandomZombieClass, true);
}

bool CroyaPlayer::IsHookProtected() const
{
	return m_HookProtected;
}

void CroyaPlayer::SetHookProtected(bool HookProtected)
{
	m_HookProtected = HookProtected;
	if (m_pCharacter) {
		m_pCharacter->SetHookProtected(HookProtected);
		if (IsHookProtected())
			m_pGameServer->SendChatTarget(m_ClientID, "Hook protection enabled");
		else
			m_pGameServer->SendChatTarget(m_ClientID, "Hook protection disabled");
	}
}

const char* CroyaPlayer::GetLanguage() const
{
	return m_Language.c_str();
}

void CroyaPlayer::SetLanguage(const char* Language)
{
	m_Language = Language;
}

IClass* CroyaPlayer::GetClass()
{
	return m_pClass;
}

void CroyaPlayer::SetClass(IClass* pClass, bool DrawPurpleThing)
{
	// purple animation begin, got from old infclass CGameContext::CreatePlayerSpawn(vec2 Pos)
	if (m_pCharacter && DrawPurpleThing) {
		vec2 PrevPos = m_pCharacter->GetPos();
		CNetEvent_Spawn* ev = (CNetEvent_Spawn*)m_pGameServer->m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn));
		if (ev)
		{
			ev->m_X = (int)PrevPos.x;
			ev->m_Y = (int)PrevPos.y;
		}
	}
	// purple animation end
	for (int& each : m_pPlayer->m_TeeInfos.m_aUseCustomColors) {
		each = 1;
	}
	m_pClass = pClass;
	const CSkin& skin = m_pClass->GetSkin();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[0] = skin.GetBodyColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[1] = skin.GetMarkingColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[2] = skin.GetDecorationColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[3] = skin.GetHandsColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[4] = skin.GetFeetColor();
	m_pPlayer->m_TeeInfos.m_aSkinPartColors[5] = skin.GetEyesColor();
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[0], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[0]), "%s", skin.GetBodyName());
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[1]), "%s", skin.GetMarkingName());
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[2], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[2]), "%s", skin.GetDecorationName());
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[3], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[3]), "%s", skin.GetHandsName());
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[4], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[4]), "%s", skin.GetFeetName());
	str_format(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[5], sizeof(m_pPlayer->m_TeeInfos.m_aaSkinPartNames[5]), "%s", skin.GetEyesName());

	if (m_pClass->IsInfectedClass()) {
		m_Infected = true;
	}
	else {
		m_Infected = false;
	}

	for (const CPlayer* each : m_pGameServer->m_apPlayers) {
		if (each) {
			m_pGameServer->SendSkinChange(m_pPlayer->GetCID(), each->GetCID());
		}
	}
	if (m_pCharacter) {
		m_pCharacter->DestroyChildEntities();
		m_pCharacter->SetInfected(m_pClass->IsInfectedClass());
		m_pCharacter->ResetWeaponsHealth();
		m_pClass->InitialWeaponsHealth(m_pCharacter);
	}

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s", m_pClass->GetName().c_str());
	if (str_comp(GetLanguage(), "english") == 0)
		m_pGameServer->SendBroadcast(aBuf, m_pPlayer->GetCID());
	else
		m_pGameServer->SendBroadcast(Localize(aBuf, GetLanguage()), m_pPlayer->GetCID());
	if (m_pGameController->IsEveryoneInfected()) {
	}
}
