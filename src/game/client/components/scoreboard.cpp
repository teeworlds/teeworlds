/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/localization.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
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
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render goals
<<<<<<< HEAD
	if(m_pClient->m_Snap.m_pGameobj)
=======
	y += 10.0f;
	if(m_pClient->m_Snap.m_pGameInfoObj)
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit)
		{
			char aBuf[64];
<<<<<<< HEAD
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameobj->m_ScoreLimit);
=======
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
			TextRender()->Text(0, x, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit)
		{
			char aBuf[64];
<<<<<<< HEAD
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameobj->m_TimeLimit);
=======
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
			TextRender()->Text(0, x+220.0f, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum && m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent)
		{
			char aBuf[64];
<<<<<<< HEAD
			str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameobj->m_RoundCurrent, m_pClient->m_Snap.m_pGameobj->m_RoundNum);
=======
			str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent, m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
			float tw = TextRender()->TextWidth(0, 20.0f, aBuf, -1);
			TextRender()->Text(0, x+w-tw-20.0f, y, 20.0f, aBuf, -1);
		}
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 140.0f;	

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

<<<<<<< HEAD
		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == TEAM_SPECTATORS)
			{
				if(Count)
					str_append(aBuffer, ", ", sizeof(aBuffer));
				str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, sizeof(aBuffer));
				Count++;
			}
		}
=======
	// Headline
	y += 10.0f;
	TextRender()->Text(0, x, y, 28.0f, Localize("Spectators"), w-20.0f);

	// spectator names
	y += 30.0f;
	char aBuffer[1024*4];
	aBuffer[0] = 0;
	bool Multiple = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			str_append(aBuffer, ", ", sizeof(aBuffer));
		str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, sizeof(aBuffer));
		Multiple = true;
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	}
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, 22.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = w-20.0f;
	Cursor.m_MaxLines = 4;
	TextRender()->TextEx(&Cursor, aBuffer, -1);
}

void CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle)
{
	if(Team == TEAM_SPECTATORS)
		return;

<<<<<<< HEAD
	//float ystart = y;
	float h = 740.0f;
=======
	float h = 760.0f;
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
<<<<<<< HEAD
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y, w, h, 17.0f);
=======
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 17.0f);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	Graphics()->QuadsEnd();

	// render title
	float TitleFontsize = 40.0f;
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
	TextRender()->Text(0, x+30, y, TitleFontsize, pTitle, -1);

<<<<<<< HEAD
	float tw = TextRender()->TextWidth(0, 48, pTitle, -1);
	TextRender()->Text(0, x+10, y, 48, pTitle, -1);

	if(m_pClient->m_Snap.m_pGameobj)
	{
		char aBuf[128];
		int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue;
		str_format(aBuf, sizeof(aBuf), "%d", Score);
		tw = TextRender()->TextWidth(0, 48, aBuf, -1);
		TextRender()->Text(0, x+w-tw-30, y, 48, aBuf, -1);
	}

	y += 54.0f;

	// render headlines
	TextRender()->Text(0, x+10, y, 24.0f, Localize("Score"), -1);
	TextRender()->Text(0, x+125, y, 24.0f, Localize("Name"), -1);
	TextRender()->Text(0, x+w-70, y, 24.0f, Localize("Ping"), -1);
	y += 29.0f;

	float FontSize = 35.0f;
	float LineHeight = 50.0f;
	float TeeSizeMod = 1.0f;
	float TeeOffset = 0.0f;
	
	if(m_pClient->m_Snap.m_aTeamSize[Team] > 13)
