/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/gamecore.h> // get_angle
#include <game/client/ui.h>
#include <game/client/render.h>
#include "damageind.h"

CDamageInd::CDamageInd()
{
	m_Lastupdate = 0;
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
		i->m_Life = 0.75f;
		i->m_Dir = Dir*-1;
		i->m_StartAngle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
	}
}

void CDamageInd::OnRender()
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	for(int i = 0; i < m_NumItems;)
	{
		vec2 Pos = mix(m_aItems[i].m_Pos+m_aItems[i].m_Dir*75.0f, m_aItems[i].m_Pos, clamp((m_aItems[i].m_Life-0.60f)/0.15f, 0.0f, 1.0f));

		m_aItems[i].m_Life -= Client()->FrameTime();
		if(m_aItems[i].m_Life < 0.0f)
			DestroyI(&m_aItems[i]);
		else
		{
			Graphics()->SetColor(1.0f,1.0f,1.0f, m_aItems[i].m_Life/0.1f);
			Graphics()->QuadsSetRotation(m_aItems[i].m_StartAngle + m_aItems[i].m_Life * 2.0f);
			RenderTools()->SelectSprite(SPRITE_STAR1);
			RenderTools()->DrawSprite(Pos.x, Pos.y, 48.0f);
			i++;
		}
	}
	Graphics()->QuadsEnd();
}
