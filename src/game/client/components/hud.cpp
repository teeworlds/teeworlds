#include <stdio.h>

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/demo.h>
#include <engine/shared/config.h>
#include <engine/serverbrowser.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/teecomp.h>

#include "statboard.h"
#include "controls.h"
#include "camera.h"
#include "hud.h"
#include "voting.h"
#include "binds.h"
#include "menus.h"
#include "race_demo.h"
#include "skins.h"

CHud::CHud()
{
	// won't work if zero
	m_AverageFPS = 1.0f;
	
	OnReset();
}
	
void CHud::OnReset()
{
	// race
	m_CheckpointTick = 0;
	m_CheckpointDiff = 0;
	m_RaceTime = 0;
	m_Record = 0;
	m_LocalRecord = -1.0f;
	m_LastReceivedTimeTick = -1;
	
	// lvlx
	m_GotRequest = false;
	m_Cid = -1;
	m_Level = 0;
	m_Weapon = 0;
	m_Str = 0;
	m_Sta = 0;
	m_Dex = 0;
	m_Int = 0;
	
	m_LastAnswer = -1;
	
	m_SpreeId = -1;
	m_EndedBy = -1;
	m_SpreeNum = 0;
	m_SpreeInitTick = -1;
}

void CHud::RenderGameTimer()
{
	if(!g_Config.m_ClRenderTime || g_Config.m_ClClearHud || g_Config.m_ClClearAll)
		return;
		
	float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
	Graphics()->MapScreen(0, 0, 300.0f*Graphics()->ScreenAspect(), 300.0f);
	
	if(!m_pClient->m_Snap.m_pGameobj->m_SuddenDeath)
	{
		char Buf[32];
		int Time = 0;
		if(m_pClient->m_Snap.m_pGameobj->m_TimeLimit)
		{
			Time = m_pClient->m_Snap.m_pGameobj->m_TimeLimit*60 - ((Client()->GameTick()-m_pClient->m_Snap.m_pGameobj->m_RoundStartTick)/Client()->GameTickSpeed());

			if(m_pClient->m_Snap.m_pGameobj->m_GameOver)
				Time  = 0;
		}
		else
			Time = (Client()->GameTick()-m_pClient->m_Snap.m_pGameobj->m_RoundStartTick)/Client()->GameTickSpeed();

		str_format(Buf, sizeof(Buf), "%d:%02d", Time/60, Time%60);
		float FontSize = 10.0f;
		float w = TextRender()->TextWidth(0, FontSize, Buf, -1);
		TextRender()->Text(0, Half-w/2, 2, FontSize, Buf, -1);
	}
}

void CHud::RenderSuddenDeath()
{
	if(!g_Config.m_ClRenderTime || g_Config.m_ClClearHud || g_Config.m_ClClearAll)
		return;
	
	if(m_pClient->m_Snap.m_pGameobj->m_SuddenDeath)
	{
		float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
		const char *pText = Localize("Sudden Death");
		float FontSize = 12.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1);
		TextRender()->Text(0, Half-w/2, 2, FontSize, pText, -1);
	}
}

void CHud::RenderScoreHud()
{
	if(!g_Config.m_ClRenderScore || g_Config.m_ClClearHud || g_Config.m_ClClearAll)
		return;
		
	int GameFlags = m_pClient->m_Snap.m_pGameobj->m_Flags;
	float Whole = 300*Graphics()->ScreenAspect();
	
	// render small score hud
	if(!m_pClient->m_IsRace && !(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver) && (GameFlags&GAMEFLAG_TEAMS))
	{
		char aScoreTeam[2][32];
		str_format(aScoreTeam[0], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed);
		str_format(aScoreTeam[1], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue);
		float aScoreTeamWidth[2] = {TextRender()->TextWidth(0, 14.0f, aScoreTeam[0], -1), TextRender()->TextWidth(0, 14.0f, aScoreTeam[1], -1)};
		float ScoreWidthMax = max(max(aScoreTeamWidth[0], aScoreTeamWidth[1]), TextRender()->TextWidth(0, 14.0f, "100", -1));
		float Split = 3.0f;
		float ImageSize = GameFlags&GAMEFLAG_FLAGS ? 16.0f : Split;
		
		for(int t = 0; t < 2; t++)
		{
			// draw box
			Graphics()->BlendNormal();
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			if(t == 0)
				Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.25f);
			
			if(!g_Config.m_TcHudMatch)
			{
				if(t == 0)
					Graphics()->SetColor(1,0,0,0.25f);
				else
					Graphics()->SetColor(0,0,1,0.25f);
			}
			else
			{
				vec3 Col = CTeecompUtils::GetTeamColor(t, m_pClient->m_Snap.m_pLocalInfo->m_Team,
				g_Config.m_TcColoredTeesTeam1, g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
				Graphics()->SetColor(Col.r, Col.g, Col.b, 0.25f);
			}
			RenderTools()->DrawRoundRectExt(Whole-ScoreWidthMax-ImageSize-2*Split, 245.0f+t*20, ScoreWidthMax+ImageSize+2*Split, 18.0f, 5.0f, CUI::CORNER_L);
			Graphics()->QuadsEnd();

			// draw score
			TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreTeamWidth[t])/2-Split, 245.0f+t*20, 14.0f, aScoreTeam[t], -1);

			if(GameFlags&GAMEFLAG_FLAGS && m_pClient->m_Snap.m_paFlags[t])
			{
				if(m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy == -2 || (m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy == -1 && ((Client()->GameTick()/10)&1)))
				{
					// draw flag
					Graphics()->BlendNormal();
					if(g_Config.m_TcColoredFlags)
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME_GRAY].m_Id);
					else
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
					Graphics()->QuadsBegin();
					RenderTools()->SelectSprite(t==0?SPRITE_FLAG_RED:SPRITE_FLAG_BLUE);
					
					if(g_Config.m_TcColoredFlags)
					{
						vec3 Col = CTeecompUtils::GetTeamColor(t,
							m_pClient->m_Snap.m_pLocalInfo->m_Team,
							g_Config.m_TcColoredTeesTeam1,
							g_Config.m_TcColoredTeesTeam2,
							g_Config.m_TcColoredTeesMethod);
						Graphics()->SetColor(Col.r, Col.g, Col.b, 1.0f);
					}
						
					IGraphics::CQuadItem QuadItem(Whole-ScoreWidthMax-ImageSize, 246.0f+t*20, ImageSize/2, ImageSize);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
				else if(m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy >= 0)
				{
					// draw name of the flag holder
					int Id = m_pClient->m_Snap.m_paFlags[t]->m_CarriedBy%MAX_CLIENTS;
					const char *pName = m_pClient->m_aClients[Id].m_aName;
					float w = TextRender()->TextWidth(0, 10.0f, pName, -1);
					TextRender()->Text(0, Whole-ScoreWidthMax-ImageSize-3*Split-w, 247.0f+t*20, 10.0f, pName, -1);

					// draw tee of the flag holder
					CTeeRenderInfo Info = m_pClient->m_aClients[Id].m_RenderInfo;
					Info.m_Size = 18.0f;
					RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
						vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, 246.0f+Info.m_Size/2+t*20));
				}
			}
		}
	}
}

