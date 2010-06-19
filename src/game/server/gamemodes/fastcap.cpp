// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "fastcap.h"

CGameControllerFC::CGameControllerFC(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	m_pGameType = "FCap";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerFC::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;
	
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = 0;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = 1;
	if(Team == -1)
		return false;
		
	CFlag *F = new CFlag(&GameServer()->m_World, Team, Pos, 0x0);
	m_apFlags[Team] = F;
	return true;
}

bool CGameControllerFC::IsOwnFlagStand(vec2 Pos, int Team)
{
	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];
			
		if(F && F->m_Team == Team && distance(F->m_Pos, Pos) < 32)
			return true;
	}
	
	return false;
}

bool CGameControllerFC::IsEnemyFlagStand(vec2 Pos, int Team)
{
	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];
			
		if(F && F->m_Team != Team && distance(F->m_Pos, Pos) < 32)
			return true;
	}
	
	return false;
}

void CGameControllerFC::OnCharacterSpawn(class CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	
	//full armor
	pChr->IncreaseArmor(10);
	
	// give nades
	pChr->GiveWeapon(WEAPON_GRENADE, 10);
}

int CGameControllerFC::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	return 0;
}

bool CGameControllerFC::CanSpawn(class CPlayer *pPlayer, vec2 *pOutPos)
{
	CSpawnEval Eval;
	
	// spectators can't spawn
	if(pPlayer->GetTeam() == -1)
		return false;

	Eval.m_FriendlyTeam = pPlayer->GetTeam();
	
	// try first enemy spawns, than normal, than own
	EvaluateSpawnType(&Eval, 1+((pPlayer->GetTeam()+1)&1));
	if(!Eval.m_Got)
	{
		EvaluateSpawnType(&Eval, 0);
		if(!Eval.m_Got)
			EvaluateSpawnType(&Eval, 1+(pPlayer->GetTeam()&1));
	}
	
	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool CGameControllerFC::CanBeMovedOnBalance(int Cid)
{
	CCharacter* Character = GameServer()->m_apPlayers[Cid]->GetCharacter();
	if(Character)
	{
		for(int fi = 0; fi < 2; fi++)
		{
			CFlag *F = m_apFlags[fi];
			if(F->m_pCarryingCCharacter == Character)
				return false;
		}
	}
	return true;
}
