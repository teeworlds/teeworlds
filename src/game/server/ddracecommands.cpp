#include "gamecontext.h"
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared vote options");

	CNetMsg_Sv_VoteClearOptions ClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&ClearOptionsMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), -1, 0);
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), 1, 0);
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), 0, 1);
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), 0, -1);
}

void CGameContext::ConMove(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), pResult->GetInteger(0), pResult->GetInteger(1));
}

void CGameContext::ConMoveRaw(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MoveCharacter(ClientId, pResult->GetVictim(), pResult->GetInteger(0), pResult->GetInteger(1), true);
}

void CGameContext::MoveCharacter(int ClientId, int Victim, int X, int Y, bool Raw)
{
	CCharacter* pChr = GetPlayerChar(ClientId);
	
	if(!pChr)
		return;

	pChr->m_Core.m_Pos.x += ((Raw) ? 1 : 32) * X;
	pChr->m_Core.m_Pos.y += ((Raw) ? 1 : 32) * Y;

	if(!g_Config.m_SvCheatTime)
		pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConMute(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	int Seconds = pResult->GetInteger(0);
	char buf[512];
	if(Seconds < 10)
		Seconds = 10;

	if(pSelf->m_apPlayers[Victim]->m_Muted < Seconds * pSelf->Server()->TickSpeed())
	{
		pSelf->m_apPlayers[Victim]->m_Muted = Seconds * pSelf->Server()->TickSpeed();
		str_format(buf, sizeof(buf), "%s muted by %s for %d seconds", pSelf->Server()->ClientName(Victim), pSelf->Server()->ClientName(ClientId), Seconds);
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, buf);
	}
}

void CGameContext::ConSetlvl3(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 3;
		pServ->SetRconLevel(Victim, 3);
	}
}

void CGameContext::ConSetlvl2(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 2;
		pServ->SetRconLevel(Victim, 2);
	}
}

void CGameContext::ConSetlvl1(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Authed = 1;
		pServ->SetRconLevel(Victim, 1);
	}
}

void CGameContext::ConLogOut(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CServer* pServ = (CServer*)pSelf->Server();
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Authed = -1;
		pServ->SetRconLevel(Victim, -1);
	}
}

void CGameContext::ConKillPlayer(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->KillCharacter(WEAPON_GAME);
		char buf[512];
		str_format(buf, sizeof(buf), "%s was killed by %s", pSelf->Server()->ClientName(Victim), pSelf->Server()->ClientName(ClientId));
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, buf);
	}
}

void CGameContext::ConNinja(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_NINJA, false);
}


void CGameContext::ConHammer(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	char buf[128];
	int type = pResult->GetInteger(0);

	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	
	if(!chr)
		return;
	
	CServer* pServ = (CServer*)pSelf->Server();
	if(type>3 || type<0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Select hammer between 0 and 3");
	}
	else
	{
		chr->m_HammerType = type;
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
		str_format(buf, sizeof(buf), "Hammer of '%s' ClientId=%d setted to %d", pServ->ClientName(ClientId), Victim, type);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConSuper(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(chr && !chr->m_Super)
	{
		chr->m_Super = true;
		chr->UnFreeze();
		chr->m_TeamBeforeSuper = chr->Team();
		dbg_msg("Teamb4super","%d",chr->m_TeamBeforeSuper = chr->Team());
		chr->Teams()->SetCharacterTeam(Victim, TEAM_SUPER);
		if(!g_Config.m_SvCheatTime)
			chr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConUnSuper(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(chr && chr->m_Super)
	{
		chr->m_Super = false;
		chr->Teams()->SetForceCharacterTeam(Victim, chr->m_TeamBeforeSuper);
	}
}

void CGameContext::ConShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_SHOTGUN, false);
}

void CGameContext::ConGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_GRENADE, false);
}

void CGameContext::ConRifle(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_RIFLE, false);
}

void CGameContext::ConWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), -1, false);
}

void CGameContext::ConUnShotgun(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_SHOTGUN, true);
}

void CGameContext::ConUnGrenade(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_GRENADE, true);
}

void CGameContext::ConUnRifle(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), WEAPON_RIFLE, true);
}

