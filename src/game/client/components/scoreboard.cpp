/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <limits.h>

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
#include <game/client/components/stats.h>

#include "menus.h"
#include "stats.h"
#include "scoreboard.h"
#include "stats.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pScoreboard = (CScoreboard *)pUserData;
	int Result = pResult->GetInteger(0);
	if(!Result)
	{
		pScoreboard->m_Activate = false;
		pScoreboard->m_Active = false;
	}
	else if(!pScoreboard->m_Active)
		pScoreboard->m_Activate = true;	
}

void CScoreboard::OnReset()
{
	m_Active = false;
	m_Activate = false;
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
	Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// render goals
	static CTextCursor s_Cursor(12.0f);
	y += 2.0f;
	if(m_pClient->m_GameInfo.m_ScoreLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
		s_Cursor.Reset();
		s_Cursor.MoveTo(x+10.0f, y);
		TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
	}
	if(m_pClient->m_GameInfo.m_TimeLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_GameInfo.m_TimeLimit);
		s_Cursor.Reset();
		TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
		float tw = s_Cursor.Width();
		s_Cursor.MoveTo(x+w/2-tw/2, y);
		TextRender()->DrawTextOutlined(&s_Cursor);
	}
	if(m_pClient->m_GameInfo.m_MatchNum && m_pClient->m_GameInfo.m_MatchCurrent)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Match", "rounds (scoreboard)"), m_pClient->m_GameInfo.m_MatchCurrent, m_pClient->m_GameInfo.m_MatchNum);
		s_Cursor.Reset();
		TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
		float tw = s_Cursor.Width();
		s_Cursor.MoveTo(x+w-tw-10.0f, y);
		TextRender()->DrawTextOutlined(&s_Cursor);
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
	static CTextCursor s_LabelCursor(12.0f);
	s_LabelCursor.Reset(g_Localization.Version() << 8 | NumSpectators);
	TextRender()->TextDeferred(&s_LabelCursor, SpectatorBuf, -1);
	float tw = s_LabelCursor.Width();

	float TextStartX = x+10.0f;
	float TextStartY = y+30.0f;
	float FontSize = 12.0f;
	float ClientIDWidth = UI()->GetClientIDRectWidth(FontSize);

	// render all the text without drawing
	static CTextCursor s_SpectatorCursors[MAX_CLIENTS];
	int MaxLines = 4;
	int Lines = 1;
	CTextCursor *pLastCursor = NULL;
	vec2 CursorPosition = vec2(TextStartX + tw + 3.0f, TextStartY);
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		s_SpectatorCursors[i].Reset();
		s_SpectatorCursors[i].m_FontSize = FontSize;

		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[0];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS || Lines > MaxLines)
			continue;

		if(pLastCursor)
		{
			TextRender()->TextDeferred(pLastCursor, ", ", -1);
			CursorPosition.x = pLastCursor->BoundingBox().Right();
		}

		CursorPosition.x += ClientIDWidth;

		if(m_pClient->m_aClients[i].m_aClan[0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", m_pClient->m_aClients[i].m_aClan);
			TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f : 1.0f, 0.7f);
			TextRender()->TextDeferred(&s_SpectatorCursors[i], aBuf, -1);
		}

		TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f :	 1.0f, 1.0f);
		TextRender()->TextDeferred(&s_SpectatorCursors[i], m_pClient->m_aClients[i].m_aName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		if(CursorPosition.x + s_SpectatorCursors[i].Width() > x + w - 15.0f)
		{
			CursorPosition.x = TextStartX + ClientIDWidth;
			CursorPosition.y += FontSize + 3.0f;
			Lines += 1;

			if(Lines > MaxLines) 
			{
				s_SpectatorCursors[i].Reset();

				TextRender()->TextDeferred(pLastCursor, "\xe2\x8b\x85\xe2\x8b\x85\xe2\x8b\x85", -1);
				continue;
			}
		}

		s_SpectatorCursors[i].MoveTo(CursorPosition.x, CursorPosition.y);
		pLastCursor = &s_SpectatorCursors[i];
	}

	// background
	float RectHeight = 3*h+((minimum(Lines, MaxLines)-1) * (FontSize + 3.0f));
	Graphics()->BlendNormal();
	CUIRect Rect = {x, y, w, RectHeight};
	Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// headline
	s_LabelCursor.MoveTo(TextStartX, TextStartY);
	TextRender()->DrawTextOutlined(&s_LabelCursor);
	
	// draw text
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(s_SpectatorCursors[i].Rendered())
		{
			vec2 ClientIDPos = s_SpectatorCursors[i].CursorPosition();
			ClientIDPos.x -= ClientIDWidth;
			UI()->DrawClientID(FontSize,  ClientIDPos, i);
			TextRender()->DrawTextOutlined(&s_SpectatorCursors[i]);
		}
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

	bool Race = m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE;

	float HeadlineHeight = 40.0f;
	float TitleFontsize = 20.0f;
	float HeadlineFontsize = 12.0f;
	float LineHeight = 20.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 2.0f;
	float PingOffset = x+Spacing, PingLength = 35.0f;
	float CountryFlagOffset = PingOffset+PingLength, CountryFlagLength = 20.f;
	float IdSize = Config()->m_ClShowUserId ? LineHeight : 0.0f;
	float ReadyLength = ReadyMode ? 10.f : 0.f;
	float TeeOffset = CountryFlagOffset+CountryFlagLength+4.0f, TeeLength = 25*TeeSizeMod;
	float NameOffset = CountryFlagOffset+CountryFlagLength+IdSize, NameLength = 128.0f-IdSize/2-ReadyLength;
	float ClanOffset = NameOffset+NameLength+ReadyLength, ClanLength = 88.0f-IdSize/2;
	float KillOffset = ClanOffset+ClanLength, KillLength = Race ? 0.0f : 24.0f;
	float DeathOffset = KillOffset+KillLength, DeathLength = Race ? 0.0f : 24.0f;
	float ScoreOffset = DeathOffset+DeathLength, ScoreLength = Race ? 83.0f : 35.0f;

	bool NoTitle = pTitle? false : true;

	// count players
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Unknown team id");
	int NumPlayers = m_pClient->m_GameInfo.m_aTeamSize[Team];
	int PlayerLines = NumPlayers;
	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		PlayerLines = maximum(m_pClient->m_GameInfo.m_aTeamSize[Team^1], PlayerLines);

	// clamp to 16
	if(PlayerLines > 16)
		PlayerLines = 16;

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
	Rect.Draw(Color);

	// render title
	if(NoTitle)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
			pTitle = Localize("Round over");
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

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_LineSpacing = 3.0f;
	s_Cursor.m_MaxWidth = -1;
	s_Cursor.m_MaxLines = -1;

	// title text
	if(Align == -1)
	{
		s_Cursor.m_FontSize = TitleFontsize;
		s_Cursor.m_Align = TEXTALIGN_LEFT;
		s_Cursor.MoveTo(x+20.0f, y+5.0f);
		TextRender()->TextDeferred(&s_Cursor, pTitle, -1);
		if(!NoTitle)
		{
			str_format(aBuf, sizeof(aBuf), " (%d)", NumPlayers);
			TextRender()->TextColor(CUI::ms_TransparentTextColor);
			TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	else
	{
		s_Cursor.m_FontSize = TitleFontsize;
		s_Cursor.m_Align = TEXTALIGN_RIGHT;
		s_Cursor.MoveTo(x+w-20.0f, y+5.0f);
		if(!NoTitle)
		{
			str_format(aBuf, sizeof(aBuf), "(%d) ", NumPlayers);
			TextRender()->TextColor(CUI::ms_TransparentTextColor);
			TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		TextRender()->TextDeferred(&s_Cursor, pTitle, -1);
	}
	TextRender()->DrawTextOutlined(&s_Cursor);

	// score record text
	const float ScoreAnchorX = Align == -1 ? (x+w-20.0f) : (x+20.0f);
	s_Cursor.m_Align = Align == -1 ? TEXTALIGN_RIGHT : TEXTALIGN_LEFT;
	s_Cursor.Reset();

	if(Race)
	{
		s_Cursor.Reset();
		if(m_pClient->m_Snap.m_pGameDataRace && Team != TEAM_BLUE)
		{
			float MapRecordFontsize = 16.0f;
			const char *pMapRecordStr = Localize("Map record");
			FormatTime(aBuf, sizeof(aBuf), m_pClient->m_Snap.m_pGameDataRace->m_BestTime, m_pClient->RacePrecision());
			s_Cursor.m_FontSize = HeadlineFontsize;
			s_Cursor.MoveTo(ScoreAnchorX, y+3.0f);

			TextRender()->TextDeferred(&s_Cursor, pMapRecordStr, -1);
			TextRender()->TextNewline(&s_Cursor);
			s_Cursor.m_FontSize = MapRecordFontsize;
			TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
		}
	}
	else
	{
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
		s_Cursor.m_FontSize = TitleFontsize;
		s_Cursor.MoveTo(ScoreAnchorX, y+5.0f);
		TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
	}
	TextRender()->DrawTextOutlined(&s_Cursor);

	// render headlines
	y += HeadlineHeight;

	Graphics()->BlendNormal();
	{
		CUIRect Rect = {x, y, w, LineHeight*(PlayerLines+1)};
		Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
	}
	if(PlayerLines)
	{
		CUIRect Rect = {x, y+LineHeight, w, LineHeight*(PlayerLines)};
		Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
	}

	s_Cursor.m_FontSize = HeadlineFontsize;
	s_Cursor.m_Align = TEXTALIGN_RIGHT;
	TextRender()->TextColor(CUI::ms_TransparentTextColor);
	s_Cursor.Reset();
	s_Cursor.MoveTo(PingOffset+PingLength, y+Spacing);
	TextRender()->TextOutlined(&s_Cursor, Localize("Ping"), -1);

	s_Cursor.m_Align = TEXTALIGN_LEFT;
	s_Cursor.Reset();
	s_Cursor.MoveTo(NameOffset+TeeLength, y+Spacing);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlined(&s_Cursor, Localize("Name"), -1);

	s_Cursor.m_Align = TEXTALIGN_CENTER;
	s_Cursor.Reset();
	s_Cursor.MoveTo(ClanOffset+ClanLength/2, y+Spacing);
	TextRender()->TextOutlined(&s_Cursor, Localize("Clan"), -1);

	if(!Race)
	{
		TextRender()->TextColor(CUI::ms_TransparentTextColor);
		s_Cursor.Reset();
		s_Cursor.MoveTo(KillOffset+KillLength/2, y+Spacing);
		TextRender()->TextOutlined(&s_Cursor, "K", -1);

		s_Cursor.Reset();
		s_Cursor.MoveTo(DeathOffset+DeathLength/2, y+Spacing);
		TextRender()->TextOutlined(&s_Cursor, "D", -1);
	}

	const char *pScoreStr = Race ? Localize("Time") : Localize("Score");
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	s_Cursor.Reset();
	s_Cursor.m_Align = Race ? TEXTALIGN_RIGHT : TEXTALIGN_CENTER;
	s_Cursor.MoveTo(ScoreOffset+(Race ? ScoreLength-3.f : ScoreLength/2), y+Spacing);
	TextRender()->TextOutlined(&s_Cursor, pScoreStr, -1);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render player entries
	y += LineHeight;
	float FontSize = HeadlineFontsize;

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

	if(Config()->m_ClHideSelfScore)
	{
		// Move local player to the bottom of the scoreboard
		for (int i = 0; i < NumRenderScoreIDs - 1 && RenderScoreIDs[i + 1] >= 0; i++)
		{
			const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[RenderScoreIDs[i]];
			if (m_pClient->m_LocalClientID == pInfo->m_ClientID)
			{
				const int Temp = RenderScoreIDs[i + 1];
				RenderScoreIDs[i + 1] = RenderScoreIDs[i];
				RenderScoreIDs[i] = Temp;
			}
		}
	}

	s_Cursor.m_MaxLines = 1;
	s_Cursor.m_FontSize = FontSize;
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
			const bool HighlightedLine = m_pClient->m_LocalClientID == pInfo->m_ClientID ||
				(Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == Snap.m_SpecInfo.m_SpectatorID);

			// background so it's easy to find the local player or the followed one in spectator mode
			if(HighlightedLine)
			{
				CUIRect Rect = {x, y, w, LineHeight};
				Rect.Draw(vec4(1.0f, 1.0f, 1.0f, 0.75f*ColorAlpha));

				// make color for own entry black
				TextColor = vec3(0.0f, 0.0f, 0.0f);
				OutlineColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
			}
			else
				OutlineColor = vec4(0.0f, 0.0f, 0.0f, 0.3f);

			// set text color
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextSecondaryColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// ping
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Latency, 0, 999));

			s_Cursor.Reset();
			s_Cursor.MoveTo(PingOffset+PingLength, y+Spacing);
			s_Cursor.m_Align = TEXTALIGN_RIGHT;
			s_Cursor.m_MaxWidth = PingLength;
			TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
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

				RenderTools()->SelectSprite(pInfo->m_ClientID == m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

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
				IGraphics::CQuadItem QuadItem(TeeOffset+TeeLength/2 - 10*TeeSizeMod, y-2.0f+Spacing, 20*TeeSizeMod, 20*TeeSizeMod);
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
			if(Config()->m_ClShowUserId)
				UI()->DrawClientID(FontSize, vec2(NameOffset+TeeLength-IdSize+Spacing, y+Spacing), pInfo->m_ClientID);

			// name
			s_Cursor.Reset();
			s_Cursor.MoveTo(NameOffset+TeeLength, y+Spacing);
			s_Cursor.m_Align = TEXTALIGN_LEFT;
			s_Cursor.m_MaxWidth = NameLength-TeeLength;
			TextRender()->TextOutlined(&s_Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, str_length(m_pClient->m_aClients[pInfo->m_ClientID].m_aName));
			
			// ready / watching
			if(ReadyMode && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_READY))
			{
				if(HighlightedLine)
					TextRender()->TextSecondaryColor(0.0f, 0.1f, 0.0f, 0.5f);
				TextRender()->TextColor(0.1f, 1.0f, 0.1f, ColorAlpha);
				s_Cursor.MoveTo(s_Cursor.BoundingBox().Right(), y+Spacing);
				s_Cursor.Reset();
				TextRender()->TextOutlined(&s_Cursor, "\xE2\x9C\x93", str_length("\xE2\x9C\x93"));
			}
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextSecondaryColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// clan
			s_Cursor.Reset();
			s_Cursor.MoveTo(ClanOffset+ClanLength/2, y+Spacing);
			s_Cursor.m_Align = TEXTALIGN_CENTER;
			s_Cursor.m_MaxWidth = ClanLength;
			TextRender()->TextOutlined(&s_Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

			if(!Race && !(m_pClient->m_LocalClientID == pInfo->m_ClientID && Config()->m_ClHideSelfScore))
			{
				// K
				TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
				str_format(aBuf, sizeof(aBuf), "%d", clamp(m_pClient->m_pStats->GetPlayerStats(pInfo->m_ClientID)->m_Frags, 0, 999));
				s_Cursor.Reset();
				s_Cursor.MoveTo(KillOffset+KillLength/2, y+Spacing);
				s_Cursor.m_MaxWidth = KillLength;
				TextRender()->TextOutlined(&s_Cursor, aBuf, -1);

				// D
				str_format(aBuf, sizeof(aBuf), "%d", clamp(m_pClient->m_pStats->GetPlayerStats(pInfo->m_ClientID)->m_Deaths, 0, 999));
				s_Cursor.Reset();
				s_Cursor.MoveTo(DeathOffset+DeathLength/2, y+Spacing);
				s_Cursor.m_MaxWidth = DeathLength;
				TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
			}

			// score
			if(Race)
			{
				aBuf[0] = 0;
				if(pInfo->m_pPlayerInfo->m_Score >= 0)
					FormatTime(aBuf, sizeof(aBuf), pInfo->m_pPlayerInfo->m_Score, m_pClient->RacePrecision());
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Score, -999, 9999));
			}

			s_Cursor.Reset();
			s_Cursor.m_Align = Race ? TEXTALIGN_RIGHT : TEXTALIGN_CENTER;
			s_Cursor.MoveTo(ScoreOffset+(Race ? ScoreLength-3.f : ScoreLength/2), y+Spacing);
			s_Cursor.m_MaxWidth = ScoreLength;
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			if(!Race && !(m_pClient->m_LocalClientID == pInfo->m_ClientID && Config()->m_ClHideSelfScore))
			{
				TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
			}

			y += LineHeight;
		}
		else
		{
			int HoleSize = HoleSizes[-1-RenderScoreIDs[i]];

			char aBuf[64], aBuf2[64];
			str_format(aBuf, sizeof(aBuf), Localize("%d other players"), HoleSize);
			str_format(aBuf2, sizeof(aBuf2), "\xe2\x8b\x85\xe2\x8b\x85\xe2\x8b\x85 %s", aBuf);

			s_Cursor.Reset();
			s_Cursor.m_Align = TEXTALIGN_LEFT;
			s_Cursor.m_MaxWidth = NameLength;
			s_Cursor.MoveTo(NameOffset+TeeLength, y+Spacing);

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			TextRender()->TextOutlined(&s_Cursor, aBuf2, -1);
			y += LineHeight;
		}
	}
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);

	return HeadlineHeight+LineHeight*(PlayerLines+1);
}

