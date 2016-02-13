#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "layout.h"

/* H LAYOUT *************************************************************/
		
CModAPI_ClientGui_HListLayout::CModAPI_ClientGui_HListLayout(CModAPI_ClientGui_Config *pConfig, int Style, int Model) :
	CModAPI_ClientGui_Widget(pConfig),
	m_Style(Style),
	m_Model(Model)
{
	
}

CModAPI_ClientGui_HListLayout::~CModAPI_ClientGui_HListLayout()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
}

void CModAPI_ClientGui_HListLayout::Clear()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
	m_Childs.clear();
	
	Update();
}

void CModAPI_ClientGui_HListLayout::Update_FillingFirst()
{
	if(m_Childs.size() == 0)
		return;
	
	int Margin = (m_Style == MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME ? m_pConfig->m_LayoutMargin : 0);
	int AvailableSpace = m_Rect.w - 2*Margin;
		
	if(m_Childs.size() > 1)
		AvailableSpace -= m_pConfig->m_LayoutSpacing * (m_Childs.size() - 1);
		
	for(int i=1; i<m_Childs.size(); i++)
	{
		AvailableSpace -= m_Childs[i]->m_Rect.w;
	}
	
	m_Childs[0]->SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x + Margin,
		m_Rect.y + Margin,
		AvailableSpace,
		m_Rect.h - Margin*2
	));
	
	int PosX = m_Childs[0]->m_Rect.x + m_Childs[0]->m_Rect.w;
	for(int i=1; i<m_Childs.size(); i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			PosX + m_pConfig->m_LayoutSpacing,
			m_Rect.y + Margin,
			m_Childs[i]->m_Rect.w,
			m_Rect.h - Margin*2
		));
		
		PosX += m_pConfig->m_LayoutSpacing + m_Childs[i]->m_Rect.w;
	}
}

void CModAPI_ClientGui_HListLayout::Update_FillingLast()
{
	if(m_Childs.size() == 0)
		return;
	
	int Margin = (m_Style == MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME ? m_pConfig->m_LayoutMargin : 0);
	int AvailableSpace = m_Rect.w - 2*Margin;
		
	if(m_Childs.size() > 1)
		AvailableSpace -= m_pConfig->m_LayoutSpacing * (m_Childs.size() - 1);
		
	for(int i=0; i<m_Childs.size()-1; i++)
	{
		AvailableSpace -= m_Childs[i]->m_Rect.w;
	}
	
	int PosX = m_Rect.x + Margin;
	for(int i=0; i<m_Childs.size()-1; i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			PosX,
			m_Rect.y + Margin,
			m_Childs[i]->m_Rect.w,
			m_Rect.h - Margin*2
		));
		
		PosX += m_Childs[i]->m_Rect.w + m_pConfig->m_LayoutSpacing;
	}
	
	m_Childs[m_Childs.size()-1]->SetRect(CModAPI_ClientGui_Rect(
		PosX,
		m_Rect.y + Margin,
		AvailableSpace,
		m_Rect.h - Margin*2
	));
}

void CModAPI_ClientGui_HListLayout::Update_FillingAll()
{
	if(m_Childs.size() == 0)
		return;
	
	int Margin = (m_Style == MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME ? m_pConfig->m_LayoutMargin : 0);
	int AvailableSpace = m_Rect.w - 2*Margin;

	if(m_Childs.size() > 1)
		AvailableSpace -= m_pConfig->m_LayoutSpacing * (m_Childs.size() - 1);
		
	int Width = AvailableSpace / m_Childs.size();
	
	int PosX = m_Rect.x + Margin;
	for(int i=0; i<m_Childs.size()-1; i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			PosX,
			m_Rect.y + Margin,
			Width,
			m_Rect.h - Margin*2
		));
		
		PosX += Width + m_pConfig->m_LayoutSpacing;
	}
	
	m_Childs[m_Childs.size()-1]->SetRect(CModAPI_ClientGui_Rect(
		PosX,
		m_Rect.y + Margin,
		m_Rect.x + m_Rect.w - Margin - PosX,
		m_Rect.h - Margin*2
	));
}

