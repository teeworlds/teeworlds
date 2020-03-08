/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>
#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>

#include "menus.h"
#include "controls.h"
#include "camera.h"
#include "hud.h"
#include "voting.h"
#include "binds.h"

CHud::CHud()
{
	// won't work if zero
	m_AverageFPS = 1.0f;

	m_WarmupHideTick = 0;
	m_CheckpointTime = 0;
}

void CHud::OnReset()
{
	m_WarmupHideTick = 0;
	m_CheckpointTime = 0;
}

void CHud::RenderGameTimer()
{
	float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;

	if(!(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH))
	{
		int Time = 0;
		if(m_pClient->m_GameInfo.m_TimeLimit && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_WARMUP))
		{
			Time = m_pClient->m_GameInfo.m_TimeLimit*60 - ((Client()->GameTick()-m_pClient->m_Snap.m_pGameData->m_GameStartTick)/Client()->GameTickSpeed());

			if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
				Time = 0;
		}
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
			Time = m_pClient->m_Snap.m_pGameData->m_GameStateEndTick/Client()->GameTickSpeed();
		else
			Time = (Client()->GameTick()-m_pClient->m_Snap.m_pGameData->m_GameStartTick)/Client()->GameTickSpeed();

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d:%02d", Time/60, Time%60);
		float FontSize = 10.0f;
		float w = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
		// last 60 sec red, last 10 sec blink
		if(m_pClient->m_GameInfo.m_TimeLimit && Time <= 60 && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_WARMUP))
		{
			float Alpha = Time <= 10 && (2*time_get()/time_freq()) % 2 ? 0.5f : 1.0f;
			TextRender()->TextColor(1.0f, 0.25f, 0.25f, Alpha);
		}
		TextRender()->Text(0, Half-w/2, 2, FontSize, aBuf, -1.0f);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void CHud::RenderPauseTimer()
{
	if((m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED)) == GAMESTATEFLAG_PAUSED)
	{
		char aBuf[256];
		const char *pText = Localize("Game paused");
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 50, FontSize, pText, -1.0f);

		FontSize = 16.0f;
		if(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0)
		{
			if(m_pClient->m_Snap.m_NotReadyCount == 1)
				str_format(aBuf, sizeof(aBuf), Localize("%d player not ready"), m_pClient->m_Snap.m_NotReadyCount);
			else if(m_pClient->m_Snap.m_NotReadyCount > 1)
				str_format(aBuf, sizeof(aBuf), Localize("%d players not ready"), m_pClient->m_Snap.m_NotReadyCount);
			else
				return;
			RenderReadyUpNotification();
		}
		else
		{
			float Seconds = static_cast<float>(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick-Client()->GameTick())/SERVER_TICK_SPEED;
			if(Seconds < 5)
				str_format(aBuf, sizeof(aBuf), "%.1f", Seconds);
			else
				str_format(aBuf, sizeof(aBuf), "%d", round_to_int(Seconds));
		}
		w = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 75, FontSize, aBuf, -1.0f);
	}
}

void CHud::RenderStartCountdown()
{
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_STARTCOUNTDOWN)
	{
		const char *pText = Localize("Game starts in");
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 50, FontSize, pText, -1.0f);

		if(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0)
			return;

		FontSize = 16.0f;
		char aBuf[32];
		int Seconds = (m_pClient->m_Snap.m_pGameData->m_GameStateEndTick-Client()->GameTick()+SERVER_TICK_SPEED-1)/SERVER_TICK_SPEED;
		str_format(aBuf, sizeof(aBuf), "%d", Seconds);
		w = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 75, FontSize, aBuf, -1.0f);
	}
}

void CHud::RenderDeadNotification()
{
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags == 0 && m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS &&
		m_pClient->m_Snap.m_pLocalInfo && (m_pClient->m_Snap.m_pLocalInfo->m_PlayerFlags&PLAYERFLAG_DEAD))
	{
		const char *pText = Localize("Wait for next round");
		float FontSize = 16.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 50, FontSize, pText, -1.0f);
	}
}

void CHud::RenderSuddenDeath()
{
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH)
	{
		float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
		const char *pText = Localize("Sudden Death");
		float FontSize = 12.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
		TextRender()->Text(0, Half-w/2, 2, FontSize, pText, -1.0f);
	}
}

