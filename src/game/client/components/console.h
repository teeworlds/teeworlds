/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
		struct CBacklogEntry
		{
			float m_YOffset;
			char m_aText[1];
		};
		TStaticRingBuffer<CBacklogEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_Backlog;
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

		bool m_IsCommand;
		char m_aCommandName[IConsole::TEMPCMD_NAME_LENGTH];
		char m_aCommandHelp[IConsole::TEMPCMD_HELP_LENGTH];
		char m_aCommandParams[IConsole::TEMPCMD_PARAMS_LENGTH];

		CInstance(int t);
		void Init(CGameConsole *pGameConsole);

		void ClearBacklog();
		void ClearHistory();

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
	int m_PrintCBIndex;

	int m_ConsoleType;
	int m_ConsoleState;
	float m_StateChangeEnd;
	float m_StateChangeDuration;

	void Toggle(int Type);
	void Dump(int Type);

	static void PossibleCommandsRenderCallback(const char *pStr, void *pUser);
	static void ClientConsolePrintCallback(const char *pStr, void *pUserData);
	static void ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

public:
	enum
	{
		CONSOLETYPE_LOCAL=0,
		CONSOLETYPE_REMOTE,
	};

	CGameConsole();

	void PrintLine(int Type, const char *pLine);

	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Events);
};
#endif
