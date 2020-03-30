/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/keys.h>
#include <engine/input.h>
#include "lineinput.h"

CLineInput::CLineInput()
{
	Clear();
	m_pInput = 0;
}

void CLineInput::Clear()
{
	mem_zero(m_Str, sizeof(m_Str));
	m_Len = 0;
	m_CursorPos = 0;
	m_SelectionStart = 0;
	m_NumChars = 0;
}

void CLineInput::Init(IInput *pInput)
{
	m_pInput = pInput;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_Str, pString, sizeof(m_Str));
	m_Len = str_length(m_Str);
	m_CursorPos = m_Len;
	m_NumChars = 0;
	int Offset = 0;
	while(pString[Offset])
	{
		Offset = str_utf8_forward(pString, Offset);
		++m_NumChars;
	}
}

void CLineInput::GetSelection(char *pBuf, int BufSize) const
{
	int Len = m_CursorPos - m_SelectionStart;
	str_format(pBuf, BufSize, "%.*s", absolute(Len), &m_Str[Len > 0 ? m_SelectionStart : m_CursorPos]);
}

bool CLineInput::MoveWordStop(char c)
{
	// jump to spaces and special ASCII characters
	return ((32 <= c && c <= 47) || //  !"#$%&'()*+,-./
			(58 <= c && c <= 64) || // :;<=>?@
			(91 <= c && c <= 96));  // [\]^_`
}

bool CLineInput::Manipulate(IInput::CEvent Event, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pSelectionStart, int *pNumCharsPtr, IInput *pInput)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int SelectionStart = *pSelectionStart;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

	int SelectionLeft = min(CursorPos, SelectionStart);
	int SelectionLength = absolute(CursorPos - SelectionStart);

	if(Event.m_Flags&IInput::FLAG_TEXT &&
		!(KEY_LCTRL <= Event.m_Key && Event.m_Key <= KEY_RGUI))
	{
		// gather string stats
		int CharCount = 0;
		int CharSize = 0;
		while(Event.m_aText[CharSize])
		{
			int NewCharSize = str_utf8_forward(Event.m_aText, CharSize);
			if(NewCharSize != CharSize)
			{
				++CharCount;
				CharSize = NewCharSize;
			}
		}

		// if it doesn't fit, don't do anything
		if(Len - (SelectionLength ? SelectionLength : 1) + CharSize >= StrMaxSize)
			return Changes;

		if(SelectionLength)
		{
			NumChars -= str_utf8_charcount(pStr + SelectionLeft, SelectionLength);
			str_remove_segment(pStr, SelectionLeft, SelectionLeft + SelectionLength, Len);
			CursorPos = SelectionLeft;
			Len -= SelectionLength;
		}

		// add new string
		if(CharCount)
		{
			if(Len+CharSize < StrMaxSize && CursorPos+CharSize < StrMaxSize && NumChars+CharCount < StrMaxChars)
			{
				mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len-CursorPos+1); // +1 == null term
				for(int i = 0; i < CharSize; i++)
					pStr[CursorPos+i] = Event.m_aText[i];
				CursorPos += CharSize;
				Len += CharSize;
				NumChars += CharCount;
				Changes = true;
			}
		}
	}

	bool Selecting = false;
	bool Deselect = false;
	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		int Key = Event.m_Key;
		bool MoveWord = false;
#ifdef CONF_PLATFORM_MACOSX
		if(pInput && (pInput->KeyIsPressed(KEY_LALT) || pInput->KeyIsPressed(KEY_RALT)))
#else
		if(pInput && (pInput->KeyIsPressed(KEY_LCTRL) || pInput->KeyIsPressed(KEY_RCTRL)))
