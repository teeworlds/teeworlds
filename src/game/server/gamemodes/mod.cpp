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
#include <infcroya/classes/smoker.h>

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "InfCroya";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode

	classes["Default"] = new CDefault();
	classes["Smoker"] = new CSmoker();
}

CGameControllerMOD::~CGameControllerMOD()
{
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
	players[ClientID]->OnCharacterSpawn();
}

int CGameControllerMOD::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	int ClientID = pVictim->GetPlayer()->GetCID();

	players[ClientID]->SetCharacter(nullptr);
	players[ClientID]->SetClass(classes["Smoker"]);
	for (const auto each : GameServer()->m_apPlayers) {
		if (each) {
			GameServer()->SendSkinChange(each->GetCID(), each->GetCID());
			each->m_Score = 0;
		}
	}

	return 0;
}

void CGameControllerMOD::OnPlayerConnect(CPlayer* pPlayer)
{
	int ClientID = pPlayer->GetCID();

	Server()->SetClientCountry(ClientID, 36);
	players[ClientID] = new CroyaPlayer(ClientID, pPlayer);

	players[ClientID]->SetClass(classes["Default"]);

	for (const auto each : GameServer()->m_apPlayers) {
		if (each) {
			GameServer()->SendSkinChange(each->GetCID(), each->GetCID());
			each->m_Score = 0;
		}
	}
}

void CGameControllerMOD::OnPlayerDisconnect(CPlayer* pPlayer)
{
	int ClientID = pPlayer->GetCID();

	delete players[ClientID];
	players[ClientID] = nullptr;
}