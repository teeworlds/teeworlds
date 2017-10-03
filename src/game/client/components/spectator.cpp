/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/render.h>

#include "spectator.h"


void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active &&
		(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK || pSelf->DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER))
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	int SpectatorID = pResult->GetInteger(0);
	if (SpectatorID >= 0)
		((CSpectator *)pUserData)->Spectate(SPEC_PLAYER, SpectatorID);
	else
		((CSpectator *)pUserData)->Spectate(-SpectatorID, -1);
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpecMode = SPEC_PLAYER;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW)
	{
		if(pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			NewSpecMode = SPEC_FLAGRED;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}
		else
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}
	}
	else
	{
		if(pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FLAGRED)
		{
			NewSpecMode = SPEC_FLAGBLUE;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID + 1; i < MAX_CLIENTS; i++)
			{
				if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}

		if(!GotNewSpectatorID)
		{
			for(int i = 0; i < pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i++)
			{
				if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}

		if(!GotNewSpectatorID && pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			NewSpecMode = SPEC_FLAGRED;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpecMode, NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpecMode = SPEC_PLAYER;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW)
	{
		for(int i = MAX_CLIENTS -1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
		if(!GotNewSpectatorID && pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			NewSpecMode = SPEC_FLAGBLUE;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}
	}
	else
	{
		if(pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FLAGBLUE)
		{
			NewSpecMode = SPEC_FLAGRED;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID - 1; i > -1; i--)
			{
				if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}

		if(!GotNewSpectatorID)
		{
			for(int i = MAX_CLIENTS - 1; i > pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i--)
			{
				if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
				break;
			}
		}

		if(!GotNewSpectatorID && pSelf->m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			NewSpecMode = SPEC_FLAGBLUE;
			NewSpectatorID = -1;
			GotNewSpectatorID = true;
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpecMode, NewSpectatorID);
}

CSpectator::CSpectator()
{
	OnReset();
}

void CSpectator::OnConsoleInit()
{
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnMouseMove(float x, float y)
{
	if(!m_Active)
		return false;

	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
	return true;
}

void CSpectator::OnRelease()
{
	OnReset();
}

void CSpectator::OnRender()
{
	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpecMode != NO_SELECTION)
				Spectate(m_SelectedSpecMode, m_SelectedSpectatorID);
			m_WasActive = false;
		}
		return;
	}

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpecMode = NO_SELECTION;
	m_SelectedSpectatorID = -1;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	CUIRect Rect = {Width/2.0f-300.0f, Height/2.0f-300.0f, 600.0f, 600.0f};
	Graphics()->BlendNormal();
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.3f), 20.0f);

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -280.0f, 280.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	// draw selections
	float FontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	bool Selected = false;

	if(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS)
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW)
		{
			Rect.x = Width/2.0f-280.0f;
			Rect.y = Height/2.0f-280.0f;
			Rect.w = 270.0f;
			Rect.h = 60.0f;
			RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
		}

		if(m_SelectorMouse.x >= -280.0f && m_SelectorMouse.x <= -10.0f &&
			m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
		{
			m_SelectedSpecMode = SPEC_FREEVIEW;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f-240.0f, Height/2.0f-265.0f, FontSize, Localize("Free-View"), -1);
	}

	//
	float x = 20.0f, y = -270;
	if (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
	{
		for(int Flag = SPEC_FLAGRED; Flag <= SPEC_FLAGBLUE; ++Flag)
		{
			if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == Flag)
			{
				Rect.x = Width/2.0f+x-10.0f;
				Rect.y = Height/2.0f+y-10.0f;
				Rect.w = 120.0f;
				Rect.h = 60.0f;
				RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
			}

			Selected = false;
			if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+110.0f &&
				m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
			{
				m_SelectedSpecMode = Flag;
				Selected = true;
			}

			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(Flag == SPEC_FLAGRED ? SPRITE_FLAG_RED : SPRITE_FLAG_BLUE);

			float Size = LineHeight/1.5f + (Selected ? 12.0f : 8.0f);
			float FlagWidth = Width/2.0f + x + 40.0f + (Selected ? -3.0f : -2.0f);
			float FlagHeight = Height/2.0f + y + (Selected ? -6.0f : -4.0f);

			IGraphics::CQuadItem QuadItem(FlagWidth, FlagHeight, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			x+=140.0f;
		}
	}

	x = -270.0f, y = StartY;
	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS ||
			(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS && (m_pClient->m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_DEAD ||
			m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != m_pClient->m_aClients[i].m_Team || i == m_pClient->m_LocalClientID)))
			continue;

		if(++Count%9 == 0)
		{
			x += 290.0f;
			y = StartY;
		}

		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_PLAYER && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
		{
			Rect.x = Width/2.0f+x-10.0f;
			Rect.y = Height/2.0f+y-10.0f;
			Rect.w = 270.0f;
			Rect.h = 60.0f;
			RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), 20.0f);
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+260.0f &&
			m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
		{
			m_SelectedSpecMode = SPEC_PLAYER;
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%2d: %s", i, g_Config.m_ClShowsocial ? m_pClient->m_aClients[i].m_aName : "");
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, aBuf, 220.0f);

		// flag
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataFlag && (m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed == i ||
			m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue == i))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(m_pClient->m_aClients[i].m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width/2.0f+x-LineHeight/5.0f, Height/2.0f+y-LineHeight/3.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Width/2.0f+x+20.0f, Height/2.0f+y+20.0f));

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// draw cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+Width/2.0f, m_SelectorMouse.y+Height/2.0f, 48.0f, 48.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CSpectator::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedSpecMode = NO_SELECTION;
	m_SelectedSpectatorID = -1;
}

void CSpectator::Spectate(int SpecMode, int SpectatorID)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecMode = clamp(SpecMode, 0, NUM_SPECMODES-1);
		m_pClient->m_DemoSpecID = clamp(SpectatorID, -1, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SpecMode && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpecMode = SpecMode;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
