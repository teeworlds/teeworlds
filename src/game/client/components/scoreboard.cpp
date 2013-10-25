/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/localization.h>
#include <game/client/render.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>

#include "menus.h"
#include "scoreboard.h"


CScoreboard::CScoreboard()
{
	m_ScoreboardHeight = 0;
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

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aPlayerStats[i].Reset();
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
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(x, y, w, h, 5.0f);
	Graphics()->QuadsEnd();

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
		str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Match"), m_pClient->m_GameInfo.m_MatchCurrent, m_pClient->m_GameInfo.m_MatchNum);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);
		TextRender()->Text(0, x+w-tw-10.0f, y, 12.0f, aBuf, -1);
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 60.0f;

	int NumSpectators = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
			NumSpectators++;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(x, y, w, h, 5.0f);
	Graphics()->QuadsEnd();

	char aBuf[64];

	// Headline
	y += 22.0f;
	str_format(aBuf, sizeof(aBuf), "%s (%d):", Localize("Spectators"), NumSpectators);
	TextRender()->Text(0, x+10.0f, y, 12.0f, aBuf, w-20.0f);

	float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1);

	// spectator names
	x += tw+2.0f+10.0f;
	bool Multiple = false;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, 12.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = w-tw-20.0f;
	Cursor.m_MaxLines = 2;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			TextRender()->TextEx(&Cursor, ", ", -1);
		if(pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING)
			TextRender()->TextColor(1.0f, 1.0f, 0.0f, 1.0f);
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[i].m_aName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Multiple = true;
	}
}

void CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, int Align)
{
	if(Team == TEAM_SPECTATORS)
		return;

	float HeadlineHeight = 40.0f;
	float TitleFontsize = 20.0f;
	float HeadlineFontsize = 12.0f;
	float LineHeight = 20.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 2.0f;
	/*float ScoreOffset = x+2.0f, ScoreLength = 45.0f;
	float TeeOffset = ScoreOffset+ScoreLength, TeeLength = 25*TeeSizeMod;
	float NameOffset = TeeOffset+TeeLength, NameLength = 140.0f-TeeLength;
	float PingOffset = x+w-35.0f, PingLength = 25.0f;
	float CountryOffset = PingOffset-PingLength*2.0f, CountryLength = TeeSizeMod*30.0f;
	float ClanOffset = NameOffset+NameLength, ClanLength = 90.0f-CountryLength;*/
	float NameOffset = x+4.0f, NameLength = 136.0f;
	float TeeOffset = x+4.0f, TeeLength = 25*TeeSizeMod;
	float ClanOffset = NameOffset+NameLength, ClanLength = 90.0f;
	float KillOffset = ClanOffset+ClanLength, KillLength = 30.0f;
	float DeathOffset = KillOffset+KillLength, DeathLength = 30.0f;
	float ScoreOffset = DeathOffset+DeathLength, ScoreLength = 35.0f;
	float PingOffset = ScoreOffset+ScoreLength, PingLength = 35.0f;
	float tw = 0.0f;

	// count players
	int NumPlayers = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team == Team)
			NumPlayers++;
	m_PlayerLines = max(m_PlayerLines, NumPlayers);

	char aBuf[128] = {0};

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	if(Team == TEAM_RED && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		Graphics()->SetColor(0.975f, 0.17f, 0.17f, 0.75f);
	else if(Team == TEAM_BLUE)
		Graphics()->SetColor(0.17f, 0.46f, 0.975f, 0.75f);
	else
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRect(x, y, w, HeadlineHeight, 5.0f);
	Graphics()->QuadsEnd();

	// render title
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
			pTitle = Localize("Round over");
		else
			pTitle = Localize("Scoreboard");
		str_copy(aBuf, pTitle, sizeof(aBuf));
	}
	else
	{
		if(Team == TEAM_BLUE)
			str_format(aBuf, sizeof(aBuf), "(%d) %s", NumPlayers, pTitle);
		else
			str_format(aBuf, sizeof(aBuf), "%s (%d)", pTitle, NumPlayers);
	}

	if(Align == -1)
		TextRender()->Text(0, x+20.0f, y+5.0f, TitleFontsize, aBuf, -1);
	else
	{
		tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
		TextRender()->Text(0, x+w-tw-20.0f, y+5.0f, TitleFontsize, aBuf, -1);
	}

	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
	{
		int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue;
		str_format(aBuf, sizeof(aBuf), "%d", Score);
	}
	else
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW &&
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

	// calculate measurements
	/*if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 12)
	{
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
	}
	else if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 8)
	{
		LineHeight = 50.0f;
		TeeSizeMod = 0.9f;
		Spacing = 8.0f;
	}*/

	// render headlines
	y += HeadlineHeight;

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(x, y, w, LineHeight*(m_PlayerLines+1), 5.0f);
	if(m_PlayerLines)
		RenderTools()->DrawRoundRect(x, y+LineHeight, w, LineHeight*(m_PlayerLines), 5.0f);
	Graphics()->QuadsEnd();

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->Text(0, NameOffset, y+Spacing, HeadlineFontsize, Localize("Name"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(NameOffset, y, NameLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("Clan"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(ClanOffset, y, ClanLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("K"), -1);
	TextRender()->Text(0, KillOffset+KillLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("K"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(KillOffset, y, KillLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("D"), -1);
	TextRender()->Text(0, DeathOffset+DeathLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("D"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(DeathOffset, y, DeathLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	TextRender()->Text(0, ScoreOffset+ScoreLength/2-tw/2, y+Spacing, HeadlineFontsize, Localize("Score"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(ScoreOffset, y, ScoreLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, PingOffset+PingLength-tw, y+Spacing, HeadlineFontsize, Localize("Ping"), -1);
	/*Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawRoundRect(PingOffset, y, PingLength, LineHeight, 5.0f);
	Graphics()->QuadsEnd();*/
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render player entries
	y += LineHeight;
	float FontSize = HeadlineFontsize;
	CTextCursor Cursor;

	for(int RenderDead = 0; RenderDead < 2; ++RenderDead)
	{
		float ColorAlpha = RenderDead ? 0.5f : 1.0f;
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// make sure that we render the correct team
			const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
			if(!pInfo->m_pPlayerInfo || m_pClient->m_aClients[pInfo->m_ClientID].m_Team != Team || (!RenderDead && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)) ||
				(RenderDead && !(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD)))
				continue;

			// background so it's easy to find the local player or the followed one in spectator mode
			if(m_pClient->m_LocalClientID == pInfo->m_ClientID || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
			{
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.75f*ColorAlpha);
				RenderTools()->DrawRoundRect(x, y, w, LineHeight, 5.0f);
				Graphics()->QuadsEnd();
			}

			// avatar
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEADTEE].m_Id);
				Graphics()->QuadsBegin();
				IGraphics::CQuadItem QuadItem(TeeOffset, y+Spacing, 20*TeeSizeMod, 20*TeeSizeMod);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size = 20*TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2));
			}

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
				IGraphics::CQuadItem QuadItem(TeeOffset+0.0f, y-5.0f-Spacing/2.0f, Size/2.0f, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// name
			// todo: improve visual player ready state
			if(!(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_READY))
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, ColorAlpha);
			else if(RenderDead && pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_WATCHING)
				TextRender()->TextColor(1.0f, 1.0f, 0.0f, ColorAlpha);
			TextRender()->SetCursor(&Cursor, NameOffset+TeeLength, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = NameLength-TeeLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);

			// clan
			tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ClanLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);

			// K
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f*ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", m_aPlayerStats[pInfo->m_ClientID].m_Kills);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			TextRender()->SetCursor(&Cursor, KillOffset+KillLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = KillLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// D
			str_format(aBuf, sizeof(aBuf), "%d", m_aPlayerStats[pInfo->m_ClientID].m_Deaths);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			TextRender()->SetCursor(&Cursor, DeathOffset+DeathLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = DeathLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// score
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Score, -999, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			TextRender()->SetCursor(&Cursor, ScoreOffset+ScoreLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ScoreLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// country flag
			/*vec4 Color(1.0f, 1.0f, 1.0f, 0.5f*ColorAlpha);
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &Color,
												CountryOffset, y+(Spacing+TeeSizeMod*5.0f)/2.0f, CountryLength, LineHeight-Spacing-TeeSizeMod*5.0f);*/

			// ping
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Latency, 0, 999));
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = PingLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			y += LineHeight;
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	m_ScoreboardHeight = HeadlineHeight+LineHeight*(m_PlayerLines+1);
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(x, 0.0f, 180.0f, 50.0f, 15.0f, CUI::CORNER_B);
	Graphics()->QuadsEnd();

	//draw the red dot
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(x+20, 15.0f, 20.0f, 20.0f, 10.0f);
	Graphics()->QuadsEnd();

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+50.0f, 10.0f, 20.0f, aBuf, -1);
}

void CScoreboard::OnRender()
{
	if(!Active())
		return;

	// don't render scoreboard if menu is open
	if(m_pClient->m_pMenus->IsActive())
		return;

	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();


	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	float Width = Screen.w;
	float w = 364.0f;

	if(m_pClient->m_Snap.m_pGameData)
	{
		if(!(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS))
		{
			RenderScoreboard(Width/2-w/2, 70.0f, w, 0, 0, -1);

			RenderSpectators(Width/2-w/2, 73.0f+m_ScoreboardHeight, w);
			RenderGoals(Width/2-w/2, 73.0f+m_ScoreboardHeight, w);
		}
		else if(m_pClient->m_Snap.m_pGameDataTeam)
		{
			const char *pRedClanName = GetClanName(TEAM_RED);
			const char *pBlueClanName = GetClanName(TEAM_BLUE);

			if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			{
				char aText[256];
				str_copy(aText, Localize("Draw!"), sizeof(aText));

				if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue)
				{
					if(pRedClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
					else
						str_copy(aText, Localize("Red team wins!"), sizeof(aText));
				}
				else if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed)
				{
					if(pBlueClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
					else
						str_copy(aText, Localize("Blue team wins!"), sizeof(aText));
				}

				float w = TextRender()->TextWidth(0, 86.0f, aText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, aText, -1);
			}
			else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
			{
				char aText[256];
				str_copy(aText, Localize("Round over!"), sizeof(aText));

				float w = TextRender()->TextWidth(0, 86.0f, aText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, aText, -1);
			}

			RenderScoreboard(Width/2-w-1.5f, 70.0f, w, TEAM_RED, pRedClanName ? pRedClanName : Localize("Red team"), -1);
			RenderScoreboard(Width/2+1.5f, 70.0f, w, TEAM_BLUE, pBlueClanName ? pBlueClanName : Localize("Blue team"), 1);

			RenderSpectators(Width/2-w-1.5f, 73.0f+m_ScoreboardHeight, w*2.0f+3.0f);
			RenderGoals(Width/2-w-1.5f, 73.0f+m_ScoreboardHeight, w*2.0f+3.0f);
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
}

bool CScoreboard::Active()
{
	// if we activly wanna look on the scoreboard
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
