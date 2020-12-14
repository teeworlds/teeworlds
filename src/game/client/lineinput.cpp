/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/keys.h>
#include <engine/input.h>
#include <engine/textrender.h>
#include <engine/graphics.h>

#include "lineinput.h"

IInput *CLineInput::s_pInput = 0;
ITextRender *CLineInput::s_pTextRender = 0;
IGraphics *CLineInput::s_pGraphics = 0;

CLineInput *CLineInput::s_apActiveInputs[MAX_ACTIVE_INPUTS] = { 0 };
unsigned CLineInput::s_NumActiveInputs = 0;

void CLineInput::SetBuffer(char *pStr, int MaxSize, int MaxChars)
{
	if(m_pStr && m_pStr == pStr)
		return;
	const char *pLastStr = m_pStr;
	m_pStr = pStr;
	m_MaxSize = MaxSize;
	m_MaxChars = MaxChars;
	m_WasChanged = m_pStr && pLastStr && m_WasChanged;
	if(!pLastStr)
	{
		m_ScrollOffset = 0;
	}
	if(m_pStr && m_pStr != pLastStr)
	{
		UpdateStrData();
	}
}

void CLineInput::Clear()
{
	mem_zero(m_pStr, m_MaxSize);
	UpdateStrData();
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_pStr, pString, m_MaxSize);
	UpdateStrData();
	SetCursorOffset(m_Len);
}

void CLineInput::SetRange(const char *pString, int Begin, int End)
{
	int RemovedCharSize, RemovedCharCount;
	str_utf8_stats(m_pStr+Begin, End-Begin+1, &RemovedCharSize, &RemovedCharCount);

	int AddedCharSize, AddedCharCount;
	str_utf8_stats(pString, m_MaxSize, &AddedCharSize, &AddedCharCount);

	if(m_Len + AddedCharSize - RemovedCharSize >= m_MaxSize || m_NumChars + AddedCharCount - RemovedCharCount > m_MaxChars)
		str_utf8_stats(pString, m_MaxSize - m_Len + RemovedCharSize, &AddedCharSize, &AddedCharCount);

	if(RemovedCharSize || AddedCharSize)
	{
		if(AddedCharSize < RemovedCharSize)
		{
			if(AddedCharSize)
				mem_copy(m_pStr + Begin, pString, AddedCharSize);
			mem_move(m_pStr + Begin + AddedCharSize, m_pStr + Begin + RemovedCharSize, m_Len - Begin - AddedCharSize);
		}
		else if(AddedCharSize > RemovedCharSize)
			mem_move(m_pStr + End + AddedCharSize - RemovedCharSize, m_pStr + End, m_Len - End);

		if(AddedCharSize >= RemovedCharSize)
			mem_copy(m_pStr + Begin, pString, AddedCharSize);

		m_CursorPos = End - RemovedCharSize + AddedCharSize;
		m_Len += AddedCharSize - RemovedCharSize;
		m_NumChars += AddedCharCount - RemovedCharCount;
		m_WasChanged = true;
		m_pStr[m_Len] = '\0';
		m_SelectionStart = m_SelectionEnd = m_CursorPos;
	}
}

void CLineInput::UpdateStrData()
{
	str_utf8_stats(m_pStr, m_MaxSize, &m_Len, &m_NumChars);
	if(m_CursorPos < 0 || m_CursorPos > m_Len)
		m_SelectionStart = m_SelectionEnd = m_CursorPos = m_Len;
}

bool CLineInput::MoveWordStop(char c)
{
	// jump to spaces and special ASCII characters
	return ((32 <= c && c <= 47) || //  !"#$%&'()*+,-./
			(58 <= c && c <= 64) || // :;<=>?@
			(91 <= c && c <= 96));  // [\]^_`
}

void CLineInput::SetCursorOffset(int Offset)
{
	m_SelectionStart = m_SelectionEnd = m_CursorPos = clamp(Offset, 0, m_Len);
}

void CLineInput::SetSelection(int Start, int End)
{
	if(Start > End)
	{
		int Temp = Start;
		Start = End;
		End = Temp;
	}
	m_SelectionStart = clamp(Start, 0, m_Len);
	m_SelectionEnd = clamp(End, 0, m_Len);
}