#endif
			MoveWord = true;

		if(Key == KEY_BACKSPACE && (SelectionLength || CursorPos > 0))
		{
			int SegmentLeft = CursorPos, SegmentRight = CursorPos;
			if(!SelectionLength)
			{
				do
				{
					SegmentLeft = str_utf8_rewind(pStr, SegmentLeft);
					NumChars -= 1;
				} while(MoveWord && SegmentLeft > 0 && !MoveWordStop(pStr[SegmentLeft - 1]));
			}
			else
			{
				SegmentLeft = SelectionLeft;
				SegmentRight = SelectionLeft + SelectionLength;
				NumChars -= str_utf8_charcount(pStr + SegmentLeft, SelectionLength);
			}

			str_remove_segment(pStr, SegmentLeft, SegmentRight, Len);
			CursorPos = SegmentLeft;
			Len -= SegmentRight - SegmentLeft;
			Changes = true;
		}
		else if(Key == KEY_DELETE && (SelectionLength || CursorPos < Len))
		{
			int SegmentLeft = CursorPos, SegmentRight = CursorPos;
			if(!SelectionLength)
			{
				do
				{
					SegmentRight = str_utf8_forward(pStr, SegmentRight);
					NumChars -= 1;
				} while(MoveWord && SegmentRight < Len && !MoveWordStop(pStr[SegmentRight - 1]));
			}
			else
			{
				SegmentLeft = SelectionLeft;
				SegmentRight = SelectionLeft + SelectionLength;
				NumChars -= str_utf8_charcount(pStr + SegmentLeft, SelectionLength);
			}

			str_remove_segment(pStr, SegmentLeft, SegmentRight, Len);
			CursorPos = SegmentLeft;
			Len -= SegmentRight - SegmentLeft;
			Changes = true;
		}
		else if(Key == KEY_LEFT)
		{
			if(CursorPos > 0)
			{
				do
				{
					CursorPos = str_utf8_rewind(pStr, CursorPos);
				} while(MoveWord && CursorPos > 0 && !MoveWordStop(pStr[CursorPos - 1]));
			}
			Deselect = true;
		}
		else if(Key == KEY_RIGHT)
		{
			if(CursorPos < Len)
			{
				do
				{
					CursorPos = str_utf8_forward(pStr, CursorPos);
				} while(MoveWord && CursorPos < Len && !MoveWordStop(pStr[CursorPos - 1]));
			}
			Deselect = true;
		}
		else if(Key == KEY_HOME)
		{
			CursorPos = 0;
			Deselect = true;
		}
		else if(Key == KEY_END)
		{
			CursorPos = Len;
			Deselect = true;
		}
		else if((pInput->KeyIsPressed(KEY_LCTRL) || pInput->KeyIsPressed(KEY_RCTRL)) && Key == KEY_V)
		{
			// paste clipboard to cursor
			const char *pClipboardText = pInput->GetClipboardText();
			if(pClipboardText)
			{
				// gather string stats
				int CharCount = 0;
				int CharSize = 0;
				while(pClipboardText[CharSize])
				{
					int NewCharSize = str_utf8_forward(pClipboardText, CharSize);
					if(NewCharSize != CharSize)
					{
						++CharCount;
						CharSize = NewCharSize;
					}
				}

				// if it doesn't fit, don't do anything
				if(Len - (SelectionLength ? SelectionLength : 1) + CharSize >= StrMaxSize)
					return Changes;

				if(SelectionLength)
				{
					NumChars -= str_utf8_charcount(pStr + SelectionLeft, SelectionLength);
					str_remove_segment(pStr, SelectionLeft, SelectionLeft + SelectionLength, Len);
					CursorPos = SelectionLeft;
					Len -= SelectionLength;
				}

				// add new string
				if(CharCount)
				{
					if(Len + CharSize < StrMaxSize && CursorPos + CharSize < StrMaxSize && NumChars + CharCount < StrMaxChars)
					{
						mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len - CursorPos + 1); // +1 == null term
						for(int i = 0; i < CharSize; i++)
							pStr[CursorPos + i] = pClipboardText[i];
						CursorPos += CharSize;
						Len += CharSize;
						NumChars += CharCount;
						Changes = true;
					}
				}
			}
		}
		else if((pInput->KeyIsPressed(KEY_LCTRL) || pInput->KeyIsPressed(KEY_RCTRL)) && (Key == KEY_C || Key == KEY_X) && SelectionLength)
		{
			char aBuf[MAX_SIZE];
			str_format(aBuf, sizeof(aBuf), "%.*s", SelectionLength, pStr + SelectionLeft);
			pInput->SetClipboardText(aBuf);

			if(Key == KEY_X)
			{
				NumChars -= str_utf8_charcount(pStr + SelectionLeft, SelectionLength);
				str_remove_segment(pStr, SelectionLeft, SelectionLeft + SelectionLength, Len);
				CursorPos = SelectionLeft;
				Len -= SelectionLength;
				Changes = true;
			}
		}
		else if((pInput->KeyIsPressed(KEY_LCTRL) || pInput->KeyIsPressed(KEY_RCTRL)) && Key == KEY_A)
		{
			SelectionStart = 0;
			CursorPos = Len;
			Selecting = true;
		}
	}

	Selecting |= !Changes && pInput && (pInput->KeyIsPressed(KEY_RSHIFT) || pInput->KeyIsPressed(KEY_LSHIFT));
	if(*pCursorPosPtr != CursorPos || Changes || Selecting || Deselect)
	{
		*pSelectionStart = Selecting ? SelectionStart : CursorPos;
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

bool CLineInput::ProcessInput(IInput::CEvent e)
{
	return Manipulate(e, m_Str, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_SelectionStart, &m_NumChars, m_pInput);
}
