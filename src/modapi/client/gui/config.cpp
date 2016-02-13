#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/input.h>
#include <game/client/render.h>

#include "config.h"

CModAPI_ClientGui_Config::CModAPI_ClientGui_Config(IGraphics *pGraphics, CRenderTools *pRenderTools, ITextRender *pTextRender, IInput *pInput, IGraphics::CTextureHandle Texture) :
	m_pGraphics(pGraphics),
	m_pRenderTools(pRenderTools),
	m_pTextRender(pTextRender),
	m_pInput(pInput),
	m_Texture(Texture)
{
	m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL] = 12.0f;
	m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	
	m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_HEADER] = 16.0f;
	m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_HEADER] = vec4(1.0f, 1.0f, 0.5f, 1.0f);
	
	m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2] = 14.0f;
	m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2] = vec4(1.0f, 1.0f, 0.5f, 1.0f);
	
	m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_NONE] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL] = vec4(0.0f, 0.0f, 0.0f, 0.25f);
	m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT] = vec4(1.0f, 1.0f, 1.0f, 0.5f);
	m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_MOUSEOVER] = vec4(1.0f, 1.0f, 1.0f, 0.25f);
	
	m_IconSize = 16;
	m_LabelMargin = 4;
	m_ButtonHeight = 22;
	m_SliderWidth = 11;
	m_SliderHeight = 100;
	m_LayoutSpacing = 4;
	m_LayoutMargin = 4;
}
