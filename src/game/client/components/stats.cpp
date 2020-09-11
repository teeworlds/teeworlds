#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <game/client/animstate.h>
#include <game/client/components/menus.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <generated/client_data.h>
#include "stats.h"

CStats::CStats()
{
	OnReset();
}

void CStats::CPlayerStats::Reset()
{
	m_IngameTicks		= 0;
	m_Frags				= 0;
	m_Deaths			= 0;
	m_Suicides			= 0;
	m_BestSpree			= 0;
	m_CurrentSpree		= 0;
	for(int j = 0; j < NUM_WEAPONS; j++)
	{
		m_aFragsWith[j]		= 0;
		m_aDeathsFrom[j]	= 0;
	}
	m_FlagGrabs			= 0;
	m_FlagCaptures		= 0;
	m_CarriersKilled	= 0;
	m_KillsCarrying		= 0;
	m_DeathsCarrying	= 0;
}

void CStats::OnReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aStats[i].Reset();

	m_Active = false;
	m_Activate = false;

	m_ScreenshotTaken = false;
	m_ScreenshotTime = -1;
}

bool CStats::IsActive() const
{
	// force statboard after three seconds of game over if autostatscreenshot is on
	if(Config()->m_ClAutoStatScreenshot && m_ScreenshotTime > -1 && m_ScreenshotTime < time_get())
		return true;

	return m_Active;
}

void CStats::OnRelease()
{
	m_Active = false;
}

void CStats::ConKeyStats(IConsole::IResult *pResult, void *pUserData)
{
	CStats *pStats = (CStats *)pUserData;
	int Result = pResult->GetInteger(0);
	if(!Result)
	{
		pStats->m_Activate = false;
		pStats->m_Active = false;
	}
	else if(!pStats->m_Active)
		pStats->m_Activate = true;
}

void CStats::OnConsoleInit()
{
	Console()->Register("+stats", "", CFGFLAG_CLIENT, ConKeyStats, this, "Show stats");
}

void CStats::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		
		if(pMsg->m_Weapon != -3)	// team switch
			m_aStats[pMsg->m_Victim].m_Deaths++;
		m_aStats[pMsg->m_Victim].m_CurrentSpree = 0;
		if(pMsg->m_Weapon >= 0)
			m_aStats[pMsg->m_Victim].m_aDeathsFrom[pMsg->m_Weapon]++;
		if((pMsg->m_ModeSpecial & 1) && (pMsg->m_Weapon != -3))
			m_aStats[pMsg->m_Victim].m_DeathsCarrying++;
		if(pMsg->m_Victim != pMsg->m_Killer)
		{
			m_aStats[pMsg->m_Killer].m_Frags++;
			m_aStats[pMsg->m_Killer].m_CurrentSpree++;

			if(m_aStats[pMsg->m_Killer].m_CurrentSpree > m_aStats[pMsg->m_Killer].m_BestSpree)
				m_aStats[pMsg->m_Killer].m_BestSpree = m_aStats[pMsg->m_Killer].m_CurrentSpree;
			if(pMsg->m_Weapon >= 0)
				m_aStats[pMsg->m_Killer].m_aFragsWith[pMsg->m_Weapon]++;
			if(pMsg->m_ModeSpecial & 1)
				m_aStats[pMsg->m_Killer].m_CarriersKilled++;
			if(pMsg->m_ModeSpecial & 2)
				m_aStats[pMsg->m_Killer].m_KillsCarrying++;
		}
		else if(pMsg->m_Weapon != -3)
			m_aStats[pMsg->m_Victim].m_Suicides++;
	}
}

void CStats::OnRender()
{
	// TODO: ADDBACK: draw statboard
}

void CStats::UpdatePlayTime(int Ticks)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			m_aStats[i].m_IngameTicks += Ticks;
	}
}

void CStats::OnMatchStart()
{
	OnReset();
}

void CStats::OnFlagGrab(int ClientID)
{
	m_aStats[ClientID].m_FlagGrabs++;
}

void CStats::OnFlagCapture(int ClientID)
{
	m_aStats[ClientID].m_FlagCaptures++;
}

void CStats::OnPlayerEnter(int ClientID, int Team)
{
	m_aStats[ClientID].Reset();
}

void CStats::OnPlayerLeave(int ClientID)
{
	m_aStats[ClientID].Reset();
}

void CStats::AutoStatScreenshot()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->AutoStatScreenshot_Start();
}
