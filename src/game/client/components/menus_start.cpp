/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>

#include <engine/shared/config.h>

#include <game/localization.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "menus.h"

void CMenus::RenderStartMenu(CUIRect MainView)
{
	MainView.VMargin(200.0f, &MainView);
	MainView.HMargin(137.0f, &MainView);
	RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

	MainView.Margin(5.0f, &MainView);

	CUIRect Button;

	MainView.HSplitTop(70.0f, &Button, &MainView);
	RenderTools()->DrawUIRect(&Button, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(70.0f, &Button, &MainView);
	RenderTools()->DrawUIRect(&Button, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(70.0f, &Button, &MainView);
	RenderTools()->DrawUIRect(&Button, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

	MainView.HSplitTop(5.0f, 0, &MainView); // little space
	MainView.HSplitTop(70.0f, &Button, &MainView);
	RenderTools()->DrawUIRect(&Button, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);
}

void CMenus::RenderLogo(CUIRect MainView)
{
	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(MainView.w/2-170, 80, 330, 50);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}