void CHud::RenderScoreHud()
{
	// render small score hud
	if(!(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER)))
	{
		int GameFlags = m_pClient->m_GameInfo.m_GameFlags;
		float Whole = 300*Graphics()->ScreenAspect();
		float StartY = 229.0f;
		const float TeamOffset = 20.0f;

		if(GameFlags&GAMEFLAG_TEAMS && m_pClient->m_Snap.m_pGameDataTeam && !(GameFlags&GAMEFLAG_RACE))
		{
			char aScoreTeam[2][32];
			str_format(aScoreTeam[TEAM_RED], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed);
			str_format(aScoreTeam[TEAM_BLUE], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue);
			float aScoreTeamWidth[2] = { TextRender()->TextWidth(0, 14.0f, aScoreTeam[TEAM_RED], -1, -1.0f), TextRender()->TextWidth(0, 14.0f, aScoreTeam[TEAM_BLUE], -1, -1.0f) };
			float ScoreWidthMax = max(max(aScoreTeamWidth[TEAM_RED], aScoreTeamWidth[TEAM_BLUE]), TextRender()->TextWidth(0, 14.0f, "100", -1, -1.0f));
			float Split = 3.0f;
			float ImageSize = GameFlags&GAMEFLAG_FLAGS ? 16.0f : Split;

			for(int t = 0; t < NUM_TEAMS; t++)
			{
				// draw box
				CUIRect Rect = {Whole-ScoreWidthMax-ImageSize-2*Split, StartY+t*TeamOffset, ScoreWidthMax+ImageSize+2*Split, 18.0f};
				Graphics()->BlendNormal();
				RenderTools()->DrawUIRect(&Rect, t == 0 ? vec4(1.0f, 0.0f, 0.0f, 0.25f) : vec4(0.0f, 0.0f, 1.0f, 0.25f), CUI::CORNER_L, 5.0f);

				// draw score
				TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreTeamWidth[t])/2-Split, StartY+t*TeamOffset, 14.0f, aScoreTeam[t], -1.0f);

				if(GameFlags&GAMEFLAG_SURVIVAL)
				{
					// draw number of alive players
					char aBuf[32];
					str_format(aBuf, sizeof(aBuf), m_pClient->m_Snap.m_AliveCount[t]==1 ? Localize("%d player left") : Localize("%d players left"),
								m_pClient->m_Snap.m_AliveCount[t]);
					float w = TextRender()->TextWidth(0, 8.0f, aBuf, -1, -1.0f);
					TextRender()->Text(0, min(Whole-w-1.0f, Whole-ScoreWidthMax-ImageSize-2*Split), StartY+(t+1)*TeamOffset-3.0f, 8.0f, aBuf, -1.0f);
				}
				StartY += 8.0f;
			}

			if(GameFlags&GAMEFLAG_FLAGS && m_pClient->m_Snap.m_pGameDataFlag)
			{
				int FlagCarrier[2] = { m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed, m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue };
				int FlagDropTick[2] = { m_pClient->m_Snap.m_pGameDataFlag->m_FlagDropTickRed, m_pClient->m_Snap.m_pGameDataFlag->m_FlagDropTickBlue };
				StartY = 229.0f;

				for(int t = 0; t < 2; t++)
				{
					int BlinkTimer = (FlagDropTick[t] != 0 && (Client()->GameTick()-FlagDropTick[t])/Client()->GameTickSpeed() >= 25) ? 10 : 20;
					if(FlagCarrier[t] == FLAG_ATSTAND || (FlagCarrier[t] == FLAG_TAKEN && ((Client()->GameTick()/BlinkTimer)&1)))
					{
						// draw flag
						Graphics()->BlendNormal();
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
						Graphics()->QuadsBegin();
						RenderTools()->SelectSprite(t==0?SPRITE_FLAG_RED:SPRITE_FLAG_BLUE);
						IGraphics::CQuadItem QuadItem(Whole-ScoreWidthMax-ImageSize, StartY+1.0f+t*TeamOffset, ImageSize/2, ImageSize);
						Graphics()->QuadsDrawTL(&QuadItem, 1);
						Graphics()->QuadsEnd();
					}
					else if(FlagCarrier[t] >= 0)
					{
						// draw name of the flag holder
						int ID = FlagCarrier[t]%MAX_CLIENTS;
						char aName[64];
						str_format(aName, sizeof(aName), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[ID].m_aName : "");
						float w = TextRender()->TextWidth(0, 8.0f, aName, -1, -1.0f) + RenderTools()->GetClientIdRectSize(8.0f);

						CTextCursor Cursor;
						float x = min(Whole-w-1.0f, Whole-ScoreWidthMax-ImageSize-2*Split);
						float y = StartY+(t+1)*TeamOffset-3.0f;
						TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);

						RenderTools()->DrawClientID(TextRender(), &Cursor, ID);
						TextRender()->TextEx(&Cursor, aName, -1);

						// draw tee of the flag holder
						CTeeRenderInfo Info = m_pClient->m_aClients[ID].m_RenderInfo;
						Info.m_Size = 18.0f;
						RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
							vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, StartY+1.0f+Info.m_Size/2+t*TeamOffset));
					}
					StartY += 8.0f;
				}
			}
		}
		else
		{
			int Local = -1;
			int aPos[2] = { 1, 2 };
			CGameClient::CPlayerInfoItem aPlayerInfo[2] = {{0}};
			int i = 0;
			for(int t = 0; t < 2 && i < MAX_CLIENTS && m_pClient->m_Snap.m_aInfoByScore[i].m_pPlayerInfo; ++i)
			{
				if(m_pClient->m_aClients[m_pClient->m_Snap.m_aInfoByScore[i].m_ClientID].m_Team != TEAM_SPECTATORS)
				{
					aPlayerInfo[t] = m_pClient->m_Snap.m_aInfoByScore[i];
					if(aPlayerInfo[t].m_ClientID == m_pClient->m_LocalClientID)
						Local = t;
					++t;
				}
			}
			// search local player info if not a spectator, nor within top2 scores
			if(Local == -1 && m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS)
			{
				for(; i < MAX_CLIENTS && m_pClient->m_Snap.m_aInfoByScore[i].m_pPlayerInfo; ++i)
				{
					if(m_pClient->m_aClients[m_pClient->m_Snap.m_aInfoByScore[i].m_ClientID].m_Team != TEAM_SPECTATORS)
						++aPos[1];
					if(m_pClient->m_Snap.m_aInfoByScore[i].m_ClientID == m_pClient->m_LocalClientID)
					{
						aPlayerInfo[1] = m_pClient->m_Snap.m_aInfoByScore[i];
						Local = 1;
						break;
					}
				}
			}
			char aScore[2][32];
			for(int t = 0; t < 2; ++t)
			{
				if(aPlayerInfo[t].m_pPlayerInfo)
				{
					if(GameFlags&GAMEFLAG_RACE)
						FormatTime(aScore[t], sizeof(aScore[0]), aPlayerInfo[t].m_pPlayerInfo->m_Score, m_pClient->RacePrecision());
					else
						str_format(aScore[t], sizeof(aScore[0]), "%d", aPlayerInfo[t].m_pPlayerInfo->m_Score);
				}
				else
				{
					aScore[t][0] = 0;
				}
			}

			float FontSize = (GameFlags&GAMEFLAG_RACE) ? 10.f : 14.f;
			float aScoreWidth[2] = {TextRender()->TextWidth(0, FontSize, aScore[0], -1, -1.0f), TextRender()->TextWidth(0, FontSize, aScore[1], -1, -1.0f)};
			float ScoreWidthMax = max(max(aScoreWidth[0], aScoreWidth[1]), TextRender()->TextWidth(0, FontSize, "10", -1, -1.0f));
			float Split = 3.0f, ImageSize = 16.0f, PosSize = 16.0f;

			if(GameFlags&GAMEFLAG_SURVIVAL)
			{
				// draw number of alive players
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), m_pClient->m_Snap.m_AliveCount[0] == 1 ? Localize("%d player left") : Localize("%d players left"),
					m_pClient->m_Snap.m_AliveCount[0]);
				float w = TextRender()->TextWidth(0, 8.0f, aBuf, -1, -1.0f);
				TextRender()->Text(0, min(Whole - w - 1.0f, Whole - ScoreWidthMax - ImageSize - 2 * Split), StartY - 12.0f, 8.0f, aBuf, -1.0f);
			}

			for(int t = 0; t < 2; t++)
			{
				// draw box
				CUIRect Rect = {Whole-ScoreWidthMax-ImageSize-2*Split-PosSize, StartY+t*TeamOffset, ScoreWidthMax+ImageSize+2*Split+PosSize, 18.0f};
				Graphics()->BlendNormal();
				RenderTools()->DrawUIRect(&Rect, t == Local ? vec4(1.0f, 1.0f, 1.0f, 0.25f) : vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_L, 5.0f);

				// draw score
				float Spacing = (GameFlags&GAMEFLAG_RACE) ? 2.f : 0.f;
				TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreWidth[t])/2-Split, StartY+t*TeamOffset+Spacing, FontSize, aScore[t], -1.0f);

				if(aPlayerInfo[t].m_pPlayerInfo)
				{
					// draw name
					int ID = aPlayerInfo[t].m_ClientID;
					char aName[64];
					str_format(aName, sizeof(aName), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[ID].m_aName : "");
					float w = TextRender()->TextWidth(0, 8.0f, aName, -1, -1.0f) + RenderTools()->GetClientIdRectSize(8.0f);

					CTextCursor Cursor;
					float x = min(Whole-w-1.0f, Whole-ScoreWidthMax-ImageSize-2*Split-PosSize);
					float y = StartY+(t+1)*TeamOffset-3.0f;
					TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);

					RenderTools()->DrawClientID(TextRender(), &Cursor, ID);
					TextRender()->TextEx(&Cursor, aName, -1);

					// draw tee
					CTeeRenderInfo Info = m_pClient->m_aClients[ID].m_RenderInfo;
					Info.m_Size = 18.0f;
					RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
						vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, StartY+1.0f+Info.m_Size/2+t*TeamOffset));
				}

				// draw position
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), "%d.", aPos[t]);
				TextRender()->Text(0, Whole-ScoreWidthMax-ImageSize-Split-PosSize, StartY+2.0f+t*TeamOffset, 10.0f, aBuf, -1.0f);

				StartY += 8.0f;
			}
		}
	}
}

