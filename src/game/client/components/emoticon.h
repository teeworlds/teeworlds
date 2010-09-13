// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef GAME_CLIENT_COMPONENTS_EMOTICON_H
#define GAME_CLIENT_COMPONENTS_EMOTICON_H
#include <base/vmath.h>
#include <game/client/component.h>

class CEmoticon : public CComponent
{
	void DrawCircle(float x, float y, float r, int Segments);
	
	bool m_WasActive;
	bool m_Active;
	
	vec2 m_SelectorMouse;
	int m_SelectedEmote;

	static void ConKeyEmoticon(IConsole::IResult *pResult, void *pUserData);
	static void ConEmote(IConsole::IResult *pResult, void *pUserData);
	
public:
	CEmoticon();
	
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);

	void Emote(int Emoticon);
};

#endif
