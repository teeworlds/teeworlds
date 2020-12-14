/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm>

#include <engine/keys.h>
#include <engine/input.h>
#include <engine/textrender.h>
#include <engine/graphics.h>

#include "lineinput.h"

IInput *CLineInput::s_pInput = 0;
ITextRender *CLineInput::s_pTextRender = 0;
IGraphics *CLineInput::s_pGraphics = 0;

CLineInput *CLineInput::s_pActiveInput = 0;
EInputPriority CLineInput::s_ActiveInputPriority = NONE;

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
		m_ScrollOffset = 0;
	if(m_pStr && m_pStr != pLastStr)
		UpdateStrData();
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
	if(Begin > End)
		std::swap(Begin, End);
	Begin = clamp(Begin, 0, m_Len);
	End = clamp(End, 0, m_Len);

	int RemovedCharSize, RemovedCharCount;
	str_utf8_stats(m_pStr + Begin, End - Begin + 1, m_MaxChars, &RemovedCharSize, &RemovedCharCount);

	int AddedCharSize, AddedCharCount;
	str_utf8_stats(pString, m_MaxSize - m_Len + RemovedCharSize, m_MaxChars - m_NumChars + RemovedCharCount, &AddedCharSize, &AddedCharCount);

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

void CLineInput::Insert(const char *pString, int Begin)
{
	SetRange(pString, Begin, Begin);
}

void CLineInput::Append(const char *pString)
{
	Insert(pString, m_Len);
}

void CLineInput::UpdateStrData()
{
	str_utf8_stats(m_pStr, m_MaxSize, m_MaxChars, &m_Len, &m_NumChars);
	if(m_CursorPos < 0 || m_CursorPos > m_Len)
		SetCursorOffset(m_CursorPos);
}

void CLineInput::MoveCursor(EMoveDirection Direction, bool MoveWord, const char *pStr, int MaxSize, int *pCursorPos)
{
	// Check whether cursor position is initially on space or non-space character.
	// When forwarding, check character to the right of the cursor position.
	// When rewinding, check character to the left of the cursor position (rewind first).
	int PeekCursorPos = Direction == FORWARD ? *pCursorPos : str_utf8_rewind(pStr, *pCursorPos);
	const char *pTemp = pStr + PeekCursorPos;
	bool AnySpace = str_utf8_is_whitespace(str_utf8_decode(&pTemp));
	bool AnyWord = !AnySpace;
	while(true)
	{
		if(Direction == FORWARD)
			*pCursorPos = str_utf8_forward(pStr, *pCursorPos);
		else
			*pCursorPos = str_utf8_rewind(pStr, *pCursorPos);
		if(!MoveWord || *pCursorPos <= 0 || *pCursorPos >= MaxSize)
			break;
		PeekCursorPos = Direction == FORWARD ? *pCursorPos : str_utf8_rewind(pStr, *pCursorPos);
		pTemp = pStr + PeekCursorPos;
		const bool CurrentSpace = str_utf8_is_whitespace(str_utf8_decode(&pTemp));
		const bool CurrentWord = !CurrentSpace;
		if(Direction == FORWARD && AnySpace && !CurrentSpace)
			break; // Forward: Stop when next (right) character is non-space after seeing at least one space character.
		else if(Direction == REWIND && AnyWord && !CurrentWord)
			break; // Rewind: Stop when next (left) character is space after seeing at least one non-space character.
		AnySpace |= CurrentSpace;
		AnyWord |= CurrentWord;
	}
}

void CLineInput::SetCursorOffset(int Offset)
{
	m_SelectionStart = m_SelectionEnd = m_CursorPos = clamp(Offset, 0, m_Len);
}

