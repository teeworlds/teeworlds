/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/keys.h>
#include "lineinput.h"

CLineInput::CLineInput()
{
	Clear();
}

void CLineInput::Clear()
{
	mem_zero(m_Str, sizeof(m_Str));
	m_Len = 0;
	m_CursorPos = 0;
	m_NumChars = 0;
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

bool CLineInput::Manipulate(IInput::CEvent Event, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

	if(Event.m_Flags&IInput::FLAG_TEXT)
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
		if(Key == KEY_BACKSPACE && CursorPos > 0)
		{
			int NewCursorPos = str_utf8_rewind(pStr, CursorPos);
			int CharSize = CursorPos-NewCursorPos;
			mem_move(pStr+NewCursorPos, pStr+CursorPos, Len - NewCursorPos - CharSize + 1); // +1 == null term
			CursorPos = NewCursorPos;
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if(Key == KEY_DELETE && CursorPos < Len)
		{
			int p = str_utf8_forward(pStr, CursorPos);
			int CharSize = p-CursorPos;
			mem_move(pStr + CursorPos, pStr + CursorPos + CharSize, Len - CursorPos - CharSize + 1); // +1 == null term
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if(Key == KEY_LEFT && CursorPos > 0)
			CursorPos = str_utf8_rewind(pStr, CursorPos);
		else if(Key == KEY_RIGHT && CursorPos < Len)
			CursorPos = str_utf8_forward(pStr, CursorPos);
		else if(Key == KEY_HOME)
			CursorPos = 0;
		else if(Key == KEY_END)
			CursorPos = Len;
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

bool CLineInput::ProcessInput(IInput::CEvent e)
{
	return Manipulate(e, m_Str, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_NumChars);
}
