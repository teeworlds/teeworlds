// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#ifndef GAME_CLIENT_COMPONENTS_CONSOLE_H
#define GAME_CLIENT_COMPONENTS_CONSOLE_H
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

class CGameConsole : public CComponent
{
	class CInstance
	{
	public:
		TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_Backlog;
		TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
		char *m_pHistoryEntry;

		CLineInput m_Input;
		int m_Type;
		int m_CompletionEnumerationCount;
		int m_BacklogActPage;
		
	public:
		CGameConsole *m_pGameConsole;
		
		char m_aCompletionBuffer[128];
		int m_CompletionChosen;
		int m_CompletionFlagmask;
		float m_CompletionRenderOffset;
		
		IConsole::CCommandInfo *m_pCommand;

		CInstance(int t);
		void Init(CGameConsole *pGameConsole);

		void ClearBacklog();

		void ExecuteLine(const char *pLine);
		
		void OnInput(IInput::CEvent Event);
		void PrintLine(const char *pLine);
		
		const char *GetString() const { return m_Input.GetString(); }
		static void PossibleCommandsCompleteCallback(const char *pStr, void *pUser);
	};
	
	class IConsole *m_pConsole;
	
	CInstance m_LocalConsole;
	CInstance m_RemoteConsole;
	
	CInstance *CurrentConsole();
	float TimeNow();
	
	int m_ConsoleType;
	int m_ConsoleState;
	float m_StateChangeEnd;
	float m_StateChangeDuration;

	void Toggle(int Type);

	static void PossibleCommandsRenderCallback(const char *pStr, void *pUser);
	static void ClientConsolePrintCallback(const char *pStr, void *pUserData);
	static void ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	
public:
	CGameConsole();

	void PrintLine(int Type, const char *pLine);

	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Events);
};
#endif
