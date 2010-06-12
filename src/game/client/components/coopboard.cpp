#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/motd.h>
#include "coopboard.h"
#include "camera.h"
#include "controls.h"


CCoopboard::CCoopboard()
{
	OnReset();
}

void CCoopboard::ConKeyCoopboard(IConsole::IResult *pResult, void *pUserData)
{
	((CCoopboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CCoopboard::OnReset()
{
	m_Active = false;
	m_CoopJustActivated = false;
	m_HasPartner = 0;
	m_Partner = -1;
	m_LastRequest = -1;
}

void CCoopboard::OnConsoleInit()
{
	Console()->Register("+coopboard", "", CFGFLAG_CLIENT, ConKeyCoopboard, this, "Show scoreboard");
}

void CCoopboard::RenderCoopboard()
{
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	
	float w = 320.0f;
	float y = 230.0f;
	float x = Width/2-w;

	float FontSize = 35.0f;
	float LineHeight = 50.0f;
	float TeeSizeMod = 1.0f;
	
	// find players
	const CNetObj_PlayerInfo *paPlayers[MAX_CLIENTS] = {0};
	int NumPlayers = 0;
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == -1 && pInfo->m_ClientId != m_pClient->m_Snap.m_LocalCid && pInfo->m_ClientId != m_Partner && !str_find(m_pClient->m_aClients[pInfo->m_ClientId].m_aName, "[NOT LOGGED IN]"))
			{
				paPlayers[NumPlayers] = pInfo;
				NumPlayers++;
			}
		}
	}
	
	if(!NumPlayers && !m_HasPartner)
		return;
	
	// just for wrong info from server
	if(!m_HasPartner && m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
		m_HasPartner = true;
		
	// get num of entries
	int NumEntries;
	if(m_HasPartner)
		NumEntries = NumPlayers+1;
	else
		NumEntries = NumPlayers;
				
	Graphics()->MapScreen(0, 0, Width, Height);
	
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(Width/2-w, 200.0f, w*2, 60+(NumEntries)*50, 17.0f);
	Graphics()->QuadsEnd();
	
	// set the marked player
	if(m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
	{
		int CursorPosY =( int)m_pClient->m_pControls->m_TargetPos.y;
		if(CursorPosY >= m_StartY)
		{
			m_Marked = ((CursorPosY - m_StartY) / 32);
			
			if(m_Marked >= NumEntries)
			{
				m_StartY = CursorPosY - ((NumEntries)*32);
				m_Marked = NumEntries-1;
			}
		}
		else
			m_StartY = CursorPosY;
	}
	else
	{
		int CameraPosY = (int)m_pClient->m_pCamera->m_Center.y;
		if(CameraPosY >= m_StartY)
		{
			m_Marked = ((CameraPosY - m_StartY) / 32);
				
			if(m_Marked >= NumEntries)
			{
				m_StartY = CameraPosY - ((NumEntries)*32);
				m_Marked = NumEntries-1;
			}
		}
		else
			m_StartY = CameraPosY;
	}
	
	// render player scores
	for(int i = 0; i <= NumPlayers; i++)
	{
		if(i == NumPlayers)
		{
			if(m_HasPartner)
			{
				if(i == m_Marked)
				{
					// background so it's easy to find the local player
					Graphics()->TextureSet(-1);
					Graphics()->QuadsBegin();
					Graphics()->SetColor(1,1,1,0.25f);
					RenderTools()->DrawRoundRect(x+10, y, w*2-20, LineHeight*0.95f, 17.0f);
					Graphics()->QuadsEnd();
				}
				
				// render without text
				TextRender()->Text(0, x+80, y, FontSize, "Delete Partner", -1);
			}
		}
		else
		{
			const CNetObj_PlayerInfo *pInfo = paPlayers[i];

			// dont show not logged in
			if(str_find(m_pClient->m_aClients[pInfo->m_ClientId].m_aName, "[NOT LOGGED IN]"))
				continue;
				
			// make sure that we render the correct team

			if(i == m_Marked)
			{
				// background so it's easy to find the local player
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1,1,1,0.25f);
				RenderTools()->DrawRoundRect(x+10, y, w*2-20, LineHeight*0.95f, 17.0f);
				Graphics()->QuadsEnd();
			}

			// render name
			TextRender()->Text(0, x+80, y, FontSize, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1);
			
			// render avatar	
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientId].m_RenderInfo;
			TeeInfo.m_Size *= TeeSizeMod;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1,0), vec2(x+50, y+28));
		}
		
		y += LineHeight;
	}
	
	const CNetObj_PlayerInfo *pPlayer = 0;
	if(m_Marked < NumPlayers)
		pPlayer = paPlayers[m_Marked];
		
	RequestPartner(NumPlayers, pPlayer);
}

void CCoopboard::RequestPartner(int NumPlayers, const CNetObj_PlayerInfo *pPlayer)
{
	if(Input()->KeyPresses(KEY_MOUSE_1))
	{
		if(Client()->GameTick() > m_LastRequest+Client()->GameTickSpeed()*3)
		{
			// delete partner first
			if(m_HasPartner)
			{
				CNetMsg_Cl_CoopRequest Msg;
				Msg.m_Cid = -1;
				Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
			}
			
			CNetMsg_Cl_CoopRequest Msg;
			if(pPlayer)
				Msg.m_Cid = pPlayer->m_ClientId;
			else
				Msg.m_Cid = -1;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
			
			m_LastRequest = Client()->GameTick();
		}
	}
}

void CCoopboard::OnRender()
{
	if(!m_pClient->m_IsLvlx || !m_pClient->m_LoggedIn)
		return;
		
	// if we activly wanna look on the scoreboard	
	if(m_pClient->m_IsCoop && m_Active && m_pClient->m_Snap.m_pGameobj && !m_pClient->m_Snap.m_pGameobj->m_GameOver)
	{
		if(!m_CoopJustActivated)
		{
			if(m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
				m_StartY = (int)m_pClient->m_pControls->m_TargetPos.y;
			else
				m_StartY = (int)m_pClient->m_pCamera->m_Center.y;
		}
		
		m_CoopJustActivated = true;
		
		// clear motd
		m_pClient->m_pMotd->Clear();
		
		RenderCoopboard();
	}
	else if(m_CoopJustActivated)
		m_CoopJustActivated = false;
		
}

void CCoopboard::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_COOPRESULT)
	{
		CNetMsg_Sv_CoopResult *pMsg = (CNetMsg_Sv_CoopResult *)pRawMsg;
		
		if(pMsg->m_Cid < 0)
		{
			m_HasPartner = false;
			m_Partner = -1;
		}
		else
		{
			m_HasPartner = true;
			m_Partner = pMsg->m_Cid;
		}
	}
}