void CGameContext::ConUnWeapons(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), -1, true);
}

void CGameContext::ConAddWeapon(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), pResult->GetInteger(0), false);
}

void CGameContext::ConRemoveWeapon(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(ClientId, pResult->GetVictim(), pResult->GetInteger(0), true);
}

void CGameContext::ModifyWeapons(int ClientId, int Victim, int Weapon, bool Remove)
{
	if(clamp(Weapon, -1, NUM_WEAPONS - 1) != Weapon)
	{
		Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "invalid weapon id");
		return;
	}
	
	CCharacter* pChr = GetPlayerChar(Victim);
	if(!pChr)
		return;
	
	if(Weapon == -1)
	{
		if(Remove && (pChr->m_ActiveWeapon == WEAPON_SHOTGUN || pChr->m_ActiveWeapon == WEAPON_GRENADE || pChr->m_ActiveWeapon == WEAPON_RIFLE))
			pChr->m_ActiveWeapon = WEAPON_GUN;
		
		if(Remove)
		{
			pChr->m_aWeapons[WEAPON_SHOTGUN].m_Got = false;
			pChr->m_aWeapons[WEAPON_GRENADE].m_Got = false;
			pChr->m_aWeapons[WEAPON_RIFLE].m_Got = false;
		}
		else
			pChr->GiveAllWeapons();	
	}
	else if(Weapon != WEAPON_NINJA)
	{
		if(Remove && pChr->m_ActiveWeapon == Weapon)
			pChr->m_ActiveWeapon = WEAPON_GUN;
		
		if(Remove)
			pChr->m_aWeapons[Weapon].m_Got = false;
		else
			pChr->GiveWeapon(Weapon, -1);
	}
	else
	{
		if(Remove)
		{
			Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "you can't remove ninja");
			return;
		}
		
		pChr->GiveNinja();
	}

	if(!Remove && !g_Config.m_SvCheatTime)
		pChr->m_DDRaceState =	DDRACE_CHEAT;
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	int TeleTo = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS-1);
	if(pSelf->m_apPlayers[TeleTo])
	{
		{
			CCharacter* chr = pSelf->GetPlayerChar(Victim);
			if(chr)
			{
				chr->m_Core.m_Pos = pSelf->m_apPlayers[TeleTo]->m_ViewPos;
				if(!g_Config.m_SvCheatTime)
					chr->m_DDRaceState = DDRACE_CHEAT;
			}
		}
	}
}

