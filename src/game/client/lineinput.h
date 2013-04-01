/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_LINEINPUT_H
#define GAME_CLIENT_LINEINPUT_H

#include <engine/input.h>

// line input helter
class CLineInput
{
	enum
	{
		MAX_SIZE=512,
		MAX_CHARS=MAX_SIZE/4,
	};
	char m_Str[MAX_SIZE];
	int m_Len;
	int m_CursorPos;
	int m_NumChars;
public:
	static bool Manipulate(IInput::CEvent e, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr);

	class CCallback
	{
	public:
		virtual ~CCallback() {}
		virtual bool Event(IInput::CEvent e) = 0;
	};

	CLineInput();
	void Clear();
	void ProcessInput(IInput::CEvent e);
	void Set(const char *pString);
	const char *GetString() const { return m_Str; }
	int GetLength() const { return m_Len; }
	int GetCursorOffset() const { return m_CursorPos; }
	void SetCursorOffset(int Offset) { m_CursorPos = Offset > m_Len ? m_Len : Offset < 0 ? 0 : Offset; }
};

#endif