void CModAPI_ClientGui_HListLayout::Update_FillingNone()
{
	int Margin = (m_Style == MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME ? m_pConfig->m_LayoutMargin : 0);
	
	int PosX = m_Rect.x + Margin;
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			PosX,
			m_Rect.y + Margin,
			m_Childs[i]->m_Rect.w,
			m_Rect.h - Margin*2
		));
		
		PosX += m_Childs[i]->m_Rect.w + m_pConfig->m_LayoutSpacing;
	}
}

void CModAPI_ClientGui_HListLayout::Update()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
	}
	
	switch(m_Model)
	{
		case MODAPI_CLIENTGUI_LAYOUTFILLING_NONE:
			Update_FillingNone();
			break;
		case MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST:
			Update_FillingFirst();
			break;
		case MODAPI_CLIENTGUI_LAYOUTFILLING_LAST:
			Update_FillingLast();
			break;
		case MODAPI_CLIENTGUI_LAYOUTFILLING_ALL:
			Update_FillingAll();
			break;
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
	}
}
	
void CModAPI_ClientGui_HListLayout::Render()
{
	//Background
	if(m_Style == MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME)
	{
		CUIRect rect;
		rect.x = m_Rect.x;
		rect.y = m_Rect.y;
		rect.w = m_Rect.w;
		rect.h = m_Rect.h;
		RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), s_LayoutCornerRadius);
	}
	
	//Childs
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Render();
	}
}

void CModAPI_ClientGui_HListLayout::Add(CModAPI_ClientGui_Widget* pWidget)
{
	m_Childs.add(pWidget);
}

void CModAPI_ClientGui_HListLayout::OnMouseOver(int X, int Y, int KeyState)
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseOver(X, Y, KeyState);
	}
}

void CModAPI_ClientGui_HListLayout::OnMouseButtonClick(int X, int Y)
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseButtonClick(X, Y);
	}
}

void CModAPI_ClientGui_HListLayout::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseMotion(RelX, RelY, KeyState);
	}
}

void CModAPI_ClientGui_HListLayout::OnMouseButtonRelease()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseButtonRelease();
	}
}

void CModAPI_ClientGui_HListLayout::OnInputEvent()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnInputEvent();
	}
}

/* V LAYOUT *************************************************************/

CModAPI_ClientGui_VListLayout::CSeparator::CSeparator(CModAPI_ClientGui_Config* pConfig) :
	CModAPI_ClientGui_Widget(pConfig)
{
	SetHeight(20);
}

void CModAPI_ClientGui_VListLayout::CSeparator::Render()
{
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(1.0f, 1.0f, 0.5f, 1.0f);
	
	float y = m_Rect.y + m_Rect.h * 0.7;
	IGraphics::CLineItem Line(m_Rect.x, y, m_Rect.x + m_Rect.w, y);
	Graphics()->LinesDraw(&Line, 1);
	
	Graphics()->LinesEnd();
}

CModAPI_ClientGui_VListLayout::CSlider::CSlider(CModAPI_ClientGui_VListLayout* pLayout) :
	CModAPI_ClientGui_VSlider(pLayout->m_pConfig),
	m_pLayout(pLayout)
{
	
}

void CModAPI_ClientGui_VListLayout::CSlider::OnNewPosition(float Pos)
{
	m_pLayout->SetScroll(Pos);
}

CModAPI_ClientGui_VListLayout::CModAPI_ClientGui_VListLayout(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig)
{
	m_pSlider = new CModAPI_ClientGui_VListLayout::CSlider(this);
	m_ShowScrollBar = false;
	m_ScrollValue = 0;
}

CModAPI_ClientGui_VListLayout::~CModAPI_ClientGui_VListLayout()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
	if(m_pSlider) delete m_pSlider;
}

void CModAPI_ClientGui_VListLayout::Clear()
{
	for(int i=0; i<m_Childs.size(); i++)
	{
		delete m_Childs[i];
	}
	m_Childs.clear();
	
	Update();
}

