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

		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_apPlayerInfos[0];
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

float CScoreboard::RenderScoreboard(float x, float y, float w, int Align)
{
	// ready mode
	const CGameClient::CSnapState &Snap = m_pClient->m_Snap;

	bool Race = m_pClient->m_GameInfo.m_GameFlags & GAMEFLAG_RACE;
	bool TeamPlay = m_pClient->m_GameInfo.m_GameFlags & GAMEFLAG_TEAMS;

	float HeadlineHeight = 40.0f;
	float LineHeight = 24.0f;

	int NumPlayers = m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED] + m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE];
	int PlayerLines = maximum(1, minimum(NumPlayers, 12));

	// background
	Graphics()->BlendNormal();
	const vec4 BackgroundColor(0.0f, 0.0f, 0.0f, 0.3f);

	CUIRect MainView = {x, y, w, HeadlineHeight + (PlayerLines + 1) * LineHeight};
	MainView.Draw(BackgroundColor);

	CUIRect Header;
	MainView.HSplitTop(HeadlineHeight, &Header, &MainView);
	Header.Draw(BackgroundColor);

	Header.VSplitLeft(20.0f, 0, &Header);
	Header.VSplitRight(20.0f, &Header, 0);

	const char *pTitle = Client()->GetCurrentMapName();
	if(Snap.m_pGameData && Snap.m_pGameData->m_GameStateFlags & GAMESTATEFLAG_GAMEOVER)
		pTitle = Localize("Game over");
	else if(Snap.m_pGameData && Snap.m_pGameData->m_GameStateFlags & GAMESTATEFLAG_ROUNDOVER)
		pTitle = Localize("Round over");
	UI()->DoLabel(&Header, pTitle, 20.0f, TEXTALIGN_ML, -1.0f, false);

	char aBuf[256];
	if(Race)
	{
		FormatTime(aBuf, sizeof(aBuf), m_pClient->m_Snap.m_pGameDataRace->m_BestTime, m_pClient->RacePrecision());
	}
	else
	{
		int Score = 0;
		if(TeamPlay)
		{
			int PlayerID = m_pClient->m_LocalClientID;
			if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0 && m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Active)
				PlayerID = m_pClient->m_Snap.m_SpecInfo.m_SpectatorID;
			int CheckTeam = m_pClient->m_aClients[PlayerID].m_Team;
			if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0 &&
				m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Active)
			{
				CheckTeam = m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Team;
			}
			Score = CheckTeam == TEAM_RED ? m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue;
		}
		else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0 &&
			m_pClient->m_Snap.m_apPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
		{
			Score = m_pClient->m_Snap.m_apPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
		}
		else if(m_pClient->m_Snap.m_pLocalInfo)
		{
			Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
		}
		str_format(aBuf, sizeof(aBuf), "%d", Score);
	}

	UI()->DoLabel(&Header, aBuf, 20.0f, TEXTALIGN_MR, -1.0f, false);

	MainView.HSplitBottom(LineHeight, &MainView, &Header);
	Header.Draw(BackgroundColor);
	
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0, 0, 0, 0);
	ScrollParams.m_ScrollUnit = 60.0f; // inconsistent margin, 3 category header per scroll
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_SCROLL_WITHOUT_HOVERD | CScrollRegionParams::FLAG_HIDDEN_SCROLLBAR;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;

	if(TeamPlay)
	{
		RenderTeamScoreboard(TEAM_RED, MainView, LineHeight, &s_ScrollRegion);
		RenderTeamScoreboard(TEAM_BLUE, MainView, LineHeight, &s_ScrollRegion);
	}
	else
	{
		RenderTeamScoreboard(-1, MainView, LineHeight, &s_ScrollRegion);
	}

	s_ScrollRegion.End();

	return LineHeight + HeadlineHeight + (PlayerLines + 1) * LineHeight;
}

