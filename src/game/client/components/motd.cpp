/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/client/gameclient.h>

#include "motd.h"

void CMotd::Clear()
{
	m_ServerMotdTime = 0;
}

bool CMotd::IsActive()
{
	return time_get() < m_ServerMotdTime;
}

void CMotd::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_ONLINE || OldState == IClient::STATE_OFFLINE)
		Clear();
}

void CMotd::OnRender()
{
	if(!IsActive())
		return;

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	float h = 800.0f;
	float w = 650.0f;
	float x = Width/2 - w/2;
	float y = 150.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 40.0f);
	Graphics()->QuadsEnd();

	TextRender()->Text(0, x+40.0f, y+40.0f, 32.0f, m_aServerMotd, (int)(w-80.0f));
}

void CMotd::OnMessage(int MsgType, void *pRawMsg)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(MsgType == NETMSGTYPE_SV_MOTD)
	{
		CNetMsg_Sv_Motd *pMsg = (CNetMsg_Sv_Motd *)pRawMsg;

		// process escaping
		str_copy(m_aServerMotd, pMsg->m_pMessage, sizeof(m_aServerMotd));
		for(int i = 0; m_aServerMotd[i]; i++)
		{
			if(m_aServerMotd[i] == '\\')
			{
				if(m_aServerMotd[i+1] == 'n')
				{
					m_aServerMotd[i] = ' ';
					m_aServerMotd[i+1] = '\n';
					i++;
				}
			}
		}

		if(m_aServerMotd[0] && g_Config.m_ClMotdTime)
			m_ServerMotdTime = time_get()+time_freq()*g_Config.m_ClMotdTime;
		else
			m_ServerMotdTime = 0;
	}
}

bool CMotd::OnInput(IInput::CEvent Event)
{
	if(IsActive() && Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		Clear();
		return true;
	}
	return false;
}

