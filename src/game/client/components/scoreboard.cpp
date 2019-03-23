/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/localization.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>

#include "menus.h"
#include "scoreboard.h"


CScoreboard::CScoreboard()
{
	m_PlayerLines = 0;
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnReset()
{
	m_Active = false;
	m_PlayerLines = 0;
	m_SkipPlayerStatsReset = false;
	for(int i = 0; i < MAX_CLIENTS; i++)
		ResetPlayerStats(i);
}

void CScoreboard::OnRelease()
{
	m_Active = false;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
	float h = 20.0f;

	Graphics()->BlendNormal();
	CUIRect Rect = {x, y, w, h};
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);

	// render goals
	y += 2.0f;
	if(m_pClient->m_GameInfo.m_ScoreLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
		TextRender()->Text(0, x+10.0f, y, 12.0f, aBuf, -1);
	}
	if(m_pClient->m_GameInfo.m_TimeLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_GameInfo.m_TimeLimit);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);
		TextRender()->Text(0, x+w/2-tw/2, y, 12.0f, aBuf, -1);
	}
	if(m_pClient->m_GameInfo.m_MatchNum && m_pClient->m_GameInfo.m_MatchCurrent)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Match", "rounds (scoreboard)"), m_pClient->m_GameInfo.m_MatchCurrent, m_pClient->m_GameInfo.m_MatchNum);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);
		TextRender()->Text(0, x+w-tw-10.0f, y, 12.0f, aBuf, -1);
	}
}

float CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 20.0f;

	int NumSpectators = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
			NumSpectators++;

	char aBuf[64];
	char SpectatorBuf[64];
	str_format(SpectatorBuf, sizeof(SpectatorBuf), "%s (%d):", Localize("Spectators"), NumSpectators);
	float tw = TextRender()->TextWidth(0, 12.0f, SpectatorBuf, -1);

	// do all the text without rendering it once
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, 12.0f, TEXTFLAG_ALLOW_NEWLINE);
	Cursor.m_LineWidth = w-17.0f;
	Cursor.m_StartX -= tw+3.0f;
	Cursor.m_MaxLines = 4;
	bool Multiple = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			TextRender()->TextEx(&Cursor, ", ", -1);
		if(g_Config.m_ClShowUserId && Cursor.m_LineCount <= Cursor.m_MaxLines)
		{
			Cursor.m_X += Cursor.m_FontSize;
		}
		if(m_pClient->m_aClients[i].m_aClan[0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", m_pClient->m_aClients[i].m_aClan);
			TextRender()->TextEx(&Cursor, aBuf, -1);
		}
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[i].m_aName, -1);
		Multiple = true;
	}

	// background
	float RectHeight = 3*h+(Cursor.m_Y-Cursor.m_StartY);
	Graphics()->BlendNormal();
	CUIRect Rect = {x, y, w, RectHeight};
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);

	// Headline
	y += 30.0f;
	TextRender()->Text(0, x+10.0f, y, 12.0f, SpectatorBuf, w-20.0f);

	// spectator names and now render everything
	x += tw+2.0f+10.0f;
	Multiple = false;
	TextRender()->SetCursor(&Cursor, x, y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_ALLOW_NEWLINE);
	Cursor.m_LineWidth = w-17.0f;
	Cursor.m_StartX -= tw+3.0f;
	Cursor.m_MaxLines = 4;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			TextRender()->TextEx(&Cursor, ", ", -1);
		if(g_Config.m_ClShowUserId && Cursor.m_LineCount <= Cursor.m_MaxLines)
		{
			RenderTools()->DrawClientID(TextRender(), &Cursor, i);
		}
		if(m_pClient->m_aClients[i].m_aClan[0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", m_pClient->m_aClients[i].m_aClan);
			TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f : 1.0f, 0.7f);
			TextRender()->TextEx(&Cursor, aBuf, -1);
		}
		TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f :	 1.0f, 1.0f);
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[i].m_aName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Multiple = true;
	}

	return RectHeight;
}

float CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, int Align)
{
	if(Team == TEAM_SPECTATORS)
		return 0.0f;

	// ready mode
	const CGameClient::CSnapState& Snap = m_pClient->m_Snap;
	const bool ReadyMode = Snap.m_pGameData && (Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_WARMUP)) && Snap.m_pGameData->m_GameStateEndTick == 0;

	float HeadlineHeight = 40.0f;
	float TitleFontsize = 20.0f;
	float HeadlineFontsize = 12.0f;
	float LineHeight = 20.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 2.0f;
	float PingOffset = x+Spacing, PingLength = 35.0f;
	float CountryFlagOffset = PingOffset+PingLength, CountryFlagLength = 20.f;
	float IdSize = g_Config.m_ClShowUserId ? LineHeight : 0.0f;
	float ReadyLength = ReadyMode ? 10.f : 0.f;
	float TeeOffset = CountryFlagOffset+CountryFlagLength+4.0f, TeeLength = 25*TeeSizeMod;
	float NameOffset = CountryFlagOffset+CountryFlagLength+IdSize, NameLength = 128.0f-IdSize/2-ReadyLength;
	float ClanOffset = NameOffset+NameLength+ReadyLength, ClanLength = 88.0f-IdSize/2;
	float KillOffset = ClanOffset+ClanLength, KillLength = 24.0f;
	float DeathOffset = KillOffset+KillLength, DeathLength = 24.0f;
	float ScoreOffset = DeathOffset+DeathLength, ScoreLength = 35.0f;
	float tw = 0.0f;

	bool NoTitle = pTitle? false : true;

	// count players
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Unknown team id");
	int NumPlayers = m_pClient->m_GameInfo.m_aTeamSize[Team];
	m_PlayerLines = max(m_PlayerLines, NumPlayers);

	// clamp to 16
	if(m_PlayerLines > 16)
		m_PlayerLines = 16;

	char aBuf[128] = {0};

	// background
	Graphics()->BlendNormal();
	vec4 Color;
	if(Team == TEAM_RED && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		Color = vec4(0.975f, 0.17f, 0.17f, 0.75f);
	else if(Team == TEAM_BLUE)
		Color = vec4(0.17f, 0.46f, 0.975f, 0.75f);
	else
		Color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
	CUIRect Rect = {x, y, w, HeadlineHeight};
	RenderTools()->DrawRoundRect(&Rect, Color, 5.0f);

	// render title
	if(NoTitle)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
		{
			pTitle = Localize("Round over");
			m_SkipPlayerStatsReset = true;
		}
		else
			pTitle = Localize("Scoreboard");
	}
	else
	{
		if(Team == TEAM_BLUE)
			str_format(aBuf, sizeof(aBuf), "(%d) %s", NumPlayers, pTitle);
		else
			str_format(aBuf, sizeof(aBuf), "%s (%d)", pTitle, NumPlayers);
	}

	if(Align == -1)
	{
		tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1);
		TextRender()->Text(0, x+20.0f, y+5.0f, TitleFontsize, pTitle, -1);
		if(!NoTitle)
		{
			str_format(aBuf, sizeof(aBuf), " (%d)", NumPlayers);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
			TextRender()->Text(0, x+20.0f+tw, y+5.0f, TitleFontsize, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	else
	{
		tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1);
		if(!NoTitle)
		{
			str_format(aBuf, sizeof(aBuf), "(%d) ", NumPlayers);
			float PlayersTextWidth = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
			TextRender()->Text(0, x+w-tw-PlayersTextWidth-20.0f, y+5.0f, TitleFontsize, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1);
		TextRender()->Text(0, x+w-tw-20.0f, y+5.0f, TitleFontsize, pTitle, -1);
	}

	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
	{
		int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue;
		str_format(aBuf, sizeof(aBuf), "%d", Score);
	}
	else
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0 &&
			m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
		{
			int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
		else if(m_pClient->m_Snap.m_pLocalInfo)
		{
			int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	if(Align == -1)
	{
		tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
		TextRender()->Text(0, x+w-tw-20.0f, y+5.0f, TitleFontsize, aBuf, -1);
	}
	else
		TextRender()->Text(0, x+20.0f, y+5.0f, TitleFontsize, aBuf, -1);

	// render headlines
	y += HeadlineHeight;

	Graphics()->BlendNormal();
	{
		CUIRect Rect = {x, y, w, LineHeight*(m_PlayerLines+1)};
		RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);
	}
	if(m_PlayerLines)
	{
		CUIRect Rect = {x, y+LineHeight, w, LineHeight*(m_PlayerLines)};
		RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, PingOffset+PingLength-tw, y+Spacing, HeadlineFontsize, Localize("Ping"), -1);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->Text(0, NameOffset+ TeeLength, y+Spacing, HeadlineFontsize, Localize("Name"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("Clan"), -1);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, "K", -1);
	TextRender()->Text(0, KillOffset+KillLength/2-tw/2, y+Spacing, HeadlineFontsize, "K", -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, "D", -1);
	TextRender()->Text(0, DeathOffset+DeathLength/2-tw/2, y+Spacing, HeadlineFontsize, "D", -1);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	TextRender()->Text(0, ScoreOffset+ScoreLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("Score"), -1);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render player entries
	y += LineHeight;
	float FontSize = HeadlineFontsize;
	CTextCursor Cursor;

	const int MAX_IDS = 16;
	int RenderScoreIDs[MAX_IDS];
	int NumRenderScoreIDs = 0;
	int HoleSizes[2];
	for(int i = 0; i < MAX_IDS; ++i)
		RenderScoreIDs[i] = -1;

	// Non vanilla scoreboard, for now, some parts of the scoreboard are omitted
	if(NumPlayers > MAX_IDS)
	{
		for(int RenderDead = 0; RenderDead < 2 && NumRenderScoreIDs < MAX_IDS-1; ++RenderDead)
		{
			for(int i = 0; i < MAX_CLIENTS && NumRenderScoreIDs < MAX_IDS-1; i++)
			{
				// make sure that we render the correct team
				const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
				if(!pInfo->m_pPlayerInfo || m_pClient->m_aClients[pInfo->m_ClientID].m_Team != Team || (!RenderDead && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)) ||
					(RenderDead && !(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)))
					continue;

				RenderScoreIDs[NumRenderScoreIDs] = i;
				NumRenderScoreIDs++;
			}
		}
		NumRenderScoreIDs = MAX_IDS;
		RenderScoreIDs[MAX_IDS-1] = -1;
		HoleSizes[0] = m_pClient->m_GameInfo.m_aTeamSize[Team] - (MAX_IDS-1);

		if(m_pClient->m_LocalClientID != -1 && (m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == Team || m_pClient->m_Snap.m_SpecInfo.m_Active))
		{
			int Classment = -1;
			int TeamScoreIDs[MAX_CLIENTS];
			for(int RenderDead = 0, j = 0; RenderDead < 2; ++RenderDead)
			{
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					// make sure that we render the correct team
					const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
					if(!pInfo->m_pPlayerInfo || m_pClient->m_aClients[pInfo->m_ClientID].m_Team != Team || (!RenderDead && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)) ||
						(RenderDead && !(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)))
						continue;

					if(m_pClient->m_LocalClientID == pInfo->m_ClientID || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
						Classment = j;

					TeamScoreIDs[j] = i;
					j++;
				}
			}

			if(Classment < MAX_IDS-1) {}
			else if(Classment == m_pClient->m_GameInfo.m_aTeamSize[Team] - 1)
			{
				HoleSizes[0] = Classment - MAX_IDS-2;
				RenderScoreIDs[MAX_IDS-3] = -1;
				RenderScoreIDs[MAX_IDS-2] = TeamScoreIDs[Classment-1];
				RenderScoreIDs[MAX_IDS-1] = TeamScoreIDs[Classment];
			}
			else if(Classment == m_pClient->m_GameInfo.m_aTeamSize[Team] - 2)
			{
				HoleSizes[0] = Classment - MAX_IDS-3;
				RenderScoreIDs[MAX_IDS-4] = -1;
				RenderScoreIDs[MAX_IDS-3] = TeamScoreIDs[Classment-1];
				RenderScoreIDs[MAX_IDS-2] = TeamScoreIDs[Classment];
				RenderScoreIDs[MAX_IDS-1] = TeamScoreIDs[Classment+1];
			}
			else if(Classment == m_pClient->m_GameInfo.m_aTeamSize[Team] - 3)
			{
				HoleSizes[0] = Classment - MAX_IDS-4;
				RenderScoreIDs[MAX_IDS-5] = -1;
				RenderScoreIDs[MAX_IDS-4] = TeamScoreIDs[Classment-1];
				RenderScoreIDs[MAX_IDS-3] = TeamScoreIDs[Classment];
				RenderScoreIDs[MAX_IDS-2] = TeamScoreIDs[Classment+1];
				RenderScoreIDs[MAX_IDS-1] = TeamScoreIDs[Classment+2];
			}
			else if(Classment < m_pClient->m_GameInfo.m_aTeamSize[Team] - 3)
			{
				HoleSizes[0] = Classment - MAX_IDS-4;
				RenderScoreIDs[MAX_IDS-5] = -1;
				RenderScoreIDs[MAX_IDS-4] = TeamScoreIDs[Classment-1];
				RenderScoreIDs[MAX_IDS-3] = TeamScoreIDs[Classment];
				RenderScoreIDs[MAX_IDS-2] = TeamScoreIDs[Classment+1];
				HoleSizes[1] = m_pClient->m_GameInfo.m_aTeamSize[Team] - Classment - 2;
				RenderScoreIDs[MAX_IDS-1] = -2;
			}
		}
	}
	else // Normal scoreboard
	{
		for(int RenderDead = 0; RenderDead < 2; ++RenderDead)
		{
			for(int i = 0; i < MAX_CLIENTS && NumRenderScoreIDs < MAX_IDS; i++)
			{
				// make sure that we render the correct team
				const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
				if(!pInfo->m_pPlayerInfo || m_pClient->m_aClients[pInfo->m_ClientID].m_Team != Team || (!RenderDead && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)) ||
					(RenderDead && !(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)))
					continue;

				RenderScoreIDs[NumRenderScoreIDs] = i;
				NumRenderScoreIDs++;
			}
		}
	}

	for(int i = 0 ; i < NumRenderScoreIDs ; i++)
	{
		if(RenderScoreIDs[i] >= 0)
		{
			const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[RenderScoreIDs[i]];
			bool RenderDead = pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD;
			float ColorAlpha = RenderDead ? 0.5f : 1.0f;
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);

			// color for text
			vec3 TextColor = vec3(1.0f, 1.0f, 1.0f);
			vec4 OutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			const bool HighlightedLine = m_pClient->m_LocalClientID == pInfo->m_ClientID || (Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == Snap.m_SpecInfo.m_SpectatorID);

			// background so it's easy to find the local player or the followed one in spectator mode
			if(HighlightedLine)
			{
				CUIRect Rect = {x, y, w, LineHeight};
				RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.75f*ColorAlpha), 5.0f);

				// make color for own entry black
				TextColor = vec3(0.0f, 0.0f, 0.0f);
				OutlineColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
			}
			else
				OutlineColor = vec4(0.0f, 0.0f, 0.0f, 0.3f);

			// set text color
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextOutlineColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// ping
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Latency, 0, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = PingLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);

			// country flag
			const vec4 CFColor(1, 1, 1, 0.75f * ColorAlpha);
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &CFColor,
				CountryFlagOffset, y + 3.0f, 30.0f, LineHeight-5.0f);

			// flag
			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && m_pClient->m_Snap.m_pGameDataFlag &&
				(m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed == pInfo->m_ClientID ||
				m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue == pInfo->m_ClientID))
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				RenderTools()->SelectSprite(m_pClient->m_aClients[pInfo->m_ClientID].m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

				float Size = LineHeight;
				IGraphics::CQuadItem QuadItem(TeeOffset+4.0f, y-2.0f-Spacing/2.0f, Size/2.0f, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// avatar
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEADTEE].m_Id);
				Graphics()->QuadsBegin();
				if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
				{
					vec4 Color = m_pClient->m_pSkins->GetColorV4(m_pClient->m_pSkins->GetTeamColor(true, 0, m_pClient->m_aClients[pInfo->m_ClientID].m_Team, SKINPART_BODY), false);
					Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
				}
				IGraphics::CQuadItem QuadItem(TeeOffset+TeeLength/2 - 10*TeeSizeMod, y+Spacing, 20*TeeSizeMod, 20*TeeSizeMod);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size = 20*TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2+Spacing));
			}

			// TODO: make an eye icon or something
			if(RenderDead && pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_WATCHING)
				TextRender()->TextColor(1.0f, 1.0f, 0.0f, ColorAlpha);

			// id
			if(g_Config.m_ClShowUserId)
			{
				TextRender()->SetCursor(&Cursor, NameOffset+TeeLength-IdSize+Spacing, y+Spacing, FontSize, TEXTFLAG_RENDER);
				RenderTools()->DrawClientID(TextRender(), &Cursor, pInfo->m_ClientID);
			}

			// name
			TextRender()->SetCursor(&Cursor, NameOffset+TeeLength, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = NameLength-TeeLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, str_length(m_pClient->m_aClients[pInfo->m_ClientID].m_aName));
			// ready / watching
			if(ReadyMode && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_READY))
			{
				if(HighlightedLine)
					TextRender()->TextOutlineColor(0.0f, 0.1f, 0.0f, 0.5f);
				TextRender()->TextColor(0.1f, 1.0f, 0.1f, ColorAlpha);
				TextRender()->SetCursor(&Cursor, Cursor.m_X, y+Spacing, FontSize, TEXTFLAG_RENDER);
				TextRender()->TextEx(&Cursor, "\xE2\x9C\x93", str_length("\xE2\x9C\x93"));
			}
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextOutlineColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// clan
			tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1/*, TEXTFLAG_STOP_AT_END, ClanLength*/);
			TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ClanLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

			// K
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(m_aPlayerStats[pInfo->m_ClientID].m_Kills, 0, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, KillOffset+KillLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = KillLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// D
			str_format(aBuf, sizeof(aBuf), "%d", clamp(m_aPlayerStats[pInfo->m_ClientID].m_Deaths, 0, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, DeathOffset+DeathLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = DeathLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// score
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Score, -999, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, ScoreOffset+ScoreLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = ScoreLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			y += LineHeight;
		}
		else
		{
			int HoleSize = HoleSizes[-1-RenderScoreIDs[i]];

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			TextRender()->SetCursor(&Cursor, NameOffset+TeeLength, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = NameLength;
			char aBuf[64], aBuf2[64];
			str_format(aBuf, sizeof(aBuf), Localize("%d other players"), HoleSize);
			str_format(aBuf2, sizeof(aBuf2), "\xe2\x8b\x85\xe2\x8b\x85\xe2\x8b\x85 %s", aBuf);
			TextRender()->TextEx(&Cursor, aBuf2, -1);
			y += LineHeight;
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);

	return HeadlineHeight+LineHeight*(m_PlayerLines+1);
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	CUIRect RectBox = {x, 0.0f, 180.0f, 50.0f};
	vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.4f);
	Graphics()->BlendNormal();
	RenderTools()->DrawUIRect(&RectBox, Color, CUI::CORNER_B, 15.0f);

	//draw the red dot
	CUIRect RectRedDot = {x+20, 15.0f, 20.0f, 20.0f};
	Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(&RectRedDot, Color, 10.0f);

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+50.0f, 10.0f, 20.0f, aBuf, -1);
}

