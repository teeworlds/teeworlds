// LordSk
#include "input_console.h"

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <generated/client_data.h>
#include <game/client/ui.h>

void CEditorInputConsole::Init(IConsole* pConsole, IGraphics* pGraphics, CUI* pUI)
{
	m_pConsole = pConsole;
	m_pGraphics = pGraphics;
	m_pUI = pUI;
}

void CEditorInputConsole::Render()
{
	if(!m_IsOpen)
		return;

	const CUIRect ScreenRect = *m_pUI->Screen();
	Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x + ScreenRect.w,
		ScreenRect.y + ScreenRect.h);

	const float ConsoleWidth = ScreenRect.w;
	const float ConsoleHeight = ScreenRect.h / 3;

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BG].m_Id);
	Graphics()->QuadsBegin();

	Graphics()->SetColor(0.1, 0.1, 0.8, 0.95f);
	Graphics()->QuadsSetSubset(0,-ConsoleHeight*0.075f,ConsoleWidth*0.075f*0.5f,0);
	IGraphics::CQuadItem QuadBg(0, 0, ConsoleWidth, ConsoleHeight);
	Graphics()->QuadsDrawTL(&QuadBg, 1);

	Graphics()->QuadsEnd();
}

void CEditorInputConsole::OnInput(IInput::CEvent Event)
{
	if(m_IsOpen && Event.m_Key == KEY_ESCAPE && (Event.m_Flags&IInput::FLAG_PRESS))
	{
		m_IsOpen = false;
		return;
	}
}

void CEditorInputConsole::ToggleOpen()
{
	m_IsOpen ^= 1;
}