void CHud::RenderWarmupTimer()
{
	// render warmup timer
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_WARMUP)
	{
		char aBuf[256];
		float FontSize = 20.0f;
		float w = 0.0f;
		const char *pText = Localize("Warmup");

		if(m_WarmupHideTick == 0 || (time_get() - m_WarmupHideTick) / time_freq() < 10)
		{
			w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 50, FontSize, pText, -1.0f);
		}
		else
		{
			TextRender()->TextColor(1, 1, 0.5f, 1);
			TextRender()->Text(0x0, 10, 45, 8, pText, -1.0f);
		}

		FontSize = 16.0f;
		if(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0)
		{
			if(m_pClient->m_Snap.m_NotReadyCount == 1)
			{
				str_format(aBuf, sizeof(aBuf), Localize("%d player not ready"), m_pClient->m_Snap.m_NotReadyCount);
				RenderReadyUpNotification();
			}
			else if(m_pClient->m_Snap.m_NotReadyCount > 1)
			{
				str_format(aBuf, sizeof(aBuf), Localize("%d players not ready"), m_pClient->m_Snap.m_NotReadyCount);
				RenderReadyUpNotification();
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), Localize("wait for more players"));
				if(m_WarmupHideTick == 0)
					m_WarmupHideTick = time_get();
			}
		}
		else
		{
			float Seconds = static_cast<float>(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick-Client()->GameTick())/SERVER_TICK_SPEED;
			if(Seconds < 5)
				str_format(aBuf, sizeof(aBuf), "%.1f", Seconds);
			else
				str_format(aBuf, sizeof(aBuf), "%d", round_to_int(Seconds));
		}

		if(m_WarmupHideTick == 0 || (time_get() - m_WarmupHideTick) / time_freq() < 10)
		{
			w = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect() - w/2, 75, FontSize, aBuf, -1.0f);
		}
		else
		{
			TextRender()->Text(0x0, 10, 54, 6, aBuf, -1.0f);
			TextRender()->TextColor(1, 1, 1, 1);
		}
	}
	else if((m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0 && m_pClient->m_Snap.m_NotReadyCount > 0) || m_pClient->m_Snap.m_pGameData->m_GameStateEndTick != 0)
		m_WarmupHideTick = 0;
}