void CModAPI_ClientGui_VListLayout::Update()
{
	m_ChildrenHeight = 0;
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->SetRect(CModAPI_ClientGui_Rect(
			m_Rect.x + m_pConfig->m_LayoutSpacing,
			m_Rect.y + m_ChildrenHeight,
			m_Rect.w - m_pConfig->m_LayoutSpacing*2,
			m_Childs[i]->m_Rect.h
		));
		
		m_ChildrenHeight += m_Childs[i]->m_Rect.h + m_pConfig->m_LayoutSpacing;
	}
	
	//Add scrole bar
	if(m_ChildrenHeight > m_Rect.h)
	{
		m_ShowScrollBar = true;
		m_ScrollValue = 0;
		
		m_pSlider->SetRect(CModAPI_ClientGui_Rect(
			m_Rect.x + m_Rect.w - m_pSlider->m_Rect.w,
			m_Rect.y,
			m_pSlider->m_Rect.w,
			m_Rect.h
		));
		
		m_pSlider->Update();
		
		for(int i=0; i<m_Childs.size(); i++)
		{
			m_Childs[i]->SetWidth(m_Rect.w - m_pConfig->m_LayoutSpacing*3 - m_pSlider->m_Rect.w);
		}
	}
	else
	{
		m_ShowScrollBar = false;
		m_ScrollValue = 0;
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Update();
	}
}
	
void CModAPI_ClientGui_VListLayout::Render()
{
	//Background
	CUIRect rect;
	rect.x = m_Rect.x;
	rect.y = m_Rect.y;
	rect.w = m_Rect.w;
	rect.h = m_Rect.h;
	RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), s_LayoutCornerRadius);
	
	//Childs
	Graphics()->ClipEnable(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->Render();
	}
	Graphics()->ClipDisable();
	
	//Scrollbar
	if(m_ShowScrollBar)
	{
		m_pSlider->Render();
	}
}

void CModAPI_ClientGui_VListLayout::Add(CModAPI_ClientGui_Widget* pWidget)
{
	m_Childs.add(pWidget);
}

void CModAPI_ClientGui_VListLayout::AddSeparator()
{
	Add(new CModAPI_ClientGui_VListLayout::CSeparator(m_pConfig));
}

void CModAPI_ClientGui_VListLayout::OnMouseOver(int X, int Y, int KeyState)
{
	if(m_ShowScrollBar)
	{
		m_pSlider->OnMouseOver(X, Y, KeyState);
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseOver(X, Y, KeyState);
	}
}

void CModAPI_ClientGui_VListLayout::OnMouseButtonClick(int X, int Y)
{
	if(m_ShowScrollBar)
	{
		m_pSlider->OnMouseButtonClick(X, Y);
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseButtonClick(X, Y);
	}
}

void CModAPI_ClientGui_VListLayout::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	if(m_ShowScrollBar)
	{
		m_pSlider->OnMouseMotion(RelX, RelY, KeyState);
	}
	else
	{
		for(int i=0; i<m_Childs.size(); i++)
		{
			m_Childs[i]->OnMouseMotion(RelX, RelY, KeyState);
		}
	}
}

void CModAPI_ClientGui_VListLayout::OnMouseButtonRelease()
{
	if(m_ShowScrollBar)
	{
		m_pSlider->OnMouseButtonRelease();
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnMouseButtonRelease();
	}
}

void CModAPI_ClientGui_VListLayout::OnInputEvent()
{
	if(m_ShowScrollBar)
	{
		m_pSlider->OnInputEvent();
	}
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->OnInputEvent();
	}
}

void CModAPI_ClientGui_VListLayout::SetScroll(float Pos)
{
	int ScrollHeight = m_ChildrenHeight - m_Rect.h;
	int LastScrollValue = m_ScrollValue;
	m_ScrollValue = ScrollHeight * Pos;
	int ScrollDelta = LastScrollValue - m_ScrollValue;
	
	for(int i=0; i<m_Childs.size(); i++)
	{
		m_Childs[i]->SetY(m_Childs[i]->m_Rect.y + ScrollDelta);
		m_Childs[i]->Update();
	}
}