void CHud::RenderWarmupTimer()
{
	if(!g_Config.m_ClRenderWarmup || g_Config.m_ClClearAll)
		return;
	
	// render warmup timer
	if(m_pClient->m_Snap.m_pGameobj->m_Warmup)
	{
		char Buf[256];
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize, Localize("Warmup"), -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 50, FontSize, Localize("Warmup"), -1);

		int Seconds = m_pClient->m_Snap.m_pGameobj->m_Warmup/SERVER_TICK_SPEED;
		if(Seconds < 5)
			str_format(Buf, sizeof(Buf), "%d.%d", Seconds, (m_pClient->m_Snap.m_pGameobj->m_Warmup*10/SERVER_TICK_SPEED)%10);
		else
			str_format(Buf, sizeof(Buf), "%d", Seconds);
		w = TextRender()->TextWidth(0, FontSize, Buf, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 75, FontSize, Buf, -1);
	}	
}

void CHud::MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup)
{
	float Points[4];
	RenderTools()->MapscreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), m_pClient->m_pCamera->m_Zoom, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

void CHud::RenderFps()
{
	if(g_Config.m_ClShowfps)
	{
		// calculate avg. fps
		float FPS = 1.0f / Client()->FrameTime();
		m_AverageFPS = (m_AverageFPS*(1.0f-(1.0f/m_AverageFPS))) + (FPS*(1.0f/m_AverageFPS));
		char Buf[512];
		str_format(Buf, sizeof(Buf), "%d", (int)m_AverageFPS);
		TextRender()->Text(0, m_Width-10-TextRender()->TextWidth(0,12,Buf,-1), 5, 12, Buf, -1);
	}
}

void CHud::RenderConnectionWarning()
{
	if(Client()->ConnectionProblems())
	{
		const char *pText = Localize("Connection Problems...");
		float w = TextRender()->TextWidth(0, 24, pText, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-w/2, 50, 24, pText, -1);
	}
}

void CHud::RenderTeambalanceWarning()
{
	if(!g_Config.m_ClWarningTeambalance || g_Config.m_ClClearAll)
		return;
	
	// render prompt about team-balance
	bool Flash = time_get()/(time_freq()/2)%2 == 0;
	if (m_pClient->m_Snap.m_pGameobj && (m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS) != 0)
	{	
		int TeamDiff = m_pClient->m_Snap.m_aTeamSize[0]-m_pClient->m_Snap.m_aTeamSize[1];
		if (g_Config.m_ClWarningTeambalance && (TeamDiff >= 2 || TeamDiff <= -2))
		{
			const char *pText = Localize("Please balance teams!");
			if(Flash)
				TextRender()->TextColor(1,1,0.5f,1);
			else
				TextRender()->TextColor(0.7f,0.7f,0.2f,1.0f);
			TextRender()->Text(0x0, 5, 50, 6, pText, -1);
			TextRender()->TextColor(1,1,1,1);
		}
	}
}


