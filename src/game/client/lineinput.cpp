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

vec2 CLineInput::s_CompositionWindowPosition = vec2(0, 0);
float CLineInput::s_CompositionLineHeight = 0.0f;

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
	m_TextVersion = 0;
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
		IGraphics::CQuadItem SelectionQuads[3];
		SelectionQuads[NumQuads++] = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, BoundingBox.Right() - StartPos.x, LineHeight*HeightWeight);
		const float SecondSegmentY = StartPos.y - VAlignOffset + LineHeight;
		if(EndPos.y - StartPos.y > LineHeight)
		{
			const float MiddleSegmentHeight = EndPos.y - StartPos.y - LineHeight;
			SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, BoundingBox.w, MiddleSegmentHeight);
			SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY + MiddleSegmentHeight, EndPos.x - BoundingBox.x, LineHeight*HeightWeight);
		}
		else
			SelectionQuads[NumQuads++] = IGraphics::CQuadItem(BoundingBox.x, SecondSegmentY, EndPos.x - BoundingBox.x, LineHeight*HeightWeight);
		s_pGraphics->QuadsDrawTL(SelectionQuads, NumQuads);
	}
	else // single line selection
	{
		IGraphics::CQuadItem SelectionQuad = IGraphics::CQuadItem(StartPos.x, StartPos.y - VAlignOffset, EndPos.x - StartPos.x, LineHeight*HeightWeight);
		s_pGraphics->QuadsDrawTL(&SelectionQuad, 1);
	}
	s_pGraphics->QuadsEnd();
}

void CLineInput::SetCompositionWindowPosition(vec2 Anchor, float LineHeight)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = s_pGraphics->ScreenWidth();
	int ScreenHeight = s_pGraphics->ScreenHeight();
	s_pGraphics->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth / (ScreenX1 - ScreenX0), ScreenHeight / (ScreenY1 - ScreenY0));
	s_CompositionWindowPosition = Anchor * ScreenScale;
	s_CompositionLineHeight = LineHeight * ScreenScale.y;
	s_pInput->SetCompositionWindowPosition(s_CompositionWindowPosition.x, s_CompositionWindowPosition.y - s_CompositionLineHeight, s_CompositionLineHeight);
}

void CLineInput::Clear()
{
	mem_zero(m_pStr, m_MaxSize);
	UpdateStrData();
	m_TextVersion++;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_pStr, pString, m_MaxSize);
	UpdateStrData();
	SetCursorOffset(m_Len);
	m_TextVersion++;
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
	m_TextVersion++;
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
	m_TextVersion += m_WasChanged;

	return m_WasChanged;
}

void CLineInput::Render()
{
	if(!m_pStr)
		return;

	if(IsActive())
	{
		const int CompositionCursor = s_pInput->GetCompositionCursor();
		const bool HasComposition = s_pInput->HasComposition();
		const int CompositionStart = GetCursorOffset() + CompositionCursor;
		const int CompositionEnd = CompositionStart + s_pInput->GetCompositionSelectedLength();

		const int VAlign = m_TextCursor.m_Align&TEXTALIGN_MASK_VERT;

		if(HasComposition)
		{
			m_TextCursor.Reset(-1); // composition is dynamic
			s_pTextRender->TextDeferred(&m_TextCursor, m_pStr, GetCursorOffset());
			s_pTextRender->TextDeferred(&m_TextCursor, s_pInput->GetComposition(), -1);
			s_pTextRender->TextDeferred(&m_TextCursor, m_pStr + GetCursorOffset(), -1);
			s_pTextRender->DrawTextOutlined(&m_TextCursor);
			DrawSelection(0.1f, GetCursorOffset(), GetCursorOffset()+s_pInput->GetCompositionLength(), vec4(0.7f, 0.7f, 0.7f, 0.7f));
		}
		else
		{
			m_TextCursor.Reset(m_TextVersion);
			s_pTextRender->TextOutlined(&m_TextCursor, m_pStr, -1);
		}

		// render selection
		if(GetSelectionLength() || HasComposition)
		{
			const int Start = HasComposition ? CompositionStart : GetSelectionStart();
			const int End = HasComposition ? CompositionEnd : GetSelectionEnd();
			DrawSelection(1.0f, Start, End, vec4(0.3f, 0.3f, 0.3f, 0.3f));
		}

		static CTextCursor s_MarkerCursor;
		s_MarkerCursor.m_FontSize = m_TextCursor.m_FontSize;
		s_MarkerCursor.Reset(s_MarkerCursor.m_FontSize);
		s_MarkerCursor.m_Align = VAlign | TEXTALIGN_CENTER;
		s_pTextRender->TextDeferred(&s_MarkerCursor, "｜", -1);
		vec2 Offset = s_pTextRender->CaretPosition(&m_TextCursor, HasComposition ? CompositionStart : GetCursorOffset());
		s_MarkerCursor.MoveTo(Offset);

		// render blinking caret
		if((2*time_get()/time_freq())%2)
		{
			s_pTextRender->DrawTextOutlined(&s_MarkerCursor);
		}

		vec2 CursorPosition = s_pTextRender->CaretPosition(&m_TextCursor, GetCursorOffset());
		s_MarkerCursor.MoveTo(CursorPosition);
		CTextBoundingBox BoundingBox = s_MarkerCursor.BoundingBox();
		SetCompositionWindowPosition(vec2(BoundingBox.Right(), BoundingBox.Bottom()), BoundingBox.h);
	}
	else
	{
		m_TextCursor.Reset(m_TextVersion);
		s_pTextRender->TextOutlined(&m_TextCursor, m_pStr, -1);
	}
}

void CLineInput::RenderCandidates()
{
	if(s_pInput->HasComposition() && s_pInput->GetCandidateCount() > 0)
	{
		const float FontSize = 7.0f;
		const float HMargin = 8.0f;
		const float VMargin = 4.0f;
		const float Height = 300;
		const float Width = Height*s_pGraphics->ScreenAspect();
		int ScreenWidth = s_pGraphics->ScreenWidth();
		int ScreenHeight = s_pGraphics->ScreenHeight();

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
			char aBuf[32];

			if(i == s_pInput->GetCandidateSelectedIndex())
				SelectedCandidateY = s_CandidateCursor.Height();

			if(i > 0)
				s_pTextRender->TextNewline(&s_CandidateCursor);
			
			str_format(aBuf, 32, "%d. ", (i+1)%10);

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
	s_pInput->StartTextInput();
}

void CLineInput::OnDeactivate()
{
	s_pInput->StopTextInput();
}