void CHud::RenderFps()
{
	if(Config()->m_ClShowfps)
	{
		// calculate avg. fps
		float FPS = 1.0f / Client()->RenderFrameTime();
		m_AverageFPS = (m_AverageFPS*(1.0f-(1.0f/m_AverageFPS))) + (FPS*(1.0f/m_AverageFPS));
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", (int)m_AverageFPS);
		TextRender()->Text(0, m_Width-10-TextRender()->TextWidth(0,12,aBuf,-1, -1.0f), 5, 12, aBuf, -1.0f);
	}
}

void CHud::RenderConnectionWarning()
{
	if(Client()->ConnectionProblems())
	{
		const char *pText = Localize("Connection Problems...");
		float w = TextRender()->TextWidth(0, 24, pText, -1, -1.0f);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-w/2, 50, 24, pText, -1.0f);
	}
}

void CHud::RenderTeambalanceWarning()
{
	// render prompt about team-balance
	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS && Config()->m_ClWarningTeambalance && m_pClient->m_ServerSettings.m_TeamBalance &&
		absolute(m_pClient->m_GameInfo.m_aTeamSize[TEAM_RED]-m_pClient->m_GameInfo.m_aTeamSize[TEAM_BLUE]) >= NUM_TEAMS)
	{
		bool Flash = time_get()/(time_freq()/2)%2 == 0;
		const char *pText = Localize("Please balance teams!");
		if(Flash)
			TextRender()->TextColor(1,1,0.5f,1);
		else
			TextRender()->TextColor(0.7f,0.7f,0.2f,1.0f);
		TextRender()->Text(0x0, 5, 50, 6, pText, -1.0f);
		TextRender()->TextColor(1,1,1,1);
	}
}