void CLineInput::SetSelection(int Start, int End)
{
	if(Start > End)
		std::swap(Start, End);
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
		SetRange(Event.m_aText, m_SelectionStart, m_SelectionEnd);

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
			if(SelectionLength && !MoveWord)
			{
				SetRange("", m_SelectionStart, m_SelectionEnd);
			}
			else
			{
				// If in MoveWord-mode, backspace will delete the word before the selection
				if(SelectionLength)
					m_SelectionEnd = m_CursorPos = m_SelectionStart;
				if(m_CursorPos > 0)
				{
					int NewCursorPos = m_CursorPos;
					MoveCursor(REWIND, MoveWord, m_pStr, m_Len, &NewCursorPos);
					SetRange("", NewCursorPos, m_CursorPos);
				}
				m_SelectionStart = m_SelectionEnd = m_CursorPos;
			}
		}
		else if(Event.m_Key == KEY_DELETE)
		{
			if(SelectionLength && !MoveWord)
			{
				SetRange("", m_SelectionStart, m_SelectionEnd);
			}
			else
			{
				// If in MoveWord-mode, delete will delete the word after the selection
				if(SelectionLength)
					m_SelectionStart = m_CursorPos = m_SelectionEnd;
				if(m_CursorPos < m_Len)
				{
					int EndCursorPos = m_CursorPos;
					MoveCursor(FORWARD, MoveWord, m_pStr, m_Len, &EndCursorPos);
					SetRange("", m_CursorPos, EndCursorPos);
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
				MoveCursor(REWIND, MoveWord, m_pStr, m_Len, &m_CursorPos);
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
				MoveCursor(FORWARD, MoveWord, m_pStr, m_Len, &m_CursorPos);
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
			if(Selecting)
			{
				if(SelectionLength && m_CursorPos == m_SelectionEnd)
					m_SelectionEnd = m_SelectionStart;
			}
			else
				m_SelectionEnd = 0;
			m_CursorPos = 0;
			m_SelectionStart = 0;
		}
		else if(Event.m_Key == KEY_END)
		{
			if(Selecting)
			{
				if(SelectionLength && m_CursorPos == m_SelectionStart)
					m_SelectionStart = m_SelectionEnd;
			}
			else
				m_SelectionStart = m_Len;
			m_CursorPos = m_Len;
			m_SelectionEnd = m_Len;
		}
		else if(CtrlPressed && Event.m_Key == KEY_V)
		{
			const char *pClipboardText = s_pInput->GetClipboardText();
			if(pClipboardText)
				SetRange(pClipboardText, m_SelectionStart, m_SelectionEnd);
		}
		else if(CtrlPressed && (Event.m_Key == KEY_C || Event.m_Key == KEY_X) && SelectionLength)
		{
			char *pSelection = m_pStr + m_SelectionStart;
			char TempChar = pSelection[SelectionLength];
			pSelection[SelectionLength] = '\0';
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
				IGraphics::CQuadItem aSelectionQuads[3];
				aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, BoundingBox.Right() - StartPos.x, LineHeight);
				const float SecondSegmentY = StartPos.y - VAlignOffset + LineHeight;
				if(EndPos.y - StartPos.y > LineHeight)
				{
					const float MiddleSegmentHeight = EndPos.y - StartPos.y - LineHeight;
					aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, BoundingBox.w, MiddleSegmentHeight);
					aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY + MiddleSegmentHeight, EndPos.x - BoundingBox.x, LineHeight);
				}
				else
					aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, EndPos.x - BoundingBox.x, LineHeight);
				s_pGraphics->QuadsDrawTL(aSelectionQuads, NumQuads);
			}
			else // single line selection
			{
				IGraphics::CQuadItem SelectionQuad(StartPos.x, StartPos.y - VAlignOffset, EndPos.x - StartPos.x, LineHeight);
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

void CLineInput::Activate(EInputPriority Priority)
{
	if(IsActive())
		return;
	if(s_ActiveInputPriority != NONE && Priority < s_ActiveInputPriority)
		return; // do not replace a higher priority input
	if(s_pActiveInput)
		s_pActiveInput->OnDeactivate();
	s_pActiveInput = this;
	s_pActiveInput->OnActivate();
	s_ActiveInputPriority = Priority;
}

void CLineInput::Deactivate()
{
	if(!IsActive())
		return;
	s_pActiveInput->OnDeactivate();
	s_pActiveInput = 0x0;
	s_ActiveInputPriority = NONE;
}

void CLineInput::OnActivate()
{
}

void CLineInput::OnDeactivate()
{
}
