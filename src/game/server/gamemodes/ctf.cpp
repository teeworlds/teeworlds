/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include "ctf.h"

CGameControllerCTF::CGameControllerCTF(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// game
	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	m_pGameType = "CTF";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

// balancing
bool CGameControllerCTF::CanBeMovedOnBalance(int ClientID) const
{
	CCharacter* Character = GameServer()->m_apPlayers[ClientID]->GetCharacter();
	if(Character)
	{
		for(int fi = 0; fi < 2; fi++)
		{
			CFlag *F = m_apFlags[fi];
			if(F && F->GetCarrier() == Character)
				return false;
		}
	}
	return true;
}

// event
int CGameControllerCTF::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponID)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, WeaponID);
	int HadFlag = 0;

	// drop flags
	for(int i = 0; i < 2; i++)
	{
		CFlag *F = m_apFlags[i];
		if(F && pKiller && pKiller->GetCharacter() && F->GetCarrier() == pKiller->GetCharacter())
			HadFlag |= 2;
		if(F && F->GetCarrier() == pVictim)
		{
			GameServer()->SendGameMsg(GAMEMSG_CTF_DROP, -1);
			F->Drop();

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
				pKiller->m_Score++;

			HadFlag |= 1;
		}
	}

	return HadFlag;
}

void CGameControllerCTF::OnFlagReturn(CFlag *pFlag)
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
	GameServer()->SendGameMsg(GAMEMSG_CTF_RETURN, -1);
}

bool CGameControllerCTF::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;

	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;
	if(Team == -1 || m_apFlags[Team])
		return false;

	CFlag *F = new CFlag(&GameServer()->m_World, Team, Pos);
	m_apFlags[Team] = F;
	return true;
}

// game
bool CGameControllerCTF::DoWincheckMatch()
{
	// check score win condition
	if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
	{
		if(m_SuddenDeath)
		{
			if(m_aTeamscore[TEAM_RED]/100 != m_aTeamscore[TEAM_BLUE]/100)
			{
				EndMatch();
				return true;
			}
		}
		else
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
			{
				EndMatch();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
}

// general
void CGameControllerCTF::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameDataFlag *pGameDataFlag = static_cast<CNetObj_GameDataFlag *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATAFLAG, 0, sizeof(CNetObj_GameDataFlag)));
	if(!pGameDataFlag)
		return;

	pGameDataFlag->m_FlagDropTickRed = 0;
	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->IsAtStand())
			pGameDataFlag->m_FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->GetCarrier() && m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierRed = m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierRed = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickRed = m_apFlags[TEAM_RED]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierRed = FLAG_MISSING;
	pGameDataFlag->m_FlagDropTickBlue = 0;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->IsAtStand())
			pGameDataFlag->m_FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->GetCarrier() && m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierBlue = m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierBlue = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickBlue = m_apFlags[TEAM_BLUE]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierBlue = FLAG_MISSING;
}

void CGameControllerCTF::Tick()
{
	IGameController::Tick();

	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];

		if(!F)
			continue;

		//
		if(F->GetCarrier())
		{
			if(m_apFlags[fi^1] && m_apFlags[fi^1]->IsAtStand())
			{
				if(distance(F->GetPos(), m_apFlags[fi^1]->GetPos()) < CFlag::ms_PhysSize + CCharacter::ms_PhysSize)
				{
					// CAPTURE! \o/
					m_aTeamscore[fi^1] += 100;
					F->GetCarrier()->GetPlayer()->m_Score += 5;
					float Diff = Server()->Tick() - F->GetGrabTick();

					char aBuf[64];
					str_format(aBuf, sizeof(aBuf), "flag_capture player='%d:%s' team=%d time=%.2f",
						F->GetCarrier()->GetPlayer()->GetCID(),
						Server()->ClientName(F->GetCarrier()->GetPlayer()->GetCID()),
						F->GetCarrier()->GetPlayer()->GetTeam(),
						Diff / (float)Server()->TickSpeed()
					);
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

					GameServer()->SendGameMsg(GAMEMSG_CTF_CAPTURE, fi, F->GetCarrier()->GetPlayer()->GetCID(), Diff, -1);
					for(int i = 0; i < 2; i++)
						m_apFlags[i]->Reset();
					// do a win check(capture could trigger win condition)
					if(DoWincheckMatch())
						return;
				}
			}
		}
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(F->GetPos(), CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->GetPos(), apCloseCCharacters[i]->GetPos(), NULL, NULL))
					continue;

				if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == F->GetTeam())
				{
					// return the flag
					if(!F->IsAtStand())
					{
						CCharacter *pChr = apCloseCCharacters[i];
						pChr->GetPlayer()->m_Score += 1;

						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "flag_return player='%d:%s' team=%d",
							pChr->GetPlayer()->GetCID(),
							Server()->ClientName(pChr->GetPlayer()->GetCID()),
							pChr->GetPlayer()->GetTeam()
						);
						GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
						GameServer()->SendGameMsg(GAMEMSG_CTF_RETURN, -1);
						F->Reset();
					}
				}
				else
				{
					// take the flag
					if(F->IsAtStand())
						m_aTeamscore[fi^1]++;

					F->Grab(apCloseCCharacters[i]);

					F->GetCarrier()->GetPlayer()->m_Score += 1;

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s' team=%d",
						F->GetCarrier()->GetPlayer()->GetCID(),
						Server()->ClientName(F->GetCarrier()->GetPlayer()->GetCID()),
						F->GetCarrier()->GetPlayer()->GetTeam()
					);
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					GameServer()->SendGameMsg(GAMEMSG_CTF_GRAB, fi, -1);
					break;
				}
			}
		}
	}
	// do a win check(grabbing flags could trigger win condition)
	DoWincheckMatch();
}