void CHud::RenderVoting()
{
	if(!m_pClient->m_pVoting->IsVoting() || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	CUIRect Rect = {-10.0f, 58.0f, 119.0f, 46.0f};
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.4f), 5.0f);

	TextRender()->TextColor(1,1,1,1);

	CTextCursor Cursor;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), Localize("%ds left"), m_pClient->m_pVoting->SecondsLeft());
	float tw = TextRender()->TextWidth(0x0, 6, aBuf, -1, -1.0f);
	TextRender()->SetCursor(&Cursor, 5.0f+100.0f-tw, 60.0f, 6.0f, TEXTFLAG_RENDER);
	TextRender()->TextEx(&Cursor, aBuf, -1);

	TextRender()->SetCursor(&Cursor, 5.0f, 60.0f, 6.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = 100.0f-tw;
	Cursor.m_MaxLines = 3;
	TextRender()->TextEx(&Cursor, m_pClient->m_pVoting->VoteDescription(), -1);

	// reason
	str_format(aBuf, sizeof(aBuf), "%s %s", Localize("Reason:"), m_pClient->m_pVoting->VoteReason());
	TextRender()->SetCursor(&Cursor, 5.0f, 79.0f, 6.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = 100.0f;
	TextRender()->TextEx(&Cursor, aBuf, -1);

	CUIRect Base = {5, 88, 100, 4};
	m_pClient->m_pVoting->RenderBars(Base, false);

	char aBufYes[64], aBufNo[64];
	m_pClient->m_pBinds->GetKey("vote yes", aBufYes, sizeof(aBufYes));
	m_pClient->m_pBinds->GetKey("vote no", aBufNo, sizeof(aBufNo));
	str_format(aBuf, sizeof(aBuf), "%s - %s", aBufYes, Localize("Vote yes"));
	Base.y += Base.h+1;
	UI()->DoLabel(&Base, aBuf, 6.0f, CUI::ALIGN_LEFT);

	str_format(aBuf, sizeof(aBuf), "%s - %s", Localize("Vote no"), aBufNo);
	UI()->DoLabel(&Base, aBuf, 6.0f, CUI::ALIGN_RIGHT);
}

void CHud::RenderCursor()
{
	if(!m_pClient->m_Snap.m_pLocalCharacter || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	vec2 Pos = *m_pClient->m_pCamera->GetCenter();
	RenderTools()->MapScreenToGroup(Pos.x, Pos.y, Layers()->GameGroup(), m_pClient->m_pCamera->GetZoom());
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	// render cursor
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteCursor);
	float CursorSize = 64;
	RenderTools()->DrawSprite(m_pClient->m_pControls->m_TargetPos.x, m_pClient->m_pControls->m_TargetPos.y, CursorSize);
	Graphics()->QuadsEnd();
}

void CHud::RenderNinjaBar(float x, float y, float Progress)
{
	Progress = clamp(Progress, 0.0f, 1.0f);
	const float EndWidth = 6.0f;
	const float BarHeight = 12.0f;
	const float WholeBarWidth = 120.f;
	const float MiddleBarWidth = WholeBarWidth - (EndWidth * 2.0f);

	IGraphics::CQuadItem QuadStartFull(x, y, EndWidth, BarHeight);
	RenderTools()->SelectSprite(&g_pData->m_aSprites[SPRITE_NINJA_BAR_FULL_LEFT]);
	Graphics()->QuadsDrawTL(&QuadStartFull, 1);
	x += EndWidth;

	const float FullBarWidth = MiddleBarWidth * Progress;
	const float EmptyBarWidth = MiddleBarWidth - FullBarWidth;

	// full bar
	IGraphics::CQuadItem QuadFull(x, y, FullBarWidth, BarHeight);

	CDataSprite SpriteBarFull = g_pData->m_aSprites[SPRITE_NINJA_BAR_FULL];
	// prevent pixel puree, select only a small slice
	if(Progress < 0.1f)
	{
		int spx = SpriteBarFull.m_X;
		int spy = SpriteBarFull.m_Y;
		float w = SpriteBarFull.m_W * 0.1f; // magic here
		int h = SpriteBarFull.m_H;
		int cx = SpriteBarFull.m_pSet->m_Gridx;
		int cy = SpriteBarFull.m_pSet->m_Gridy;
		float x1 = spx/(float)cx;
		float x2 = (spx+w-1/32.0f)/(float)cx;
		float y1 = spy/(float)cy;
		float y2 = (spy+h-1/32.0f)/(float)cy;

		Graphics()->QuadsSetSubset(x1, y1, x2, y2);
	}
	else
		RenderTools()->SelectSprite(&SpriteBarFull);

	Graphics()->QuadsDrawTL(&QuadFull, 1);

	// empty bar
	// select the middle portion of the sprite so we don't get edge bleeding
	const CDataSprite SpriteBarEmpty = g_pData->m_aSprites[SPRITE_NINJA_BAR_EMPTY];
	{
		float spx = SpriteBarEmpty.m_X + 0.1f;
		float spy = SpriteBarEmpty.m_Y;
		float w = SpriteBarEmpty.m_W * 0.5f;
		int h = SpriteBarEmpty.m_H;
		int cx = SpriteBarEmpty.m_pSet->m_Gridx;
		int cy = SpriteBarEmpty.m_pSet->m_Gridy;
		float x1 = spx/(float)cx;
		float x2 = (spx+w-1/32.0f)/(float)cx;
		float y1 = spy/(float)cy;
		float y2 = (spy+h-1/32.0f)/(float)cy;

		Graphics()->QuadsSetSubset(x1, y1, x2, y2);
	}

	IGraphics::CQuadItem QuadEmpty(x + FullBarWidth, y, EmptyBarWidth, BarHeight);
	Graphics()->QuadsDrawTL(&QuadEmpty, 1);

	x += MiddleBarWidth;

	IGraphics::CQuadItem QuadEndEmpty(x, y, EndWidth, BarHeight);
	RenderTools()->SelectSprite(&g_pData->m_aSprites[SPRITE_NINJA_BAR_EMPTY_RIGHT]);
	Graphics()->QuadsDrawTL(&QuadEndEmpty, 1);
}

void CHud::RenderHealthAndAmmo(const CNetObj_Character *pCharacter)
{
	if(!pCharacter)
		return;

	float x = 5;
	float y = 5;
	int i;
	IGraphics::CQuadItem Array[10];

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->WrapClamp();

	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render ammo
	if(pCharacter->m_Weapon == WEAPON_NINJA)
	{
		const int Max = g_pData->m_Weapons.m_Ninja.m_Duration * Client()->GameTickSpeed() / 1000;
		float NinjaProgress = clamp(pCharacter->m_AmmoCount-Client()->GameTick(), 0, Max) / (float)Max;
		RenderNinjaBar(x, y+24.f, NinjaProgress);
	}
	else
	{
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[pCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteProj);
		if(pCharacter->m_Weapon == WEAPON_GRENADE)
		{
			for(i = 0; i < min(pCharacter->m_AmmoCount, 10); i++)
				Array[i] = IGraphics::CQuadItem(x+1+i*12, y+24, 10, 10);
		}
		else
		{
			for(i = 0; i < min(pCharacter->m_AmmoCount, 10); i++)
				Array[i] = IGraphics::CQuadItem(x+i*12, y+24, 12, 12);
		}
		Graphics()->QuadsDrawTL(Array, i);
	}

	int h = 0;

	// render health
	RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
	for(; h < min(pCharacter->m_Health, 10); h++)
		Array[h] = IGraphics::CQuadItem(x+h*12,y,12,12);
	Graphics()->QuadsDrawTL(Array, h);

	i = 0;
	RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
	for(; h < 10; h++)
		Array[i++] = IGraphics::CQuadItem(x+h*12,y,12,12);
	Graphics()->QuadsDrawTL(Array, i);

	// render armor meter
	h = 0;
	RenderTools()->SelectSprite(SPRITE_ARMOR_FULL);
	for(; h < min(pCharacter->m_Armor, 10); h++)
		Array[h] = IGraphics::CQuadItem(x+h*12,y+12,12,12);
	Graphics()->QuadsDrawTL(Array, h);

	i = 0;
	RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
	for(; h < 10; h++)
		Array[i++] = IGraphics::CQuadItem(x+h*12,y+12,12,12);
	Graphics()->QuadsDrawTL(Array, i);
	Graphics()->QuadsEnd();
	Graphics()->WrapNormal();
}

void CHud::RenderSpectatorHud()
{
	// draw the box
	const float Width = m_Width * 0.25f - 2.0f;
	CUIRect Rect = {m_Width-Width, m_Height-15.0f, Width, 15.0f};
	RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.4f), CUI::CORNER_TL, 5.0f);

	// draw the text
	char aName[64];
	const int SpecID = m_pClient->m_Snap.m_SpecInfo.m_SpectatorID;
	const int SpecMode = m_pClient->m_Snap.m_SpecInfo.m_SpecMode;
	str_format(aName, sizeof(aName), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_aName : "");
	char aBuf[128];

	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, m_Width-Width+6.0f, m_Height-13.0f, 8.0f, TEXTFLAG_RENDER);

	str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Spectate"));
	TextRender()->TextEx(&Cursor, aBuf, -1);

	switch(SpecMode)
	{
	case SPEC_FREEVIEW:
		str_format(aBuf, sizeof(aBuf), "%s", Localize("Free-View"));
		break;
	case SPEC_PLAYER:
		str_format(aBuf, sizeof(aBuf), "%s", aName);
		break;
	case SPEC_FLAGRED:
	case SPEC_FLAGBLUE:
		char aFlag[64];
		str_format(aFlag, sizeof(aFlag), SpecMode == SPEC_FLAGRED ? Localize("red flag") : Localize("blue flag"));

		if(SpecID != -1)
			str_format(aBuf, sizeof(aBuf), "%s (%s)", aFlag, aName);
		else
			str_format(aBuf, sizeof(aBuf), "%s", aFlag);
		break;
	}

	if(SpecMode == SPEC_PLAYER || SpecID != -1)
		RenderTools()->DrawClientID(TextRender(), &Cursor, SpecID);

	TextRender()->TextEx(&Cursor, aBuf, -1);
}

