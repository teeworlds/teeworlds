/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mod.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <vector>
#include <algorithm>
#include <infcroya/croyaplayer.h>
#include <infcroya/classes/default.h>
#include <infcroya/classes/biologist.h>
#include <infcroya/classes/smoker.h>
#include <engine/shared/config.h>
#ifdef CONF_GEOLOCATION
	#include <infcroya/geolocation/geolocation.h>
#endif

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "InfCroya";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode

#ifdef CONF_GEOLOCATION
	geolocation = new Geolocation("GeoLite2-Country.mmdb");
#endif
	classes[Class::DEFAULT] = new CDefault();
	classes[Class::BIOLOGIST] = new CBiologist();
	classes[Class::SMOKER] = new CSmoker();
}

CGameControllerMOD::~CGameControllerMOD()
{
#ifdef CONF_GEOLOCATION
	delete geolocation;
#endif
	for (const auto& c : classes) {
		delete c.second;
	}
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();
}

void CGameControllerMOD::OnCharacterSpawn(CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();

	players[ClientID]->SetCharacter(pChr);
	players[ClientID]->OnCharacterSpawn(pChr);
}

int CGameControllerMOD::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	int ClientID = pVictim->GetPlayer()->GetCID();

	players[ClientID]->OnCharacterDeath(pVictim, pKiller, Weapon);
	return 0;
}

void CGameControllerMOD::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();

#ifdef CONF_GEOLOCATION
	char aAddrStr[NETADDR_MAXSTRSIZE];
	Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));
	std::string ip(aAddrStr);
	Server()->SetClientCountry(ClientID, geolocation->get_country_iso_numeric_code(ip));
#endif
	players[ClientID] = new CroyaPlayer(ClientID, pPlayer, GameServer(), this, classes);
	players[ClientID]->SetClass(classes[Class::DEFAULT]);
	GameServer()->SendChat(-1, CHAT_ALL, ClientID, g_Config.m_SvWelcome);
}

void CGameControllerMOD::OnPlayerDisconnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerDisconnect(pPlayer);
	int ClientID = pPlayer->GetCID();

	delete players[ClientID];
	players[ClientID] = nullptr;
}

bool CGameControllerMOD::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if (players[ClientID1]->IsHuman() == players[ClientID2]->IsHuman() && ClientID1 != ClientID2)
		return true;
	return false;
}

bool CGameControllerMOD::IsEveryoneInfected() const
{
	bool EveryoneInfected = true;
	for (CPlayer* each : GameServer()->m_apPlayers) {
		if (each) {
			CCharacter* pChr = each->GetCharacter();
			if (pChr && !pChr->IsInfected()) {
				EveryoneInfected = false;
				break;
			}
		}
	}
	return EveryoneInfected;
}
