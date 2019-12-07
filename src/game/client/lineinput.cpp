/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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

bool CLineInput::MoveWordStop(char c)
{
	// jump to spaces and special ASCII characters
	return ((32 <= c && c <= 47) || //  !"#$%&'()*+,-./
			(58 <= c && c <= 64) || // :;<=>?@
			(91 <= c && c <= 96));  // [\]^_`
}

bool CLineInput::Manipulate(IInput::CEvent Event, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr, IInput *pInput)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

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
		if(Key == KEY_BACKSPACE && CursorPos > 0)
		{
			int NewCursorPos = CursorPos;
			do
			{
				NewCursorPos = str_utf8_rewind(pStr, NewCursorPos);
				NumChars -= 1;
			} while(MoveWord && NewCursorPos > 0 && !MoveWordStop(pStr[NewCursorPos - 1]));
			int CharSize = CursorPos-NewCursorPos;
			mem_move(pStr+NewCursorPos, pStr+CursorPos, Len - NewCursorPos - CharSize + 1); // +1 == null term
			CursorPos = NewCursorPos;
			Len -= CharSize;
			Changes = true;
		}
		else if(Key == KEY_DELETE && CursorPos < Len)
		{
			int EndCursorPos = CursorPos;
			do
			{
				EndCursorPos = str_utf8_forward(pStr, EndCursorPos);
				NumChars -= 1;
			} while(MoveWord && EndCursorPos < Len && !MoveWordStop(pStr[EndCursorPos - 1]));
			int CharSize = EndCursorPos - CursorPos;
			mem_move(pStr + CursorPos, pStr + CursorPos + CharSize, Len - CursorPos - CharSize + 1); // +1 == null term
			Len -= CharSize;
			Changes = true;
		}
		else if(Key == KEY_LEFT && CursorPos > 0)
		{
			do
			{
				CursorPos = str_utf8_rewind(pStr, CursorPos);
			} while(MoveWord && CursorPos > 0 && !MoveWordStop(pStr[CursorPos - 1]));
		}
		else if(Key == KEY_RIGHT && CursorPos < Len)
		{
			do
			{
				CursorPos = str_utf8_forward(pStr, CursorPos);
			} while(MoveWord && CursorPos < Len && !MoveWordStop(pStr[CursorPos - 1]));
		}
		else if(Key == KEY_HOME)
			CursorPos = 0;
		else if(Key == KEY_END)
			CursorPos = Len;
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
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

bool CLineInput::ProcessInput(IInput::CEvent e)
{
	return Manipulate(e, m_Str, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_NumChars, m_pInput);
}