void CScoreboard::OnRender()
{
	// check if we need to reset the player stats
	if(!m_SkipPlayerStatsReset && m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStartTick == Client()->GameTick())
	{
		m_SkipPlayerStatsReset = true;
		for(int i = 0; i < MAX_CLIENTS; i++)
			ResetPlayerStats(i);
	}
	else if(m_SkipPlayerStatsReset && m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStartTick != Client()->GameTick())
		m_SkipPlayerStatsReset = false;

	// close the motd if we actively wanna look on the scoreboard
	if(m_Active)
		m_pClient->m_pMotd->Clear();

	if(!Active())
		return;

	// don't render scoreboard if menu or motd is open
	if(m_pClient->m_pMenus->IsActive() || m_pClient->m_pMotd->IsActive())
		return;

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	float Width = Screen.w;
	float y = 85.f;
	float w = 364.0f;
	float FontSize = 86.0f;

	const char* pCustomRedClanName = GetClanName(TEAM_RED);
	const char* pCustomBlueClanName = GetClanName(TEAM_BLUE);
	const char* pRedClanName = pCustomRedClanName ? pCustomRedClanName : Localize("Red team");
	const char* pBlueClanName = pCustomBlueClanName ? pCustomBlueClanName : Localize("Blue team");

	if(m_pClient->m_Snap.m_pGameData)
	{
		if(!(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS))
		{
			float ScoreboardHeight = RenderScoreboard(Width/2-w/2, y, w, 0, 0, -1);

			float SpectatorHeight = RenderSpectators(Width/2-w/2, y+3.0f+ScoreboardHeight, w);
			RenderGoals(Width/2-w/2, y+3.0f+ScoreboardHeight, w);

			// scoreboard size
			m_TotalRect.x = Width/2-w/2;
			m_TotalRect.y = y;
			m_TotalRect.w = w;
			m_TotalRect.h = ScoreboardHeight+SpectatorHeight+3.0f;
		}
		else if(m_pClient->m_Snap.m_pGameDataTeam)
		{
			float ScoreboardHeight = RenderScoreboard(Width/2-w-1.5f, y, w, TEAM_RED, pRedClanName, -1);
			RenderScoreboard(Width/2+1.5f, y, w, TEAM_BLUE, pBlueClanName, 1);

			float SpectatorHeight = RenderSpectators(Width/2-w-1.5f, y+3.0f+ScoreboardHeight, w*2.0f+3.0f);
			RenderGoals(Width/2-w-1.5f, y+3.0f+ScoreboardHeight, w*2.0f+3.0f);

			// scoreboard size
			m_TotalRect.x = Width/2-w-1.5f;
			m_TotalRect.y = y;
			m_TotalRect.w = w*2.0f+3.0f;
			m_TotalRect.h = ScoreboardHeight+SpectatorHeight+3.0f;
		}
	}

	Width = 400*3.0f*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, 400*3.0f);
	if(m_pClient->m_Snap.m_pGameData && (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) && m_pClient->m_Snap.m_pGameDataTeam)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
		{
			char aText[256];

			if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue)
				str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
			else if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed)
				str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
			else
				str_copy(aText, Localize("Draw!"), sizeof(aText));

			float tw = TextRender()->TextWidth(0, FontSize, aText, -1);
			TextRender()->Text(0, Width/2-tw/2, 39, FontSize, aText, -1);
		}
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
		{
			char aText[256];
			str_copy(aText, Localize("Round over!"), sizeof(aText));

			float tw = TextRender()->TextWidth(0, FontSize, aText, -1);
			TextRender()->Text(0, Width/2-tw/2, 39, FontSize, aText, -1);
			m_SkipPlayerStatsReset = true;
		}
	}

	RenderRecordingNotification((Width/7)*4);
}

