#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <game/client/render.h>
#include <game/client/lineinput.h>

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
	CModAPI_ClientGui_IconButton(pIntegerEdit->m_pConfig, 2),
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
	m_pDecreaseButton(0),
	m_CursorCharPos(-1),
	m_UnderMouse(false),
	m_Clicked(false)
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
		
		if(m_UnderMouse)
			RenderTools()->DrawRoundRect(&rect, vec4(1.0f, 1.0f, 1.0f, s_ButtonOpacity), s_ButtonCornerRadius);
		else
			RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_ButtonOpacity), s_ButtonCornerRadius);
		
		int Value = GetValue();
		if(m_LastValue != Value)
		{
			str_format(m_aIntegerText, sizeof(m_aIntegerText), "%d", GetValue());
			m_LastValue = Value;
		}
		
		int TextSize = m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL];
		
		int ContentX = m_IntegerRect.x + m_IntegerRect.w/2;
		int ContentY = m_IntegerRect.y + m_IntegerRect.h/2;
	
		int TextWidth = TextRender()->TextWidth(0, m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL], m_aIntegerText, -1);
		int PosX = ContentX - TextWidth/2;
		int PosY = ContentY - m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL]*0.7;
		
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, PosX, PosY, TextSize, TEXTFLAG_RENDER);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextEx(&Cursor, m_aIntegerText, -1);
	
		// render the cursor
				// cursor position
		if(m_CursorCharPos >= 0)
		{
			float CursorPos = PosX;
			CursorPos += TextRender()->TextWidth(0, TextSize, m_aIntegerText, m_CursorCharPos);
			
			if((2*time_get()/time_freq()) % 2)
			{
				Graphics()->TextureClear();
				Graphics()->LinesBegin();
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				
				float x = CursorPos;
				float y0 = m_Rect.y + m_Rect.h/2 - TextSize*0.6;
				float y1 = m_Rect.y + m_Rect.h/2 + TextSize*0.6;
				IGraphics::CLineItem Line(x, y0, x, y1);
				Graphics()->LinesDraw(&Line, 1);
				
				Graphics()->LinesEnd();
			}
		}
	}
	
	m_pIncreaseButton->Render();
	m_pDecreaseButton->Render();
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnButtonClick(int X, int Y, int Button)
{
	if(Button == KEY_MOUSE_1)
	{
		if(m_IntegerRect.IsInside(X, Y))
		{
			m_Clicked = true;
			
			float FontSize = m_pConfig->m_TextSize[MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL];

			int TextLength = str_length(m_aIntegerText);
			int CursorCharPos = 0;
			float CursorPos = m_Rect.x+m_pConfig->m_LabelMargin;
			float TextWidth = 0.0f;
			for(CursorCharPos = 0; CursorCharPos <= TextLength; CursorCharPos++)
			{
				float NewTextWidth = TextRender()->TextWidth(0, FontSize, m_aIntegerText, CursorCharPos);
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
			ApplyTextEntry();
		}
	}
		
	m_pIncreaseButton->OnButtonClick(X, Y, Button);
	m_pDecreaseButton->OnButtonClick(X, Y, Button);
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnButtonRelease(int Button)
{
	m_pIncreaseButton->OnButtonRelease(Button);
	m_pDecreaseButton->OnButtonRelease(Button);
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState)
{
	if(m_IntegerRect.IsInside(X, Y))
	{
		m_UnderMouse = true;
	}
	else
	{
		m_UnderMouse = false;
	}
		
	m_pIncreaseButton->OnMouseOver(X, Y, RelX, RelY, KeyState);
	m_pDecreaseButton->OnMouseOver(X, Y, RelX, RelY, KeyState);
}

void CModAPI_ClientGui_AbstractIntegerEdit::ApplyTextEntry()
{
	m_Clicked = false;
	m_CursorCharPos = -1;
	int Value = atoi(m_aIntegerText);
	SetValue(Value);
	
	m_LastValue = Value;
	str_format(m_aIntegerText, sizeof(m_aIntegerText), "%d", Value);
}

void CModAPI_ClientGui_AbstractIntegerEdit::OnInputEvent()
{
	if(Input()->KeyIsPressed(KEY_RETURN) || Input()->KeyIsPressed(KEY_KP_ENTER))
	{
		ApplyTextEntry();
	}
	else
	{
		if(m_CursorCharPos >= 0)
		{
			for(int i = 0; i < Input()->NumEvents(); i++)
			{
				int Len = str_length(m_aIntegerText);
				int NumChars = Len;
				int ReturnValue = CLineInput::Manipulate(Input()->GetEvent(i), m_aIntegerText, 64, 64, &Len, &m_CursorCharPos, &NumChars);
			}
		}
	}
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
