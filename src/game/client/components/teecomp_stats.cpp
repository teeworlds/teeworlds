#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <game/generated/client_data.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/teecomp.h>
#include "teecomp_stats.h"

CTeecompStats::CTeecompStats()
{
	m_Mode = 0;
	m_StatsClientID = -1;
	m_ScreenshotTaken = false;
	m_ScreenshotTime = -1;
}

void CTeecompStats::OnReset()
{
	for(int i=0; i<MAX_CLIENTS; i++)
		m_pClient->m_aStats[i].Reset();
	m_Mode = 0;
	m_StatsClientID = -1;
	m_ScreenshotTaken = false;
	m_ScreenshotTime = -1;
}

void CTeecompStats::ConKeyStats(IConsole::IResult *pResult, void *pUserData)
{
	if(pResult->GetInteger(0) != 0)
		((CTeecompStats *)pUserData)->m_Mode = pResult->GetInteger(1);
	else
		((CTeecompStats *)pUserData)->m_Mode = 0;
}

void CTeecompStats::ConKeyNext(IConsole::IResult *pResult, void *pUserData)
{
	CTeecompStats *pStats = (CTeecompStats *)pUserData;
	if(pStats->m_Mode != 2)
		return;

	if(pResult->GetInteger(0) == 0)
	{
		pStats->m_StatsClientID++;
		pStats->m_StatsClientID %= MAX_CLIENTS;
		pStats->CheckStatsClientID();
	}
}

void CTeecompStats::OnConsoleInit()
{
	Console()->Register("+stats", "i", CFGFLAG_CLIENT, ConKeyStats, this, "Show stats");
	Console()->Register("+next_stats", "", CFGFLAG_CLIENT, ConKeyNext, this, "Next player Stats");
}

bool CTeecompStats::IsActive()
{
	return (m_Mode > 0);
}

void CTeecompStats::CheckStatsClientID()
{
	if(m_StatsClientID == -1)
		m_StatsClientID = m_pClient->m_Snap.m_LocalClientID;

	int Prev = m_StatsClientID;
	while(!m_pClient->m_aStats[m_StatsClientID].m_Active)
	{
		m_StatsClientID++;
		m_StatsClientID %= MAX_CLIENTS;
		if(m_StatsClientID == Prev)
		{
			m_StatsClientID = -1;
			m_Mode = 0;
			break;
		}
	}
}

void CTeecompStats::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		CGameClient::CClientStats *pStats = m_pClient->m_aStats;

		pStats[pMsg->m_Victim].m_Deaths++;
		pStats[pMsg->m_Victim].m_CurrentSpree = 0;
		if(pMsg->m_Weapon >= 0)
			pStats[pMsg->m_Victim].m_aDeathsFrom[pMsg->m_Weapon]++;
		if(pMsg->m_ModeSpecial & 1)
			pStats[pMsg->m_Victim].m_DeathsCarrying++;
		if(pMsg->m_Victim != pMsg->m_Killer)
		{
			pStats[pMsg->m_Killer].m_Frags++;
			pStats[pMsg->m_Killer].m_CurrentSpree++;
			
			// play spree sound
			if(g_Config.m_ClSpreesounds && m_pClient->m_Snap.m_LocalClientID == pMsg->m_Killer && pStats[pMsg->m_Killer].m_CurrentSpree % 5 == 0)
			{
				int SpreeType = pStats[pMsg->m_Killer].m_CurrentSpree/5 - 1;
				switch(SpreeType)
				{
				case 0:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_KILLINGSPREE, 0);
					break;
				case 1:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_RAMPAGE, 0);
					break;
				case 2:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_DOMINATING, 0);
					break;
				case 3:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_UNSTOPPABLE, 0);
					break;
				case 4:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_GODLIKE, 0);
				}
			}
			
			if(pStats[pMsg->m_Killer].m_CurrentSpree > pStats[pMsg->m_Killer].m_BestSpree)
				pStats[pMsg->m_Killer].m_BestSpree = pStats[pMsg->m_Killer].m_CurrentSpree;
			if(pMsg->m_Weapon >= 0)
				pStats[pMsg->m_Killer].m_aFragsWith[pMsg->m_Weapon]++;
			if(pMsg->m_ModeSpecial & 1)
				pStats[pMsg->m_Killer].m_CarriersKilled++;
			if(pMsg->m_ModeSpecial & 2)
				pStats[pMsg->m_Killer].m_KillsCarrying++;
		}
		else
			pStats[pMsg->m_Victim].m_Suicides++;
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID < 0)
		{
			const char *p;
			const char *pLookFor = "flag was captured by '";
			if(str_find(pMsg->m_pMessage, pLookFor) != 0)
			{
				// server info
				CServerInfo CurrentServerInfo;
				Client()->GetServerInfo(&CurrentServerInfo);
			
				p = str_find(pMsg->m_pMessage, pLookFor);
				char aName[64];
				p += str_length(pLookFor);
				str_copy(aName, p, sizeof(aName));
				// remove capture time
				if(str_comp(aName+str_length(aName)-9, " seconds)") == 0)
				{
					char *c = aName+str_length(aName)-10;
					while(c > aName)
					{
						c--;
						if(*c == '(')
						{
							*(c-1) = 0;
							break;
						}
					}
				}
				// remove the ' at the end
				aName[str_length(aName)-1] = 0;
				
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!m_pClient->m_aStats[i].m_Active)
						continue;

					if(str_comp(m_pClient->m_aClients[i].m_aName, aName) == 0)
					{
						m_pClient->m_aStats[i].m_FlagCaptures++;
						break;
					}
				}
			}
		}
	}
}

