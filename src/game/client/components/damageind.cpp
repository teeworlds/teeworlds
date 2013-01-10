/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/gamecore.h> // get_angle
#include <game/client/ui.h>
#include <game/client/render.h>
#include "damageind.h"

CDamageInd::CDamageInd()
{
	m_NumItems = 0;
}

CDamageInd::CItem *CDamageInd::CreateI()
{
	if (m_NumItems < MAX_ITEMS)
	{
		CItem *p = &m_aItems[m_NumItems];
		m_NumItems++;
		return p;
	}
	return 0;
}

void CDamageInd::DestroyI(CDamageInd::CItem *i)
{
	m_NumItems--;
	*i = m_aItems[m_NumItems];
}

void CDamageInd::Create(vec2 Pos, vec2 Dir)
{
	CItem *i = CreateI();
	if (i)
	{
		i->m_Pos = Pos;
		i->m_StartTime = Client()->LocalTime();
		i->m_Dir = Dir*-1;
		i->m_StartAngle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
	}
}

void CDamageInd::OnRender()
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	static float s_LastLocalTime = Client()->LocalTime();
	for(int i = 0; i < m_NumItems;)
	{
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
			if(pInfo->m_Paused)
				m_aItems[i].m_StartTime += Client()->LocalTime()-s_LastLocalTime;
			else
				m_aItems[i].m_StartTime += (Client()->LocalTime()-s_LastLocalTime)*(1.0f-pInfo->m_Speed);
		}
		else
		{
			if(m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED)
				m_aItems[i].m_StartTime += Client()->LocalTime()-s_LastLocalTime;
		}

		float Life = 0.75f - (Client()->LocalTime() - m_aItems[i].m_StartTime);
		if(Life < 0.0f)
			DestroyI(&m_aItems[i]);
		else
		{
			vec2 Pos = mix(m_aItems[i].m_Pos+m_aItems[i].m_Dir*75.0f, m_aItems[i].m_Pos, clamp((Life-0.60f)/0.15f, 0.0f, 1.0f));
			Graphics()->SetColor(1.0f,1.0f,1.0f, Life/0.1f);
			Graphics()->QuadsSetRotation(m_aItems[i].m_StartAngle + Life * 2.0f);
			RenderTools()->SelectSprite(SPRITE_STAR1);
			RenderTools()->DrawSprite(Pos.x, Pos.y, 48.0f);
			i++;
		}
	}
	s_LastLocalTime = Client()->LocalTime();
	Graphics()->QuadsEnd();
}

void CDamageInd::OnReset()
{
	m_NumItems = 0;
}