void CHud::RenderVoting()
{
	if(!m_pClient->m_pVoting->IsVoting())
		return;
	
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.40f);
	RenderTools()->DrawRoundRect(-10, 60-2, 100+10+4+5, 28, 5.0f);
	Graphics()->QuadsEnd();

	TextRender()->TextColor(1,1,1,1);

	char Buf[512];
	TextRender()->Text(0x0, 5, 60, 6, m_pClient->m_pVoting->VoteDescription(), -1);

	str_format(Buf, sizeof(Buf), Localize("%ds left"), m_pClient->m_pVoting->SecondsLeft());
	float tw = TextRender()->TextWidth(0x0, 6, Buf, -1);
	TextRender()->Text(0x0, 5+100-tw, 60, 6, Buf, -1);
	

	CUIRect Base = {5, 70, 100, 4};
	m_pClient->m_pVoting->RenderBars(Base, false);
	
	const char *pYesKey = m_pClient->m_pBinds->GetKey("vote yes");
	const char *pNoKey = m_pClient->m_pBinds->GetKey("vote no");
	str_format(Buf, sizeof(Buf), "%s - Vote Yes", pYesKey);
	Base.y += Base.h+1;
	UI()->DoLabel(&Base, Buf, 6.0f, -1);

	str_format(Buf, sizeof(Buf), "Vote No - %s", pNoKey);
	UI()->DoLabel(&Base, Buf, 6.0f, 1);
}

void CHud::RenderCursor()
{
	if(!m_pClient->m_Snap.m_pLocalCharacter || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;
		
	MapscreenToGroup(m_pClient->m_pCamera->m_Center.x, m_pClient->m_pCamera->m_Center.y, Layers()->GameGroup());
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	// render cursor
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteCursor);
	float CursorSize = 64;
	RenderTools()->DrawSprite(m_pClient->m_pControls->m_TargetPos.x, m_pClient->m_pControls->m_TargetPos.y, CursorSize);
	Graphics()->QuadsEnd();
}

void CHud::RenderHealthAndAmmo()
{
	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->MapScreen(0,0,m_Width,300);
	
	Graphics()->QuadsBegin();
	
	IGraphics::CQuadItem Array[10];
	if(g_Config.m_ClRenderAmmo)
	{
		// if weaponstage is active, put a "glow" around the stage ammo
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteProj);
		int i;
		for (i = 0; i < min(m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount, 10); i++)
			Array[i] = IGraphics::CQuadItem(x+i*12,y+24,10,10);
		Graphics()->QuadsDrawTL(Array, i);
	}
	Graphics()->QuadsEnd();

	Graphics()->QuadsBegin();
	int h = 0;

	// render health
	if(g_Config.m_ClRenderHp)
	{
		RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
		for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Health, 10); h++)
			Array[h] = IGraphics::CQuadItem(x+h*12,y,10,10);
		Graphics()->QuadsDrawTL(Array, h);

		int i;
		RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
		for(i = 0; h < 10; h++)
			Array[i++] = IGraphics::CQuadItem(x+h*12,y,10,10);
		Graphics()->QuadsDrawTL(Array, i);

		// render armor meter
		h = 0;
		RenderTools()->SelectSprite(SPRITE_ARMOR_FULL);
		for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Armor, 10); h++)
			Array[h] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
		Graphics()->QuadsDrawTL(Array, h);

		i = 0;
		RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
		for(; h < 10; h++)
			Array[i++] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
		Graphics()->QuadsDrawTL(Array, i);
	}
	Graphics()->QuadsEnd();
}

void CHud::RenderSpeedmeter()
{
	if(!g_Config.m_TcSpeedmeter)
		return;
		
	// We calculate the speed instead of getting it from character.velocity cause it's buggy when
	// walking in front of a wall or when using the ninja sword
	static float Speed;
	static vec2 OldPos;
	static const int SMOOTH_TABLE_SIZE = 16;
	static const int ACCEL_THRESHOLD = 25;
	static float SmoothTable[SMOOTH_TABLE_SIZE];
	static int SmoothIndex = 0;

	SmoothTable[SmoothIndex] = distance(m_pClient->m_LocalCharacterPos, OldPos)/Client()->FrameTime();
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		float Mult = DemoPlayer()->BaseInfo()->m_Speed;
		SmoothTable[SmoothIndex] /= Mult;
	}
	SmoothIndex = (SmoothIndex + 1) % SMOOTH_TABLE_SIZE;
	OldPos = m_pClient->m_LocalCharacterPos;
	Speed = 0;
	for(int i = 0; i < SMOOTH_TABLE_SIZE; i++)
		Speed += SmoothTable[i];
	Speed /= SMOOTH_TABLE_SIZE;

	int t = (m_pClient->m_Snap.m_pGameobj->m_Flags & GAMEFLAG_TEAMS)? -1 : 1;
	int LastIndex = SmoothIndex - 1;
	if(LastIndex < 0)
		LastIndex = SMOOTH_TABLE_SIZE - 1;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	if(g_Config.m_TcSpeedmeterAccel && Speed - SmoothTable[LastIndex] > ACCEL_THRESHOLD)
		Graphics()->SetColor(0.6f, 0.1f, 0.1f, 0.25f);
	else if(g_Config.m_TcSpeedmeterAccel && Speed - SmoothTable[LastIndex] < -ACCEL_THRESHOLD)
		Graphics()->SetColor(0.1f, 0.6f, 0.1f, 0.25f);
	else
		Graphics()->SetColor(0.1, 0.1, 0.1, 0.25);
	RenderTools()->DrawRoundRect(m_Width-45, 245+t*20, 55, 18, 5.0f);
 	Graphics()->QuadsEnd();

	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%.0f", Speed/10);
	TextRender()->Text(0, m_Width-5-TextRender()->TextWidth(0,12,aBuf,-1), 246+t*20, 12, aBuf, -1);
}