void CTeecompStats::OnRender()
{
	// auto stat screenshot stuff
	if(g_Config.m_TcStatScreenshot)
	{
		if(m_ScreenshotTime < 0 && m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			m_ScreenshotTime = time_get()+time_freq()*3;
		
		if(m_ScreenshotTime > -1 && m_ScreenshotTime < time_get())
			m_Mode = 1;

		if(!m_ScreenshotTaken && m_ScreenshotTime > -1 && m_ScreenshotTime+time_freq()/5 < time_get())
		{
			AutoStatScreenshot();
			m_ScreenshotTaken = true;
		}
	}
	
	switch(m_Mode)
	{
		case 1:
			RenderGlobalStats();
			break;
		case 2:
			RenderIndividualStats();
			break;
	}
}

void CTeecompStats::RenderGlobalStats()
{
	if(m_Mode != 1)
		return;

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	float w = 250.0f;
	float h = 750.0f;
	
	const CNetObj_PlayerInfo *apPlayers[MAX_CLIENTS] = {0};
	int NumPlayers = 0;
	int i = 0;

	// sort red or dm players by score
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];

		if(!pInfo || !m_pClient->m_aStats[pInfo->m_ClientID].m_Active || pInfo->m_Team != TEAM_RED)
			continue;

		apPlayers[NumPlayers] = pInfo;
		NumPlayers++;
	}

	// sort blue players by score after
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];

			if(!pInfo || !m_pClient->m_aStats[pInfo->m_ClientID].m_Active || pInfo->m_Team != TEAM_BLUE)
				continue;

			apPlayers[NumPlayers] = pInfo;
			NumPlayers++;
		}
	}

	for(i = 0; i < 9; i++)
		if(g_Config.m_TcStatboardInfos & (1<<i))
		{
			if((1<<i) == (TC_STATS_BESTSPREE))
				w += 140;
			else
				w += 100;
		}
		
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGCAPTURES)
		w += 100;

	bool aDisplayWeapon[NUM_WEAPONS] = {false};
	if(g_Config.m_TcStatboardInfos & TC_STATS_WEAPS)
	{
		w += 10;
		for(i=0; i<NumPlayers; i++)
		{
			const CGameClient::CClientStats pStats = m_pClient->m_aStats[apPlayers[i]->m_ClientID];
			for(int j=0; j<NUM_WEAPONS; j++)
				aDisplayWeapon[j] = aDisplayWeapon[j] || pStats.m_aFragsWith[j] || pStats.m_aDeathsFrom[j];
		}
		for(i=0; i<NUM_WEAPONS; i++)
			if(aDisplayWeapon[i])
				w += 80;
	}

	float x = Width/2-w/2;
	float y = 200.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 17.0f);
	Graphics()->QuadsEnd();

	float tw;
	int px = 325;

	TextRender()->Text(0, x+10, y-5, 24.0f, Localize("Name"), -1);
	const char *apHeaders[] = { Localize("Frags"), Localize("Deaths"), Localize("Suicides"), Localize("Ratio"), Localize("Net"), Localize("FPM"), Localize("Spree"), Localize("Best spree"), Localize("Grabs") };
	for(i = 0; i < 9; i++)
		if(g_Config.m_TcStatboardInfos & (1<<i))
		{
			if(1<<i == TC_STATS_BESTSPREE)
				px += 40.0f;
			tw = TextRender()->TextWidth(0, 24.0f, apHeaders[i], -1);
			TextRender()->Text(0, x+px-tw, y-5, 24.0f, apHeaders[i], -1);
			px += 100;
		}

	if(g_Config.m_TcStatboardInfos & TC_STATS_WEAPS)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		for(i = 0, px-=40; i < NUM_WEAPONS; i++)
		{
			if(!aDisplayWeapon[i])
				continue;

			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[i].m_pSpriteBody);
			if(i == 0)
				RenderTools()->DrawSprite(x+px, y+10, g_pData->m_Weapons.m_aId[i].m_VisualSize*0.8);
			else
				RenderTools()->DrawSprite(x+px, y+10, g_pData->m_Weapons.m_aId[i].m_VisualSize);
			px += 80;
		}
		Graphics()->QuadsEnd();
		px += 40;
	}

	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGCAPTURES)
	{
		px -= 40;
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation(0.78f);
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
		RenderTools()->DrawSprite(x+px, y+15, 48);
		Graphics()->QuadsEnd();
	}

	y += 29.0f;

	float FontSize = 30.0f;
	float LineHeight = 50.0f;
	float TeeSizemod = 1.0f;
	float TeeOffset = 0.0f;
	
	if(NumPlayers > 14)
	{
		FontSize = 30.0f;
		LineHeight = 40.0f;
		TeeSizemod = 0.8f;
		TeeOffset = -5.0f;
	}

	for(int j = 0; j < NumPlayers; j++)
	{
		const CNetObj_PlayerInfo *pInfo = apPlayers[j];
		const CGameClient::CClientStats Stats = m_pClient->m_aStats[pInfo->m_ClientID];

		if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
		{
			// background so it's easy to find the local player
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1,1,1,0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20, LineHeight*0.95f, 17.0f);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo Teeinfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
		Teeinfo.m_Size *= TeeSizemod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &Teeinfo, EMOTE_NORMAL, vec2(1,0), vec2(x+28, y+28+TeeOffset));

		char aBuf[128];
		if(g_Config.m_TcStatId)
		{
			str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_ClientID);
			TextRender()->Text(0, x, y, FontSize, aBuf, -1);
		}

		CTextCursor Cursor;
		tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
		TextRender()->SetCursor(&Cursor, x+64, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 220;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
		//TextRender()->Text(0, x+64, y, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

		px = 325;

		if(g_Config.m_TcStatboardInfos & TC_STATS_FRAGS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Frags);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_DEATHS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_SUICIDES)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Suicides);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_RATIO)
		{
			if(Stats.m_Deaths == 0)
				str_format(aBuf, sizeof(aBuf), "--");
			else
				str_format(aBuf, sizeof(aBuf), "%.2f", (float)(Stats.m_Frags)/Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_NET)
		{
			str_format(aBuf, sizeof(aBuf), "%+d", Stats.m_Frags-Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_FPM)
		{
			float Fpm = (float)(Stats.m_Frags*60)/((float)(Client()->GameTick()-Stats.m_JoinDate)/Client()->GameTickSpeed());
			str_format(aBuf, sizeof(aBuf), "%.1f", Fpm);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_SPREE)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_CurrentSpree);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_TcStatboardInfos & TC_STATS_BESTSPREE)
		{
			px += 40;
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_BestSpree);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGGRABS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_FlagGrabs);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		for(i = 0, px=px-40; i < NUM_WEAPONS; i++)
		{
			if(!aDisplayWeapon[i])
				continue;

			str_format(aBuf, sizeof(aBuf), "%d/%d", Stats.m_aFragsWith[i], Stats.m_aDeathsFrom[i]);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x+px-tw/2, y, FontSize, aBuf, -1);
			px += 80;
		}
		if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGCAPTURES)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_FlagCaptures);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		y += LineHeight;
	}
}

