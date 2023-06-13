/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/localization.h>
#include <game/client/render.h>

#include "spectator.h"

bool CSpectator::CanSpectate()
{
	return m_pClient->m_Snap.m_SpecInfo.m_Active
		&& (Client()->State() != IClient::STATE_DEMOPLAYBACK || DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER);
}

void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->CanSpectate())
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->CanSpectate())
		pSelf->Spectate(pResult->GetInteger(0), pResult->GetInteger(1));
}

bool CSpectator::SpecModePossible(int SpecMode, int SpectatorID)
{
	switch(SpecMode)
	{
	case SPEC_PLAYER:
		if(!m_pClient->m_aClients[SpectatorID].m_Active || m_pClient->m_aClients[SpectatorID].m_Team == TEAM_SPECTATORS)
		{
			return false;
		}
		if(m_pClient->m_LocalClientID != -1
			&& m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS
			&& (SpectatorID == m_pClient->m_LocalClientID
				|| m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != m_pClient->m_aClients[SpectatorID].m_Team
				|| (m_pClient->m_Snap.m_paPlayerInfos[SpectatorID] && (m_pClient->m_Snap.m_paPlayerInfos[SpectatorID]->m_PlayerFlags&PLAYERFLAG_DEAD))))
		{
			return false;
		}
		return true;
	case SPEC_FLAGRED:
	case SPEC_FLAGBLUE:
		return m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS;
	case SPEC_FREEVIEW:
		return m_pClient->m_LocalClientID == -1 || m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS;
	default:
		dbg_assert(false, "invalid spec mode");
		return false;
	}
}

static void IterateSpecMode(int Direction, int *pSpecMode, int *pSpectatorID)
{
	dbg_assert(Direction == -1 || Direction == 1, "invalid direction");
	if(*pSpecMode == SPEC_PLAYER)
	{
		*pSpectatorID += Direction;
		if(0 <= *pSpectatorID && *pSpectatorID < MAX_CLIENTS)
		{
			return;
		}
		*pSpectatorID = -1;
	}
	*pSpecMode = (*pSpecMode + Direction + NUM_SPECMODES) % NUM_SPECMODES;
	if(*pSpecMode == SPEC_PLAYER)
	{
		*pSpectatorID = 0;
		if(Direction == -1)
		{
			*pSpectatorID = MAX_CLIENTS - 1;
		}
	}
}

void CSpectator::HandleSpectateNextPrev(int Direction)
{
	if(!m_pClient->m_Snap.m_SpecInfo.m_Active || (Client()->State() == IClient::STATE_DEMOPLAYBACK && DemoPlayer()->GetDemoType() != IDemoPlayer::DEMOTYPE_SERVER))
		return;

	int NewSpecMode = m_pClient->m_Snap.m_SpecInfo.m_SpecMode;
	int NewSpectatorID = -1;
	if(NewSpecMode == SPEC_PLAYER)
	{
		NewSpectatorID = m_pClient->m_Snap.m_SpecInfo.m_SpectatorID;
	}

	// Ensure the loop terminates even if no spec modes are possible.
	for(int i = 0; i < NUM_SPECMODES + MAX_CLIENTS; i++)
	{
		IterateSpecMode(Direction, &NewSpecMode, &NewSpectatorID);
		if(SpecModePossible(NewSpecMode, NewSpectatorID))
		{
			Spectate(NewSpecMode, NewSpectatorID);
			return;
		}
	}
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	((CSpectator *)pUserData)->HandleSpectateNextPrev(1);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	((CSpectator *)pUserData)->HandleSpectateNextPrev(-1);
}

CSpectator::CSpectator()
{
	OnReset();
}