bool CLineInput::ProcessInput(const IInput::CEvent &Event)
{
	// update derived attributes to handle external changes to the buffer
	UpdateStrData();

	const int OldCursorPos = m_CursorPos;
	const bool Selecting = s_pInput->KeyIsPressed(KEY_LSHIFT) || s_pInput->KeyIsPressed(KEY_RSHIFT);
	const int SelectionLength = GetSelectionLength();

	if(Event.m_Flags&IInput::FLAG_TEXT && !(KEY_LCTRL <= Event.m_Key && Event.m_Key <= KEY_RGUI))
	{
		if(SelectionLength)
			SetRange(Event.m_aText, m_SelectionStart, m_SelectionEnd);
		else
			Append(Event.m_aText);
	}

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		const bool CtrlPressed = s_pInput->KeyIsPressed(KEY_LCTRL) || s_pInput->KeyIsPressed(KEY_RCTRL);

#ifdef CONF_PLATFORM_MACOSX
		const bool MoveWord = s_pInput->KeyIsPressed(KEY_LALT) || s_pInput->KeyIsPressed(KEY_RALT);
#else
		const bool MoveWord = CtrlPressed;
#endif

		if(Event.m_Key == KEY_BACKSPACE)
		{
			if(SelectionLength)
			{
				SetRange("", m_SelectionStart, m_SelectionEnd);
			}
			else
			{
				if(m_CursorPos > 0)
				{
					int NewCursorPos = m_CursorPos;
					do
					{
						NewCursorPos = str_utf8_rewind(m_pStr, NewCursorPos);
						m_NumChars -= 1;
					} while(MoveWord && NewCursorPos > 0 && !MoveWordStop(m_pStr[NewCursorPos - 1]));
					int CharSize = m_CursorPos-NewCursorPos;
					mem_move(m_pStr+NewCursorPos, m_pStr+m_CursorPos, m_Len - NewCursorPos - CharSize + 1); // +1 == null term
					m_CursorPos = NewCursorPos;
					m_Len -= CharSize;
					m_WasChanged = true;
				}
				m_SelectionStart = m_SelectionEnd = m_CursorPos;
			}
		}
		else if(Event.m_Key == KEY_DELETE)
		{
			if(SelectionLength)
			{
				SetRange("", m_SelectionStart, m_SelectionEnd);
			}
			else
			{
				if(m_CursorPos < m_Len)
				{
					int EndCursorPos = m_CursorPos;
					do
					{
						EndCursorPos = str_utf8_forward(m_pStr, EndCursorPos);
						m_NumChars -= 1;
					} while(MoveWord && EndCursorPos < m_Len && !MoveWordStop(m_pStr[EndCursorPos - 1]));
					int CharSize = EndCursorPos - m_CursorPos;
					mem_move(m_pStr + m_CursorPos, m_pStr + m_CursorPos + CharSize, m_Len - m_CursorPos - CharSize + 1); // +1 == null term
					m_Len -= CharSize;
					m_WasChanged = true;
				}
				m_SelectionStart = m_SelectionEnd = m_CursorPos;
			}
		}
		else if(Event.m_Key == KEY_LEFT)
		{
			if(SelectionLength && !Selecting)
			{
				m_CursorPos = m_SelectionStart;
			}
			else if(m_CursorPos > 0)
			{
				do
				{
					m_CursorPos = str_utf8_rewind(m_pStr, m_CursorPos);
				} while(MoveWord && m_CursorPos > 0 && !MoveWordStop(m_pStr[m_CursorPos - 1]));

				if(Selecting)
				{
					if(m_SelectionStart == OldCursorPos) // expand start first
						m_SelectionStart = m_CursorPos;
					else if(m_SelectionEnd == OldCursorPos)
						m_SelectionEnd = m_CursorPos;
				}
			}

			if(!Selecting)
				m_SelectionStart = m_SelectionEnd = m_CursorPos;
		}
		else if(Event.m_Key == KEY_RIGHT)
		{
			if(SelectionLength && !Selecting)
			{
				m_CursorPos = m_SelectionEnd;
			}
			else if(m_CursorPos < m_Len)
			{
				do
				{
					m_CursorPos = str_utf8_forward(m_pStr, m_CursorPos);
				} while(MoveWord && m_CursorPos < m_Len && !MoveWordStop(m_pStr[m_CursorPos - 1]));

				if(Selecting)
				{
					if(m_SelectionEnd == OldCursorPos) // expand end first
						m_SelectionEnd = m_CursorPos;
					else if(m_SelectionStart == OldCursorPos)
						m_SelectionStart = m_CursorPos;
				}
			}

			if(!Selecting)
				m_SelectionStart = m_SelectionEnd = m_CursorPos;
		}
		else if(Event.m_Key == KEY_HOME)
		{
			m_CursorPos = 0;
			m_SelectionStart = 0;
			if(!Selecting)
				m_SelectionEnd = 0;
		}
		else if(Event.m_Key == KEY_END)
		{
			m_CursorPos = m_Len;
			m_SelectionEnd = m_Len;
			if(!Selecting)
				m_SelectionStart = m_Len;
		}
		else if(CtrlPressed && Event.m_Key == KEY_V)
		{
			const char *pClipboardText = s_pInput->GetClipboardText();
			if(pClipboardText)
			{
				if(SelectionLength)
					SetRange(pClipboardText, m_SelectionStart, m_SelectionEnd);
				else
					Append(pClipboardText);
			}
		}
		else if(CtrlPressed && (Event.m_Key == KEY_C || Event.m_Key == KEY_X) && SelectionLength)
		{
			char *pSelection = m_pStr+m_SelectionStart;
			char TempChar = pSelection[SelectionLength];
			pSelection[SelectionLength] = 0;
			s_pInput->SetClipboardText(pSelection);
			pSelection[SelectionLength] = TempChar;
			if(Event.m_Key == KEY_X)
				SetRange("", m_SelectionStart, m_SelectionEnd);
		}
		else if(CtrlPressed && Event.m_Key == KEY_A)
		{
			m_SelectionStart = 0;
			m_SelectionEnd = m_CursorPos = m_Len;
		}
	}

	m_WasChanged |= OldCursorPos != m_CursorPos;
	m_WasChanged |= SelectionLength != GetSelectionLength();
	return m_WasChanged;
}

