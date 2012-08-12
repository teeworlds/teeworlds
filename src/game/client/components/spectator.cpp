/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

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
	((CSpectator *)pUserData)->Spectate(pResult->GetInteger(0));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
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
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID + 1; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
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
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = MAX_CLIENTS -1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID - 1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_aClients[i].m_Active || pSelf->m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
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
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
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
			if(m_SelectedSpectatorID != NO_SELECTION)
				Spectate(m_SelectedSpectatorID);
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
	m_SelectedSpectatorID = NO_SELECTION;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f);
	RenderTools()->DrawRoundRect(Width/2.0f-300.0f, Height/2.0f-300.0f, 600.0f, 600.0f, 20.0f);
	Graphics()->QuadsEnd();

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
		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(Width/2.0f-280.0f, Height/2.0f-280.0f, 270.0f, 60.0f, 20.0f);
			Graphics()->QuadsEnd();
		}

		if(m_SelectorMouse.x >= -280.0f && m_SelectorMouse.x <= -10.0f &&
			m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
		{
			m_SelectedSpectatorID = SPEC_FREEVIEW;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f-240.0f, Height/2.0f-265.0f, FontSize, Localize("Free-View"), -1);
	}

	float x = -270.0f, y = StartY;
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

		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(Width/2.0f+x-10.0f, Height/2.0f+y-10.0f, 270.0f, 60.0f, 20.0f);
			Graphics()->QuadsEnd();
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+260.0f &&
			m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
		{
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, m_pClient->m_aClients[i].m_aName, 220.0f);

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
	m_SelectedSpectatorID = NO_SELECTION;
}

void CSpectator::Spectate(int SpectatorID)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecID = clamp(SpectatorID, (int)SPEC_FREEVIEW, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
