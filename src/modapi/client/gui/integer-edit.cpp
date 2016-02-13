#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "integer-edit.h"

CModAPI_ClientGui_AbstractIntegerEdit::CIncreaseButton::CIncreaseButton(CModAPI_ClientGui_AbstractIntegerEdit *pIntegerEdit) :
	CModAPI_ClientGui_IconButton(pIntegerEdit->m_pConfig, 3),
	m_pIntegerEdit(pIntegerEdit)
{
	
}

void CModAPI_ClientGui_AbstractIntegerEdit::CIncreaseButton::MouseClickAction()
{
	m_pIntegerEdit->SetValue(m_pIntegerEdit->GetValue()+1);
}

CModAPI_ClientGui_AbstractIntegerEdit::CDecreaseButton::CDecreaseButton(CModAPI_ClientGui_AbstractIntegerEdit *pIntegerEdit) :
	CModAPI_ClientGui_IconButton(pIntegerEdit->m_pConfig, 2)	,
	m_pIntegerEdit(pIntegerEdit)
{
	
}

void CModAPI_ClientGui_AbstractIntegerEdit::CDecreaseButton::MouseClickAction()
{
	m_pIntegerEdit->SetValue(m_pIntegerEdit->GetValue()-1);
}

CModAPI_ClientGui_AbstractIntegerEdit::CModAPI_ClientGui_AbstractIntegerEdit(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig),
	m_pIncreaseButton(0),
	m_pDecreaseButton(0)
{
	m_Rect.h = m_pConfig->m_ButtonHeight;
	
	m_pIncreaseButton = new CModAPI_ClientGui_AbstractIntegerEdit::CIncreaseButton(this);
	m_pDecreaseButton = new CModAPI_ClientGui_AbstractIntegerEdit::CDecreaseButton(this);
	
	m_IntegerRect.h = m_Rect.h;
}

CModAPI_ClientGui_AbstractIntegerEdit::~CModAPI_ClientGui_AbstractIntegerEdit()
{
	delete m_pIncreaseButton;
	delete m_pDecreaseButton;
}

void CModAPI_ClientGui_AbstractIntegerEdit::Update()
{
	m_pIncreaseButton->SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x + m_Rect.w - m_pIncreaseButton->m_Rect.w,
		m_Rect.y,
		m_pIncreaseButton->m_Rect.w,
		m_pIncreaseButton->m_Rect.h
	));
	
	m_pDecreaseButton->SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x + m_Rect.w - m_pDecreaseButton->m_Rect.w - m_pIncreaseButton->m_Rect.w - 4.0f,
		m_Rect.y,
		m_pDecreaseButton->m_Rect.w,
		m_pDecreaseButton->m_Rect.h
	));
	
	m_IntegerRect.x = m_Rect.x;
	m_IntegerRect.y = m_Rect.y;
	m_IntegerRect.w = (m_pDecreaseButton->m_Rect.x - 4.0f) - m_Rect.x;
	m_IntegerRect.h = m_Rect.h;
}
	
void CModAPI_ClientGui_AbstractIntegerEdit::Render()
{
	//Integer
	{
		CUIRect rect;
		rect.x = m_IntegerRect.x;
		rect.y = m_IntegerRect.y;
		rect.w = m_IntegerRect.w;
		rect.h = m_IntegerRect.h;
		RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_ButtonOpacity), s_ButtonCornerRadius);
		
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", GetValue());
		
		int TextSize = m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL];
		
		int ContentX = m_IntegerRect.x + m_IntegerRect.w/2;
		int ContentY = m_IntegerRect.y + m_IntegerRect.h/2;
	
		int TextWidth = TextRender()->TextWidth(0, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], aBuf, -1);
		int PosX = ContentX - TextWidth/2;
		int PosY = ContentY - m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL]*0.7;
		
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, PosX, PosY, TextSize, TEXTFLAG_RENDER);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}
	
	m_pIncreaseButton->Render();
	m_pDecreaseButton->Render();
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnMouseButtonClick(int X, int Y)
{
	m_pIncreaseButton->OnMouseButtonClick(X, Y);
	m_pDecreaseButton->OnMouseButtonClick(X, Y);
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnMouseButtonRelease()
{
	m_pIncreaseButton->OnMouseButtonRelease();
	m_pDecreaseButton->OnMouseButtonRelease();
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnMouseOver(int X, int Y, int KeyState)
{
	m_pIncreaseButton->OnMouseOver(X, Y, KeyState);
	m_pDecreaseButton->OnMouseOver(X, Y, KeyState);
}

CModAPI_ClientGui_IntegerEdit::CModAPI_ClientGui_IntegerEdit(CModAPI_ClientGui_Config *pConfig, int DefaultValue) :
	CModAPI_ClientGui_AbstractIntegerEdit(pConfig),
	m_Value(DefaultValue)
{
	
}
	
void CModAPI_ClientGui_IntegerEdit::SetValue(int v)
{
	m_Value = v;
}
	
int CModAPI_ClientGui_IntegerEdit::GetValue()
{
	return m_Value;
}

CModAPI_ClientGui_ExternalIntegerEdit::CModAPI_ClientGui_ExternalIntegerEdit(CModAPI_ClientGui_Config *pConfig, int* Memory) :
	CModAPI_ClientGui_AbstractIntegerEdit(pConfig),
	m_Memory(Memory)
{
	
}
	
void CModAPI_ClientGui_ExternalIntegerEdit::SetValue(int v)
{
	*m_Memory = v;
}
	
int CModAPI_ClientGui_ExternalIntegerEdit::GetValue()
{
	return *m_Memory;
}