void CSpectator::OnConsoleInit()
{
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i[mode] i[target]", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnCursorMove(float x, float y, int CursorType)
{
	if(!m_Active)
		return false;

	UI()->ConvertCursorMove(&x, &y, CursorType);
	m_SelectorMouse += vec2(x, y);
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

	int TotalCount = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!SpecModePossible(SPEC_PLAYER, i))
			continue;
		TotalCount++;
	}

	int ColumnSize = 8;
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	if(TotalCount > 16)
	{
		ColumnSize = 16;
		ScaleY = 0.5f;
	}
	if(TotalCount > 48)
		ScaleX = 2.0f;
	else if(TotalCount > 32)
		ScaleX = 1.5f;

	// draw background
	const float ScreenHeight = 400.0f * 3.0f;
	const float ScreenWidth = ScreenHeight * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, ScreenWidth, ScreenHeight);

	const float Height = 600.0f;
	const float Width = 600.0f;
	const float Margin = 20.0f;

	const vec2 CenterOffset(ScreenWidth / 2.0f, ScreenHeight / 2.0f);
	CUIRect BackgroundRect = {CenterOffset.x - Width / 2.0f * ScaleX, CenterOffset.y - Height / 2.0f, Width * ScaleX, Height};
	Graphics()->BlendNormal();
	BackgroundRect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.3f), Margin);

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -Width / 2.0f * ScaleX + Margin, Width / 2.0f * ScaleX - Margin);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -Height / 2.0f + Margin, Height / 2.0f - Margin);

	const float FontSize = 20.0f;

	// draw free-view selection
	if(m_pClient->m_LocalClientID == -1 || m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS)
	{
		CUIRect FreeViewRect;
		FreeViewRect.x = CenterOffset.x - 280.0f;
		FreeViewRect.y = CenterOffset.y - 280.0f;
		FreeViewRect.w = 270.0f;
		FreeViewRect.h = 60.0f;
		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_FREEVIEW)
			FreeViewRect.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), 10.0f);

		const bool Selected = FreeViewRect.Inside(m_SelectorMouse + CenterOffset);
		if(Selected)
			m_SelectedSpecMode = SPEC_FREEVIEW;

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected ? 1.0f : 0.5f);
		static CTextCursor s_FreeViewLabelCursor;
		s_FreeViewLabelCursor.m_Align = TEXTALIGN_ML;
		s_FreeViewLabelCursor.m_FontSize = FontSize;
		s_FreeViewLabelCursor.MoveTo(FreeViewRect.x + 40.0f, FreeViewRect.y + FreeViewRect.h / 2.0f);
		s_FreeViewLabelCursor.Reset((g_Localization.Version() << 1) | (Selected ? 1 : 0));
		TextRender()->TextOutlined(&s_FreeViewLabelCursor, Localize("Free-View"), -1);
	}

	// draw flag selection
	float x = Margin, y = -270.0f;
	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
	{
		for(int Flag = SPEC_FLAGRED; Flag <= SPEC_FLAGBLUE; ++Flag)
		{
			CUIRect FlagRect;
			FlagRect.x = CenterOffset.x + x - 10.0f;
			FlagRect.y = CenterOffset.y + y - 10.0f;
			FlagRect.w = 120.0f;
			FlagRect.h = 60.0f;
			if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == Flag)
				FlagRect.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), 10.0f);

			const bool Selected = FlagRect.Inside(m_SelectorMouse + CenterOffset);
			if(Selected)
				m_SelectedSpecMode = Flag;

			const float Size = 60.0f / 1.5f + (Selected ? 12.0f : 8.0f);
			const vec2 FlagSize = vec2(Size / 2.0f, Size);
			const vec2 FlagPos = FlagRect.Center() - FlagSize / 2.0f;

			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(Flag == SPEC_FLAGRED ? SPRITE_FLAG_RED : SPRITE_FLAG_BLUE);
			IGraphics::CQuadItem QuadItem(FlagPos.x, FlagPos.y, FlagSize.x, FlagSize.y);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			x += FlagRect.w + Margin;
		}
	}

	const float PlayerStartY = -210.0f + Margin * ScaleY;
	x = -Width / 2.0f * ScaleX + 30.0f, y = PlayerStartY;

	// draw player selection
	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!SpecModePossible(SPEC_PLAYER, i))
			continue;

		if(Count != 0 && Count % ColumnSize == 0)
		{
			x += 290.0f;
			y = PlayerStartY;
		}
		Count++;

		CUIRect PlayerRect;
		PlayerRect.x = CenterOffset.x + x - 10.0f;
		PlayerRect.y = CenterOffset.y + y + 10.0f - Margin * ScaleY;
		PlayerRect.w = 270.0f;
		PlayerRect.h = 60.0f * ScaleY;
		if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SPEC_PLAYER && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
			PlayerRect.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), 10.0f);

		const bool Selected = PlayerRect.Inside(m_SelectorMouse + CenterOffset);
		if(Selected)
		{
			m_SelectedSpecMode = SPEC_PLAYER;
			m_SelectedSpectatorID = i;
		}

		// carried flag
		float PosX = PlayerRect.x + PlayerRect.h / 2.0f;
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS
			&& m_pClient->m_Snap.m_pGameDataFlag
			&& (m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed == i || m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue == i))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(i == m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
			IGraphics::CQuadItem QuadItem(PosX - PlayerRect.h / 4.0f, PlayerRect.y - PlayerRect.h * 0.05f, PlayerRect.h / 2.0f, PlayerRect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// tee
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		TeeInfo.m_Size *= ScaleY;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(PosX, PlayerRect.y + PlayerRect.h * 0.6f));
		PosX += PlayerRect.h / 2.0f;

		// client ID and name
		PosX += UI()->DrawClientID(FontSize, vec2(PosX, PlayerRect.y + PlayerRect.h / 2.0f - FontSize * 0.6f), i);
		if(Config()->m_ClShowsocial)
		{
			static CTextCursor s_PlayerNameCursor;
			s_PlayerNameCursor.m_Align = TEXTALIGN_ML;
			s_PlayerNameCursor.m_FontSize = FontSize;
			s_PlayerNameCursor.Reset();
			s_PlayerNameCursor.MoveTo(PosX, PlayerRect.y + PlayerRect.h / 2.0f);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected ? 1.0f : 0.5f);
			TextRender()->TextOutlined(&s_PlayerNameCursor, m_pClient->m_aClients[i].m_aName, -1);
		}

		y += PlayerRect.h;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	RenderTools()->RenderCursor(m_SelectorMouse + CenterOffset, 48.0f);
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

	if(m_pClient->m_Snap.m_SpecInfo.m_SpecMode == SpecMode && (SpecMode != SPEC_PLAYER || m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID))
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpecMode = SpecMode;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
