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
	CLineInput m_Input;

	inline IGraphics* Graphics() { return m_pGraphics; }
	inline CUI* UI() { return m_pUI; }
	inline ITextRender* TextRender() { return m_pTextRender; }

public:

	void Init(IConsole* pConsole, IGraphics* pGraphics, CUI* pUI, ITextRender* pTexRender);
	void Render();
	void OnInput(IInput::CEvent Event);

	void ToggleOpen();
};

#endif
