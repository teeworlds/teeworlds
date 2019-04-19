#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <game/client/animstate.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <generated/client_data.h>
#include "stats.h"

CStats::CStats()
{
	m_Active = false;
}

void CStats::OnReset()
{
	for(int i=0; i<MAX_CLIENTS; i++)
		m_pClient->m_aStats[i].Reset();
	m_Active = false;
}

void CStats::ConKeyStats(IConsole::IResult *pResult, void *pUserData)
{
	((CStats *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
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
}

void CStats::OnRender()
{
	if(!m_Active)
		return;

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	float w = 250.0f;
	float h = 750.0f;

	int apPlayers[MAX_CLIENTS] = {0};
	int NumPlayers = 0;
	int i;
	for(i=0; i<MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_aClients[i].m_Active)
			continue;

		apPlayers[NumPlayers] = i;
		NumPlayers++;
	}

	for(int i=0; i<9; i++)
		if(g_Config.m_ClStatboardInfos & (1<<i))
		{
			if((1<<i) == (TC_STATS_BESTSPREE))
				w += 140;
			else
				w += 100;
		}

	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_ClStatboardInfos&TC_STATS_FLAGCAPTURES)
		w += 100;

	bool aDisplayWeapon[NUM_WEAPONS] = {false};
	if(g_Config.m_ClStatboardInfos & TC_STATS_WEAPS)
	{
		w += 10;
		for(i=0; i<NumPlayers; i++)
		{
			const CGameClient::CClientStats pStats = m_pClient->m_aStats[apPlayers[i]];
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
	{
		CUIRect Rect = {x-10.f, y-10.f, w, h};
		RenderTools()->DrawRoundRect(&Rect, vec4(0,0,0,0.5f), 17.0f);
	}

	float tw;
	int px = 325;

	TextRender()->Text(0, x+10, y-5, 24.0f, Localize("Name"), -1);
	const char *apHeaders[] = { Localize("Frags"), Localize("Deaths"), Localize("Suicides"), Localize("Ratio"), Localize("Net"), Localize("FPM"), Localize("Spree"), Localize("Best spree"), Localize("Grabs") };
	for(i=0; i<9; i++)
		if(g_Config.m_ClStatboardInfos & (1<<i))
		{
			if(1<<i == TC_STATS_BESTSPREE)
				px += 40.0f;
			tw = TextRender()->TextWidth(0, 24.0f, apHeaders[i], -1);
			TextRender()->Text(0, x+px-tw, y-5, 24.0f, apHeaders[i], -1);
			px += 100;
		}

	if(g_Config.m_ClStatboardInfos & TC_STATS_WEAPS)
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

	if(m_pClient->m_Snap.m_pGameData && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_ClStatboardInfos&TC_STATS_FLAGCAPTURES)
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
	for(int j=0; j<NumPlayers; j++)
	{
		// workaround
		if(j == 16)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "... %d other players", NumPlayers-j);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x+64, y, FontSize, aBuf, -1);
			px += 100;
			break;
		}

		const CGameClient::CClientStats Stats = m_pClient->m_aStats[apPlayers[j]];		

		if(apPlayers[j] == m_pClient->m_LocalClientID)
		{
			// background so it's easy to find the local player
			CUIRect Rect = {x, y, w-20, LineHeight*0.95f};
			RenderTools()->DrawRoundRect(&Rect, vec4(1,1,1,0.25f), 17.0f);
		}

		CTeeRenderInfo Teeinfo = m_pClient->m_aClients[apPlayers[j]].m_RenderInfo;
		Teeinfo.m_Size *= TeeSizemod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &Teeinfo, EMOTE_NORMAL, vec2(1,0), vec2(x+28, y+28+TeeOffset));

		char aBuf[128];
		CTextCursor Cursor;
		tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[apPlayers[j]].m_aName, -1);
		TextRender()->SetCursor(&Cursor, x+64, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 220;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[apPlayers[j]].m_aName, -1);

		px = 325;
		if(g_Config.m_ClStatboardInfos & TC_STATS_FRAGS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Frags);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_DEATHS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_SUICIDES)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_Suicides);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_RATIO)
		{
			if(Stats.m_Deaths == 0)
				str_format(aBuf, sizeof(aBuf), "--");
			else
				str_format(aBuf, sizeof(aBuf), "%.2f", (float)(Stats.m_Frags)/Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_NET)
		{
			str_format(aBuf, sizeof(aBuf), "%+d", Stats.m_Frags-Stats.m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_FPM)
		{
			float Fpm = (float)(Stats.m_Frags*60)/((float)(Client()->GameTick()-Stats.m_JoinDate)/Client()->GameTickSpeed());
			str_format(aBuf, sizeof(aBuf), "%.1f", Fpm);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_SPREE)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_CurrentSpree);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(g_Config.m_ClStatboardInfos & TC_STATS_BESTSPREE)
		{
			px += 40;
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_BestSpree);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		if(m_pClient->m_Snap.m_pGameData && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_ClStatboardInfos&TC_STATS_FLAGGRABS)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_FlagGrabs);
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
		if(m_pClient->m_Snap.m_pGameData && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && g_Config.m_ClStatboardInfos&TC_STATS_FLAGCAPTURES)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Stats.m_FlagCaptures);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, x-tw+px, y, FontSize, aBuf, -1);
			px += 100;
		}
		y += LineHeight;
	}
}