void CHud::RenderSpectate()
{
	if(m_pClient->m_Freeview)
		TextRender()->Text(0, 4*Graphics()->ScreenAspect(), 4, 8, "Freeview", -1);
	else
	{
		char aBuf[96];
		str_format(aBuf, sizeof(aBuf), "Following: %s", m_pClient->m_aClients[m_pClient->m_SpectateCid].m_aName);
		TextRender()->Text(0, 4*Graphics()->ScreenAspect(), 4, 8, aBuf, -1);
	}
}

void CHud::RenderTime()
{
	if(!m_pClient->m_IsRace)
		return;
	
	// check racestate
	if(m_pClient->m_pRaceDemo->GetRaceState() != CRaceDemo::RACE_FINISHED && m_LastReceivedTimeTick+Client()->GameTickSpeed()*2 < Client()->GameTick())
	{
		m_pClient->m_pRaceDemo->m_RaceState = CRaceDemo::RACE_NONE;
		return;
	}
		
	if(m_RaceTime)
	{
		if(!m_FinishTime && m_pClient->m_pRaceDemo->GetRaceState() == CRaceDemo::RACE_FINISHED)
			m_FinishTime = m_pClient->m_pRaceDemo->GetFinishTime();

		char aBuf[64];
		if(m_FinishTime)
		{
			str_format(aBuf, sizeof(aBuf), "Finish time: %02d:%05.2f", (int)m_FinishTime/60, m_FinishTime-((int)m_FinishTime/60*60));
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0,12,aBuf,-1)/2, 20, 12, aBuf, -1);
		}
		else if(m_pClient->m_pRaceDemo->GetRaceState() == CRaceDemo::RACE_STARTED)
		{
			str_format(aBuf, sizeof(aBuf), "%02d:%02d.%d", m_RaceTime/60, m_RaceTime%60, m_RaceTick/10);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0,12,"00:00.00",-1)/2, 20, 12, aBuf, -1); // use fixed value for text width so its not shaky
		}
	
		if(g_Config.m_ClShowCheckpointDiff && m_CheckpointTick+Client()->GameTickSpeed()*6 > Client()->GameTick())
		{
			str_format(aBuf, sizeof(aBuf), "%+5.2f", m_CheckpointDiff);
			
			// calculate alpha (4 sec 1 than get lower the next 2 sec)
			float a = 1.0f;
			if(m_CheckpointTick+Client()->GameTickSpeed()*4 < Client()->GameTick() && m_CheckpointTick+Client()->GameTickSpeed()*6 > Client()->GameTick())
			{
				// lower the alpha slowly to blend text out
				a = ((float)(m_CheckpointTick+Client()->GameTickSpeed()*6) - (float)Client()->GameTick()) / (float)(Client()->GameTickSpeed()*2);
			}
			
			if(m_CheckpointDiff > 0)
				TextRender()->TextColor(1.0f,0.5f,0.5f,a); // red
			else if(m_CheckpointDiff < 0)
				TextRender()->TextColor(0.5f,1.0f,0.5f,a); // green
			else if(!m_CheckpointDiff)
				TextRender()->TextColor(1,1,1,a); // white
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0, 10, aBuf, -1)/2, 33, 10, aBuf, -1);
			
			TextRender()->TextColor(1,1,1,1);
		}
	}
	m_RaceTick+=100/Client()->GameTickSpeed();
	
	if(m_RaceTick >= 100)
		m_RaceTick = 0;
}

void CHud::RenderRecord()
{
	if(!m_pClient->m_IsRace)
		return;
		
	if(m_Record && g_Config.m_ClShowServerRecord && g_Config.m_ClShowRecords)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Server best:");
		TextRender()->Text(0, 5, 40, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_Record/60, m_Record-((int)m_Record/60*60));
		TextRender()->Text(0, 53, 40, 6, aBuf, -1);
	}
		
	if(m_pClient->m_pRaceDemo->GetFinishTime() && (m_LocalRecord == -1 || m_LocalRecord > m_pClient->m_pRaceDemo->GetFinishTime()))
		m_LocalRecord = m_pClient->m_pRaceDemo->GetFinishTime();
	
	if(m_LocalRecord > 0 && g_Config.m_ClShowLocalRecord && g_Config.m_ClShowRecords)
	{				
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Personal best:");
		TextRender()->Text(0, 5, 47, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_LocalRecord/60, m_LocalRecord-((int)m_LocalRecord/60*60));
		TextRender()->Text(0, 53, 47, 6, aBuf, -1);
	}
}

