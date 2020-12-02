/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_LINEINPUT_H
#define GAME_CLIENT_LINEINPUT_H

#include <engine/input.h>

// line input helter
class CLineInput
{
	char *m_pStr;
	int m_Len;
	int m_MaxSize;
	int m_MaxChars;
	int m_CursorPos;
	int m_NumChars;
	float m_ScrollOffset;
	bool m_WasChanged;

	static IInput *s_pInput;

	void UpdateStrData();
	static bool MoveWordStop(char c);

public:
	static void Init(IInput *pInput) { s_pInput = pInput; }

	CLineInput() { SetBuffer(0, 0, 0); }
	CLineInput(char *pStr, int MaxSize) { SetBuffer(pStr, MaxSize, MaxSize); }
	CLineInput(char *pStr, int MaxSize, int MaxChars) { SetBuffer(pStr, MaxSize, MaxChars); }

	void SetBuffer(char *pStr, int MaxSize) { SetBuffer(pStr, MaxSize, MaxSize); }
	void SetBuffer(char *pStr, int MaxSize, int MaxChars);

	void Clear();
	bool ProcessInput(const IInput::CEvent &Event);
	void Set(const char *pString);
	void Append(const char *pString);
	const char *GetString() const { return m_pStr; }
	int GetMaxSize() const { return m_MaxSize; }
	int GetMaxChars() const { return m_MaxChars; }
	int GetLength() const { return m_Len; }
	int GetCursorOffset() const { return m_CursorPos; }
	void SetCursorOffset(int Offset) { m_CursorPos = Offset > m_Len ? m_Len : Offset < 0 ? 0 : Offset; }
	float GetScrollOffset() const { return m_ScrollOffset; }
	void SetScrollOffset(float ScrollOffset) { m_ScrollOffset = ScrollOffset; }
	bool WasChanged() { bool Changed = m_WasChanged; m_WasChanged = false; return Changed; }
};

#endif
