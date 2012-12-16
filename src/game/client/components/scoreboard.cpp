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

#include "scoreboard.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnReset()
{
	m_Active = false;
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
	float h = 50.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render goals
	y += 10.0f;
	if(m_pClient->m_GameInfo.m_ScoreLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
		TextRender()->Text(0, x+10.0f, y, 20.0f, aBuf, -1);
	}
	if(m_pClient->m_GameInfo.m_TimeLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_GameInfo.m_TimeLimit);
		TextRender()->Text(0, x+230.0f, y, 20.0f, aBuf, -1);
	}
	if(m_pClient->m_GameInfo.m_MatchNum && m_pClient->m_GameInfo.m_MatchCurrent)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Match"), m_pClient->m_GameInfo.m_MatchCurrent, m_pClient->m_GameInfo.m_MatchNum);
		float tw = TextRender()->TextWidth(0, 20.0f, aBuf, -1);
		TextRender()->Text(0, x+w-tw-10.0f, y, 20.0f, aBuf, -1);
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 140.0f;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// Headline
	y += 10.0f;
	TextRender()->Text(0, x+10.0f, y, 28.0f, Localize("Spectators"), w-20.0f);

	// spectator names
	y += 30.0f;
	bool Multiple = false;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x+10.0f, y, 22.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = w-20.0f;
	Cursor.m_MaxLines = 4;
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

void CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle)
{
	if(Team == TEAM_SPECTATORS)
		return;

	float h = 760.0f;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 17.0f);
	Graphics()->QuadsEnd();

	// render title
	float TitleFontsize = 40.0f;
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
			pTitle = Localize("Round over");
		else
			pTitle = Localize("Score board");
	}
	TextRender()->Text(0, x+20.0f, y, TitleFontsize, pTitle, -1);

	char aBuf[128] = {0};
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
	float tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
	TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);

	// calculate measurements
	x += 10.0f;
	float LineHeight = 60.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 16.0f;
	if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 12)
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
	}

	float ScoreOffset = x+10.0f, ScoreLength = 60.0f;
	float TeeOffset = ScoreOffset+ScoreLength, TeeLength = 60*TeeSizeMod;
	float NameOffset = TeeOffset+TeeLength, NameLength = 300.0f-TeeLength;
	float PingOffset = x+610.0f, PingLength = 65.0f;
	float CountryOffset = PingOffset-(LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f, CountryLength = (LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f;
	float ClanOffset = x+370.0f, ClanLength = 230.0f-CountryLength;

	// render headlines
	y += 50.0f;
	float HeadlineFontsize = 22.0f;
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	TextRender()->Text(0, ScoreOffset+ScoreLength-tw, y, HeadlineFontsize, Localize("Score"), -1);

	TextRender()->Text(0, NameOffset, y, HeadlineFontsize, Localize("Name"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y, HeadlineFontsize, Localize("Clan"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, PingOffset+PingLength-tw, y, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render player entries
	y += HeadlineFontsize*2.0f;
	float FontSize = 24.0f;
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
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f*ColorAlpha);
				RenderTools()->DrawRoundRect(x, y, w-20.0f, LineHeight, 15.0f);
				Graphics()->QuadsEnd();
			}

			// score
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Score, -999, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, ScoreOffset+ScoreLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ScoreLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

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

			// avatar
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEADTEE].m_Id);
				Graphics()->QuadsBegin();
				IGraphics::CQuadItem QuadItem(TeeOffset, y, 64*TeeSizeMod, 64*TeeSizeMod);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2));
			}

			// name
			// todo: improve visual player ready state
			if(!(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_READY))
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, ColorAlpha);
			else if(RenderDead && pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_WATCHING)
				TextRender()->TextColor(1.0f, 1.0f, 0.0f, ColorAlpha);
			TextRender()->SetCursor(&Cursor, NameOffset, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = NameLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);

			// clan
			tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
			TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ClanLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

			// country flag
			vec4 Color(1.0f, 1.0f, 1.0f, 0.5f*ColorAlpha);
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &Color,
												CountryOffset, y+(Spacing+TeeSizeMod*5.0f)/2.0f, CountryLength, LineHeight-Spacing-TeeSizeMod*5.0f);

			// ping
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Latency, 0, 1000));
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = PingLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			y += LineHeight+Spacing;
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
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

	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();


	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 700.0f;

	if(m_pClient->m_Snap.m_pGameData)
	{
		if(!(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS))
			RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);
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

			RenderScoreboard(Width/2-w-5.0f, 150.0f, w, TEAM_RED, pRedClanName ? pRedClanName : Localize("Red team"));
			RenderScoreboard(Width/2+5.0f, 150.0f, w, TEAM_BLUE, pBlueClanName ? pBlueClanName : Localize("Blue team"));
		}

		RenderGoals(Width/2-w/2, 150+760+10, w);
		RenderSpectators(Width/2-w/2, 150+760+10+50+10, w);
	}

	RenderRecordingNotification((Width/7)*4);
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
