// LordSk
#ifndef GAME_EDITOR_INPUT_CONSOLE_H
#define GAME_EDITOR_INPUT_CONSOLE_H

class IConsole;

class CEditorInputConsole
{
	IConsole* m_pConsole;

public:

	void Init(IConsole* pConsole);
};

#endif