void CHud::RenderLvlxHealthAndAmmo()
{
	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->MapScreen(0,0,m_Width,300);
	
	Graphics()->QuadsBegin();
	
	if(g_Config.m_ClShortHpDisplay)
	{
		if(g_Config.m_ClRenderAmmo)
		{
			// if weaponstage is active, put a "glow" around the stage ammo
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteProj);
			IGraphics::CQuadItem Item;
			//for (i = 0; i < min(m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount, 10); i++)
			Item = IGraphics::CQuadItem(x,y+24,10,10);
			Graphics()->QuadsDrawTL(&Item, 1);
		}
		Graphics()->QuadsEnd();

		if(g_Config.m_ClRenderHp)
		{
			Graphics()->QuadsBegin();

			// render health
			RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
			IGraphics::CQuadItem Item = IGraphics::CQuadItem(x,y,10,10);
			Graphics()->QuadsDrawTL(&Item, 1);

			RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
			Item = IGraphics::CQuadItem(x,y,10,10);
			Graphics()->QuadsDrawTL(&Item, 1);

			// render armor meter
			RenderTools()->SelectSprite(SPRITE_ARMOR_FULL);
			Item = IGraphics::CQuadItem(x,y+12,10,10);
			Graphics()->QuadsDrawTL(&Item, 1);

			RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
			Item = IGraphics::CQuadItem(x,y+12,10,10);
			Graphics()->QuadsDrawTL(&Item, 1);
			Graphics()->QuadsEnd();
			
			// render hp text
			y = 3;
			char aBuf[8];
			
			// Hearts
			str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pLocalCharacter->m_Health);
			TextRender()->Text(0, x+12, y, 9, aBuf, -1);
			
			// Armor
			str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pLocalCharacter->m_Armor);
			TextRender()->Text(0, x+12, y+12, 9, aBuf, -1);
			
			// Ammo
			str_format(aBuf, sizeof(aBuf), "%d", m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount);
			TextRender()->Text(0, x+12, y+24, 9, aBuf, -1);
		}
	}
	else
	{
		// get values
		int Ammo = clamp(99 + m_pClient->m_Int, 100, 300) / 10;
		int Hp = clamp(99 + m_pClient->m_Sta, 100, 300) / 10;
		
		if(m_pClient->m_Level == 0 || m_pClient->m_Level == -1)
		{
			Ammo = 30;
			Hp = 100;
		}
		else if(m_pClient->m_Level < -1)
		{
			Graphics()->QuadsEnd();
			return;
		}
		
		if(g_Config.m_ClRenderAmmo)
		{
			// if weaponstage is active, put a "glow" around the stage ammo
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteProj);
			IGraphics::CQuadItem Array[100];
			int i;
			for(i = 0; i < min(m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount, Ammo); i++)
				Array[i] = IGraphics::CQuadItem(x+i*12,y+24,10,10);
			Graphics()->QuadsDrawTL(Array, i);
		}
		Graphics()->QuadsEnd();

		if(g_Config.m_ClRenderHp)
		{
			Graphics()->QuadsBegin();
			int h = 0;

			// render health
			IGraphics::CQuadItem Array[100];
			RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
			for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Health, Hp); h++)
				Array[h] = IGraphics::CQuadItem(x+h*12,y,10,10);
			Graphics()->QuadsDrawTL(Array, h);

			int i = 0;
			RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
			for(; h < Hp; h++)
				Array[i++] = IGraphics::CQuadItem(x+h*12,y,10,10);
			Graphics()->QuadsDrawTL(Array, i);

			// render armor meter
			h = 0;
			RenderTools()->SelectSprite(SPRITE_ARMOR_FULL);
			for(; h < min(m_pClient->m_Snap.m_pLocalCharacter->m_Armor, Hp); h++)
				Array[h] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
			Graphics()->QuadsDrawTL(Array, h);

			i = 0;
			RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
			for(; h < Hp; h++)
				Array[i++] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
			Graphics()->QuadsDrawTL(Array, i);
			Graphics()->QuadsEnd();
		}
	}
}

