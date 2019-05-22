#include "croyaplayer.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <infcroya/classes/class.h>
#include <infcroya/entities/biologist-mine.h>
#include <infcroya/localization/localization.h>
#include <game/server/gamemodes/mod.h>
#include <infcroya/entities/circle.h>
#include <infcroya/entities/inf-circle.h>
#include <limits>

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

	m_RespawnPointsNum = 1;
	m_RespawnPointsDefaultNum = 1;
	m_RespawnPointPlaced = false;
	m_RespawnPointPos = vec2(0, 0);
	m_RespawnPointDefaultCooldown = 3;
	m_RespawnPointCooldown = 0;
}

CroyaPlayer::~CroyaPlayer()
{
}

void CroyaPlayer::Tick()
{
	if (IsHuman() && m_pCharacter) {
		// Infect when inside infection zone circle
		auto circle = GetClosestInfCircle();
		if (circle) {
			float dist = distance(m_pCharacter->GetPos(), circle->GetPos());
			if (dist < circle->GetRadius()) {
				m_pCharacter->Infect(-1);
			}
		}

		// Take damage when outside of safezone circle
		auto inf_circle = GetClosestCircle();
		if (inf_circle) {
			float dist = distance(m_pCharacter->GetPos(), inf_circle->GetPos());
			int Dmg = 1;
			if (dist > inf_circle->GetRadius() && m_pGameServer->Server()->Tick() % m_pGameServer->Server()->TickSpeed() == 0) { // each second
				m_pCharacter->TakeDamage(vec2(0, 0), vec2(0, 0), Dmg, -1, WEAPON_SELF);
				printf("This is printed on win pc & linux vps, but only on windows pc take damage work??? wtf\n");
			}
		}
	}

	if (IsZombie() && m_pCharacter) {
		// Increase health when inside infection zone circle
		auto inf_circle = GetClosestInfCircle();
		if (inf_circle) {
			float dist = distance(m_pCharacter->GetPos(), inf_circle->GetPos());
			if (dist < inf_circle->GetRadius() && m_pGameServer->Server()->Tick() % m_pGameServer->Server()->TickSpeed() == 0) { // each second
				m_pCharacter->IncreaseHealth(2);
				m_RespawnPointsNum = m_RespawnPointsDefaultNum;
			}
		}
	}

	if (m_RespawnPointCooldown > 0) {
		if (m_pGameServer->Server()->Tick() % m_pGameServer->Server()->TickSpeed() == 0) { // each second
			m_RespawnPointCooldown -= 1;
		}
	}
}

CCircle* CroyaPlayer::GetClosestCircle()
{
	float min_distance = std::numeric_limits<float>::max();
	CCircle* closest = nullptr;
	for (CCircle* circle : m_pGameController->GetCircles()) {
		if (!m_pCharacter)
			break;
		if (!circle)
			continue; // maybe unnecessary, not sure

		float dist = distance(m_pCharacter->GetPos(), circle->GetPos()) - circle->GetRadius();
		if (dist < min_distance) {
			min_distance = dist;
			closest = circle;
		}
	}
	if (closest) {
		return closest;
	}
	return nullptr;
}

CInfCircle* CroyaPlayer::GetClosestInfCircle()
{
	float min_distance = std::numeric_limits<float>::max();
	CInfCircle* closest = nullptr;
	for (CInfCircle* circle : m_pGameController->GetInfCircles()) {
		if (!m_pCharacter)
			break;
		if (!circle)
			continue; // maybe unnecessary, not sure

		float dist = distance(m_pCharacter->GetPos(), circle->GetPos()) - circle->GetRadius();
		if (dist < min_distance) {
			min_distance = dist;
			closest = circle;
		}
	}
	if (closest) {
		return closest;
	}
	return nullptr;
}

int CroyaPlayer::GetClassNum()
{
	int ClassNum;
	for (const auto& c : m_Classes) {
		if (m_pClass == c.second) {
			ClassNum = c.first;
			break;
		}
	}
	return ClassNum;
}

void CroyaPlayer::SetClassNum(int Class, bool DrawPurpleThing)
{
	SetClass(m_Classes[Class], DrawPurpleThing);
}

