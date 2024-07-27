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
IClient *CLineInput::s_pClient = 0;

CLineInput *CLineInput::s_pActiveInput = 0;
EInputPriority CLineInput::s_ActiveInputPriority = NONE;

vec2 CLineInput::s_CompositionWindowPosition = vec2(0, 0);
float CLineInput::s_CompositionLineHeight = 0.0f;

char CLineInput::s_aStars[128] = { '\0' };

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
		m_CursorPos = m_SelectionStart = m_SelectionEnd = 0;
		m_ScrollOffset = m_ScrollOffsetChange = 0.0f;
		m_CaretPosition = vec2(0, 0);
		m_Hidden = false;
		m_WasRendered = false;
	}
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

const char *CLineInput::GetDisplayedString()
{
	if(!IsHidden())
		return m_pStr;

	unsigned NumStars = GetNumChars();
	if(NumStars >= sizeof(s_aStars))
		NumStars = sizeof(s_aStars)-1;
	for(unsigned int i = 0; i < NumStars; ++i)
		s_aStars[i] = '*';
	s_aStars[NumStars] = '\0';
	return s_aStars;
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

int CLineInput::OffsetFromActualToDisplay(int ActualOffset) const
{
	if(!IsHidden())
		return ActualOffset;
	int DisplayOffset = 0;
	int CurrentOffset = 0;
	while(CurrentOffset < ActualOffset)
	{
		const int PrevOffset = CurrentOffset;
		CurrentOffset = str_utf8_forward(m_pStr, CurrentOffset);
		if(CurrentOffset == PrevOffset)
			break;
		DisplayOffset++;
	}
	return DisplayOffset;
}

int CLineInput::OffsetFromDisplayToActual(int DisplayOffset) const
{
	if(!IsHidden())
		return DisplayOffset;
	int ActualOffset = 0;
	for(int i = 0; i < DisplayOffset; i++)
	{
		const int PrevOffset = ActualOffset;
		ActualOffset = str_utf8_forward(m_pStr, ActualOffset);
		if(ActualOffset == PrevOffset)
			break;
	}
	return ActualOffset;
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
		const bool AltPressed = s_pInput->KeyIsPressed(KEY_LALT) || s_pInput->KeyIsPressed(KEY_RALT);

#ifdef CONF_PLATFORM_MACOS
		const bool MoveWord = AltPressed && !CtrlPressed;
#else
		const bool MoveWord = CtrlPressed && !AltPressed;
#endif

		if(Event.m_Key == KEY_BACKSPACE)
		{
			if(SelectionLength)
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
			if(SelectionLength)
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
		else if(CtrlPressed && !AltPressed && Event.m_Key == KEY_V)
		{
			const char *pClipboardText = s_pInput->GetClipboardText();
			if(pClipboardText)
				SetRange(pClipboardText, m_SelectionStart, m_SelectionEnd);
		}
		else if(CtrlPressed && !AltPressed  && (Event.m_Key == KEY_C || Event.m_Key == KEY_X) && SelectionLength)
		{
			char *pSelection = m_pStr + m_SelectionStart;
			char TempChar = pSelection[SelectionLength];
			pSelection[SelectionLength] = '\0';
			s_pInput->SetClipboardText(pSelection);
			pSelection[SelectionLength] = TempChar;
			if(Event.m_Key == KEY_X)
				SetRange("", m_SelectionStart, m_SelectionEnd);
		}
		else if(CtrlPressed && !AltPressed  && Event.m_Key == KEY_A)
		{
			m_SelectionStart = 0;
			m_SelectionEnd = m_CursorPos = m_Len;
		}
	}

	m_WasChanged |= OldCursorPos != m_CursorPos;
	m_WasChanged |= SelectionLength != GetSelectionLength();
	return m_WasChanged;
}

void CLineInput::Render(bool Changed)
{
	m_WasRendered = true;
	m_TextCursor.Reset();

	if(!m_pStr)
		return;

	const char *pDisplayStr = GetDisplayedString();

	if(IsActive())
	{
		const int CursorOffset = GetCursorOffset();
		const int DisplayCursorOffset = OffsetFromActualToDisplay(CursorOffset);
		const bool HasComposition = s_pInput->HasComposition();
		const int CompositionStart = CursorOffset + s_pInput->GetCompositionCursor();
		const int DisplayCompositionStart = OffsetFromActualToDisplay(CompositionStart);

		if(HasComposition)
		{
			const int DisplayCompositionEnd = DisplayCursorOffset + s_pInput->GetCompositionLength();
			s_pTextRender->TextDeferred(&m_TextCursor, pDisplayStr, DisplayCursorOffset);
			s_pTextRender->TextDeferred(&m_TextCursor, s_pInput->GetComposition(), -1);
			s_pTextRender->TextDeferred(&m_TextCursor, pDisplayStr + DisplayCursorOffset, -1);
			s_pTextRender->DrawTextOutlined(&m_TextCursor);
			DrawSelection(0.1f, DisplayCursorOffset, DisplayCompositionEnd, vec4(0.7f, 0.7f, 0.7f, 0.7f));
		}
		else
		{
			s_pTextRender->TextOutlined(&m_TextCursor, pDisplayStr, -1);
		}

		// render selection
		if(GetSelectionLength() || HasComposition)
		{
			const int DisplayCompositionEnd = DisplayCompositionStart + s_pInput->GetCompositionSelectedLength();
			const int Start = HasComposition ? DisplayCompositionStart : OffsetFromActualToDisplay(GetSelectionStart());
			const int End = HasComposition ? DisplayCompositionEnd : OffsetFromActualToDisplay(GetSelectionEnd());
			DrawSelection(1.0f, Start, End, vec4(0.3f, 0.3f, 0.3f, 0.3f));
		}

		static CTextCursor s_MarkerCursor;
		s_MarkerCursor.m_FontSize = m_TextCursor.m_FontSize;
		s_MarkerCursor.Reset(s_MarkerCursor.m_FontSize);
		s_MarkerCursor.m_Align = (m_TextCursor.m_Align&TEXTALIGN_MASK_VERT) | TEXTALIGN_CENTER;
		s_pTextRender->TextDeferred(&s_MarkerCursor, "｜", -1);
		s_MarkerCursor.MoveTo(s_pTextRender->CaretPosition(&m_TextCursor, HasComposition ? DisplayCompositionStart : DisplayCursorOffset));

		// render blinking caret, don't blink shortly after caret has been moved
		{
			const float LocalTime = s_pClient->LocalTime();
			static float s_LastChanged = 0.0f;
			if(Changed)
				s_LastChanged = LocalTime;
			if(fmod(LocalTime - s_LastChanged, 1.0f) < 0.5f)
				s_pTextRender->DrawTextOutlined(&s_MarkerCursor);
		}

		m_CaretPosition = s_pTextRender->CaretPosition(&m_TextCursor, DisplayCursorOffset);
		s_MarkerCursor.MoveTo(m_CaretPosition);
		CTextBoundingBox BoundingBox = s_MarkerCursor.BoundingBox();
		SetCompositionWindowPosition(vec2(BoundingBox.Right(), BoundingBox.Bottom()), BoundingBox.h);
	}
	else
	{
		s_pTextRender->TextOutlined(&m_TextCursor, pDisplayStr, -1);
	}
}

void CLineInput::RenderCandidates()
{
	// Check if the active line input was not rendered and deactivate it in that case.
	// This can happen e.g. when an input in the ingame menu is active and the menu is
	// closed or when switching between menu and editor with an active input.
	CLineInput *pActiveInput = GetActiveInput();
	if(pActiveInput != nullptr)
	{
		if(pActiveInput->m_WasRendered)
		{
			pActiveInput->m_WasRendered = false;
		}
		else
		{
			pActiveInput->Deactivate();
			return;
		}
	}

	if(!s_pInput->HasComposition() || !s_pInput->GetCandidateCount())
		return;

	const float FontSize = 7.0f;
	const float HMargin = 8.0f;
	const float VMargin = 4.0f;
	const float Height = 300;
	const float Width = Height*s_pGraphics->ScreenAspect();
	const int ScreenWidth = s_pGraphics->ScreenWidth();
	const int ScreenHeight = s_pGraphics->ScreenHeight();

	s_pGraphics->MapScreen(0, 0, Width, Height);

	static CTextCursor s_CandidateCursor;
	s_CandidateCursor.Reset();
	s_CandidateCursor.m_FontSize = FontSize;
	s_CandidateCursor.m_LineSpacing = FontSize*0.35f;
	s_CandidateCursor.m_MaxLines = -1;

	vec2 Position = s_CompositionWindowPosition / vec2(ScreenWidth, ScreenHeight) * vec2(Width, Height);
	float SelectedCandidateY = 0;
	for(int i = 0; i < s_pInput->GetCandidateCount(); ++i)
	{
		if(i == s_pInput->GetCandidateSelectedIndex())
			SelectedCandidateY = s_CandidateCursor.Height();

		if(i > 0)
			s_pTextRender->TextNewline(&s_CandidateCursor);

		char aBuf[8];
		str_format(aBuf, sizeof(aBuf), "%d. ", (i+1)%10);

		s_pTextRender->TextColor(0.6f, 0.6f, 0.6f, 1.0f);
		s_pTextRender->TextDeferred(&s_CandidateCursor, aBuf, -1);
		s_pTextRender->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		s_pTextRender->TextDeferred(&s_CandidateCursor, s_pInput->GetCandidate(i), -1);
	}

	CTextBoundingBox BoundingBox = s_CandidateCursor.BoundingBox();
	BoundingBox.x = Position.x;
	BoundingBox.y = Position.y;
	BoundingBox.w += HMargin;
	BoundingBox.h += VMargin;

	// move candidate window up if needed
	if(BoundingBox.y + FontSize * 13.5f > Height)
		BoundingBox.y -= BoundingBox.h + s_CompositionLineHeight / ScreenHeight * Height;

	// move candidate window left if needed
	if(BoundingBox.x + BoundingBox.w + HMargin > Width)
		BoundingBox.x -= BoundingBox.x + BoundingBox.w + HMargin - Width;

	s_CandidateCursor.MoveTo(vec2(BoundingBox.x+HMargin/2, BoundingBox.y+VMargin/2));

	s_pGraphics->TextureClear();
	s_pGraphics->QuadsBegin();
	s_pGraphics->BlendNormal();

	// window shadow
	s_pGraphics->SetColor(0.0f, 0.0f, 0.0f, 0.8f);
	IGraphics::CQuadItem Quad = IGraphics::CQuadItem(BoundingBox.x+0.75f, BoundingBox.y+0.75f, BoundingBox.w, BoundingBox.h);
	s_pGraphics->QuadsDrawTL(&Quad, 1);

	// window background
	s_pGraphics->SetColor(0.15f, 0.15f, 0.15f, 1.0f);
	Quad = IGraphics::CQuadItem(BoundingBox.x, BoundingBox.y, BoundingBox.w, BoundingBox.h);
	s_pGraphics->QuadsDrawTL(&Quad, 1);

	// highlight
	s_pGraphics->SetColor(0.1f, 0.4f, 0.8f, 1.0f);
	Quad = IGraphics::CQuadItem(BoundingBox.x+HMargin/4, BoundingBox.y+VMargin/2+SelectedCandidateY, BoundingBox.w-HMargin/2, FontSize*1.35f);
	s_pGraphics->QuadsDrawTL(&Quad, 1);
	s_pGraphics->QuadsEnd();

	s_pTextRender->DrawTextOutlined(&s_CandidateCursor);
}

void CLineInput::DrawSelection(float HeightWeight, int Start, int End, vec4 Color)
{
	const int VAlign = m_TextCursor.m_Align&TEXTALIGN_MASK_VERT;

	const vec2 StartPos = s_pTextRender->CaretPosition(&m_TextCursor, Start);
	const vec2 EndPos = s_pTextRender->CaretPosition(&m_TextCursor, End);
	const float LineHeight = m_TextCursor.BaseLineY()/m_TextCursor.LineCount();
	const float VAlignOffset =
		(VAlign == TEXTALIGN_TOP) ? -LineHeight*(1.0f-HeightWeight)-1.0f :
		(VAlign == TEXTALIGN_MIDDLE) ? -LineHeight*(1.0f-HeightWeight)+LineHeight/2 :
		/* TEXTALIGN_BOTTOM */ LineHeight*(1.35f-1.0f+HeightWeight)-1.0f;
	s_pGraphics->TextureClear();
	s_pGraphics->QuadsBegin();
	s_pGraphics->SetColor(Color);
	if(StartPos.y < EndPos.y) // multi line selection
	{
		CTextBoundingBox BoundingBox = m_TextCursor.BoundingBox();
		int NumQuads = 0;
		IGraphics::CQuadItem aSelectionQuads[3];
		aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, BoundingBox.Right() - StartPos.x, LineHeight*HeightWeight);
		const float SecondSegmentY = StartPos.y - VAlignOffset + LineHeight;
		if(EndPos.y - StartPos.y > LineHeight)
		{
			const float MiddleSegmentHeight = EndPos.y - StartPos.y - LineHeight;
			aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, BoundingBox.w, MiddleSegmentHeight);
			aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY + MiddleSegmentHeight, EndPos.x - BoundingBox.x, LineHeight*HeightWeight);
		}
		else
			aSelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, EndPos.x - BoundingBox.x, LineHeight*HeightWeight);
		s_pGraphics->QuadsDrawTL(aSelectionQuads, NumQuads);
	}
	else // single line selection
	{
		IGraphics::CQuadItem SelectionQuad(StartPos.x, StartPos.y - VAlignOffset, EndPos.x - StartPos.x, LineHeight*HeightWeight);
		s_pGraphics->QuadsDrawTL(&SelectionQuad, 1);
	}
	s_pGraphics->QuadsEnd();
}

void CLineInput::SetCompositionWindowPosition(vec2 Anchor, float LineHeight)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	const int ScreenWidth = s_pGraphics->ScreenWidth();
	const int ScreenHeight = s_pGraphics->ScreenHeight();
	s_pGraphics->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth / (ScreenX1 - ScreenX0), ScreenHeight / (ScreenY1 - ScreenY0));
	s_CompositionWindowPosition = Anchor * ScreenScale;
	s_CompositionLineHeight = LineHeight * ScreenScale.y;
	s_pInput->SetCompositionWindowPosition(s_CompositionWindowPosition.x, s_CompositionWindowPosition.y - s_CompositionLineHeight, s_CompositionLineHeight);
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
	s_pInput->StartTextInput();
}

void CLineInput::OnDeactivate()
{
	s_pInput->StopTextInput();
}