void CGameContext::ConTimerStop(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CServer* pServ = (CServer*)pSelf->Server();
	int Victim = pResult->GetVictim();

	char buf[128];
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	if(pSelf->m_apPlayers[Victim])
	{
		chr->m_DDRaceState=DDRACE_CHEAT;
		str_format(buf, sizeof(buf), "'%s' ClientId=%d Hasn't time now (Timer Stopped)", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConTimerStart(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CServer* pServ = (CServer*)pSelf->Server();
	int Victim = pResult->GetVictim();

	char buf[128];
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	if(pSelf->m_apPlayers[Victim])
	{
		chr->m_DDRaceState = DDRACE_STARTED;
		str_format(buf, sizeof(buf), "'%s' ClientId=%d Has time now (Timer Started)", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConTimerZero(IConsole::IResult *pResult, void *pUserData, int ClientId)
{

	CGameContext *pSelf = (CGameContext *)pUserData;
	CServer* pServ = (CServer*)pSelf->Server();
	int Victim = pResult->GetVictim();

	char buf[128];
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	if(pSelf->m_apPlayers[Victim])
	{
		chr->m_StartTime = pSelf->Server()->Tick();
		chr->m_RefreshTime = pSelf->Server()->Tick();
		chr->m_DDRaceState=DDRACE_CHEAT;
		str_format(buf, sizeof(buf), "'%s' ClientId=%d time has been reset & stopped.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConTimerReStart(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CServer* pServ = (CServer*)pSelf->Server();

	char buf[128];
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	if(pSelf->m_apPlayers[Victim])
	{
		chr->m_StartTime = pSelf->Server()->Tick();
		chr->m_RefreshTime = pSelf->Server()->Tick();
		chr->m_DDRaceState=DDRACE_STARTED;
		str_format(buf, sizeof(buf), "'%s' ClientId=%d time has been reset & stopped.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int time=-1;
	int Victim = pResult->GetVictim();

	char buf[128];

	if(pResult->NumArguments() >= 1)
		time = clamp(pResult->GetInteger(1), -1, 29999);
	
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	
	if(pSelf->m_apPlayers[Victim])
	{
		chr->Freeze(((time!=0&&time!=-1)?(pSelf->Server()->TickSpeed()*time):(-1)));
		chr->m_pPlayer->m_RconFreeze = true;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d has been Frozen.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}

}

void CGameContext::ConUnFreeze(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	char buf[128];
	CCharacter* chr = pSelf->GetPlayerChar(Victim);
	if(!chr)
		return;
	chr->m_FreezeTime=2;
	chr->m_pPlayer->m_RconFreeze = false;
	CServer* pServ = (CServer*)pSelf->Server();
	str_format(buf, sizeof(buf), "'%s' ClientId=%d has been defrosted.", pServ->ClientName(ClientId), Victim);
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
}

void CGameContext::ConInvis(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char buf[128];
	int Victim = pResult->GetVictim();

	if(!pSelf->m_apPlayers[ClientId])
		return;

	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = true;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d is now invisible.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConVis(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(!pSelf->m_apPlayers[ClientId])
		return;
	char buf[128];
	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->m_Invisible = false;
		CServer* pServ = (CServer*)pSelf->Server();
		str_format(buf, sizeof(buf), "'%s' ClientId=%d is visible.", pServ->ClientName(ClientId), Victim);
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", buf);
	}
}

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "This mod was originally created by 3DA");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Now it is maintained & coded by:");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "[Egypt]GreYFoX@GTi and [BlackTee]den among others:");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Others Helping on the code: heinrich5991, noother");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Documentation: Zeta-Hoernchen, Entities: Fisico");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Code (in the past): LemonFace and Fluxid");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Please check the changelog on DDRace.info.");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Also the commit log on github.com/GreYFoXGTi/DDRace.");
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "DDRace Mod. Version: " DDRACE_VERSION);
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Official site: DDRace.info");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "For more Info /cmdlist");
	pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Or visit DDRace.info");
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pResult->NumArguments() == 0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "/cmdlist will show a list of all chat commands");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "/help + any command will show you the help for this command");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Example /help settings will display the help about ");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		IConsole::CCommandInfo *pCmdInfo = pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER);
		if(pCmdInfo && pCmdInfo->m_pHelp)
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", pCmdInfo->m_pHelp);
		else
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConSettings(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pResult->NumArguments() == 0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "to check a server setting say /settings and setting's name, setting names are:");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "teams, cheats, collision, hooking, endlesshooking, me, ");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "hitting, oldlaser, timeout, votes, pause and scores");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		char aBuf[256];
		float ColTemp;
		float HookTemp;
		pSelf->m_Tuning.Get("player_collision", &ColTemp);
		pSelf->m_Tuning.Get("player_hooking", &HookTemp);
		if(str_comp(pArg, "cheats") == 0)
		{
			str_format(aBuf, sizeof(aBuf), "%s%s",
					g_Config.m_SvCheats?"People can cheat":"People can't cheat",
					(g_Config.m_SvCheats)?(g_Config.m_SvCheatTime)?" with time":" without time":"");
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
			if(g_Config.m_SvCheats)
			{
				str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_SvEndlessSuperHook?"super can hook you forever":"super can only hook you for limited time");
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
				str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_SvTimer?"admins have the power to control your time":"admins have no power over your time");
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
			}
		}
		else if(str_comp(pArg, "teams") == 0)
		{
			str_format(aBuf, sizeof(aBuf), "%s %s", !g_Config.m_SvTeam?"Teams are available on this server":g_Config.m_SvTeam==-1?"Teams are not available on this server":"You have to be in a team to play on this server", !g_Config.m_SvTeamStrict?"and if you die in a team only you die":"and if you die in a team all of you die");
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
		}
		else if(str_comp(pArg, "collision") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", ColTemp?"Players can collide on this server":"Players Can't collide on this server");
		}
		else if(str_comp(pArg, "hooking") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", HookTemp?"Players can hook each other on this server":"Players Can't hook each other on this server");
		}
		else if(str_comp(pArg, "endlesshooking") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvEndlessDrag?"Players can hook time is unlimited":"Players can hook time is limited");
		}
		else if(str_comp(pArg, "hitting") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvHit?"Player's weapons affect each other":"Player's weapons has no affect on each other");
		}
		else if(str_comp(pArg, "oldlaser") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvOldLaser?"Lasers can hit you if you shot them and that they pull you towards the bounce origin (Like DDRace Beta)":"Lasers can't hit you if you shot them, and they pull others towards the shooter");
		}
		else if(str_comp(pArg, "me") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvSlashMe?"Players can use /me commands the famous IRC Command":"Players Can't use the /me command");
		}
		else if(str_comp(pArg, "timeout") == 0)
		{
			str_format(aBuf, sizeof(aBuf), "The Server Timeout is currently set to %d", g_Config.m_ConnTimeout);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
		}
		else if(str_comp(pArg, "votes") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvVoteKick?"Players can use Callvote menu tab to kick offenders":"Players Can't use the Callvote menu tab to kick offenders");
			if(g_Config.m_SvVoteKick)
				str_format(aBuf, sizeof(aBuf), "Players are banned for %d second(s) if they get voted off", g_Config.m_SvVoteKickBantime);
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvVoteKickBantime?aBuf:"Players are just kicked and not banned if they get voted off");
		}
		else if(str_comp(pArg, "pause") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvPauseable?g_Config.m_SvPauseTime?"/pause is available on this server and it pauses your time too":"/pause is available on this server but it doesn't pause your time":"/pause is NOT available on this server");
		}
		else if(str_comp(pArg, "scores") == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvHideScore?"Scores are private on this server":"Scores are public on this server");
		}
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	bool printed=false;
	if(g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No blocking.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No insulting / spamming.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No fun voting / vote spamming.");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Breaking any of these rules will result in a penalty, decided by server admins.");
		printed=true;
	}
	if(g_Config.m_SvRulesLine1[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine1);
		printed=true;
	}
	if(g_Config.m_SvRulesLine2[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine2);
		printed=true;
	}
	if(g_Config.m_SvRulesLine3[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine3);
		printed=true;
	}
	if(g_Config.m_SvRulesLine4[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine4);
		printed=true;
	}
	if(g_Config.m_SvRulesLine5[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine5);
		printed=true;
	}
	if(g_Config.m_SvRulesLine6[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine6);
		printed=true;
	}
	if(g_Config.m_SvRulesLine7[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine7);
		printed=true;
	}
	if(g_Config.m_SvRulesLine8[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine8);
		printed=true;
	}
	if(g_Config.m_SvRulesLine9[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine9);
		printed=true;
	}
	if(g_Config.m_SvRulesLine10[0])
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", g_Config.m_SvRulesLine10);
		printed=true;
	}
	if(!printed)
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No Rules Defined, Kill em all!!");
}

