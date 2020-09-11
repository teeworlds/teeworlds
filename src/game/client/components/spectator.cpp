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
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active &&
		(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK || pSelf->DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER))
		pSelf->Spectate(pResult->GetInteger(0), pResult->GetInteger(1));
}

bool CSpectator::SpecModePossible(int SpecMode, int SpectatorID)
{
	int i = SpectatorID;
	switch(SpecMode)
	{
	case SPEC_PLAYER:
		if(!m_pClient->m_aClients[i].m_Active || m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
		{
			return false;
		}
		if(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != TEAM_SPECTATORS &&
			(i == m_pClient->m_LocalClientID || m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team != m_pClient->m_aClients[i].m_Team ||
			(m_pClient->m_Snap.m_paPlayerInfos[i] && (m_pClient->m_Snap.m_paPlayerInfos[i]->m_PlayerFlags&PLAYERFLAG_DEAD))))
		{
			return false;
		}
		return true;
	case SPEC_FLAGRED:
	case SPEC_FLAGBLUE:
		return m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS;
	case SPEC_FREEVIEW:
		return m_pClient->m_aClients[m_pClient->m_LocalClientID].m_Team == TEAM_SPECTATORS;
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
	// TODO: ADDBACK: draw spectator view
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
