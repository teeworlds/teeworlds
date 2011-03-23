/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/storage.h>
#include <game/client/component.h>

#include "background.h"

int CBackground::m_BackgroundTexture;

void CBackground::OnInit()
{
	m_BackgroundTexture = Graphics()->LoadTexture("editor/checker.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
}

void CBackground::OnRender()
{
	CUIRect Screen;
	Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);
	Graphics()->TextureSet(m_BackgroundTexture);
	Graphics()->BlendNormal();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->QuadsSetSubset(0,0, Screen.w/32.0f, Screen.h/32.0f);
	IGraphics::CQuadItem QuadItem(Screen.x, Screen.y, Screen.w, Screen.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}