void CTeecompStats::RenderIndividualStats()
{
	if(m_Mode != 2)
		return;
	CheckStatsClientID();
	if(m_Mode != 2)
		return;
	int m_ClientID = m_StatsClientID;
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	float w = 1200.0f;
	float x = Width/2-w/2;
	float y = 100.0f;
	float xo = 200.0f;
	float FontSize = 30.0f;
	float LineHeight = 40.0f;
	const CGameClient::CClientStats m_aStats = m_pClient->m_aStats[m_ClientID];

	Graphics()->MapScreen(0, 0, Width, Height);

	// header with name and score
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, 120.0f, 17.0f);
	Graphics()->QuadsEnd();

	CTeeRenderInfo Teeinfo = m_pClient->m_aClients[m_ClientID].m_RenderInfo;
	Teeinfo.m_Size *= 1.5f;
	RenderTools()->RenderTee(CAnimState::GetIdle(), &Teeinfo, EMOTE_NORMAL, vec2(1,0), vec2(x+xo+32, y+36));
	TextRender()->Text(0, x+xo+128, y, 48.0f, m_pClient->m_aClients[m_ClientID].m_aName, -1);

	char aBuf[64];
	if(g_Config.m_TcStatId)
	{
		str_format(aBuf, sizeof(aBuf), "%d", m_ClientID);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	}

	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score"), m_pClient->m_Snap.m_paPlayerInfos[m_ClientID]->m_Score);
	TextRender()->Text(0, x+xo, y+64, FontSize, aBuf, -1);
	int Seconds = (float)(Client()->GameTick()-m_aStats.m_JoinDate)/Client()->GameTickSpeed();
	str_format(aBuf, sizeof(aBuf), "%s: %02d:%02d", Localize("Time played"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+xo+256, y+64, FontSize, aBuf, -1);

	y += 150.0f;

	// Frags, etc. stats
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, 100.0f, 17.0f);
	Graphics()->QuadsEnd();

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f,1.0f,1.0f,1.0f);
	RenderTools()->SelectSprite(SPRITE_EYES);
	IGraphics::CQuadItem QuadItem(x+xo/2, y+40, 128, 128);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();

	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Frags"), m_aStats.m_Frags);
	TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Deaths"), m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo+200.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Suicides"), m_aStats.m_Suicides);
	TextRender()->Text(0, x+xo+400.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Spree"), m_aStats.m_CurrentSpree);
	TextRender()->Text(0, x+xo+600.0f, y, FontSize, aBuf, -1);
	y += LineHeight;

	if(m_aStats.m_Deaths == 0)
		str_format(aBuf, sizeof(aBuf), "%s: --", Localize("Ratio"));
	else
		str_format(aBuf, sizeof(aBuf), "%s: %.2f", Localize("Ratio"), (float)(m_aStats.m_Frags)/m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Net"), m_aStats.m_Frags-m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo+200.0f, y, FontSize, aBuf, -1);
	float Fpm = (float)(m_aStats.m_Frags*60)/((float)(Client()->GameTick()-m_aStats.m_JoinDate)/Client()->GameTickSpeed());
	str_format(aBuf, sizeof(aBuf), "%s: %.1f", Localize("FPM"), Fpm);
	TextRender()->Text(0, x+xo+400.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Best spree"), m_aStats.m_BestSpree);
	TextRender()->Text(0, x+xo+600.0f, y, FontSize, aBuf, -1);
	y+= LineHeight + 30.0f;

	// Weapon stats
	bool aDisplayWeapon[NUM_WEAPONS] = {false};
	int NumWeaps = 0;
	for(int i=0; i<NUM_WEAPONS; i++)
		if(m_aStats.m_aFragsWith[i] || m_aStats.m_aDeathsFrom[i])
		{
			aDisplayWeapon[i] = true;
			NumWeaps++;
		}

	if(NumWeaps)
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, LineHeight*(1+NumWeaps)+20.0f, 17.0f);
		Graphics()->QuadsEnd();

		TextRender()->Text(0, x+xo, y, FontSize, Localize("Frags"), -1);
		TextRender()->Text(0, x+xo+200.0f, y, FontSize, Localize("Deaths"), -1);
		y += LineHeight;

		for(int i=0; i<NUM_WEAPONS; i++)
		{
			if(!aDisplayWeapon[i])
				continue;

			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[i].m_pSpriteBody, 0);
			RenderTools()->DrawSprite(x+xo/2, y+24, g_pData->m_Weapons.m_aId[i].m_VisualSize);
			Graphics()->QuadsEnd();

			str_format(aBuf, sizeof(aBuf), "%d", m_aStats.m_aFragsWith[i]);
			TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
			str_format(aBuf, sizeof(aBuf), "%d", m_aStats.m_aDeathsFrom[i]);
			TextRender()->Text(0, x+xo+200.0f, y, FontSize, aBuf, -1);
			y += LineHeight;
		}
		y += 30.0f;
	}

	// Flag stats
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS)
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, LineHeight*5+20.0f, 17.0f);
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
		RenderTools()->DrawSprite(x+xo/2, y+100.0f, 192);
		Graphics()->QuadsEnd();

		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Grabs"), m_aStats.m_FlagGrabs);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Captures"), m_aStats.m_FlagCaptures);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Kills holding flag"), m_aStats.m_KillsCarrying);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Deaths with flag"), m_aStats.m_DeathsCarrying);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Carriers killed"), m_aStats.m_CarriersKilled);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
	}
}

void CTeecompStats::AutoStatScreenshot()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->AutoStatScreenshot_Start();
}
