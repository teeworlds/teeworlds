/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_LINEINPUT_H
#define GAME_CLIENT_LINEINPUT_H

#include <engine/input.h>

// line input helper
class CLineInput
{
	static class IInput *s_pInput;
	static class ITextRender *s_pTextRender;
	static class IGraphics *s_pGraphics;

	enum
	{
		MAX_ACTIVE_INPUTS = 8
	};
	static CLineInput *s_apActiveInputs[MAX_ACTIVE_INPUTS];
	static unsigned s_NumActiveInputs;

	static vec2 s_CompositionWindowPosition;
	static float s_CompositionLineHeight;

	class CTextCursor m_TextCursor;
	unsigned int m_TextVersion;

	char *m_pStr;
	int m_MaxSize;
	int m_MaxChars;
	int m_Len;
	int m_NumChars;

	int m_CursorPos;
	int m_SelectionStart;
	int m_SelectionEnd;

	float m_ScrollOffset;

	bool m_WasChanged;

	void UpdateStrData();
	static bool MoveWordStop(char c);
	void DrawSelection(float HeightWeight, int Start, int End, vec4 Color);

	void OnActivate();
	void OnDeactivate();

public:
	static void Init(class IInput *pInput, class ITextRender *pTextRender, class IGraphics *pGraphics) { s_pInput = pInput; s_pTextRender = pTextRender; s_pGraphics = pGraphics; }
	static void RenderCandidates();
	static void SetCompositionWindowPosition(vec2 Anchor, float LineHeight);

	static CLineInput *GetActiveInput() { return s_NumActiveInputs ? s_apActiveInputs[s_NumActiveInputs-1] : 0; }

	CLineInput() { SetBuffer(0, 0, 0); }
	CLineInput(char *pStr, int MaxSize) { SetBuffer(pStr, MaxSize, MaxSize); }
	CLineInput(char *pStr, int MaxSize, int MaxChars) { SetBuffer(pStr, MaxSize, MaxChars); }

	void SetBuffer(char *pStr, int MaxSize) { SetBuffer(pStr, MaxSize, MaxSize); }
	void SetBuffer(char *pStr, int MaxSize, int MaxChars);

	void Clear();
	void Set(const char *pString);
	void SetRange(const char *pString, int Begin, int End);
	void Append(const char *pString) { SetRange(pString, m_CursorPos, m_CursorPos); }

	class CTextCursor *GetCursor() { return &m_TextCursor; }
	const char *GetString() const { return m_pStr; }
	int GetMaxSize() const { return m_MaxSize; }
	int GetMaxChars() const { return m_MaxChars; }
	int GetLength() const { return m_Len; }
	int GetNumChars() const { return m_NumChars; }

	int GetCursorOffset() const { return m_CursorPos; }
	void SetCursorOffset(int Offset);
	int GetSelectionStart() const { return m_SelectionStart; }
	int GetSelectionEnd() const { return m_SelectionEnd; }
	int GetSelectionLength() const { return m_SelectionEnd - m_SelectionStart; }
	void SetSelection(int Start, int End);

	// used either for vertical or horizontal scrolling
	float GetScrollOffset() const { return m_ScrollOffset; }
	void SetScrollOffset(float ScrollOffset) { m_ScrollOffset = ScrollOffset; }

	bool ProcessInput(const IInput::CEvent &Event);
	bool WasChanged() { bool Changed = m_WasChanged; m_WasChanged = false; return Changed; }

	void Render();
	bool IsActive() const { return GetActiveInput() == this; }
	void SetActive(bool Active);
};

#endif
