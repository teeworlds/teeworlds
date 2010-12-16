/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
		float m_YOffset[2];
		int m_ClientId;
		int m_Team;
		int m_NameColor;
		char m_aName[64];
		char m_aText[512];
		bool m_Highlighted;
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
	bool m_InputUpdate;
	int m_ChatStringOffset;
	int m_OldChatStringLength;
	int m_CompletionChosen;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;
	
	static void ConSay(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConChat(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData, int ClientID);
	
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
