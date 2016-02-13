#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/client/render.h>

#include "widget.h"

CModAPI_ClientGui_Rect::CModAPI_ClientGui_Rect() :
	x(0),
	y(0),
	w(0),
	h(0)
{
	
}

CModAPI_ClientGui_Rect::CModAPI_ClientGui_Rect(int X, int Y, int W, int H) :
	x(X),
	y(Y),
	w(W),
	h(H)
{
	
}

void CModAPI_ClientGui_Rect::CenterIn(const CModAPI_ClientGui_Rect& Rect)
{
	x = Rect.x + Rect.w/2 - w/2;
	y = Rect.y + Rect.h/2 - h/2;
}

bool CModAPI_ClientGui_Rect::IsInside(int X, int Y)
{
	return (X >= x && X <= x + w && Y >= y && Y <= y + h);
}

CModAPI_ClientGui_Widget::CModAPI_ClientGui_Widget(CModAPI_ClientGui_Config *pConfig) :
	m_pConfig(pConfig)
{
	
}

CModAPI_ClientGui_Widget::~CModAPI_ClientGui_Widget()
{
	
}

void CModAPI_ClientGui_Widget::Update()
{
	
}

void CModAPI_ClientGui_Widget::SetRect(const CModAPI_ClientGui_Rect& Rect)
{
	m_Rect = Rect;
}

void CModAPI_ClientGui_Widget::SetX(int x)
{
	SetRect(CModAPI_ClientGui_Rect(
		x,
		m_Rect.y,
		m_Rect.w,
		m_Rect.h	
	));
}

void CModAPI_ClientGui_Widget::SetY(int y)
{
	SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x,
		y,
		m_Rect.w,
		m_Rect.h	
	));
}

void CModAPI_ClientGui_Widget::SetWidth(int w)
{
	SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x,
		m_Rect.y,
		w,
		m_Rect.h	
	));
}

void CModAPI_ClientGui_Widget::SetHeight(int h)
{
	SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x,
		m_Rect.y,
		m_Rect.w,	
		h
	));
}

void CModAPI_ClientGui_Widget::OnMouseOver(int X, int Y, int KeyState)
{
	
}

void CModAPI_ClientGui_Widget::OnMouseMotion(int X, int Y, int KeyState)
{
	
}

void CModAPI_ClientGui_Widget::OnMouseButtonClick(int X, int Y)
{
	
}

void CModAPI_ClientGui_Widget::OnMouseButtonRelease()
{
	
}

void CModAPI_ClientGui_Widget::OnInputEvent()
{
	
}

float CModAPI_ClientGui_Widget::s_LayoutOpacity = 0.25f;
float CModAPI_ClientGui_Widget::s_LayoutCornerRadius = 5.0f;

int CModAPI_ClientGui_Widget::s_ButtonInnerMargin = 5;
float CModAPI_ClientGui_Widget::s_ButtonOpacity = 0.25f;
float CModAPI_ClientGui_Widget::s_ButtonCornerRadius = 5.0f;