void CLineInput::Render(CTextCursor *pCursor)
{
	s_pTextRender->DrawTextOutlined(pCursor);

	if(IsActive())
	{
		const int VAlign = pCursor->m_Align&TEXTALIGN_MASK_VERT;

		// render selection
		if(GetSelectionLength())
		{
			const vec2 StartPos = s_pTextRender->CaretPosition(pCursor, GetSelectionStart());
			const vec2 EndPos = s_pTextRender->CaretPosition(pCursor, GetSelectionEnd());
			const float LineHeight = pCursor->BaseLineY()/pCursor->LineCount();
			const float VAlignOffset =
				(VAlign == TEXTALIGN_TOP) ? -1.0f :
				(VAlign == TEXTALIGN_MIDDLE) ? LineHeight/2 :
				/* TEXTALIGN_BOTTOM */ LineHeight - 1.0f;
			s_pGraphics->TextureClear();
			s_pGraphics->QuadsBegin();
			s_pGraphics->SetColor(0.3f, 0.3f, 0.3f, 0.3f);
			if(StartPos.y < EndPos.y) // multi line selection
			{
				CTextBoundingBox BoundingBox = pCursor->BoundingBox();
				int NumQuads = 0;
				IGraphics::CQuadItem SelectionQuads[3];
				SelectionQuads[NumQuads++] = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, BoundingBox.Right() - StartPos.x, LineHeight);
				const float SecondSegmentY = StartPos.y - VAlignOffset + LineHeight;
				if(EndPos.y - StartPos.y > LineHeight)
				{
					const float MiddleSegmentHeight = EndPos.y - StartPos.y - LineHeight;
					SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, BoundingBox.w, MiddleSegmentHeight);
					SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY + MiddleSegmentHeight, EndPos.x - BoundingBox.x, LineHeight);
				}
				else
					SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, EndPos.x - BoundingBox.x, LineHeight);
				s_pGraphics->QuadsDrawTL(SelectionQuads, NumQuads);
			}
			else // single line selection
			{
				IGraphics::CQuadItem SelectionQuad = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, EndPos.x - StartPos.x, LineHeight);
				s_pGraphics->QuadsDrawTL(&SelectionQuad, 1);
			}
			s_pGraphics->QuadsEnd();
		}

		// render blinking caret
		if((2*time_get()/time_freq())%2)
		{
			static CTextCursor s_MarkerCursor;
			s_MarkerCursor.Reset();
			s_MarkerCursor.m_FontSize = pCursor->m_FontSize;
			s_MarkerCursor.m_Align = VAlign | TEXTALIGN_CENTER;
			s_pTextRender->TextDeferred(&s_MarkerCursor, "｜", -1);
			s_MarkerCursor.MoveTo(s_pTextRender->CaretPosition(pCursor, GetCursorOffset()));
			s_pTextRender->DrawTextOutlined(&s_MarkerCursor);
		}
	}
}

void CLineInput::SetActive(bool Active)
{
	if(Active)
	{
		if(IsActive())
			return;
		// Try to restore active input from stack
		for(unsigned i = 0; i < s_NumActiveInputs; i++)
		{
			if(s_apActiveInputs[i] == this)
			{
				// Deactivate inputs above in the stack
				for(unsigned j = s_NumActiveInputs-1; j >= i+1; j--)
				{
					if(s_apActiveInputs[j])
					{
						s_apActiveInputs[j]->OnDeactivate();
						s_apActiveInputs[j] = 0;
					}
				}
				s_NumActiveInputs = i+1;
				OnActivate();
				return;
			}
		}
		if(s_NumActiveInputs > 0)
			s_apActiveInputs[s_NumActiveInputs-1]->OnDeactivate();
		// Activate new input
		dbg_assert(s_NumActiveInputs < MAX_ACTIVE_INPUTS, "max number of active line inputs exceeded");
		s_apActiveInputs[s_NumActiveInputs] = this;
		s_NumActiveInputs++;
		OnActivate();
	}
	else
	{
		if(!IsActive() || s_NumActiveInputs == 0)
			return;
		s_apActiveInputs[s_NumActiveInputs] = 0;
		s_NumActiveInputs--;
		OnDeactivate();
		if(s_NumActiveInputs > 0)
			s_apActiveInputs[s_NumActiveInputs-1]->OnActivate();
	}
}

void CLineInput::OnActivate()
{
}

void CLineInput::OnDeactivate()
{
}
