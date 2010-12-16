/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/*#include <game/mapitems.h>
=======
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "ctf.h"

CGameControllerCTF::CGameControllerCTF(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	m_pGameType = "CTF";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerCTF::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;
	
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = 0;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = 1;
	if(Team == -1 || m_apFlags[Team])
		return false;
		
	CFlag *F = new CFlag(&GameServer()->m_World, Team);
	F->m_StandPos = Pos;
	F->m_Pos = Pos;
	m_apFlags[Team] = F;
	GameServer()->m_World.InsertEntity(F);
	return true;
}

int CGameControllerCTF::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponID)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, WeaponID);
	int HadFlag = 0;
	
	// drop flags
	for(int i = 0; i < 2; i++)
	{
		CFlag *F = m_apFlags[i];
		if(F && pKiller && pKiller->GetCharacter() && F->m_pCarryingCharacter == pKiller->GetCharacter())
			HadFlag |= 2;
		if(F && F->m_pCarryingCharacter == pVictim)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			F->m_DropTick = Server()->Tick();
			F->m_pCarryingCharacter = 0;
			F->m_Vel = vec2(0,0);
			
			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
				pKiller->m_Score++;
				
			HadFlag |= 1;
		}
	}
	
	return HadFlag;
}

bool CGameControllerCTF::CanBeMovedOnBalance(int Cid)
{
	CCharacter* Character = GameServer()->m_apPlayers[Cid]->GetCharacter();
	if(Character)
	{
		for(int fi = 0; fi < 2; fi++)
		{
			CFlag *F = m_apFlags[fi];
			if(F->m_pCarryingCharacter == Character)
				return false;
		}
	}
	return true;
}

void CGameControllerCTF::Tick()
{
	IGameController::Tick();

	DoTeamScoreWincheck();
	
	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];
		
		if(!F)
			continue;
		
		// flag hits death-tile, reset it
		if(GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_DEATH)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
			F->Reset();
			continue;
		}
		
		//
		if(F->m_pCarryingCharacter)
		{
			// update flag position
			F->m_Pos = F->m_pCarryingCharacter->m_Pos;
			
			if(m_apFlags[fi^1] && m_apFlags[fi^1]->m_AtStand)
			{
				if(distance(F->m_Pos, m_apFlags[fi^1]->m_Pos) < CFlag::ms_PhysSize + CCharacter::ms_PhysSize)
				{
					// CAPTURE! \o/
					m_aTeamscore[fi^1] += 100;
					F->m_pCarryingCharacter->GetPlayer()->m_Score += 5;

					char aBuf[512];
					str_format(aBuf, sizeof(aBuf), "flag_capture player='%d:%s'",
						F->m_pCarryingCharacter->GetPlayer()->GetCID(),
						Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

					float CaptureTime = (Server()->Tick() - F->m_GrabTick)/(float)Server()->TickSpeed();
					if(CaptureTime <= 60)
					{
						str_format(aBuf, sizeof(aBuf), "The %s flag was captured by '%s' (%d.%s%d seconds)", fi ? "blue" : "red", Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()), (int)CaptureTime%60, ((int)(CaptureTime*100)%100)<10?"0":"", (int)(CaptureTime*100)%100);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "The %s flag was captured by '%s'", fi ? "blue" : "red", Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
					}
					GameServer()->SendChat(-1, -2, aBuf);
					for(int i = 0; i < 2; i++)
						m_apFlags[i]->Reset();
					
					GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
				}
			}
		}
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, NETOBJTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == -1 || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
					continue;
				
				if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == F->m_Number)
				{
					// return the flag
					if(!F->m_AtStand)
					{
						CCharacter *pChr = apCloseCCharacters[i];
						pChr->GetPlayer()->m_Score += 1;

						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "flag_return player='%d:%s'",
							pChr->GetPlayer()->GetCID(),
							Server()->ClientName(pChr->GetPlayer()->GetCID()));
						GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

						GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
						F->Reset();
					}
				}
				else
				{
					// take the flag
					if(F->m_AtStand)
					{
						m_aTeamscore[fi^1]++;
						F->m_GrabTick = Server()->Tick();
					}
					
					F->m_AtStand = 0;
					F->m_pCarryingCharacter = apCloseCCharacters[i];
					F->m_pCarryingCharacter->GetPlayer()->m_Score += 1;

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s'",
						F->m_pCarryingCharacter->GetPlayer()->GetCID(),
						Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					
					for(int c = 0; c < MAX_CLIENTS; c++)
					{
						if(!GameServer()->m_apPlayers[c])
							continue;
							
						if(GameServer()->m_apPlayers[c]->GetTeam() == fi)
							GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, GameServer()->m_apPlayers[c]->GetCID());
						else
							GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, GameServer()->m_apPlayers[c]->GetCID());
					}
					break;
				}
			}
			
			if(!F->m_pCarryingCharacter && !F->m_AtStand)
			{
				if(Server()->Tick() > F->m_DropTick + Server()->TickSpeed()*30)
				{
					GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
					F->Reset();
				}
				else
				{
					F->m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
					GameServer()->Collision()->MoveBox(&F->m_Pos, &F->m_Vel, vec2(F->ms_PhysSize, F->ms_PhysSize), 0.5f);
				}
			}
		}
	}
}
*/