void CHud::RenderSpectatorNotification()
{
	if(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS &&
		m_pClient->m_TeamChangeTime + 5.0f >= Client()->LocalTime())
	{
		// count non spectators
		int NumPlayers = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
				NumPlayers++;

		if(NumPlayers > 0)
		{
			const char *pText = Localize("Click on a player or a flag to follow it");
			float FontSize = 16.0f;
			float w = TextRender()->TextWidth(0, FontSize, pText, -1, -1.0f);
			TextRender()->Text(0, 150 * Graphics()->ScreenAspect() - w / 2, 30, FontSize, pText, -1.0f);
		}
	}
}

void CHud::RenderReadyUpNotification()
{
	if(!(m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_LocalClientID]->m_PlayerFlags&PLAYERFLAG_READY))
	{
		char aKey[64], aText[128];
		m_pClient->m_pBinds->GetKey("ready_change", aKey, sizeof(aKey));
		str_format(aText, sizeof(aText), Localize("When ready, press <%s>"), aKey);
		float FontSize = 16.0f;
		float w = TextRender()->TextWidth(0, FontSize, aText, -1, -1.0f);
		TextRender()->Text(0, 150 * Graphics()->ScreenAspect() - w / 2, 30, FontSize, aText, -1.0f);
	}
}

void CHud::RenderRaceTime(const CNetObj_PlayerInfoRace *pRaceInfo)
{
	if(!pRaceInfo || pRaceInfo->m_RaceStartTick == -1)
		return;

	char aBuf[32];
	int RaceTime = (Client()->GameTick() - pRaceInfo->m_RaceStartTick)*1000/Client()->GameTickSpeed();
	FormatTime(aBuf, sizeof(aBuf), RaceTime, min(m_pClient->RacePrecision(), 1));

	float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
	float w = TextRender()->TextWidth(0, 12, "00:00.0", -1, -1.0f);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_TIMERCLOCK].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_TIMERCLOCK_A);
	IGraphics::CQuadItem QuadItem(Half-w/2-18.f, 20, 15, 15);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	TextRender()->Text(0, Half-w/2, 20, 12, aBuf, -1);
}