void CHud::RenderCoop()
{
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.1, 0.1, 0.1, 0.25);
	RenderTools()->DrawRoundRect(m_Width-85, 135, 90, 60, 5.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.25);
	RenderTools()->DrawRoundRect(m_Width-83, 137, 90, 15, 3.0f);
	Graphics()->QuadsEnd();
	
	CUIRect MainView;
	MainView.x = m_Width-83;
	MainView.y = 140;
	MainView.w = 88;
	MainView.h = 75;
	
	CUIRect LeftView, RightView, Label, Name;

	MainView.VSplitLeft(MainView.w/2-12, &LeftView, &RightView);
	
	CTeeRenderInfo SkinInfo = m_pClient->m_aClients[m_Cid].m_RenderInfo;	
	SkinInfo.m_Size = UI()->Scale()*18.0f;
	
	RenderTools()->RenderTee(CAnimState::GetIdle(), &SkinInfo, 2, vec2(1, 0), vec2(LeftView.x+7, LeftView.y+5));
	
	MainView.VSplitLeft(18.0f, 0, &Name);
	
	char aBuf[128];
	
	// Name
	str_copy(aBuf, m_aName, sizeof(aBuf));
	UI()->DoLabel(&Name, aBuf, 7.0, -1);
	
	// Left
	LeftView.HSplitTop(12.0f, 0, &LeftView);
	LeftView.HSplitTop(5.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Level: %d", m_Level);
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	LeftView.HSplitTop(5.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Str: %d", m_Str);
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	LeftView.HSplitTop(5.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Sta: %d", m_Sta);
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	// Right
	RightView.HSplitTop(12.0f, 0, &RightView);
	RightView.HSplitTop(5.0f, &Label, &RightView);
	
	// get the weapon name
	if(m_Weapon < 0 || m_Weapon > 4)
		str_copy(aBuf, "Weapon: None", sizeof(aBuf));
	else
		str_copy(aBuf, "Weapon:", sizeof(aBuf));
	
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	// render weapon
	if(m_Weapon >= 0 && m_Weapon <= 4)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_Weapon].m_pSpriteBody);
		Graphics()->QuadsSetRotation(5.7);
		RenderTools()->DrawSprite(Label.x+TextRender()->TextWidth(0,5,aBuf,-1)+14, Label.y+8, g_pData->m_Weapons.m_aId[m_Weapon].m_VisualSize*0.2);
					
		Graphics()->QuadsEnd();
	}
		
	RightView.HSplitTop(5.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Dex: %d", m_Dex);
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	RightView.HSplitTop(5.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Int: %d", m_Int);
	UI()->DoLabel(&Label, aBuf, 5.0, -1);
	
	str_format(aBuf, sizeof(aBuf), "%s wants to be your Partner. Do you want to accept?", m_aName);
	TextRender()->Text(0, m_Width-83, 170, 5.4, aBuf, 80);
	
	// answer display
	LeftView.HSplitTop(20.0f, 0, &LeftView);
	RightView.HSplitTop(20.0f, 0, &RightView);
	RightView.VSplitLeft(30.0f, &RightView, 0);
	
	const char *pYesKey = m_pClient->m_pBinds->GetKey("coop yes");
	const char *pNoKey = m_pClient->m_pBinds->GetKey("coop no");
	str_format(aBuf, sizeof(aBuf), "%s - Yes", pYesKey);
	UI()->DoLabel(&LeftView, aBuf, 6.0f, -1);

	str_format(aBuf, sizeof(aBuf), "%s - No", pNoKey);
	UI()->DoLabel(&RightView, aBuf, 6.0f, 1);
}

void CHud::RenderInfo()
{
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.1, 0.1, 0.1, 0.25);
	RenderTools()->DrawRoundRect(m_Width-90, 60, 95, 70, 5.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.25);
	RenderTools()->DrawRoundRect(m_Width-88, 62, 95, 25, 3.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.15);
	RenderTools()->DrawRoundRect(m_Width-88, 89, 95, 39, 3.0f);
	Graphics()->QuadsEnd();
	
	CUIRect MainView;
	MainView.x = m_Width-85;
	MainView.y = 65;
	MainView.w = 88;
	MainView.h = 75;
	
	CUIRect Label, Name;
	
	if(m_pClient->m_pStatboard->m_Marked < 5)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_pStatboard->m_Marked].m_pSpriteBody);
		RenderTools()->DrawSprite((MainView.x+MainView.w)-20, MainView.y+9, g_pData->m_Weapons.m_aId[m_pClient->m_pStatboard->m_Marked].m_VisualSize*0.35);
	}
	else
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_LVLXSTATS].m_Id);
		Graphics()->QuadsBegin();
		
		switch(m_pClient->m_pStatboard->m_Marked)
		{
		case 5:
			RenderTools()->SelectSprite(SPRITE_STR);
			break;
		case 6:
			RenderTools()->SelectSprite(SPRITE_DEX);
			break;
		case 7:
			RenderTools()->SelectSprite(SPRITE_STA);
			break;
		case 8:
			RenderTools()->SelectSprite(SPRITE_INT);
		}
		RenderTools()->DrawSprite((MainView.x+MainView.w)-20, MainView.y+9, 40);
	}
	Graphics()->QuadsEnd();
	
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.VSplitLeft(50.0f, &Name, 0);
	char aBuf[128];
	
	switch(m_pClient->m_pStatboard->m_Marked)
	{
	case 0:
		str_copy(aBuf, "Hammer info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "51 Str: Triple Hit\n51 Dex: Automatic hammer\nWith at least 11 Int you will be able to heal your teammates. The more Int the better.", 87);
		break;
	case 1:
		str_copy(aBuf, "Gun info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "51 Str: Double gun\n41 Dex: Automatic shot\nThe more Int you have the faster is the ammo regeneration.", 87);
		break;
	case 2:
		str_copy(aBuf, "Shotgun info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "36 Dex: Enables piercing (50% chance of bullets to fly though an enemy)\nThe more Str the more bullets. The bullet range will increas linear to Int.", 87);
		break;
	case 3:
		str_copy(aBuf, "Grenade info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "51 Str: Double Explosion\n31 Int: Mines\nThe more Int the longe the mines live. Explosions get bigger with more Dex.", 87);
		break;
	case 4:
		str_copy(aBuf, "Rife info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "41 Str: Laser explosion\n81 Str 2nd explosion\n61 Dex: Enables piercing\n11 Int: Enable healing. The more int the better the healing.", 87);
		break;
	case 5:
		str_copy(aBuf, "Str info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "Str highers the attacking power. It may enable special effects for your selected weapon. Str enables startup ammo.", 87);
		break;
	case 6:
		str_copy(aBuf, "Dex info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "The more Dex you have the faster is the reloading for every weapon. It may enable special effects for your selected weapon.", 87);
		break;
	case 7:
		str_copy(aBuf, "Sta info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "Sta will higher the defence. Every 10 Sta you will gain another slot for hearts and armor. When reaching 100 Sta every collected heart means two.", 87);
		break;
	case 8:
		str_copy(aBuf, "Int info", sizeof(aBuf));
		Name.VSplitLeft(Name.w/2-TextRender()->TextWidth(0, 8.0, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 8.0, -1);
		
		// show info
		MainView.HSplitTop(20.0f, 0, &MainView);
		TextRender()->Text(0, MainView.x, MainView.y, 6, "Int will allow you to get more ammo for every weapon. It may enable special effects for your selected weapon.", 87);
	}
}

void CHud::RenderExpBar()
{
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	
	Graphics()->SetColor(0.1, 0.1, 0.1, 0.25);
	RenderTools()->DrawRoundRect(-5, 40, 90, 10, 5.0f);
	
	Graphics()->SetColor(0.2, 0.2, 0.2, 0.35);
	RenderTools()->DrawRoundRect(2, 42.5, 80, 5, 2.0f);
	
	if(m_pClient->m_Exp > 4.8)
	{
		vec4 RectColor = m_pClient->m_pSkins->GetColor(g_Config.m_ClExpBarColor);
		Graphics()->SetColor(RectColor.r, RectColor.g, RectColor.b, 0.75);
		RenderTools()->DrawRoundRect(2, 42.5, m_pClient->m_Exp*0.8, 5, 2.0f);
	}
	
	Graphics()->QuadsEnd();
	
	// show exp diff
	if(m_pClient->m_ExpDiff)
	{
		char aBuf[8];
		if(m_pClient->m_ExpDiffTick+Client()->GameTickSpeed() > Client()->GameTick())
		{
			str_format(aBuf, sizeof(aBuf), "%+.2f%%", m_pClient->m_ExpDiff);
			TextRender()->Text(0, 87, 40, 7, aBuf, -1);
		}
		else if(m_pClient->m_ExpDiffTick+Client()->GameTickSpeed()*3 > Client()->GameTick())
		{
			// blend it out slowly
			str_format(aBuf, sizeof(aBuf), "%+.2f%%", m_pClient->m_ExpDiff);
			// get alpha for textcolor
			float a = ((float)(m_pClient->m_ExpDiffTick+Client()->GameTickSpeed()*3) - (float)Client()->GameTick()) / (float)(Client()->GameTickSpeed()*2);
			TextRender()->TextColor(1, 1, 1, a);
			TextRender()->Text(0, 87, 40, 7, aBuf, -1);
		}
		else
			m_pClient->m_ExpDiff = 0;
	}
	TextRender()->TextColor(1, 1, 1, 1);
}

void CHud::RenderSpree()
{
	if(m_SpreeId < 0 || Client()->GameTick() > m_SpreeInitTick+Client()->GameTickSpeed()*5)
		return;

	// get the names
	const char* pOnSpree = m_pClient->m_aClients[m_SpreeId].m_aName;
	{
		while(1)
		{
			if(pOnSpree[0] == ']')
			{
				pOnSpree+=2;
				break;
			}
			
			// just in case
			if(pOnSpree[0] == 0)
				return;
				
			pOnSpree++;
		}
	}
	
	const char* pEndedBy = 0;
	if(m_EndedBy > -1)
	{
		pEndedBy = m_pClient->m_aClients[m_EndedBy].m_aName;
		while(1)
		{
			if(pEndedBy[0] == ']')
			{
				pEndedBy+=2;
				break;
			}
			
			// just in case
			if(pEndedBy[0] == 0)
				return;
				
			pEndedBy++;
		}
	}
	
	// Different msgs for the types
	char aSpreeMsg[11][128] = {"is on a killing spree!",
							   "is on a rampage!",
							   "is dominating!",
							   "is unstoppable!",
							   "is godlike!",
							   "is wicked sick!",
							   "is p4wn4g3!",
							   "has ludicrous skill!",
							   "has 1337 sk1llZ!",
							   "is over 9000!",
							   "rulez the server!"};
	
	// show Spree message
	char aBuf[256];
	if(m_EndedBy > -1)
		str_format(aBuf, sizeof(aBuf), "%s's %d-kills killing spree was ended by %s.", pOnSpree, m_SpreeNum, pEndedBy);
	else
		str_format(aBuf, sizeof(aBuf), "%s %s", pOnSpree, aSpreeMsg[m_SpreeNum]);
		
	if(m_SpreeInitTick+Client()->GameTickSpeed()*3 > Client()->GameTick())
		TextRender()->Text(0, 5, 54, 7, aBuf, 100);
	else if(m_SpreeInitTick+Client()->GameTickSpeed()*5 > Client()->GameTick())
	{
		// blend it out slowly
		// get alpha for textcolor
		float a = ((float)(m_SpreeInitTick+Client()->GameTickSpeed()*5) - (float)Client()->GameTick()) / (float)(Client()->GameTickSpeed()*2);
		TextRender()->TextColor(1, 1, 1, a);
		TextRender()->Text(0, 5, 54, 7, aBuf, 100);
	}

	TextRender()->TextColor(1, 1, 1, 1);
}

void CHud::OnRender()
{
	if(!m_pClient->m_Snap.m_pGameobj)
		return;
		
	m_Width = 300*Graphics()->ScreenAspect();

	bool Spectate = false;
	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team == -1)
		Spectate = true;
	
	if(!g_Config.m_ClClearHud && !g_Config.m_ClClearAll && m_pClient->m_Snap.m_pLocalCharacter && !Spectate && !(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver))
	{
		if(m_pClient->m_IsLvlx)
			RenderLvlxHealthAndAmmo();
		else
			RenderHealthAndAmmo();
		RenderSpeedmeter();
		RenderTime();
	}

	RenderGameTimer();
	RenderSuddenDeath();
	RenderScoreHud();
	RenderWarmupTimer();
	RenderFps();
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		RenderConnectionWarning();
		
	if(m_GotRequest && m_pClient->m_IsCoop && m_pClient->m_IsLvlx)
		RenderCoop();
	
	// render info
	if(m_pClient->m_IsLvlx && m_pClient->m_LoggedIn && ((m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0) || m_pClient->m_Points > 0) && m_pClient->m_pStatboard->m_Active && !m_pClient->m_Snap.m_pGameobj->m_GameOver)
		RenderInfo();
		
	// render exp bar
	if(!g_Config.m_ClClearAll && !g_Config.m_ClClearHud && g_Config.m_ClShowExpBar && m_pClient->m_IsLvlx && m_pClient->m_LoggedIn && m_pClient->m_Level > 0 && m_pClient->m_Exp >= 0)
		RenderExpBar();
		
	// render spree messages
	if(g_Config.m_ClShowSpreeMessages && m_pClient->m_IsLvlx)
		RenderSpree();

	RenderTeambalanceWarning();
	if(!g_Config.m_ClClearAll)	
	{
		if(g_Config.m_ClRenderVote)
			RenderVoting();
		if(!g_Config.m_ClClearHud)
			RenderRecord();
		if(g_Config.m_ClRenderCrosshair && !g_Config.m_ClClearHud)
			RenderCursor();
		if(g_Config.m_ClRenderViewmode && !g_Config.m_ClClearHud && Spectate && !(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver))
			RenderSpectate();
	}
}

void CHud::ConCoop(IConsole::IResult *pResult, void *pUserData)
{		
	CHud *pSelf = (CHud *)pUserData;
	
	if(!pSelf->m_GotRequest || pSelf->Client()->GameTick() < pSelf->m_LastAnswer+pSelf->Client()->GameTickSpeed()*2)
		return;
		
	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
	{
		CNetMsg_Cl_CoopAnswer Msg;
		Msg.m_Answer = 1;
		pSelf->Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		
		pSelf->m_LastAnswer = pSelf->Client()->GameTick();
	}
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
	{
		CNetMsg_Cl_CoopAnswer Msg;
		Msg.m_Answer = 0;
		pSelf->Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		
		pSelf->m_LastAnswer = pSelf->Client()->GameTick();
	}
}

void CHud::OnConsoleInit()
{
	Console()->Register("coop", "r", CFGFLAG_CLIENT, ConCoop, this, "Coop");
}

void CHud::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_RACETIME)
	{
		// activate racestate just in case
		m_pClient->m_pRaceDemo->m_RaceState = CRaceDemo::RACE_STARTED;
			
		CNetMsg_Sv_RaceTime *pMsg = (CNetMsg_Sv_RaceTime *)pRawMsg;
		m_RaceTime = pMsg->m_Time;
		m_RaceTick = 0;
		
		m_LastReceivedTimeTick = Client()->GameTick();
		
		if(m_FinishTime)
			m_FinishTime = 0;

		if(pMsg->m_Check)
		{
			m_CheckpointDiff = (float)pMsg->m_Check/100;
			m_CheckpointTick = Client()->GameTick();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalCid)
	{
			m_CheckpointTick = 0;
			m_RaceTime = 0;
		}
	}
	else if(MsgType == NETMSGTYPE_SV_RECORD)
	{
		CNetMsg_Sv_Record *pMsg = (CNetMsg_Sv_Record *)pRawMsg;
		m_Record = (float)pMsg->m_Time/100;
	}
	else if(MsgType == NETMSGTYPE_SV_COOPREQUEST)
	{
		CNetMsg_Sv_CoopRequest *pMsg = (CNetMsg_Sv_CoopRequest *)pRawMsg;
		
		m_Cid = pMsg->m_Cid;
		str_copy(m_aName, pMsg->m_pName, sizeof(m_aName));
		m_Level = pMsg->m_Level;
		m_Weapon = pMsg->m_Weapon;
		m_Str = pMsg->m_Str;
		m_Sta = pMsg->m_Sta;
		m_Dex = pMsg->m_Dex;
		m_Int = pMsg->m_Int;
		
		m_GotRequest = true;
	}
	else if(MsgType == NETMSGTYPE_SV_COOPRESULT)
		m_GotRequest = false;
	else if(MsgType == NETMSGTYPE_SV_SPREE)
	{
		CNetMsg_Sv_Spree *pMsg = (CNetMsg_Sv_Spree *)pRawMsg;
		
		m_SpreeNum = pMsg->m_Num;
		m_SpreeId = pMsg->m_Cid;
		m_EndedBy = pMsg->m_EndedBy;
		
		m_SpreeInitTick = Client()->GameTick();
	}
}
