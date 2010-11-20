/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>

#include "broadcast.h"
	
void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
}

void CBroadcast::OnRender()
{
	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);
		
	if(time_get() < m_BroadcastTime)
	{
		float w = TextRender()->TextWidth(0, 14, m_aBroadcastText, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-w/2, 35, 14, m_aBroadcastText, -1);
	}
}

void CBroadcast::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{
		CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;
		str_copy(m_aBroadcastText, pMsg->m_pMessage, sizeof(m_aBroadcastText));
		m_BroadcastTime = time_get()+time_freq()*10;
	}
}