void CHud::RenderCheckpoint()
{
	int64 TimeSinceCheckpoint = time_get() - m_CheckpointTime;
	if(m_CheckpointTime != 0 && TimeSinceCheckpoint < time_freq() * 4)
	{
		char aBuf[64];
		FormatTimeDiff(aBuf, sizeof(aBuf), m_CheckpointDiff, 2);

		// calculate alpha (2 sec 1 than get lower the next 2 sec)
		float a = 1.0f;
		if(TimeSinceCheckpoint > time_freq() * 2)
		{
			// lower the alpha slowly to blend text out
			a = (time_freq() * 4 - TimeSinceCheckpoint) / (float)(time_freq() * 2);
		}

		if(m_CheckpointDiff > 0)
			TextRender()->TextColor(1.0f,0.5f,0.5f,a); // red
		else if(m_CheckpointDiff < 0)
			TextRender()->TextColor(0.5f,1.0f,0.5f,a); // green
		else if(!m_CheckpointDiff)
			TextRender()->TextColor(1,1,1,a); // white

		float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
		float w = TextRender()->TextWidth(0, 10, aBuf, -1, -1.0f);
		TextRender()->Text(0, Half-w/2, 33, 10, aBuf, -1);

		TextRender()->TextColor(1,1,1,1);
	}
}