void CScoreboard::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;

		m_aPlayerStats[pMsg->m_Victim].m_Deaths++;
		if(pMsg->m_Victim != pMsg->m_Killer)
			m_aPlayerStats[pMsg->m_Killer].m_Kills++;
	}
	else if(MsgType == NETMSGTYPE_SV_CLIENTDROP && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		CNetMsg_Sv_ClientDrop *pMsg = (CNetMsg_Sv_ClientDrop *)pRawMsg;

		if(!m_pClient->m_aClients[pMsg->m_ClientID].m_Active)
			return;

		ResetPlayerStats(pMsg->m_ClientID);
	}
}

bool CScoreboard::Active()
{
	// if we actively wanna look on the scoreboard
	if(m_Active)
		return true;

	if(m_pClient->m_LocalClientID != -1 && m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead, don't follow a player and the game isn't paused
		if(!m_pClient->m_Snap.m_pLocalCharacter && !m_pClient->m_Snap.m_SpecInfo.m_Active &&
			!(m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
		return true;

	return false;
}

void CScoreboard::ResetPlayerStats(int ClientID)
{
	m_aPlayerStats[ClientID].Reset();
}

CUIRect CScoreboard::GetScoreboardRect()
{
	return m_TotalRect;
}

const char *CScoreboard::GetClanName(int Team)
{
	int ClanPlayers = 0;
	const char *pClanName = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_aClients[i].m_Active || m_pClient->m_aClients[i].m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = m_pClient->m_aClients[i].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(m_pClient->m_aClients[i].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return 0;
		}
	}

	if(ClanPlayers > 1 && pClanName[0])
		return pClanName;
	else
		return 0;
}

CScoreboard::CPlayerStats::CPlayerStats()
{
	m_Kills = 0;
	m_Deaths = 0;
}

void CScoreboard::CPlayerStats::Reset()
{
	m_Kills = 0;
	m_Deaths = 0;
}