void CScoreboard::RenderTeamScoreboard(int Team, CUIRect &MainView, float LineHeight, CScrollRegion *pScrollRegion)
{
	char aBuf[256];
	const float Spacing = 2.0f;
	const float LineFontSize = 12.0f;
	const vec4 GreyTextColor(1.0f, 1.0f, 1.0f, 0.75f);
	const vec4 FriendHighlightColor(0.4f, 1.0f, 0.4f, 0.4f);
	const vec4 LocalHighlightColor(1.0f, 1.0f, 1.0f, 0.5f);
	const vec4 TeamRectColor = Team == TEAM_RED ? vec4(1.0f, 0.0f, 0.0f, 0.25f) : vec4(0.0f, 0.0f, 1.0f, 0.25f);

	const bool ReadyMode = m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
		const CGameClient::CClientData *pData = &m_pClient->m_aClients[pInfo->m_ClientID];
		if(!pInfo->m_pPlayerInfo || pData->m_Team == TEAM_SPECTATORS)
			continue;
		if(Team != -1) // -1 means render all players
		{
			if(pData->m_Team != Team)
				continue;
		}
		CUIRect Playerline;
		MainView.HSplitTop(LineHeight, &Playerline, &MainView);
		pScrollRegion->AddRect(Playerline);
		if(pScrollRegion->IsRectClipped(Playerline))
			continue;
		// highlight the line
		{
			if(Team != -1)
				Playerline.Draw(TeamRectColor);
			if(pInfo->m_ClientID == m_pClient->m_LocalClientID)
				Playerline.Draw(LocalHighlightColor);
			else if(m_pClient->Friends()->IsFriend(pData->m_aName, pData->m_aClan, false))
				Playerline.Draw(FriendHighlightColor);
		}

		Playerline.VSplitLeft(Spacing * 2, 0, &Playerline);
		Playerline.VSplitRight(Spacing * 2, &Playerline, 0);

		{
			CUIRect Latency;
			Playerline.VSplitLeft(40.0f, &Latency, &Playerline);
			str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_pPlayerInfo->m_Latency);
			UI()->DoLabelColor(&Latency, GreyTextColor, aBuf, LineFontSize, TEXTALIGN_MC, -1.0f, false);
		}
		Playerline.VSplitLeft(Spacing, 0, &Playerline);
		{
			CUIRect Flag;
			Playerline.VSplitLeft(40.0f, &Flag, &Playerline);
			const vec4 CFColor(1, 1, 1, 0.75f);
			const float OffsetY = (LineHeight - Flag.w / 2.0f) / 2.0f;
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &CFColor, Flag.x, Flag.y + OffsetY, Flag.w, Flag.w / 2.0f);
		}
		Playerline.VSplitLeft(Spacing, 0, &Playerline);

		CUIRect PlayerSkin;
		Playerline.VSplitLeft(LineHeight, &PlayerSkin, &Playerline);

		if(pInfo->m_pPlayerInfo->m_PlayerFlags & PLAYERFLAG_DEAD)
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEADTEE].m_Id);
			Graphics()->QuadsBegin();
			if(m_pClient->m_GameInfo.m_GameFlags & GAMEFLAG_TEAMS)
			{
				const vec4 Color = m_pClient->m_pSkins->GetColorV4(m_pClient->m_pSkins->GetTeamColor(true, 0, m_pClient->m_aClients[pInfo->m_ClientID].m_Team, SKINPART_BODY), false);
				Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
			}
			IGraphics::CQuadItem QuadItem(PlayerSkin.x, PlayerSkin.y, PlayerSkin.w, PlayerSkin.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		else
		{
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
			TeeInfo.m_Size = LineHeight;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), PlayerSkin.Center() + vec2(0.0f, 2.0f));
		}

		{
			CUIRect PlayerName;
			Playerline.VSplitLeft(120.0f, &PlayerName, &Playerline);
			if(Config()->m_ClShowUserId)
			{
				const vec4 IdTextColor = vec4(0.1f, 0.1f, 0.1f, 1.0f);
				float ClientIDWidth = UI()->DrawClientID(LineFontSize, vec2(PlayerName.x, PlayerName.y + (LineHeight - LineFontSize) / 2.0f - 0.15f * LineFontSize), pInfo->m_ClientID, LocalHighlightColor, IdTextColor);
				PlayerName.VSplitLeft(ClientIDWidth, 0, &PlayerName);
			}
			str_copy(aBuf, pData->m_aName, sizeof(aBuf));
			UI()->DoLabel(&PlayerName, aBuf, LineFontSize, TEXTALIGN_ML, 100.0f, false);
		}
		Playerline.VSplitLeft(Spacing, 0, &Playerline);
		// ready / watching
		if(ReadyMode && pInfo->m_pPlayerInfo->m_PlayerFlags & PLAYERFLAG_READY)
		{
			CUIRect PlayerReady;
			Playerline.VSplitLeft(20.0f, &PlayerReady, &Playerline);
			UI()->DoLabelColor(&PlayerReady, vec4(0.1f, 1.0f, 0.1f, 1.0f), "\xE2\x9C\x93", LineFontSize, TEXTALIGN_ML, 20.0f, false);
		}
		Playerline.VSplitLeft(Spacing, 0, &Playerline);
		{
			CUIRect PlayerClan;
			Playerline.VSplitLeft(80.0f, &PlayerClan, &Playerline);
			str_copy(aBuf, pData->m_aClan, sizeof(aBuf));
			UI()->DoLabelColor(&PlayerClan, GreyTextColor, aBuf, LineFontSize, TEXTALIGN_MC, 80.0f, false);
		}
		Playerline.VSplitLeft(Spacing, 0, &Playerline);
		{
			CUIRect PlayerScore = Playerline;
			if(m_pClient->m_GameInfo.m_GameFlags & GAMEFLAG_RACE)
			{
				FormatTime(aBuf, sizeof(aBuf), pInfo->m_pPlayerInfo->m_Score, m_pClient->RacePrecision());
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_pPlayerInfo->m_Score);
			}
			UI()->DoLabel(&PlayerScore, aBuf, LineFontSize, TEXTALIGN_MC, -1.0f, false);
		}
	}
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
	float w = 400.0f;
	float FontSize = 86.0f;

	const char* pCustomRedClanName = GetClanName(TEAM_RED);
	const char* pCustomBlueClanName = GetClanName(TEAM_BLUE);
	const char* pRedClanName = pCustomRedClanName ? pCustomRedClanName : Localize("Red team");
	const char* pBlueClanName = pCustomBlueClanName ? pCustomBlueClanName : Localize("Blue team");

	if(m_pClient->m_Snap.m_pGameData)
	{
		float ScoreboardHeight = RenderScoreboard(Width / 2 - w / 2, y, w, 0);
		float SpectatorHeight = RenderSpectators(Width / 2 - w / 2, y + 3.0f + ScoreboardHeight, w);
		RenderGoals(Width / 2 - w / 2, y + 3.0f + ScoreboardHeight, w);

		// scoreboard size
		m_TotalRect.x = Width / 2 - w / 2;
		m_TotalRect.y = y;
		m_TotalRect.w = w;
		m_TotalRect.h = ScoreboardHeight + SpectatorHeight + 3.0f;
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