void CHud::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHECKPOINT)
	{
		CNetMsg_Sv_Checkpoint *pMsg = (CNetMsg_Sv_Checkpoint *)pRawMsg;
		m_CheckpointDiff = pMsg->m_Diff;
		m_CheckpointTime = time_get();
	}
	else if(MsgType == NETMSGTYPE_SV_KILLMSG && (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE))
	{
		// reset checkpoint time on death
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_LocalClientID)
			m_CheckpointTime = 0;
	}
}

void CHud::OnRender()
{
	if(!m_pClient->m_Snap.m_pGameData)
		return;

	// dont render hud if the menu is active
	if(m_pClient->m_pMenus->IsActive())
		return;

	bool Race = m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE;

	m_Width = 300.0f*Graphics()->ScreenAspect();
	m_Height = 300.0f;
	Graphics()->MapScreen(0.0f, 0.0f, m_Width, m_Height);

	if(Config()->m_ClShowhud)
	{
		if(m_pClient->m_Snap.m_pLocalCharacter && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER)))
		{
			RenderHealthAndAmmo(m_pClient->m_Snap.m_pLocalCharacter);
			if(Race)
			{
				RenderRaceTime(m_pClient->m_Snap.m_paPlayerInfosRace[m_pClient->m_LocalClientID]);
				RenderCheckpoint();
			}
		}
		else if(m_pClient->m_Snap.m_SpecInfo.m_Active)
		{
			if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != -1)
			{
				RenderHealthAndAmmo(&m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Cur);
				if(Race)
				{
					RenderRaceTime(m_pClient->m_Snap.m_paPlayerInfosRace[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]);
					RenderCheckpoint();
				}
			}
			RenderSpectatorHud();
			RenderSpectatorNotification();
		}

		RenderGameTimer();
		RenderPauseTimer();
		RenderStartCountdown();
		RenderDeadNotification();
		RenderSuddenDeath();
		RenderScoreHud();
		RenderWarmupTimer();
		RenderFps();
		if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
			RenderConnectionWarning();
		RenderTeambalanceWarning();
		RenderVoting();
	}
	RenderCursor();
}
