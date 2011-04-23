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
		NO_SELECTION=-2,
	};

	bool m_Active;
	bool m_WasActive;

	int m_SelectedSpectatorID;
	vec2 m_SelectorMouse;

	static void ConKeySpectator(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectate(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectateNext(IConsole::IResult *pResult, void *pUserData);
	static void ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData);

public:
	CSpectator();

	virtual void OnConsoleInit();
	virtual bool OnMouseMove(float x, float y);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnReset();

	void Spectate(int SpectatorID);
};

#endif
