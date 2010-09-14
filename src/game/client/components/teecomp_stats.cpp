#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <game/generated/client_data.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/teecomp.h>
#include "teecomp_stats.h"

CTeecompStats::CTeecompStats()
{
	m_Mode = 0;
	m_StatsCid = -1;
}

void CTeecompStats::OnReset()
{
	for(int i=0; i<MAX_CLIENTS; i++)
		m_pClient->m_aStats[i].Reset();
	m_Mode = 0;
	m_StatsCid = -1;
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
		pStats->m_StatsCid++;
		pStats->m_StatsCid %= MAX_CLIENTS;
		pStats->CheckStatsCid();
	}
}

void CTeecompStats::OnConsoleInit()
{
	Console()->Register("+stats", "i", CFGFLAG_CLIENT, ConKeyStats, this, "Show stats");
	Console()->Register("+next_stats", "", CFGFLAG_CLIENT, ConKeyNext, this, "Next player m_aStats");
}

bool CTeecompStats::IsActive()
{
	return (m_Mode > 0);
}

void CTeecompStats::CheckStatsCid()
{
	if(m_StatsCid == -1)
		m_StatsCid = m_pClient->m_Snap.m_LocalCid;

	int Prev = m_StatsCid;
	while(!m_pClient->m_aStats[m_StatsCid].m_Active)
	{
		m_StatsCid++;
		m_StatsCid %= MAX_CLIENTS;
		if(m_StatsCid == Prev)
		{
			m_StatsCid = -1;
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
			if(g_Config.m_ClSpreesounds && m_pClient->m_Snap.m_LocalCid == pMsg->m_Killer && pStats[pMsg->m_Killer].m_CurrentSpree % 5 == 0)
			{
				int SpreeType = pStats[pMsg->m_Killer].m_CurrentSpree/5 - 1;
				switch(SpreeType)
				{
				case 0:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_KILLINGSPREE, 0, vec2(0,0));
					break;
				case 1:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_RAMPAGE, 0, vec2(0,0));
					break;
				case 2:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_DOMINATING, 0, vec2(0,0));
					break;
				case 3:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_UNSTOPPABLE, 0, vec2(0,0));
					break;
				case 4:
					m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_SPREE_GODLIKE, 0, vec2(0,0));
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
		if(pMsg->m_Cid < 0)
		{
			const char *p;
			const char *pLookFor = "flag was captured by ";
			if(str_find_nocase(pMsg->m_pMessage, pLookFor) != 0)
			{
				p = str_find_nocase(pMsg->m_pMessage, pLookFor);
				char aName[64];
				p += str_length(pLookFor);
				str_copy(aName, p, sizeof(aName));
				if(str_comp_nocase(aName+str_length(aName)-9, " seconds)") == 0)
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
				for(int i=0; i<MAX_CLIENTS; i++)
				{
					if(!m_pClient->m_aStats[i].m_Active)
						continue;

					if(str_comp_nocase(m_pClient->m_aClients[i].m_aName, aName) == 0)
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
	// if we the game is over
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver && m_pClient->m_AutoScreenStatsTick > -1 && m_pClient->m_AutoScreenStatsTick == Client()->GameTick())
		m_Mode = 1;
	
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
	float w = 450.0f;
	float h = 750.0f;
	
	const CNetObj_PlayerInfo *apPlayers[MAX_CLIENTS] = {0};
	int NumPlayers = 0;
	int i;
	for(i=0; i<Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;

			if(!m_pClient->m_aStats[pInfo->m_ClientId].m_Active)
				continue;

			apPlayers[NumPlayers] = pInfo;
			NumPlayers++;
		}
	}

	for(int i=0; i<8; i++)
		if(g_Config.m_TcStatboardInfos & (1<<i))
		{
			if((1<<i) == (TC_STATS_BESTSPREE))
				w += 140;
			else
				w += 100;
		}
		
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGS)
		w += 100;

	bool aDisplayWeapon[NUM_WEAPONS] = {false};
	if(g_Config.m_TcStatboardInfos & TC_STATS_WEAPS)
	{
		w += 10;
		for(i=0; i<NumPlayers; i++)
		{
			const CGameClient::CClientStats pStats = m_pClient->m_aStats[apPlayers[i]->m_ClientId];
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
	int px = 525;

	TextRender()->Text(0, x+10, y, 24.0f, "Name", -1);
	const char *apHeaders[] = { "Frags", "Deaths", "Suicides", "Ratio", "Net", "FPM", "Spree", "Best Spree" };
	for(i=0; i<8; i++)
		if(g_Config.m_TcStatboardInfos & (1<<i))
		{
			if(1<<i == TC_STATS_BESTSPREE)
				px += 40.0f;
			tw = TextRender()->TextWidth(0, 24.0f, apHeaders[i], -1);
			TextRender()->Text(0, x+px-tw, y, 24.0f, apHeaders[i], -1);
			px += 100;
		}

	if(g_Config.m_TcStatboardInfos & TC_STATS_WEAPS)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		for(i=0, px-=40; i<NUM_WEAPONS; i++)
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

	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGS)
	{
		px -= 40;
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation(0.78f);
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
		RenderTools()->SelectSprite(x+px, y+10, 48);
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

	for(int j=0; j<NumPlayers; j++)
	{
		const CNetObj_PlayerInfo *pInfo = apPlayers[j];
		const CGameClient::CClientStats Stats = m_pClient->m_aStats[pInfo->m_ClientId];

		if(pInfo->m_Local)
		{
			// background so it's easy to find the local player
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1,1,1,0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20, LineHeight*0.95f, 17.0f);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo Teeinfo = m_pClient->m_aClients[pInfo->m_ClientId].m_RenderInfo;
		Teeinfo.m_Size *= TeeSizemod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &Teeinfo, EMOTE_NORMAL, vec2(1,0), vec2(x+28, y+28+TeeOffset));

		char aBuf[128];
		if(g_Config.m_TcStatId)
		{
			str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_ClientId);
			TextRender()->Text(0, x, y, FontSize, aBuf, -1);
		}

		TextRender()->Text(0, x+64, y, FontSize, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1);

		px = 525;

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
		for(i=0, px=px-40; i<NUM_WEAPONS; i++)
		{
			if(!aDisplayWeapon[i])
				continue;

			str_format(aBuf, sizeof(aBuf), "%d/%d", Stats.m_aFragsWith[i], Stats.m_aDeathsFrom[i]);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x+px-tw/2, y, FontSize, aBuf, -1);
			px += 80;
		}
		if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_FLAGS && g_Config.m_TcStatboardInfos&TC_STATS_FLAGS)
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
	CheckStatsCid();
	if(m_Mode != 2)
		return;
	int m_ClientId = m_StatsCid;
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	float w = 1200.0f;
	float x = Width/2-w/2;
	float y = 100.0f;
	float xo = 200.0f;
	float FontSize = 30.0f;
	float LineHeight = 40.0f;
	const CGameClient::CClientStats m_aStats = m_pClient->m_aStats[m_ClientId];

	Graphics()->MapScreen(0, 0, Width, Height);

	// header with name and score
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, 120.0f, 17.0f);
	Graphics()->QuadsEnd();

	CTeeRenderInfo Teeinfo = m_pClient->m_aClients[m_ClientId].m_RenderInfo;
	Teeinfo.m_Size *= 1.5f;
	RenderTools()->RenderTee(CAnimState::GetIdle(), &Teeinfo, EMOTE_NORMAL, vec2(1,0), vec2(x+xo+32, y+36));
	TextRender()->Text(0, x+xo+128, y, 48.0f, m_pClient->m_aClients[m_ClientId].m_aName, -1);

	char aBuf[64];
	if(g_Config.m_TcStatId)
	{
		str_format(aBuf, sizeof(aBuf), "%d", m_ClientId);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	}

	str_format(aBuf, sizeof(aBuf), "Score: %d", m_pClient->m_Snap.m_paPlayerInfos[m_ClientId]->m_Score);
	TextRender()->Text(0, x+xo, y+64, FontSize, aBuf, -1);
	int Seconds = (float)(Client()->GameTick()-m_aStats.m_JoinDate)/Client()->GameTickSpeed();
	str_format(aBuf, sizeof(aBuf), "Time played: %02d:%02d", Seconds/60, Seconds%60);
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
	RenderTools()->SelectSprite(SPRITE_DEADTEE);
	IGraphics::CQuadItem QuadItem(x+xo/2, y+40, 128, 128);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();

	str_format(aBuf, sizeof(aBuf), "Frags: %d", m_aStats.m_Frags);
	TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "Deaths: %d", m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo+200.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "Suicides: %d", m_aStats.m_Suicides);
	TextRender()->Text(0, x+xo+400.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "Spree: %d", m_aStats.m_CurrentSpree);
	TextRender()->Text(0, x+xo+600.0f, y, FontSize, aBuf, -1);
	y += LineHeight;

	if(m_aStats.m_Deaths == 0)
		str_format(aBuf, sizeof(aBuf), "Ratio: --");
	else
		str_format(aBuf, sizeof(aBuf), "Ratio: %.2f", (float)(m_aStats.m_Frags)/m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "Net: %d", m_aStats.m_Frags-m_aStats.m_Deaths);
	TextRender()->Text(0, x+xo+200.0f, y, FontSize, aBuf, -1);
	float Fpm = (float)(m_aStats.m_Frags*60)/((float)(Client()->GameTick()-m_aStats.m_JoinDate)/Client()->GameTickSpeed());
	str_format(aBuf, sizeof(aBuf), "FPM: %.1f", Fpm);
	TextRender()->Text(0, x+xo+400.0f, y, FontSize, aBuf, -1);
	str_format(aBuf, sizeof(aBuf), "Best spree: %d", m_aStats.m_BestSpree);
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

		TextRender()->Text(0, x+xo, y, FontSize, "Frags", -1);
		TextRender()->Text(0, x+xo+200.0f, y, FontSize, "Deaths", -1);
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
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_FLAGS)
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

		str_format(aBuf, sizeof(aBuf), "Grabs: %d", m_aStats.m_FlagGrabs);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "Captures: %d", m_aStats.m_FlagCaptures);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "Kills holding flag: %d", m_aStats.m_KillsCarrying);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "Deaths with flag: %d", m_aStats.m_DeathsCarrying);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
		str_format(aBuf, sizeof(aBuf), "Carriers killed: %d", m_aStats.m_CarriersKilled);
		TextRender()->Text(0, x+xo, y, FontSize, aBuf, -1);
		y += LineHeight;
	}
}
