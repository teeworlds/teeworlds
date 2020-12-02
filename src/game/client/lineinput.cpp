/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/keys.h>
#include <engine/input.h>
#include "lineinput.h"

IInput *CLineInput::s_pInput = 0;

void CLineInput::SetBuffer(char *pStr, int MaxSize, int MaxChars)
{
	if(m_pStr && m_pStr == pStr)
		return;
	const char *pLastStr = pStr;
	m_pStr = pStr;
	m_MaxSize = MaxSize;
	m_MaxChars = MaxChars;
	m_ScrollOffset = 0;
	m_WasChanged = m_pStr && pLastStr && m_WasChanged;
	if(m_pStr)
		UpdateStrData();
}

void CLineInput::Clear()
{
	mem_zero(m_pStr, m_MaxSize);
	m_Len = 0;
	m_CursorPos = 0;
	m_NumChars = 0;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_pStr, pString, m_MaxSize);
	UpdateStrData();
}

void CLineInput::Append(const char *pString)
{
	// gather string stats
	int CharCount = 0;
	int CharSize = 0;
	while(pString[CharSize])
	{
		int NewCharSize = str_utf8_forward(pString, CharSize);
		if(NewCharSize != CharSize)
		{
			++CharCount;
			CharSize = NewCharSize;
		}
	}

	// add new string
	if(CharCount && m_Len + CharSize < m_MaxSize && m_CursorPos + CharSize < m_MaxSize && m_NumChars + CharCount <= m_MaxChars)
	{
		mem_move(m_pStr + m_CursorPos + CharSize, m_pStr + m_CursorPos, m_Len - m_CursorPos + 1); // +1 == null term
		for(int i = 0; i < CharSize; i++)
			m_pStr[m_CursorPos + i] = pString[i];
		m_CursorPos += CharSize;
		m_Len += CharSize;
		m_NumChars += CharCount;
		m_WasChanged = true;
	}
}

void CLineInput::UpdateStrData()
{
	m_Len = str_length(m_pStr);
	m_CursorPos = m_Len;
	m_NumChars = 0;
	int Offset = 0;
	while(m_pStr[Offset])
	{
		Offset = str_utf8_forward(m_pStr, Offset);
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

bool CLineInput::ProcessInput(const IInput::CEvent &Event)
{
	if(m_pStr[0] == 0 && m_Len > 0)
		UpdateStrData();
	if(m_CursorPos > m_Len)
		m_CursorPos = m_Len;

	int OldCursorPos = m_CursorPos;

	if(Event.m_Flags&IInput::FLAG_TEXT && !(KEY_LCTRL <= Event.m_Key && Event.m_Key <= KEY_RGUI))
		Append(Event.m_aText);

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		int Key = Event.m_Key;

		bool MoveWord = false;
#ifdef CONF_PLATFORM_MACOSX
		if(s_pInput->KeyIsPressed(KEY_LALT) || s_pInput->KeyIsPressed(KEY_RALT))
#else
		if(s_pInput->KeyIsPressed(KEY_LCTRL) || s_pInput->KeyIsPressed(KEY_RCTRL))
#endif
			MoveWord = true;

		if(Key == KEY_BACKSPACE && m_CursorPos > 0)
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
		else if(Key == KEY_DELETE && m_CursorPos < m_Len)
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
		else if(Key == KEY_LEFT && m_CursorPos > 0)
		{
			do
			{
				m_CursorPos = str_utf8_rewind(m_pStr, m_CursorPos);
			} while(MoveWord && m_CursorPos > 0 && !MoveWordStop(m_pStr[m_CursorPos - 1]));
		}
		else if(Key == KEY_RIGHT && m_CursorPos < m_Len)
		{
			do
			{
				m_CursorPos = str_utf8_forward(m_pStr, m_CursorPos);
			} while(MoveWord && m_CursorPos < m_Len && !MoveWordStop(m_pStr[m_CursorPos - 1]));
		}
		else if(Key == KEY_HOME)
			m_CursorPos = 0;
		else if(Key == KEY_END)
			m_CursorPos = m_Len;
		else if((s_pInput->KeyIsPressed(KEY_LCTRL) || s_pInput->KeyIsPressed(KEY_RCTRL)) && Key == KEY_V)
		{
			// paste clipboard to cursor
			const char *pClipboardText = s_pInput->GetClipboardText();
			if(pClipboardText)
				Append(pClipboardText);
		}
	}

	return m_WasChanged || OldCursorPos != m_CursorPos;
}