=======
	char aBuf[128] = {0};
	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_Snap.m_pGameDataObj)
		{
			int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	else
	{
		if(m_pClient->m_Snap.m_pLocalInfo)
		{
			int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	float tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
	TextRender()->Text(0, x+w-tw-30, y, TitleFontsize, aBuf, -1);

	// calculate measurements
	x += 10.0f;
	float LineHeight = 60.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 10.0f;
	if(m_pClient->m_Snap.m_aTeamSize[Team] > 12)
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	{
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
	}
<<<<<<< HEAD
	
	// render player scores
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;
=======
	else if(m_pClient->m_Snap.m_aTeamSize[Team] > 8)
	{
		LineHeight = 50.0f;
		TeeSizeMod = 0.9f;
		Spacing = 5.0f;
	}
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb

	float ScoreOffset = x+10.0f, ScoreLength = 60.0f;
	float TeeOffset = ScoreOffset+ScoreLength, TeeLength = 60*TeeSizeMod;
	float NameOffset = TeeOffset+TeeLength, NameLength = 300.0f-TeeLength;
	float PingOffset = x+600.0f, PingLength = 70.0f;
	float CountryOffset = PingOffset-(LineHeight-Spacing-TeeSizeMod*5.0f)*96.0f/64.0f, CountryLength = (LineHeight-Spacing-TeeSizeMod*5.0f)*96.0f/64.0f;
	float ClanOffset = x+370.0f, ClanLength = 230.0f-CountryLength;	

	// render headlines
	y += 50.0f;
	float HeadlineFontsize = 22.0f;
	TextRender()->Text(0, ScoreOffset, y, HeadlineFontsize, Localize("Score"), -1);
	TextRender()->Text(0, NameOffset+10.0f, y, HeadlineFontsize, Localize("Name"), -1);
	TextRender()->Text(0, ClanOffset+10.0f, y, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, PingOffset+10.0f, y, HeadlineFontsize, Localize("Ping"), -1);

	// render player entries
	y += HeadlineFontsize*2.0f;
	float FontSize = 24.0f;
	CTextCursor Cursor;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// make sure that we render the correct team
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		// background so it's easy to find the local player
		if(pInfo->m_Local)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20.0f, LineHeight, 15.0f);
			Graphics()->QuadsEnd();
		}

<<<<<<< HEAD
		float FontSizeResize = FontSize;
		float Width;
		const float ScoreWidth = 60.0f;
		const float PingWidth = 60.0f;
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -9999, 9999));
		while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > ScoreWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+ScoreWidth-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
		
		FontSizeResize = FontSize;
		while(TextRender()->TextWidth(0, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1) > w-163.0f-PingWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+128.0f, y+(FontSize-FontSizeResize)/2, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

		FontSizeResize = FontSize;
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, -9999, 9999));
		while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > PingWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+w-35.0f-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);

		// render avatar
		if((m_pClient->m_Snap.m_paFlags[0] && m_pClient->m_Snap.m_paFlags[0]->m_CarriedBy == pInfo->m_ClientID) ||
			(m_pClient->m_Snap.m_paFlags[1] && m_pClient->m_Snap.m_paFlags[1]->m_CarriedBy == pInfo->m_ClientID))
=======
		// score
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 999));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->SetCursor(&Cursor, ScoreOffset+ScoreLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ScoreLength;
		TextRender()->TextEx(&Cursor, aBuf, -1);

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

<<<<<<< HEAD
			if(pInfo->m_Team == TEAM_RED) RenderTools()->SelectSprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
			else RenderTools()->SelectSprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
=======
			RenderTools()->SelectSprite(pInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
			
			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(TeeOffset+0.0f, y-5.0f-Spacing/2.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		
<<<<<<< HEAD
=======
		// avatar
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2));

		// name
		TextRender()->SetCursor(&Cursor, NameOffset, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = NameLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

		// clan
		TextRender()->SetCursor(&Cursor, ClanOffset, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ClanLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

		// country flag
		Graphics()->TextureSet(m_pClient->m_pCountryFlags->Get(m_pClient->m_aClients[pInfo->m_ClientID].m_Country)->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem(CountryOffset, y+(Spacing+TeeSizeMod*5.0f)/2.0f, CountryLength, LineHeight-Spacing-TeeSizeMod*5.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		
		// ping
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 1000));
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = PingLength;
		TextRender()->TextEx(&Cursor, aBuf, -1);

		y += LineHeight+Spacing;
	}
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
		
	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();
	

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	
	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 700.0f;

<<<<<<< HEAD
	if(m_pClient->m_Snap.m_pGameobj && !(m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS))
		RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);
	else
=======
	if(m_pClient->m_Snap.m_pGameInfoObj)
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	{
		if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
			RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);
		else
		{
<<<<<<< HEAD
			const char *pText = Localize("Draw!");
			if(m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue)
				pText = Localize("Red team wins!");
			else if(m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed)
				pText = Localize("Blue team wins!");
				
			float w = TextRender()->TextWidth(0, 86.0f, pText, -1);
			TextRender()->Text(0, Width/2-w/2, 39, 86.0f, pText, -1);
		}
		
		RenderScoreboard(Width/2-w-20, 150.0f, w, TEAM_RED, Localize("Red team"));
		RenderScoreboard(Width/2 + 20, 150.0f, w, TEAM_BLUE, Localize("Blue team"));
=======
		
			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && m_pClient->m_Snap.m_pGameDataObj)
			{
				const char *pText = Localize("Draw!");
				if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue)
					pText = Localize("Red team wins!");
				else if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed)
					pText = Localize("Blue team wins!");
			
				float w = TextRender()->TextWidth(0, 86.0f, pText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, pText, -1);
			}
		
			RenderScoreboard(Width/2-w-5.0f, 150.0f, w, TEAM_RED, Localize("Red team"));
			RenderScoreboard(Width/2+5.0f, 150.0f, w, TEAM_BLUE, Localize("Blue team"));
		}
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
	}

	RenderGoals(Width/2-w/2, 150+760+10, w);
	RenderSpectators(Width/2-w/2, 150+760+10+50+10, w);
	RenderRecordingNotification((Width/7)*4);
}

bool CScoreboard::Active()
{
	// if we activly wanna look on the scoreboard	
	if(m_Active)
		return true;
		
	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead
		if(!m_pClient->m_Snap.m_pLocalCharacter)
			return true;
	}

	// if the game is over
<<<<<<< HEAD
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver)
=======
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
>>>>>>> a4ce187613a2afba1dbece7d5cfb356fd29d21eb
		return true;

	return false;
}
