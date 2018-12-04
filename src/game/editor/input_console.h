// LordSk
#ifndef GAME_EDITOR_INPUT_CONSOLE_H
#define GAME_EDITOR_INPUT_CONSOLE_H

#include <engine/input.h>

class IConsole;
class IGraphics;
class CUI;

class CEditorInputConsole
{
	IConsole* m_pConsole;
	IGraphics* m_pGraphics;
	CUI* m_pUI;
	bool m_IsOpen = false;

	inline IGraphics* Graphics() { return m_pGraphics; }
	inline CUI* UI() { return m_pUI; }

public:

	void Init(IConsole* pConsole, IGraphics* pGraphics, CUI* pUI);
	void Render();
	void OnInput(IInput::CEvent Event);

	void ToggleOpen();
};

#endif
