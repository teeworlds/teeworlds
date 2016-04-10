#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "popup.h"

CModAPI_ClientGui_Popup::CModAPI_ClientGui_Popup(CModAPI_ClientGui_Config *pConfig, const CModAPI_ClientGui_Rect& CreatorRect, int Width, int Height, int Alignment) :
	CModAPI_ClientGui_Widget(pConfig),
	m_Child(0),
	m_IsClosed(false)
{
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	
	switch(Alignment)
	{
		case ALIGNMENT_LEFT:
			SetRect(CModAPI_ClientGui_Rect(
				CreatorRect.x - Width - m_pConfig->m_LayoutSpacing,
				min(CreatorRect.y, ScreenHeight - Height - m_pConfig->m_LayoutSpacing),
				Width,
				Height
			));
			break;
		case ALIGNMENT_RIGHT:
		default:
			SetRect(CModAPI_ClientGui_Rect(
				CreatorRect.x + CreatorRect.w + m_pConfig->m_LayoutSpacing,
				min(CreatorRect.y, ScreenHeight - Height - m_pConfig->m_LayoutSpacing),
				Width,
				Height
			));
	}
}

CModAPI_ClientGui_Popup::~CModAPI_ClientGui_Popup()
{
	if(m_Child)
		delete m_Child;
}

void CModAPI_ClientGui_Popup::Clear()
{
	if(m_Child)
	{
		delete m_Child;
		m_Child = 0;
		m_ChildWidth = 0;
		m_ChildHeight = 0;
	}
}

void CModAPI_ClientGui_Popup::Update()
{
	if(m_Child)
	{
		m_Child->SetRect(CModAPI_ClientGui_Rect(
			m_Rect.x + m_pConfig->m_LayoutMargin,
			m_Rect.y + m_pConfig->m_LayoutMargin,
			m_Rect.w - m_pConfig->m_LayoutMargin*2,
			m_Rect.h - m_pConfig->m_LayoutMargin*2)
		);
		m_Child->Update();
	}
}
	
void CModAPI_ClientGui_Popup::Render()
{
	if(!m_Child)
		return;
		
	//Background	
	{
		CUIRect rect;
		rect.x = m_Rect.x;
		rect.y = m_Rect.y;
		rect.w = m_Rect.w;
		rect.h = m_Rect.h;
		RenderTools()->DrawRoundRect(&rect, vec4(0.5f, 0.5f, 0.5f, 0.8f), s_LayoutCornerRadius);
	}
	
	m_Child->Render();
}

void CModAPI_ClientGui_Popup::Add(CModAPI_ClientGui_Widget* pWidget)
{
	if(m_Child) delete m_Child;
	m_Child = pWidget;
}

void CModAPI_ClientGui_Popup::OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
{
	if(m_Child)
	{
		m_Child->OnMouseOver(X, Y, RelX, RelY, KeyState);
	}
}

void CModAPI_ClientGui_Popup::OnButtonClick(int X, int Y, int Button)
{
	if(m_Rect.IsInside(X, Y))
	{
		if(m_Child)
		{
			m_Child->OnButtonClick(X, Y, Button);
		}
	}
	else if(Button == KEY_MOUSE_1)
	{
		Close();
	}
}

void CModAPI_ClientGui_Popup::OnButtonRelease(int Button)
{
	if(m_Child)
	{
		m_Child->OnButtonRelease(Button);
	}
}

void CModAPI_ClientGui_Popup::OnInputEvent()
{
	if(m_Child)
	{
		m_Child->OnInputEvent();
	}
}

void CModAPI_ClientGui_Popup::Close()
{
	m_IsClosed = true;
}

bool CModAPI_ClientGui_Popup::IsClosed()
{
	return m_IsClosed;
}