CCharacter* CroyaPlayer::GetCharacter() {
	return m_pCharacter;
}

void CroyaPlayer::SetCharacter(CCharacter* pCharacter)
{
	m_pCharacter = pCharacter;
}

CPlayer* CroyaPlayer::GetPlayer()
{
	return m_pPlayer;
}

int CroyaPlayer::GetClientID() const
{
	return m_ClientID;
}

void CroyaPlayer::OnCharacterSpawn(CCharacter* pChr)
{
	m_pClass->OnCharacterSpawn(pChr);
	m_pCharacter->SetCroyaPlayer(this);
}

void CroyaPlayer::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	m_pClass->OnCharacterDeath(pVictim, pKiller, Weapon);
	if (IsHuman()) {
		SetOldClassNum(GetClassNum());
		TurnIntoRandomZombie();
	}
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
	if (m_pGameController->IsCroyaWarmup()) {
		TurnIntoNextHumanClass();
		m_pClass->OnMouseWheelDown();
	}
	if (IsZombie() && m_pCharacter && !m_pGameController->IsCroyaWarmup() && m_RespawnPointCooldown == 0) {
		if (m_RespawnPointPlaced) {
			m_RespawnPointPos = m_pCharacter->GetPos();
		}
		else {
			m_RespawnPointPos = m_pCharacter->GetPos();
			m_RespawnPointPlaced = true;
		}
		m_RespawnPointCooldown = m_RespawnPointDefaultCooldown;
	}
}

void CroyaPlayer::OnMouseWheelUp()
{
	if (m_pGameController->IsCroyaWarmup()) {
		TurnIntoPrevHumanClass();
		m_pClass->OnMouseWheelUp();
	}
}

vec2 CroyaPlayer::GetRespawnPointPos() const
{
	return m_RespawnPointPos;
}

int CroyaPlayer::GetRespawnPointsNum() const
{
	return m_RespawnPointsNum;
}

void CroyaPlayer::SetRespawnPointsNum(int Num)
{
	m_RespawnPointsNum = Num;
}

bool CroyaPlayer::IsRespawnPointPlaced() const
{
	return m_RespawnPointPlaced;
}

void CroyaPlayer::SetRespawnPointPlaced(bool Placed)
{
	m_RespawnPointPlaced = Placed;
}

int CroyaPlayer::GetRespawnPointDefaultCooldown() const
{
	return m_RespawnPointDefaultCooldown;
}

int CroyaPlayer::GetRespawnPointCooldown()
{
	return m_RespawnPointCooldown;
}

void CroyaPlayer::SetRespawnPointCooldown(int Cooldown)
{
	m_RespawnPointCooldown = Cooldown;
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
	int RandomZombieClass = random_int_range(Class::ZOMBIE_CLASS_START + 1, Class::ZOMBIE_CLASS_END - 1);
	SetClassNum(RandomZombieClass, true);
}

void CroyaPlayer::TurnIntoRandomHuman()
{
	// Class::HUMAN_CLASS_START + 2 because of DEFAULT. DEFAULT comes right after HUMAN_CLASS_START
	int RandomHumanClass = random_int_range(Class::HUMAN_CLASS_START + 2, Class::HUMAN_CLASS_END - 1);
	SetClassNum(RandomHumanClass, true);
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

int CroyaPlayer::GetOldClassNum() const
{
	return m_OldClassNum;
}

void CroyaPlayer::SetOldClassNum(int Class)
{
	m_OldClassNum = Class;
}

const char* CroyaPlayer::GetLanguage() const
{
	return m_Language.c_str();
}

void CroyaPlayer::SetLanguage(const char* Language)
{
	m_Language = Language;
}

CGameControllerMOD* CroyaPlayer::GetGameControllerMOD()
{
	return m_pGameController;
}

IClass* CroyaPlayer::GetClass()
{
	return m_pClass;
}

void CroyaPlayer::SetClass(IClass* pClass, bool DrawPurpleThing)
{
	if (m_pCharacter && DrawPurpleThing) {
		vec2 PrevPos = m_pCharacter->GetPos();
		m_pGameServer->CreatePlayerSpawn(PrevPos); // draw purple thing
	}
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
}