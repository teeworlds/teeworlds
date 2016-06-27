#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "expand.h"

CModAPI_ClientGui_Expand::CModAPI_ClientGui_Expand(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig),
	m_pTitle(0)
{
	
}

CModAPI_ClientGui_Expand::~CModAPI_ClientGui_Expand()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
	
	if(m_pTitle)
		delete m_pTitle;
}

void CModAPI_ClientGui_Expand::Clear()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
	m_Childs.clear();
	
	Update();
}

void CModAPI_ClientGui_Expand::Update()
{
	m_pTitle->m_Rect.x = m_Rect.x;
	m_pTitle->m_Rect.y = m_Rect.y;
	m_pTitle->m_Rect.w = m_Rect.w;
	m_pTitle->Update();
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
	}
	
	int PosY = m_Rect.y + m_pTitle->m_Rect.h + m_pConfig->m_ExpandSpacing;
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			m_Rect.x + m_pConfig->m_ExpandShift,
			PosY,
			m_Rect.w - m_pConfig->m_ExpandShift,
			m_Childs[i]->m_Rect.h
		));
		
		PosY += m_Childs[i]->m_Rect.h + m_pConfig->m_ExpandSpacing;
	}
	
	m_Rect.h = m_pTitle->m_Rect.h + m_pConfig->m_ExpandSpacing*m_Childs.size();
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
		
		m_Rect.h += m_Childs[i]->m_Rect.h;
	}
}
	
void CModAPI_ClientGui_Expand::Render()
{
	m_pTitle->Render();
	
	//Childs
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Render();
	}
}

void CModAPI_ClientGui_Expand::SetTitle(CModAPI_ClientGui_Widget* pWidget)
{
	m_pTitle = pWidget;
}

void CModAPI_ClientGui_Expand::Add(CModAPI_ClientGui_Widget* pWidget)
{
	m_Childs.add(pWidget);
}

void CModAPI_ClientGui_Expand::OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
{
	m_pTitle->OnMouseOver(X, Y, RelX, RelY, KeyState);
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseOver(X, Y, RelX, RelY, KeyState);
	}
}

void CModAPI_ClientGui_Expand::OnButtonClick(int X, int Y, int Button)
{
	m_pTitle->OnButtonClick(X, Y, Button);
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnButtonClick(X, Y, Button);
	}
}

void CModAPI_ClientGui_Expand::OnButtonRelease(int Button)
{
	m_pTitle->OnButtonRelease(Button);
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnButtonRelease(Button);
	}
}

void CModAPI_ClientGui_Expand::OnInputEvent()
{
	m_pTitle->OnInputEvent();
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnInputEvent();
	}
}
