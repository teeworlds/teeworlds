#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "popup.h"

CModAPI_ClientGui_Popup::CModAPI_ClientGui_Popup(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig),
	m_Child(0)
{
	m_Toolbar = new CModAPI_ClientGui_HListLayout(pConfig);
}

CModAPI_ClientGui_Popup::~CModAPI_ClientGui_Popup()
{
	if(m_Child) delete m_Child;
	delete m_Toolbar;
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
		m_Child->m_Rect.CenterIn(m_Rect);
		m_Child->Update();
	
		m_Toolbar->SetRect(CModAPI_ClientGui_Rect(
			m_Child->m_Rect.x,
			m_Child->m_Rect.y + m_Child->m_Rect.h + 4,
			m_Child->m_Rect.w,
			m_pConfig->m_ButtonHeight + m_pConfig->m_LayoutMargin*2
			));
			
		m_Toolbar->Update();
	}
}
	
void CModAPI_ClientGui_Popup::Render()
{
	if(!m_Child)
		return;
		
	//Background	
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.9f);
	IGraphics::CQuadItem QuadItem(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	
	m_Child->Render();
	m_Toolbar->Render();
}

void CModAPI_ClientGui_Popup::Add(CModAPI_ClientGui_Widget* pWidget)
{
	if(m_Child) delete m_Child;
	m_Child = pWidget;
}

void CModAPI_ClientGui_Popup::OnMouseOver(int X, int Y, int KeyState)
{
	if(m_Child)
	{
		m_Child->OnMouseOver(X, Y, KeyState);
		m_Toolbar->OnMouseOver(X, Y, KeyState);
	}
}

void CModAPI_ClientGui_Popup::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	if(m_Child)
	{
		m_Child->OnMouseMotion(RelX, RelY, KeyState);
		m_Toolbar->OnMouseMotion(RelX, RelY, KeyState);
	}
}

void CModAPI_ClientGui_Popup::OnMouseButtonClick(int X, int Y)
{
	if(m_Child)
	{
		m_Child->OnMouseButtonClick(X, Y);
		m_Toolbar->OnMouseButtonClick(X, Y);
	}
}

void CModAPI_ClientGui_Popup::OnMouseButtonRelease()
{
	if(m_Child)
	{
		m_Child->OnMouseButtonRelease();
		m_Toolbar->OnMouseButtonRelease();
	}
}

void CModAPI_ClientGui_Popup::OnInputEvent()
{
	if(m_Child)
	{
		m_Child->OnInputEvent();
		m_Toolbar->OnInputEvent();
	}
}