void CGameContext::ConKill(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(pPlayer->m_Last_Kill && pPlayer->m_Last_Kill + pSelf->Server()->TickSpeed() * g_Config.m_SvKillDelay > pSelf->Server()->Tick())
		return;

	pPlayer->m_Last_Kill = pSelf->Server()->Tick();
	pPlayer->KillCharacter(WEAPON_SELF);
	pPlayer->m_RespawnTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * g_Config.m_SvSuicidePenalty;
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(g_Config.m_SvPauseable)
	{
		CCharacter* chr = pPlayer->GetCharacter();
		if(!pPlayer->GetTeam() && chr && (!chr->m_aWeapons[WEAPON_NINJA].m_Got || chr->m_FreezeTime) && chr->IsGrounded() && chr->m_Pos==chr->m_PrevPos && !pPlayer->m_InfoSaved)
		{
			if(pPlayer->m_Last_Pause + pSelf->Server()->TickSpeed() * g_Config.m_SvPauseFrequency <= pSelf->Server()->Tick()) {
				pPlayer->SaveCharacter();
				pPlayer->SetTeam(-1);
				pPlayer->m_InfoSaved = true;
				pPlayer->m_Last_Pause = pSelf->Server()->Tick();
			}
			else
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can\'t pause that often.");
		}
		else if(pPlayer->GetTeam()==-1 && pPlayer->m_InfoSaved)
		{
			pPlayer->m_InfoSaved = false;
			pPlayer->m_PauseInfo.m_Respawn = true;
			pPlayer->SetTeam(0);
			//pPlayer->LoadCharacter();//TODO:Check if this system Works
		}
		else if(chr)
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (chr->m_aWeapons[WEAPON_NINJA].m_Got)?"You can't use /pause while you are a ninja":(!chr->IsGrounded())?"You can't use /pause while you are a in air":"You can't use /pause while you are moving");
		else
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "No pause data saved.");
	}
	else
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Pause isn't allowed on this server.");
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(g_Config.m_SvHideScore)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Showing the top 5 is not allowed on this server.");
		return;
	}

	if(pResult->NumArguments() > 0)
		pSelf->Score()->ShowTop5(pPlayer->GetCID(), pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTop5(pPlayer->GetCID());
}

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(/*g_Config.m_SvSpamprotection && */pPlayer->m_Last_Chat && pPlayer->m_Last_Chat + pSelf->Server()->TickSpeed() + g_Config.m_SvChatDelay > pSelf->Server()->Tick())
		return;
	
	pPlayer->m_Last_Chat = pSelf->Server()->Tick();

	if(pResult->NumArguments() > 0)
		if(!g_Config.m_SvHideScore)
			pSelf->Score()->ShowRank(pPlayer->GetCID(), pResult->GetString(0), true);
		else
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pPlayer->GetCID(), pSelf->Server()->ClientName(ClientId));
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(g_Config.m_SvTeam == -1) {
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Admin disable teams");
		return;
	} else if (g_Config.m_SvTeam == 1) {
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You must join to any team and play with anybody or you will not play");
	}
	CPlayer *pPlayer = pSelf->m_apPlayers[ClientId];

	if(pResult->NumArguments() > 0)
	{
		if(pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if(((CGameControllerDDRace*)pSelf->m_pController)->m_Teams.SetCharacterTeam(pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d", pSelf->Server()->ClientName(pPlayer->GetCID()), pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			}
			else
			{
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You cannot join this team");
			}
		}
	}
	else
	{
		char aBuf[512];
		if(pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "You are in team %d", pPlayer->GetCharacter()->Team());
			pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", aBuf);
		}
	}
}


