/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>
#include <game/localization.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/teecomp.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/skins.h>
#include <game/client/components/teecomp_stats.h>
#include "scoreboard.h"

// enums for columns
enum
{
	COLUMN_SCORE_SCORE=0,
	COLUMN_SCORE_TEE,
	COLUMN_SCORE_NAME,
	COLUMN_SCORE_CLAN,
	COLUMN_SCORE_COUNTRY,
	COLUMN_SCORE_PING,

	COLUMN_SPEC_TEE=0,
	COLUMN_SPEC_NAME,
	COLUMN_SPEC_CLAN,
	COLUMN_SPEC_COUNTRY,
};

CScoreboard::CColumn CScoreboard::ms_Scoreboard[] = {CScoreboard::CColumn(Localize("Score"), 60.0f, 10.0f, CScoreboard::CColumn::ALIGN_RIGHT),
																CScoreboard::CColumn(0, 48.0f, 0.0f, CScoreboard::CColumn::ALIGN_NOTEXT),
																CScoreboard::CColumn(Localize("Name"), 250.0f, 0.0f, CScoreboard::CColumn::ALIGN_LEFT),
																CScoreboard::CColumn(Localize("Clan"), 145.0f, 0.0f, CScoreboard::CColumn::ALIGN_MIDDLE),
																CScoreboard::CColumn(0, 72.0f, 0.0f, CScoreboard::CColumn::ALIGN_NOTEXT),
																CScoreboard::CColumn(Localize("Ping"), 66.0f, 0.0f, CScoreboard::CColumn::ALIGN_RIGHT)};
CScoreboard::CColumn CScoreboard::ms_Spectatorboard[] = {CScoreboard::CColumn(0, 48.0f, 0.0f, CScoreboard::CColumn::ALIGN_NOTEXT),
																CScoreboard::CColumn(Localize("Name"), 250.0f, 0.0f, CScoreboard::CColumn::ALIGN_LEFT),
																CScoreboard::CColumn(Localize("Clan"), 145.0f, 0.0f, CScoreboard::CColumn::ALIGN_MIDDLE),
																CScoreboard::CColumn(0, 72.0f, 0.0f, CScoreboard::CColumn::ALIGN_NOTEXT)};

CScoreboard::CColumn::CColumn(const char* pTitle, float Width, float Offset, int RenderAlign)
: m_pTitle(pTitle), m_Width(Width), m_Offset(Offset), m_RenderAlign(RenderAlign)
{
	m_Active = true;
}

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

void CScoreboard::RenderGoals(float Width, float Height)
{
	if(!m_pClient->m_Snap.m_pGameInfoObj)
		return;

	float w = 700.0f;
	float h = 70.0f;
	float x = Width/2-w/2;
	float y = Height-h;

	float FontSize = 20.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRectExt(x, y, w, h, 17.0f, CUI::CORNER_T);
	Graphics()->QuadsEnd();

	y += 10.0f;

	char aBuf[64];
	if(m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit)
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit);
	else
		str_format(aBuf, sizeof(aBuf), "%s: -", Localize("Score limit"));
	TextRender()->Text(0, x+10.0f, y, FontSize, aBuf, -1);

	if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit)
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit);
	else
		str_format(aBuf, sizeof(aBuf), Localize("%s: -"), Localize("Time limit"));
	float tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
	TextRender()->Text(0, x+w/2-tw/2, y, FontSize, aBuf, -1);

	if(m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum && m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent)
		str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent, m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum);
	else
		str_format(aBuf, sizeof(aBuf), "%s: -/-", Localize("Round"));
	tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
	TextRender()->Text(0, x+w-tw-10.0f, y, FontSize, aBuf, -1);

	y += FontSize+10.0f;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Game type"), Client()->State() == IClient::STATE_DEMOPLAYBACK ? "-" : CurrentServerInfo.m_aGameType);
	TextRender()->Text(0, x+10.0f, y, FontSize, aBuf, -1);

	str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Players"), m_pClient->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxClients);
	tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
	TextRender()->Text(0, x+w/2-tw/2, y, FontSize, aBuf, -1);

	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Map"), CurrentServerInfo.m_aMap);
	else
		str_format(aBuf, sizeof(aBuf), "%s: -", Localize("Map"));
	tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
	if(tw > w/3.0f+10.0f)
		tw = w/3.0f+10.0f;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x+w-tw-10.0f, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = tw;
	TextRender()->TextEx(&Cursor, aBuf, -1);
}

