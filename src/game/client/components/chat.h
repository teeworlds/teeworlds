#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <game/client/component.h>
#include <game/client/lineinput.h>

class CChat : public CComponent
{
	CLineInput m_Input;
	
	enum 
	{
		MAX_LINES = 25,
	};

	struct CLine
	{
		int64 m_Time;
		int m_ClientId;
		int m_Team;
		int m_NameColor;
		char m_aName[64];
		char m_aText[512];
	};

	CLine m_aLines[MAX_LINES];
	int m_CurrentLine;

	// chat
	enum
	{
		MODE_NONE=0,
		MODE_ALL,
		MODE_TEAM,
	};

	int m_Mode;
	bool m_Show;
	
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);
	
public:
	CChat();

	bool IsActive() const { return m_Mode != MODE_NONE; }
	
	void AddLine(int ClientId, int Team, const char *pLine);
	
	void EnableMode(int Team);
	
	void Say(int Team, const char *pLine);
	
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);
};
#endif
