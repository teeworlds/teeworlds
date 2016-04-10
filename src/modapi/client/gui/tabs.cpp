#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "tabs.h"

CModAPI_ClientGui_Tabs::CModAPI_ClientGui_Tabs(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig),
	m_SelectedTab(-1)
{	

}

CModAPI_ClientGui_Tabs::~CModAPI_ClientGui_Tabs()
{
	for(int i=0; i<m_Tabs.size(); i++)
	{
		delete m_Tabs[i].m_pWidget;
	}
}

void CModAPI_ClientGui_Tabs::Update()
{
	m_ContentRect = CModAPI_ClientGui_Rect(
			m_Rect.x,
			m_Rect.y + m_pConfig->m_IconSize + m_pConfig->m_LabelMargin*2.0f,
			m_Rect.w,
			m_Rect.h - m_pConfig->m_IconSize - m_pConfig->m_LabelMargin*2.0f
		);
	
	for(int i=0; i<m_Tabs.size(); i++)
	{
		m_Tabs[i].m_pWidget->SetRect(m_ContentRect);
		m_Tabs[i].m_pWidget->Update();
	}
}
	
void CModAPI_ClientGui_Tabs::Render()
{	
	for(int i=0; i<m_Tabs.size(); i++)
	{
		CUIRect rect;
		rect.x = m_Tabs[i].m_Rect.x;
		rect.y = m_Tabs[i].m_Rect.y;
		rect.w = m_Tabs[i].m_Rect.w;
		rect.h = m_Tabs[i].m_Rect.h;
	
		if(m_SelectedTab == i)
			RenderTools()->DrawUIRect(&rect, m_pConfig->m_ButtonColor[MODAPI_CLIENTGUI_BUTTONSTYLE_MOUSEOVER], CUI::CORNER_T, s_ButtonCornerRadius);
		else
			RenderTools()->DrawUIRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), CUI::CORNER_T, s_ButtonCornerRadius);
			
		int SubX = m_Tabs[i].m_IconId%16;
		int SubY = m_Tabs[i].m_IconId/16;
		
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
		IGraphics::CQuadItem QuadItem(rect.x+m_pConfig->m_LabelMargin, rect.y+m_pConfig->m_LabelMargin, m_pConfig->m_IconSize, m_pConfig->m_IconSize);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	{
		CUIRect rect;
		rect.x = m_ContentRect.x;
		rect.y = m_ContentRect.y;
		rect.w = m_ContentRect.w;
		rect.h = m_ContentRect.h;
		RenderTools()->DrawUIRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), CUI::CORNER_ALL, s_LayoutCornerRadius);
	}
	
	if(m_SelectedTab >= 0)
	{
		m_Tabs[m_SelectedTab].m_pWidget->Render();
	}
}

void CModAPI_ClientGui_Tabs::AddTab(CModAPI_ClientGui_Widget* pWidget, int IconId, const char* pName)
{
	float TabButtonSize = m_pConfig->m_IconSize + m_pConfig->m_LabelMargin*2.0f;
	
	int TabId = m_Tabs.size();
	m_Tabs.add(CTab());
	CTab& Tab = m_Tabs[TabId];
	
	Tab.m_pWidget = pWidget;
	Tab.m_IconId = IconId;
	str_copy(Tab.m_aName, pName, sizeof(Tab.m_aName));
	Tab.m_Rect.x = m_Rect.x + s_LayoutCornerRadius + TabButtonSize*TabId + TabId;
	Tab.m_Rect.y = m_Rect.y;
	Tab.m_Rect.w = m_pConfig->m_IconSize + m_pConfig->m_LabelMargin*2.0f;
	Tab.m_Rect.h = Tab.m_Rect.w;
	
	for(int i=0; i<m_Tabs.size(); i++)
	{
		pWidget->SetRect(CModAPI_ClientGui_Rect(
			m_Rect.x,
			m_Rect.y + m_pConfig->m_IconSize + m_pConfig->m_LabelMargin*2.0f,
			m_Rect.w,
			m_Rect.h - m_pConfig->m_IconSize - m_pConfig->m_LabelMargin*2.0f
		));
	}
	
	if(m_SelectedTab < 0)
		m_SelectedTab = TabId;
}

void CModAPI_ClientGui_Tabs::Clear()
{
	m_SelectedTab = -1;
	for(int i=0; i<m_Tabs.size(); i++)
	{
		delete m_Tabs[i].m_pWidget;
	}
	m_Tabs.clear();
}

void CModAPI_ClientGui_Tabs::OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
{
	if(m_SelectedTab >= 0)
	{
		m_Tabs[m_SelectedTab].m_pWidget->OnMouseOver(X, Y, RelX, RelY, KeyState);
	}
}

void CModAPI_ClientGui_Tabs::OnButtonClick(int X, int Y, int Button)
{
	if(Button == KEY_MOUSE_1)
	{
		for(int i=0; i<m_Tabs.size(); i++)
		{
			if(m_Tabs[i].m_Rect.IsInside(X, Y))
			{
				m_SelectedTab = i;
				return;
			}
		}
	}
	
	if(m_SelectedTab >= 0)
	{
		m_Tabs[m_SelectedTab].m_pWidget->OnButtonClick(X, Y, Button);
	}
}

void CModAPI_ClientGui_Tabs::OnButtonRelease(int Button)
{
	if(m_SelectedTab >= 0)
	{
		m_Tabs[m_SelectedTab].m_pWidget->OnButtonRelease(Button);
	}
}

void CModAPI_ClientGui_Tabs::OnInputEvent()
{
	if(m_SelectedTab >= 0)
	{
		m_Tabs[m_SelectedTab].m_pWidget->OnInputEvent();
	}
}