void CScoreboard::RenderSpectators(float Width, float y)
{
	int NumSpectators = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(pInfo && pInfo->m_Team == TEAM_SPECTATORS)
			NumSpectators++;
	}

	if(!NumSpectators)
		return;

	float HeadlineFontsize = 22.0f;
	float HeadlineHeight = 30.0f;
	float TitleFontsize = 28.0f;
	float TitleHight = 50.0f;
	float LineHeight = 40.0f;
	float TeeSizeMod = 0.8f;

	float h = TitleHight+15.0f;
	if(NumSpectators > 1)
		h += (int)((NumSpectators+1)/2.0f) * LineHeight;
	else
		h += LineHeight;
	if(g_Config.m_TcScoreboardInfos&TC_SCORE_TITLE)
		h += HeadlineHeight;

	// get width
	float w = 30.0f;
	for(int i = 0; i < 4; i++)
		if(ms_Spectatorboard[i].m_Active)
		{
			if(NumSpectators > 1)
				w += ms_Spectatorboard[i].m_Width*2.0f;
			else
				w += ms_Spectatorboard[i].m_Width;
		}

	float x = Width/2-w/2;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();
	
	// Headline
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s (%d)", Localize("Spectators"), NumSpectators);
	TextRender()->Text(0, x+10.0f, y+10.0f, TitleFontsize, aBuf, w-20.0f);

	y += TitleHight;

	// render column titles
	float tw = 0.0f;
	float TmpX = x+10.0f;
	if(g_Config.m_TcScoreboardInfos&TC_SCORE_TITLE)
	{
		for(int s = 0; s < 2; s++)
		{
			for(int i = 0; i < 4; i++)
			{
				if(!ms_Spectatorboard[i].m_Active)
					continue;

				if(ms_Spectatorboard[i].m_RenderAlign != CColumn::ALIGN_NOTEXT)
				{
					if(ms_Spectatorboard[i].m_RenderAlign == CColumn::ALIGN_LEFT)
						TextRender()->Text(0, TmpX+ms_Spectatorboard[i].m_Offset, y, HeadlineFontsize, ms_Spectatorboard[i].m_pTitle, -1);
					else if(ms_Spectatorboard[i].m_RenderAlign == CColumn::ALIGN_RIGHT)
					{
						tw = TextRender()->TextWidth(0, HeadlineFontsize, ms_Spectatorboard[i].m_pTitle, -1);
						TextRender()->Text(0, (TmpX+ms_Spectatorboard[i].m_Width)-tw, y, HeadlineFontsize, ms_Spectatorboard[i].m_pTitle, -1);
					}
					else
					{
						tw = TextRender()->TextWidth(0, HeadlineFontsize, ms_Spectatorboard[i].m_pTitle, -1);
						TextRender()->Text(0, TmpX+ms_Spectatorboard[i].m_Offset+ms_Spectatorboard[i].m_Width/2-tw/2, y, HeadlineFontsize, ms_Spectatorboard[i].m_pTitle, -1);
					}
				}

				TmpX += ms_Spectatorboard[i].m_Width;
			}

			if(NumSpectators > 1)
				TmpX += 10.0f;
			else
				break;
		}

		y += HeadlineHeight;
	}

	float FontSize = 24.0f;
	CTextCursor Cursor;
	int PlayerNum = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		PlayerNum++;

		if(PlayerNum % 2)
			TmpX = x;
		TmpX += 10.0f;

		for(int j = 0; j < 4; j++)
		{
			if(!ms_Spectatorboard[j].m_Active)
				continue;

			if(j == COLUMN_SPEC_TEE)
			{
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TmpX+ms_Spectatorboard[j].m_Offset+ms_Spectatorboard[j].m_Width/2, y+LineHeight/2));
			}
			else if(j == COLUMN_SPEC_NAME)
			{
				TextRender()->SetCursor(&Cursor, TmpX+ms_Spectatorboard[j].m_Offset, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = ms_Spectatorboard[j].m_Width;
				if(g_Config.m_ClScoreboardClientID)
				{
					str_format(aBuf, sizeof(aBuf), "%d | %s", pInfo->m_ClientID, m_pClient->m_aClients[pInfo->m_ClientID].m_aName);
					TextRender()->TextEx(&Cursor, aBuf, -1);
				}
				else
					TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			}
			else if(j == COLUMN_SPEC_CLAN)
			{
				tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
				TextRender()->SetCursor(&Cursor, tw >= ms_Spectatorboard[j].m_Width ? TmpX+ms_Spectatorboard[j].m_Offset : TmpX+ms_Spectatorboard[j].m_Offset+ms_Spectatorboard[j].m_Width/2-tw/2, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = ms_Spectatorboard[j].m_Width;
				TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
			}
			else if(j == COLUMN_SPEC_COUNTRY)
			{
				Graphics()->TextureSet(m_pClient->m_pCountryFlags->GetByCountryCode(m_pClient->m_aClients[pInfo->m_ClientID].m_Country)->m_Texture);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
				IGraphics::CQuadItem QuadItem(TmpX+ms_Spectatorboard[j].m_Offset, y+(TeeSizeMod*5.0f)/2.0f, ms_Spectatorboard[j].m_Width, LineHeight-TeeSizeMod*5.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			TmpX += ms_Spectatorboard[j].m_Width;
		}

		if(PlayerNum % 2 == 0)
			y += LineHeight;
	}
}

int CScoreboard::RenderScoreboard(float Width, float y, int Team, const char *pTitle, bool TeamPlay)
{
	if(Team == TEAM_SPECTATORS)
		return 0;

	float HeadlineFontsize = 22.0f;
	float HeadlineHeight = 30.0f;
	float TitleFontsize = 36.0f;
	float TitleHight = 48.0f;
	float LineHeight = 40.0f;
	float TeeSizeMod = 0.8f;

	float h = TitleHight+max(m_pClient->m_Snap.m_aTeamSize[TEAM_RED], m_pClient->m_Snap.m_aTeamSize[TEAM_BLUE])*LineHeight+15.0f;
	if(g_Config.m_TcScoreboardInfos&TC_SCORE_TITLE)
		h += HeadlineHeight;
	
	// get width
	float w = 30.0f;
	for(int i = 0; i < 6; i++)
	{
		if(m_pClient->m_IsRace && i == COLUMN_SCORE_SCORE)
		{
			ms_Scoreboard[i].m_Width = 125.0f;
			ms_Scoreboard[i].m_RenderAlign = CColumn::ALIGN_MIDDLE;
		}

		if(ms_Scoreboard[i].m_Active)
			w += ms_Scoreboard[i].m_Width;
	}

	float x = Width/2-w/2;

	if(TeamPlay)
	{
		if(Team == TEAM_RED)
			x = Width/2-w-5.0f;
		else
			x = Width/2+5.0f;
	}

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 17.0f);
	Graphics()->QuadsEnd();

	// render title
	char aBuf[128];
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
	str_format(aBuf, sizeof(aBuf), "%s (%d)", pTitle, m_pClient->m_Snap.m_aTeamSize[Team]);
	TextRender()->Text(0, x+20.0f, y, TitleFontsize, aBuf, -1);
	
	float tw = 0.0f;
	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_Snap.m_pGameDataObj && !m_pClient->m_IsRace)
		{
			int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;
			str_format(aBuf, sizeof(aBuf), "%d", Score);

			tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
			TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);
		}
	}
	else
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW &&
			m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID] && !m_pClient->m_IsRace)
		{
			int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);

			tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
			TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);
		}
		else if(m_pClient->m_Snap.m_pLocalInfo && !m_pClient->m_IsRace)
		{
			int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);

			tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
			TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);
		}
	}

	y += TitleHight;

	// render column titles
	float TmpX = x+10.0f;
	if(g_Config.m_TcScoreboardInfos&TC_SCORE_TITLE)
	{
		for(int i = 0; i < 6; i++)
		{
			if(!ms_Scoreboard[i].m_Active)
				continue;

			if(ms_Scoreboard[i].m_RenderAlign != CColumn::ALIGN_NOTEXT)
			{
				if(ms_Scoreboard[i].m_RenderAlign == CColumn::ALIGN_LEFT)
					TextRender()->Text(0, TmpX+ms_Scoreboard[i].m_Offset, y, HeadlineFontsize, ms_Scoreboard[i].m_pTitle, -1);
				else if(ms_Scoreboard[i].m_RenderAlign == CColumn::ALIGN_RIGHT)
				{
					tw = TextRender()->TextWidth(0, HeadlineFontsize, ms_Scoreboard[i].m_pTitle, -1);
					TextRender()->Text(0, (TmpX+ms_Scoreboard[i].m_Width)-tw, y, HeadlineFontsize, ms_Scoreboard[i].m_pTitle, -1);
				}
				else
				{
					tw = TextRender()->TextWidth(0, HeadlineFontsize, ms_Scoreboard[i].m_pTitle, -1);
					TextRender()->Text(0, TmpX+ms_Scoreboard[i].m_Offset+ms_Scoreboard[i].m_Width/2-tw/2, y, HeadlineFontsize, ms_Scoreboard[i].m_pTitle, -1);
				}
			}

			TmpX += ms_Scoreboard[i].m_Width;
		}

		y += HeadlineHeight;
	}

	// render player entries
	float FontSize = 24.0f;
	CTextCursor Cursor;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// make sure that we render the correct team
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;
		
		// background so it's easy to find the local player or the followed one in spectator mode
		if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(x+10.0f, y, w-20.0f, LineHeight, 15.0f);
			Graphics()->QuadsEnd();
		}

		TmpX = x+10.0f;
		for(int j = 0; j < 6; j++)
		{
			if(!ms_Scoreboard[j].m_Active)
				continue;

			if(j == COLUMN_SCORE_SCORE)
			{
				if(m_pClient->m_IsRace)
				{
					// reset time
					if(pInfo->m_Score == -9999)
						m_pClient->m_aClients[pInfo->m_ClientID].m_Score = 0;
					
					float Time = m_pClient->m_aClients[pInfo->m_ClientID].m_Score;
					if(Time > 0)
					{
						str_format(aBuf, sizeof(aBuf), "%02d:%06.3f", (int)Time/60, fmod(Time,60));
						tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
						TextRender()->SetCursor(&Cursor, TmpX+ms_Scoreboard[j].m_Offset+ms_Scoreboard[j].m_Width/2-tw/2, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
						Cursor.m_LineWidth = ms_Scoreboard[j].m_Width;
						TextRender()->TextEx(&Cursor, aBuf, -1);
					}
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 999));
					tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
					TextRender()->SetCursor(&Cursor, TmpX+ms_Scoreboard[j].m_Offset+ms_Scoreboard[j].m_Width/2-tw/2, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
					Cursor.m_LineWidth = ms_Scoreboard[j].m_Width;
					TextRender()->TextEx(&Cursor, aBuf, -1);
				}
			}
			else if(j == COLUMN_SCORE_TEE)
			{
				// flag
				if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
					m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
					m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
				{
					Graphics()->BlendNormal();
					if(g_Config.m_TcColoredFlags)
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME_GRAY].m_Id);
					else
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
					Graphics()->QuadsBegin();

					RenderTools()->SelectSprite(pInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

					if(g_Config.m_TcColoredFlags)
					{
						vec3 Col = CTeecompUtils::GetTeamColor(1-pInfo->m_Team, m_pClient->m_Snap.m_pLocalInfo->m_Team, 
							g_Config.m_TcColoredTeesTeam1, g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
						Graphics()->SetColor(Col.r, Col.g, Col.b, 1.0f);
					}
					
					float Size = LineHeight;
					IGraphics::CQuadItem QuadItem(TmpX+ms_Scoreboard[j].m_Offset, y-5.0f/2.0f, Size/2.0f, Size);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}

				// tee
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size *= TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TmpX+ms_Scoreboard[j].m_Offset+ms_Scoreboard[j].m_Width/2, y+LineHeight/2));
			}
			else if(j == COLUMN_SCORE_NAME)
			{
				TextRender()->SetCursor(&Cursor, TmpX+ms_Scoreboard[j].m_Offset, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = ms_Scoreboard[j].m_Width;
				if(g_Config.m_ClScoreboardClientID)
				{
					str_format(aBuf, sizeof(aBuf), "%d | %s", pInfo->m_ClientID, m_pClient->m_aClients[pInfo->m_ClientID].m_aName);
					TextRender()->TextEx(&Cursor, aBuf, -1);
				}
				else
					TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			}
			else if(j == COLUMN_SCORE_CLAN)
			{
				tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
				TextRender()->SetCursor(&Cursor, tw >= ms_Scoreboard[j].m_Width ? TmpX+ms_Scoreboard[j].m_Offset : TmpX+ms_Scoreboard[j].m_Offset+ms_Scoreboard[j].m_Width/2-tw/2, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = ms_Scoreboard[j].m_Width;
				TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
			}
			else if(j == COLUMN_SCORE_COUNTRY)
			{
				Graphics()->TextureSet(m_pClient->m_pCountryFlags->GetByCountryCode(m_pClient->m_aClients[pInfo->m_ClientID].m_Country)->m_Texture);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
				IGraphics::CQuadItem QuadItem(TmpX+ms_Scoreboard[j].m_Offset, y+(TeeSizeMod*5.0f)/2.0f, ms_Scoreboard[j].m_Width, LineHeight-TeeSizeMod*5.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else if(j == COLUMN_SCORE_PING)
			{
				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 1000));
				tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
				TextRender()->SetCursor(&Cursor, TmpX+ms_Scoreboard[j].m_Offset+ms_Scoreboard[j].m_Width-tw, y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = ms_Scoreboard[j].m_Width;
				TextRender()->TextEx(&Cursor, aBuf, -1);
			}

			TmpX += ms_Scoreboard[j].m_Width;
		}

		y += LineHeight;
	}
	return h;
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
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

	SetActiveColumns();

	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();


	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 700.0f;
	
	// resize scoreboard for race
	if(m_pClient->m_IsRace)
		w += 40.0f;

	// scoreboard hight
	int ScoreboardHight = 50;

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
			ScoreboardHight = max(ScoreboardHight, RenderScoreboard(Width, 150.0f, 0, 0, false));
		else
		{
			char aText[64];
			const char *pRedClanName = GetClanName(TEAM_RED);
			const char *pBlueClanName = GetClanName(TEAM_BLUE);

			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && m_pClient->m_Snap.m_pGameDataObj)
			{
				str_copy(aText, Localize("Draw!"), sizeof(aText));
				if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue)
				{
					if(pRedClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
					else if(m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE && g_Config.m_TcColoredTeesMethod == 1)
						str_format(aText, sizeof(aText), Localize("%s team wins!"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam2));
					else
						str_format(aText, sizeof(aText), Localize("%s team wins!"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam1));
				}
				else if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed)
				{
					if(pBlueClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
					else if(m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE && g_Config.m_TcColoredTeesMethod == 1)
						str_format(aText, sizeof(aText), Localize("%s team wins!"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam1));
					else
						str_format(aText, sizeof(aText), Localize("%s team wins!"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam2));
				}

				float w = TextRender()->TextWidth(0, 86.0f, aText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, aText, -1);
			}

			if(m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE && g_Config.m_TcColoredTeesMethod == 1)
				str_format(aText, sizeof(aText), Localize("%s team"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam2));
			else
				str_format(aText, sizeof(aText), Localize("%s team"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam1));
			ScoreboardHight = max(ScoreboardHight, RenderScoreboard(Width, 150.0f, TEAM_RED, pRedClanName ? pRedClanName : Localize(aText), true));
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE && g_Config.m_TcColoredTeesMethod == 1)
				str_format(aText, sizeof(aText), Localize("%s team"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam1));
			else
				str_format(aText, sizeof(aText), Localize("%s team"), CTeecompUtils::RgbToName(g_Config.m_TcColoredTeesTeam2));
			ScoreboardHight = max(ScoreboardHight, RenderScoreboard(Width, 150.0f, TEAM_BLUE, pBlueClanName ? pBlueClanName : Localize(aText), true));	
		}
	}

	RenderGoals(Width, Height);
	RenderSpectators(Width, 150.0f+ScoreboardHight+20.0f);
	RenderRecordingNotification((Width/7)*4);
}

