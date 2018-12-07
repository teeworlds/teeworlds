// LordSk
#ifndef GAME_EDITOR_INPUT_CONSOLE_H
#define GAME_EDITOR_INPUT_CONSOLE_H

#include <engine/input.h>
#include <engine/shared/ringbuffer.h>
#include <game/client/lineinput.h>

class IConsole;
class IGraphics;
class CUI;
class ITextRender;

class CEditorInputConsole
{
	IConsole* m_pConsole;
	IGraphics* m_pGraphics;
	ITextRender* m_pTextRender;
	CUI* m_pUI;
	bool m_IsOpen = false;

	struct CBacklogEntry
	{
		float m_YOffset;
		bool m_Highlighted;
		char m_aText[1];
	};
	TStaticRingBuffer<CBacklogEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_Backlog;
	TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
	char *m_pHistoryEntry;
	CLineInput m_Input;

	inline IGraphics* Graphics() { return m_pGraphics; }
	inline CUI* UI() { return m_pUI; }
	inline ITextRender* TextRender() { return m_pTextRender; }

	static void StaticConsolePrintCallback(const char *pLine, void *pUser, bool Highlighted);
	void ConsolePrintCallback(const char *pLine, bool Highlighted);

public:

	void Init(IConsole* pConsole, IGraphics* pGraphics, CUI* pUI, ITextRender* pTexRender);
	void Render();
	void OnInput(IInput::CEvent Event);

	void ToggleOpen();
	inline bool IsOpen() const { return m_IsOpen; }
};

#endif
