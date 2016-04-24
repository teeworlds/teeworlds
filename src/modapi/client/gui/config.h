#ifndef MODAPI_CLIENT_GUI_CONFIG_H
#define MODAPI_CLIENT_GUI_CONFIG_H

#include <engine/graphics.h>

enum
{
	MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL = 0,
	MODAPI_CLIENTGUI_TEXTSTYLE_HEADER,
	MODAPI_CLIENTGUI_TEXTSTYLE_HEADER2,
	MODAPI_CLIENTGUI_NUM_TEXTSTYLES,
};

enum
{
	MODAPI_CLIENTGUI_BUTTONSTYLE_NONE = 0,
	MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL,
	MODAPI_CLIENTGUI_BUTTONSTYLE_HIGHLIGHT,
	MODAPI_CLIENTGUI_BUTTONSTYLE_MOUSEOVER,
	MODAPI_CLIENTGUI_NUM_BUTTONSTYLES,
};

typedef void (*CModAPI_ClientGui_ShowHintFunc)(const char*, void*);

class CModAPI_ClientGui_Config
{
public:
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class CRenderTools *m_pRenderTools;
	class IInput *m_pInput;
	
	IGraphics::CTextureHandle m_Texture;
	
	float m_TextSize[MODAPI_CLIENTGUI_NUM_TEXTSTYLES];
	vec4 m_TextColor[MODAPI_CLIENTGUI_NUM_TEXTSTYLES];
	vec4 m_ButtonColor[MODAPI_CLIENTGUI_NUM_BUTTONSTYLES];
	
	int m_IconSize;
	int m_LabelMargin;
	int m_ButtonHeight;
	int m_SliderWidth;
	int m_SliderHeight;
	int m_LayoutSpacing;
	int m_LayoutMargin;
	
	CModAPI_ClientGui_ShowHintFunc m_fShowHint;
	void* m_pShowHintData;
	
public:
	CModAPI_ClientGui_Config(class IGraphics *pGraphics, class CRenderTools *pRenderTools, class ITextRender *pTextRender, class IInput *pInput, IGraphics::CTextureHandle Texture);
	void ShowHind(const char* pText);
};

#endif
