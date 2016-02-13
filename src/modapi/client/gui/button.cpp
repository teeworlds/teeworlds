#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "button.h"

CModAPI_ClientGui_AbstractButton::CModAPI_ClientGui_AbstractButton(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig),
	m_UnderMouse(false),
	m_ButtonStyle(MODAPI_CLIENTGUI_BUTTONSTYLE_NORMAL)
{	
	m_Rect.w = m_pConfig->m_ButtonHeight;
	m_Rect.h = m_pConfig->m_ButtonHeight;
}
	
void CModAPI_ClientGui_AbstractButton::Render()
{
	CUIRect rect;
	rect.x = m_Rect.x;
	rect.y = m_Rect.y;
	rect.w = m_Rect.w;
	rect.h = m_Rect.h;
	
	if(m_UnderMouse)
		RenderTools()->DrawRoundRect(&rect, m_pConfig->m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_MOUSEOVER], s_ButtonCornerRadius);
	else if(m_ButtonStyle != MODAPI_CLIENTGUI_BUTTONSTYLE_NONE)
		RenderTools()->DrawRoundRect(&rect, m_pConfig->m_ButtonColor[m_ButtonStyle], s_ButtonCornerRadius);
}

void CModAPI_ClientGui_AbstractButton::SetButtonStyle(int Style)
{
	m_ButtonStyle = Style;
}

void CModAPI_ClientGui_AbstractButton::OnMouseOver(int X, int Y, int KeyState)
{
	if(m_Rect.IsInside(X, Y))
	{
		m_UnderMouse = true;
	}
	else
	{
		m_UnderMouse = false;
	}
}

void CModAPI_ClientGui_AbstractButton::OnMouseButtonClick(int X, int Y)
{
	if(m_Rect.IsInside(X, Y))
	{
		m_Clicked = true;
	}
}

void CModAPI_ClientGui_AbstractButton::OnMouseButtonRelease()
{
	if(m_UnderMouse && m_Clicked)
	{
		m_Clicked = false;
		MouseClickAction();
	}
}

/* ICON BUTTON ********************************************************/

CModAPI_ClientGui_IconButton::CModAPI_ClientGui_IconButton(CModAPI_ClientGui_Config *pConfig, int IconId) :
	CModAPI_ClientGui_AbstractButton(pConfig),
	m_IconId(IconId)
{
	
}

void CModAPI_ClientGui_IconButton::Render()
{
	CModAPI_ClientGui_AbstractButton::Render();
	
	int PosX = m_Rect.x + m_Rect.w/2;
	int PosY = m_Rect.y + m_Rect.h/2;
	
	int SubX = m_IconId%16;
	int SubY = m_IconId/16;
	
	Graphics()->TextureSet(m_pConfig->m_Texture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
	IGraphics::CQuadItem QuadItem(PosX - m_pConfig->m_IconSize/2, PosY - m_pConfig->m_IconSize/2, m_pConfig->m_IconSize, m_pConfig->m_IconSize);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CModAPI_ClientGui_IconButton::SetIcon(int IconId)
{
	m_IconId = IconId;
}

/* TEXT BUTTON ********************************************************/

CModAPI_ClientGui_TextButton::CModAPI_ClientGui_TextButton(CModAPI_ClientGui_Config *pConfig, const char* pText, int IconId) :
	CModAPI_ClientGui_AbstractButton(pConfig)
{
	SetText(pText);
	SetIcon(IconId);
	
	m_Centered = true;
	
	m_Rect.w = m_pConfig->m_ButtonHeight;
	m_Rect.h = m_pConfig->m_ButtonHeight;
}

void CModAPI_ClientGui_TextButton::Render()
{
	CModAPI_ClientGui_AbstractButton::Render();
		
	int TextWidth = TextRender()->TextWidth(0, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], m_aText, -1);
	
	int PosX;
	if(m_Centered)
	{
		int CenterX = m_Rect.x + m_Rect.w/2;
		
		if(m_IconId >= 0)
			CenterX += m_pConfig->m_IconSize/2;
		
		PosX = CenterX - TextWidth/2;
	}
	else
	{
		PosX = m_Rect.x + m_pConfig->m_LabelMargin;
		
		if(m_IconId >= 0)
			PosX += m_pConfig->m_IconSize;
	}
	
	int CenterY = m_Rect.y + m_Rect.h/2;
	int PosY = CenterY - m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL]*0.7;
	
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, PosX, PosY, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], TEXTFLAG_RENDER);
	TextRender()->TextColor(m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].x, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].y, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].z, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].w);
	TextRender()->TextEx(&Cursor, m_aText, -1);
	
	if(m_IconId >= 0)
	{
		float IconSize = 16.0f;
		
		PosX -= m_pConfig->m_IconSize + 4; //Icon size and space
		
		int SubX = m_IconId%16;
		int SubY = m_IconId/16;
		
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
		IGraphics::CQuadItem QuadItem(PosX, CenterY - m_pConfig->m_IconSize/2, m_pConfig->m_IconSize, m_pConfig->m_IconSize);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CModAPI_ClientGui_TextButton::SetText(const char* pText)
{
	if(pText)
		str_copy(m_aText, pText, sizeof(m_aText));
}

void CModAPI_ClientGui_TextButton::SetIcon(int IconId)
{
	m_IconId = IconId;
}

/* EXTERNAL TEXT BUTTON ********************************************************/

CModAPI_ClientGui_ExternalTextButton::CModAPI_ClientGui_ExternalTextButton(class CModAPI_ClientGui_Config *pConfig, char* pText) :
	CModAPI_ClientGui_AbstractButton(pConfig),
	m_pText(pText),
	m_Centered(true)
{
	m_Rect.w = m_pConfig->m_ButtonHeight;
	m_Rect.h = m_pConfig->m_ButtonHeight;
}

void CModAPI_ClientGui_ExternalTextButton::Render()
{
	CModAPI_ClientGui_AbstractButton::Render();
	
	if(!m_pText)
		return;
	
	if(m_Centered)
	{
		int CenterX = m_Rect.x + m_Rect.w/2;
		int CenterY = m_Rect.y + m_Rect.h/2;
		
		int TextWidth = 0;
		
		TextWidth = TextRender()->TextWidth(0, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], m_pText, -1);
		int PosX = CenterX - TextWidth/2;
		int PosY = CenterY - m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL]*0.7;
		
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, PosX, PosY, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], TEXTFLAG_RENDER);
		TextRender()->TextColor(m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].x, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].y, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].z, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].w);
		TextRender()->TextEx(&Cursor, m_pText, -1);
	}
	else
	{
		int CenterY = m_Rect.y + m_Rect.h/2;
		int PosY = CenterY - m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL]*0.7;
		
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, m_Rect.x + m_pConfig->m_LabelMargin, PosY, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], TEXTFLAG_RENDER);
		TextRender()->TextColor(m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].x, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].y, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].z, m_pConfig->m_TextColor[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL].w);
		TextRender()->TextEx(&Cursor, m_pText, -1);
	}
}
