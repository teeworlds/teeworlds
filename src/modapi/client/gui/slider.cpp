#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "slider.h"

CModAPI_ClientGui_AbstractSlider::CModAPI_ClientGui_AbstractSlider(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_Widget(pConfig)
{	
	m_UnderMouse = false;
	m_ButtonDown = false;
	m_Pos = 0.0f;
}
	
void CModAPI_ClientGui_AbstractSlider::Render()
{
	CUIRect rail;
	rail.x = m_RailRect.x;
	rail.y = m_RailRect.y;
	rail.w = m_RailRect.w;
	rail.h = m_RailRect.h;
	RenderTools()->DrawRoundRect(&rail, vec4(1.0f, 1.0f, 1.0f, 0.25f), m_pConfig->m_SliderWidth/2);
	
	CUIRect slider;
	slider.x = m_SliderRect.x;
	slider.y = m_SliderRect.y;
	slider.w = m_SliderRect.w;
	slider.h = m_SliderRect.h;
	
	if(m_UnderMouse || m_ButtonDown)
	{
		RenderTools()->DrawRoundRect(&slider, vec4(1.0f, 1.0f, 1.0f, 1.0f), m_pConfig->m_SliderWidth/2);
	}
	else
	{
		RenderTools()->DrawRoundRect(&slider, vec4(1.0f, 1.0f, 1.0f, 0.5f), m_pConfig->m_SliderWidth/2);
	}
}

void CModAPI_ClientGui_AbstractSlider::OnMouseOver(int X, int Y, int KeyState)
{
	if(m_SliderRect.IsInside(X, Y))
	{
		m_UnderMouse = true;
	}
	else
	{
		m_UnderMouse = false;
	}
}

void CModAPI_ClientGui_AbstractSlider::OnMouseButtonRelease()
{
	m_ButtonDown = false;
}

CModAPI_ClientGui_HSlider::CModAPI_ClientGui_HSlider(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_AbstractSlider(pConfig)
{
	m_SliderRect.w = m_pConfig->m_SliderHeight;
	m_SliderRect.h = m_pConfig->m_SliderWidth;
	
	m_RailRect.h = m_SliderRect.h;
	m_Rect.h = m_SliderRect.h;
}

void CModAPI_ClientGui_HSlider::Update()
{
	m_RailRect.x = m_Rect.x;
	m_RailRect.y = m_Rect.y + m_Rect.h/2 -m_RailRect.h/2;
	m_RailRect.w = m_Rect.w;
	
	m_SliderRect.x = m_Rect.x + (int)(m_Pos*static_cast<float>(m_Rect.w - m_SliderRect.w));
	m_SliderRect.y = m_Rect.y + m_Rect.h/2 -m_SliderRect.h/2;
}

void CModAPI_ClientGui_HSlider::OnMouseButtonClick(int X, int Y)
{
	if(m_Rect.IsInside(X, Y))
	{
		m_SliderRect.x = clamp(X - m_SliderRect.w/2, m_Rect.x, m_Rect.x + m_Rect.w - m_SliderRect.w);
		
		m_Pos = static_cast<float>(m_SliderRect.x - m_Rect.x)/static_cast<float>(m_Rect.w - m_SliderRect.w);
		OnNewPosition(m_Pos);
		
		m_ButtonDown = true;
	}
}

void CModAPI_ClientGui_HSlider::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	if(m_ButtonDown && RelX != 0)
	{
		m_SliderRect.x = clamp(m_SliderRect.x + RelX, m_Rect.x, m_Rect.x + m_Rect.w - m_SliderRect.w);
		
		m_Pos = static_cast<float>(m_SliderRect.x - m_Rect.x)/static_cast<float>(m_Rect.w - m_SliderRect.w);
		OnNewPosition(m_Pos);
	}
}
	
void CModAPI_ClientGui_HSlider::SetSliderSize(int Size)
{
	m_SliderRect.w = Size;
}

CModAPI_ClientGui_VSlider::CModAPI_ClientGui_VSlider(CModAPI_ClientGui_Config *pConfig) :
	CModAPI_ClientGui_AbstractSlider(pConfig)
{
	m_SliderRect.w = m_pConfig->m_SliderWidth;
	m_SliderRect.h = m_pConfig->m_SliderHeight;
	
	m_RailRect.w = m_SliderRect.w;
	m_Rect.w = m_SliderRect.w;
}

void CModAPI_ClientGui_VSlider::Update()
{
	m_RailRect.x = m_Rect.x;
	m_RailRect.y = m_Rect.y;
	m_RailRect.h = m_Rect.h;
	
	m_SliderRect.x = m_Rect.x;
	m_SliderRect.y = m_Rect.y + (int)(m_Pos*static_cast<float>(m_Rect.h - m_SliderRect.h));
}

void CModAPI_ClientGui_VSlider::OnMouseButtonClick(int X, int Y)
{
	if(m_Rect.IsInside(X, Y))
	{
		m_SliderRect.y = clamp(Y - m_SliderRect.h/2, m_Rect.y, m_Rect.y + m_Rect.h - m_SliderRect.h);
		
		m_Pos = static_cast<float>(m_SliderRect.y - m_Rect.y)/static_cast<float>(m_Rect.h - m_SliderRect.h);
		OnNewPosition(m_Pos);
		
		m_ButtonDown = true;
	}
}

void CModAPI_ClientGui_VSlider::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	if(m_ButtonDown && RelY != 0)
	{
		m_SliderRect.y = clamp(m_SliderRect.y + RelY, m_Rect.y, m_Rect.y + m_Rect.h - m_SliderRect.h);
		
		m_Pos = static_cast<float>(m_SliderRect.y - m_Rect.y)/static_cast<float>(m_Rect.h - m_SliderRect.h);
		OnNewPosition(m_Pos);
	}
}
	
void CModAPI_ClientGui_VSlider::SetSliderSize(int Size)
{
	m_SliderRect.h = Size;
}
