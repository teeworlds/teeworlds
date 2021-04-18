/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include "damageind.h"

CDamageInd::CDamageInd()
{
	m_NumItems = 0;
}

CDamageInd::CItem *CDamageInd::CreateItem()
{
	if(m_NumItems < MAX_ITEMS)
	{
		CItem *pItem = &m_aItems[m_NumItems];
		m_NumItems++;
		return pItem;
	}
	return 0;
}

void CDamageInd::DestroyItem(CDamageInd::CItem *pItem)
{
	m_NumItems--;
	if(pItem != &m_aItems[m_NumItems])
		*pItem = m_aItems[m_NumItems];
}

void CDamageInd::Create(vec2 Pos, vec2 Dir)
{
	CItem *pItem = CreateItem();
	if(pItem)
	{
		pItem->m_Pos = Pos;
		pItem->m_LifeTime = 0.75f;
		pItem->m_Dir = Dir*-1;
		pItem->m_StartAngle = (random_float() - 1.0f) * 2.0f * pi;
	}
}

void CDamageInd::OnRender()
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	const float Now = Client()->LocalTime();
	static float s_LastLocalTime = Now;
	for(int i = 0; i < m_NumItems;)
	{
		m_aItems[i].m_LifeTime -= (Now - s_LastLocalTime) * m_pClient->GetAnimationPlaybackSpeed();
		if(m_aItems[i].m_LifeTime < 0.0f)
			DestroyItem(&m_aItems[i]);
		else
		{
			vec2 Pos = mix(m_aItems[i].m_Pos+m_aItems[i].m_Dir*75.0f, m_aItems[i].m_Pos, clamp((m_aItems[i].m_LifeTime-0.60f)/0.15f, 0.0f, 1.0f));
			const float Alpha = clamp(m_aItems[i].m_LifeTime * 10.0f, 0.0f, 1.0f); // 0.1 -> 0.0 == 1.0 -> 0.0
			Graphics()->SetColor(1.0f*Alpha, 1.0f*Alpha, 1.0f*Alpha, Alpha);
			Graphics()->QuadsSetRotation(m_aItems[i].m_StartAngle + m_aItems[i].m_LifeTime * 2.0f);
			RenderTools()->SelectSprite(SPRITE_STAR1);
			RenderTools()->DrawSprite(Pos.x, Pos.y, 48.0f);
			i++;
		}
	}
	Graphics()->QuadsEnd();
	s_LastLocalTime = Now;
}

void CDamageInd::OnReset()
{
	m_NumItems = 0;
}