void CScoreboard::SetActiveColumns()
{
	ms_Scoreboard[COLUMN_SCORE_CLAN].m_Active = g_Config.m_TcScoreboardInfos&TC_SCORE_CLAN;
	ms_Scoreboard[COLUMN_SCORE_COUNTRY].m_Active = g_Config.m_TcScoreboardInfos&TC_SCORE_COUNTRY;
	ms_Scoreboard[COLUMN_SCORE_PING].m_Active = g_Config.m_TcScoreboardInfos&TC_SCORE_PING;

	ms_Spectatorboard[COLUMN_SPEC_CLAN].m_Active = g_Config.m_TcScoreboardInfos&TC_SCORE_CLAN;
	ms_Spectatorboard[COLUMN_SPEC_COUNTRY].m_Active = g_Config.m_TcScoreboardInfos&TC_SCORE_COUNTRY;
}

bool CScoreboard::Active()
{
	// if statboard active dont show scoreboard
	if(m_pClient->m_pTeecompStats->IsActive())
		return false;

	// if we activly wanna look on the scoreboard	
	if(m_Active)
		return true;

	if(g_Config.m_ClRenderScoreboard && !g_Config.m_ClClearAll && m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead
		if(!m_pClient->m_Snap.m_pLocalCharacter)
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return true;

	return false;
}

const char *CScoreboard::GetClanName(int Team)
{
	int ClanPlayers = 0;
	const char *pClanName = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = m_pClient->m_aClients[pInfo->m_ClientID].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, pClanName) == 0)
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