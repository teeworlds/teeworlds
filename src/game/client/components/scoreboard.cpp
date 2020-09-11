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
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);

	// render goals
	y += 2.0f;
	// TODO: ADDBACK: draw goals
}

float CScoreboard::RenderSpectators(float x, float y, float w)
{
	// TODO: ADDBACK: draw spectators
	return 20;
}

float CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, int Align)
{
	return 0.0f;
	// TODO: ADDBACK: draw scoreboard
}

void CScoreboard::RenderRecordingNotification(float x, float w)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	CUIRect RectBox = {x, 0.0f, w, 50.0f};
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
	// TODO: ADDBACK: draw recording time
}

void CScoreboard::RenderNetworkQuality(float x, float w)
{
	//draw the box
	CUIRect RectBox = {x, 0.0f, w, 50.0f};
	vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.4f);
	const float LineHeight = 17.0f;
	int Score = Client()->GetInputtimeMarginStabilityScore();

	Graphics()->BlendNormal();
	RenderTools()->DrawUIRect(&RectBox, Color, CUI::CORNER_B, 15.0f);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_NETWORKICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_NETWORK_GOOD);
	IGraphics::CQuadItem QuadItem(x+20.0f, 12.5f, 25.0f, 25.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	x += 50.0f;
	// TODO: ADDBACK: draw network quality
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
		RenderTools()->DrawUIRect(&LocalBarRect, vec4(0.9f,0.9f,0.9f,1.0f), 0, 0);
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

			// TODO: ADDBACK: draw round over text
		}
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
		{
			char aText[256];
			str_copy(aText, Localize("Round over!"), sizeof(aText));

			// TODO: ADDBACK: draw round over text
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
