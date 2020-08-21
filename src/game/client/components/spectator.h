/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SPECTATOR_H
#define GAME_CLIENT_COMPONENTS_SPECTATOR_H
#include <base/vmath.h>

#include <game/client/component.h>

class CSpectator : public CComponent
{
	enum
	{
		NO_SELECTION=-1,
	};

	bool m_Active;
	bool m_WasActive;

	int m_SelectedSpectatorID;
	int m_SelectedSpecMode;
	vec2 m_SelectorMouse;

	bool SpecModePossible(int SpecMode, int SpectatorID);
	void HandleSpectateNextPrev(int Direction);

	static void ConKeySpectator(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectate(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectateNext(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData);

public:
	CSpectator();

	virtual void OnConsoleInit();
	virtual bool OnCursorMove(float x, float y, int CursorType);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnReset();

	void Spectate(int SpecMode, int SpectatorID);
};

#endif
