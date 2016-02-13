#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/input.h>
#include <game/client/render.h>
#include <game/client/lineinput.h>

#include "text-edit.h"

CModAPI_ClientGui_AbstractTextEdit::CModAPI_ClientGui_AbstractTextEdit(CModAPI_ClientGui_Config *pConfig, int TextMaxSize, int Style) :
	CModAPI_ClientGui_Widget(pConfig),
	m_UnderMouse(false),
	m_CursorCharPos(-1),
	m_TextStyle(Style),
	m_TextMaxSize(TextMaxSize)
{	
	m_Rect.w = m_pConfig->m_ButtonHeight;
	m_Rect.h = m_pConfig->m_ButtonHeight;
}
	
void CModAPI_ClientGui_AbstractTextEdit::Render()
{
	
	CUIRect rect;
	rect.x = m_Rect.x;
	rect.y = m_Rect.y;
	rect.w = m_Rect.w;
	rect.h = m_Rect.h;
	
	if(m_UnderMouse)
		RenderTools()->DrawRoundRect(&rect, vec4(1.0f, 1.0f, 1.0f, s_ButtonOpacity), s_ButtonCornerRadius);
	else
		RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_ButtonOpacity), s_ButtonCornerRadius);
	
	char* pText = GetTextPtr();
	if(!pText)
		return;
		
	int ContentX = m_Rect.x;
	int ContentY = m_Rect.y + m_Rect.h/2;
	
	int TextWidth = 0;
	float FontSize = m_pConfig->m_TextSize[m_TextStyle];
	vec4 FontColor = m_pConfig->m_TextColor[m_TextStyle];
	
	TextWidth = TextRender()->TextWidth(0, FontSize, pText, -1);
	int PosX = ContentX;
	int PosY = ContentY - FontSize*0.7;
	
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, PosX+m_pConfig->m_LabelMargin, PosY, FontSize, TEXTFLAG_RENDER);
	TextRender()->TextColor(FontColor.x, FontColor.y, FontColor.z, FontColor.w);
	TextRender()->TextEx(&Cursor, pText, -1);
	
	// render the cursor
			// cursor position
	if(m_CursorCharPos >= 0)
	{
		float CursorPos = m_Rect.x+m_pConfig->m_LabelMargin;
		CursorPos += TextRender()->TextWidth(0, FontSize, pText, m_CursorCharPos);
		
		if((2*time_get()/time_freq()) % 2)
		{
			Graphics()->TextureClear();
			Graphics()->LinesBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			float x = CursorPos;
			float y0 = m_Rect.y + m_Rect.h/2 - FontSize*0.6;
			float y1 = m_Rect.y + m_Rect.h/2 + FontSize*0.6;
			IGraphics::CLineItem Line(x, y0, x, y1);
			Graphics()->LinesDraw(&Line, 1);
			
			Graphics()->LinesEnd();
		}
	}
}

void CModAPI_ClientGui_AbstractTextEdit::OnMouseOver(int X, int Y, int KeyState)
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

void CModAPI_ClientGui_AbstractTextEdit::OnMouseButtonClick(int X, int Y)
{
	char* pText = GetTextPtr();
	
	if(!pText)
		return;
	
	if(m_Rect.IsInside(X, Y))
	{
		m_Clicked = true;
		
		float FontSize = m_pConfig->m_TextSize[m_TextStyle];

		int TextLength = str_length(pText);
		int CursorCharPos = 0;
		float CursorPos = m_Rect.x+m_pConfig->m_LabelMargin;
		float TextWidth = 0.0f;
		for(CursorCharPos = 0; CursorCharPos <= TextLength; CursorCharPos++)
		{
			float NewTextWidth = TextRender()->TextWidth(0, FontSize, pText, CursorCharPos);
			float CharWidth = NewTextWidth - TextWidth;
			TextWidth = NewTextWidth;
			
			if(X < CursorPos + CharWidth)
			{
				if(X > CursorPos + CharWidth/2)
				{
					CursorCharPos++;
					CursorPos += CharWidth;
				}
				break;
			}
			CursorPos += CharWidth;
		}
		
		m_CursorCharPos = CursorCharPos;
	}
	else
	{
		m_Clicked = false;
		m_CursorCharPos = -1;
	}
}

void CModAPI_ClientGui_AbstractTextEdit::OnInputEvent()
{
	char* pText = GetTextPtr();
	
	if(!pText)
		return;
		
	if(m_CursorCharPos >= 0)
	{
		for(int i = 0; i < Input()->NumEvents(); i++)
		{
			int Len = str_length(pText);
			int NumChars = Len;
			int ReturnValue = CLineInput::Manipulate(Input()->GetEvent(i), pText, m_TextMaxSize, m_TextMaxSize, &Len, &m_CursorCharPos, &NumChars);
		}
	}
}