void CScoreboard::RenderRecordingNotification(float x, float w)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	CUIRect RectBox = {x, 0.0f, w, 50.0f};
	vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.4f);
	Graphics()->BlendNormal();
	RectBox.Draw(Color, 15.0f, CUIRect::CORNER_B);

	//draw the red dot
	CUIRect RectRedDot = {x+20, 15.0f, 20.0f, 20.0f};
	Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	RectRedDot.Draw(Color, 10.0f);

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	
	static CTextCursor s_Cursor(20.0f);
	s_Cursor.Reset(((int64)g_Localization.Version() << 32) | Seconds);
	s_Cursor.MoveTo(x+50.0f, 10.0f);
	TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
}

void CScoreboard::RenderNetworkQuality(float x, float w)
{
	//draw the box
	CUIRect RectBox = {x, 0.0f, w, 50.0f};
	vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.4f);
	const float LineHeight = 17.0f;
	int Score = Client()->GetInputtimeMarginStabilityScore();

	Graphics()->BlendNormal();
	RectBox.Draw(Color, 15.0f, CUIRect::CORNER_B);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_NETWORKICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_NETWORK_GOOD);
	IGraphics::CQuadItem QuadItem(x+20.0f, 12.5f, 25.0f, 25.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	x += 50.0f;
	static CTextCursor s_Cursor(20.0f);
	s_Cursor.Reset(0);
	s_Cursor.MoveTo(x, 10.0f);
	TextRender()->TextOutlined(&s_Cursor, "NET", -1);
	x += 50.0f;
	float y = 0.0f;

	const int NumBars = 5;
	int ScoreThresolds[NumBars] = {INT_MAX, 1000, 250, 50, -80};
	CUIRect BarRect = {
		x - 4.0f,
		y + LineHeight,
		6.0f,
		LineHeight
	};

	for(int Bar = 0; Bar < NumBars && Score <= ScoreThresolds[Bar]; Bar++)
	{
		BarRect.x += BarRect.w + 3.0f;
		CUIRect LocalBarRect = BarRect;
		LocalBarRect.h = BarRect.h*(Bar+2)/(float)NumBars+1.0f;
		LocalBarRect.y = BarRect.y + BarRect.h - LocalBarRect.h;
		LocalBarRect.Draw(vec4(0.9f,0.9f,0.9f,1.0f), 0.0f, CUIRect::CORNER_NONE);
	}
}

