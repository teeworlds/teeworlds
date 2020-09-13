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
#include "motd.h"
#include "scoreboard.h"
#include "stats.h"

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

bool CHud::IsLargeWarmupTimerShown()
{
	return !m_pClient->m_pMotd->IsActive()
		&& !m_pClient->m_pScoreboard->IsActive()
		&& !m_pClient->m_pStats->IsActive()
		&& (m_WarmupHideTick == 0 || (time_get() - m_WarmupHideTick) / time_freq() < 10); // inactivity based
}

void CHud::RenderGameTimer()
{
	float x = 300.0f*Graphics()->ScreenAspect()/2.0f;
	float FontSize = 10.0f;

	int Time = 0;
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH)
		Time = 0;
	else if(m_pClient->m_GameInfo.m_TimeLimit && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_WARMUP))
	{
		Time = m_pClient->m_GameInfo.m_TimeLimit*60 - ((Client()->GameTick()-m_pClient->m_Snap.m_pGameData->m_GameStartTick)/Client()->GameTickSpeed());

		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
			Time = 0;
	}
	else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
		Time = m_pClient->m_Snap.m_pGameData->m_GameStateEndTick/Client()->GameTickSpeed();
	else
		Time = (Client()->GameTick()-m_pClient->m_Snap.m_pGameData->m_GameStartTick)/Client()->GameTickSpeed();

	// TODO: ADDBACK: render timer
}

void CHud::RenderPauseTimer()
{
	// TODO: ADDBACK: render timer
}

void CHud::RenderStartCountdown()
{
	// TODO: ADDBACK: render countdown
}

void CHud::RenderNetworkIssueNotification()
{
	float x = 300.0f*Graphics()->ScreenAspect()/2.0f - 15.0f;
	float FontSize = 10.0f;

	// render network indicator when 2/5 and under
	const int NetScore = Client()->GetInputtimeMarginStabilityScore();
	if(NetScore > 250)
	{
		float Margin = 2.0f;
		x = x-5.0f-FontSize-Margin*2;
		Graphics()->BlendNormal();
		CUIRect RectBox = {x, 4-Margin, FontSize+2*Margin, FontSize+2*Margin};
		vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.3f);
		RenderTools()->DrawUIRect(&RectBox, Color, CUI::CORNER_ALL, 1.0f);
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_NETWORKICONS].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_NETWORK_BAD);
		IGraphics::CQuadItem QuadItem(x+Margin, 4, FontSize, FontSize);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CHud::RenderDeadNotification()
{
	// TODO: ADDBACK: render dead notification
}

void CHud::RenderScoreHud()
{
	// TODO: ADDBACK: render score hud
}

void CHud::RenderWarmupTimer()
{
	static CTextCursor s_Cursor;
	s_Cursor.m_MaxLines = 2;

	// render warmup timer
	if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_WARMUP)
	{
		char aBuf[256];
		const char *pText = Localize("Warmup");
		const bool LargeTimer = IsLargeWarmupTimerShown();

		if(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick == 0)
		{
			const int CursorVersion = g_Localization.Version() << 9 | m_pClient->m_Snap.m_NotReadyCount << 1 | LargeTimer;
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
			s_Cursor.Reset(CursorVersion);
		}
		else
		{
			float Seconds = static_cast<float>(m_pClient->m_Snap.m_pGameData->m_GameStateEndTick-Client()->GameTick())/SERVER_TICK_SPEED;
			if(Seconds < 5)
				str_format(aBuf, sizeof(aBuf), "%.1f", Seconds);
			else
				str_format(aBuf, sizeof(aBuf), "%d", round_to_int(Seconds));
			s_Cursor.Reset();
		}

		if(LargeTimer)
		{
			s_Cursor.MoveTo(150 * Graphics()->ScreenAspect(), 50);
			s_Cursor.m_Align = TEXTALIGN_TC;
			s_Cursor.m_LineSpacing = 5.0f;

			s_Cursor.m_FontSize = 20.0f;
			TextRender()->TextDeferred(&s_Cursor, pText, -1);
			s_Cursor.m_FontSize = 16.0f;
			TextRender()->TextNewline(&s_Cursor);
			TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
			TextRender()->DrawTextOutlined(&s_Cursor);
		}
		else
		{
			s_Cursor.MoveTo(10, 45);
			s_Cursor.m_Align = TEXTALIGN_TL;
			s_Cursor.m_LineSpacing = 1.0f;
			
			TextRender()->TextColor(1, 1, 0.5f, 1);
			s_Cursor.m_FontSize = 8.0f;
			TextRender()->TextDeferred(&s_Cursor, pText, -1);
			s_Cursor.m_FontSize = 6.0f;
			TextRender()->TextNewline(&s_Cursor);
			TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
			TextRender()->DrawTextOutlined(&s_Cursor);
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
		static CTextCursor s_Cursor(12);
		s_Cursor.MoveTo(m_Width-34, 5);

		// calculate avg. fps
		float FPS = 1.0f / Client()->RenderFrameTime();
		m_AverageFPS = (m_AverageFPS*(1.0f-(1.0f/m_AverageFPS))) + (FPS*(1.0f/m_AverageFPS));
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", (int)m_AverageFPS);

		s_Cursor.Reset();
		TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
	}
}

void CHud::RenderConnectionWarning()
{
	// TODO: ADDBACK: render connection warning
}

void CHud::RenderTeambalanceWarning()
{
	// TODO: ADDBACK: render team balancing warning
}

void CHud::RenderVoting()
{
	// TODO: ADDBACK: render voting
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

	static CTextCursor s_SpectateLabelCursor(8.0f);
	s_SpectateLabelCursor.MoveTo(m_Width-Width+6.0f, m_Height-13.0f);
	s_SpectateLabelCursor.Reset(g_Localization.Version());
	str_format(aBuf, sizeof(aBuf), "%s: ", Localize("Spectate"));
	TextRender()->TextOutlined(&s_SpectateLabelCursor, aBuf, -1);

	static CTextCursor s_SpectateTargetCursor(8.0f);
	s_SpectateTargetCursor.MoveTo(s_SpectateLabelCursor.BoundingBox().m_Max.x+3.0f, m_Height-13.0f);
	s_SpectateTargetCursor.Reset();

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
		str_format(aFlag, sizeof(aFlag), SpecMode == SPEC_FLAGRED ? Localize("Red Flag") : Localize("Blue Flag"));

		if(SpecID != -1)
			str_format(aBuf, sizeof(aBuf), "%s (%s)", aFlag, aName);
		else
			str_format(aBuf, sizeof(aBuf), "%s", aFlag);
		break;
	}

	if(SpecMode == SPEC_PLAYER || SpecID != -1)
		RenderTools()->DrawClientID(TextRender(), &s_SpectateTargetCursor, SpecID);

	TextRender()->TextOutlined(&s_SpectateTargetCursor, aBuf, -1);
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
			// TODO: ADDBACK: draw following notification
		}
	}
}

void CHud::RenderReadyUpNotification()
{
	if(!(m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_LocalClientID]->m_PlayerFlags&PLAYERFLAG_READY))
	{
		// TODO: ADDBACK: draw ready notification
	}
}

void CHud::RenderRaceTime(const CNetObj_PlayerInfoRace *pRaceInfo)
{
	if(!pRaceInfo || pRaceInfo->m_RaceStartTick == -1)
		return;

	// TODO: ADDBACK: draw race time
}

void CHud::RenderCheckpoint()
{
	// TODO: ADDBACK: draw checkpoint
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
		RenderNetworkIssueNotification();
		RenderDeadNotification();
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
