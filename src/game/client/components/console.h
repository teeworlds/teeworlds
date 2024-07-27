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
			bool m_Highlighted;
			char m_aText[1];
		};
		TStaticRingBuffer<CBacklogEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_Backlog;
		TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
		char *m_pHistoryEntry;

		CLineInputBuffered<256> m_Input;
		const char *m_pName;
		int m_Type;
		int m_BacklogActPage;

		CGameConsole *m_pGameConsole;

		char m_aCompletionBuffer[128];
		int m_CompletionChosen;
		char m_aCompletionBufferArgument[128];
		int m_CompletionChosenArgument;
		int m_CompletionFlagmask;
		float m_CompletionRenderOffset;
		float m_CompletionRenderOffsetChange;

		bool m_IsCommand;
		char m_aCommandName[IConsole::TEMPCMD_NAME_LENGTH];
		char m_aCommandHelp[IConsole::TEMPCMD_HELP_LENGTH];
		char m_aCommandParams[IConsole::TEMPCMD_PARAMS_LENGTH];

		CInstance(int t);
		void Init(CGameConsole *pGameConsole);

		void ClearBacklog();
		void ClearHistory();
		void Reset() { m_CompletionRenderOffset = 0.0f; m_CompletionRenderOffsetChange = 0.0f; }

		void ExecuteLine(const char *pLine);

		void OnInput(IInput::CEvent Event);
		void PrintLine(const char *pLine, bool Highlighted);

		const char *GetString() const { return m_Input.GetString(); }
		static void PossibleCommandsCompleteCallback(int Index, const char *pStr, void *pUser);
		static void PossibleArgumentsCompleteCallback(int Index, const char *pStr, void *pUser);
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

	static void PossibleCommandsRenderCallback(int Index, const char *pStr, void *pUser);
	static void ClientConsolePrintCallback(const char *pStr, void *pUserData, bool Highlighted);
	static void ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpRemoteConsole(IConsole::IResult *pResult, void *pUserData);
#ifdef CONF_DEBUG
	static void DumpCommandsCallback(int Index, const char *pStr, void *pUser);
	static void ConDumpCommands(IConsole::IResult *pResult, void *pUserData);
#endif
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

public:
	enum
	{
		CONSOLETYPE_LOCAL=0,
		CONSOLETYPE_REMOTE,
	};

	CGameConsole();

	bool IsConsoleActive();
	void PrintLine(int Type, const char *pLine);

	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Events);
};
#endif