void CScoreboard::OnRender()
{
	// don't render scoreboard if menu or statboard is open
	if(m_pClient->m_pMenus->IsActive() || m_pClient->m_pStats->IsActive())
		return;

	// postpone the active state till the render area gets updated during the rendering
	if(m_Activate)
	{
		m_Active = true;
		m_Activate = false;
	}

	// close the motd if we actively wanna look on the scoreboard
	if(m_Active)
		m_pClient->m_pMotd->Clear();

	if(!IsActive())
		return;

	UI()->MapScreen();

	float Width = UI()->Screen()->w;
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

	const float Height = 400.0f * 3.0f;
	Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);
	static CTextCursor s_Cursor(FontSize);
	s_Cursor.m_Align = TEXTALIGN_TC;
	s_Cursor.MoveTo(Width/2, 39);
	s_Cursor.Reset();
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

			TextRender()->TextOutlined(&s_Cursor, aText, -1);
		}
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
		{
			char aText[256];
			str_copy(aText, Localize("Round over!"), sizeof(aText));

			TextRender()->TextOutlined(&s_Cursor, aText, -1);
		}
	}

	RenderRecordingNotification((Width/7.0f)*4, 180.0f);
	RenderNetworkQuality((Width/7.0f)*4 + 180.0f + 90.0f, 170.0f); 
}

bool CScoreboard::IsActive() const
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