void CGameContext::ConToggleFly(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter* pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();
	if(pChr && pChr->m_Super)
	{
		pChr->m_Fly = !pChr->m_Fly;
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pChr->m_Fly) ? "Fly enabled" : "Fly disabled");
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256 + 24];
	
	str_format(aBuf, 256 + 24, "'%s' %s", pSelf->Server()->ClientName(ClientId), pResult->GetString(0));
	if(g_Config.m_SvSlashMe)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, ClientId);
	else
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "/me is disabled on this server, admin can enable it by using sv_slash_me");
}

void CGameContext::ConToggleEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();

	if(pChr)
	{
		pChr->m_EyeEmote = !pChr->m_EyeEmote;
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", (pChr->m_EyeEmote) ? "You can now use the preset eye emotes." : "You don't have any eye emotes, remember to bind some. (until you die)");
	}
}

void CGameContext::ConToggleBroadcast(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();

	if(pChr)
		pChr->m_BroadCast = !pChr->m_BroadCast;
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData, int ClientId)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	CCharacter *pChr = pSelf->m_apPlayers[ClientId]->GetCharacter();
	
	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else 
	{
	  if (pChr)
		{
			if (!str_comp(pResult->GetString(0), "angry"))
			  pChr->m_EmoteType = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
			  pChr->m_EmoteType = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
			  pChr->m_EmoteType = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
			  pChr->m_EmoteType = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
			  pChr->m_EmoteType = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
			  pChr->m_EmoteType = EMOTE_SURPRISE;
			else
			{
				pSelf->Console()->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "info", "Unkown emote... Say /emote");
			}
			
			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);
			  
			pChr->m_EmoteStop = pSelf->Server()->Tick() + Duration * pSelf->Server()->TickSpeed();
		}
	}
}
