/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <base/system.h>
#include <base/tl/array.h>
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>
#include <game/commands.h>

class CChat : public CComponent
{
	enum
	{
		MAX_LINES = 250,
		MAX_CHAT_PAGES = 10,
		MAX_LINE_LENGTH = 512,
	};

	char m_aInputBuf[MAX_LINE_LENGTH];
	CLineInput m_Input;

	struct CLine
	{
		int64 m_Time;
		vec2 m_Size;
		int m_ClientID;
		int m_TargetID;
		int m_Mode;
		int m_NameColor;
		char m_aName[MAX_NAME_ARRAY_SIZE];
		char m_aText[MAX_LINE_LENGTH];
		bool m_Highlighted;
	};

	CLine m_aLines[MAX_LINES];
	int m_CurrentLine;

	// chat sounds
	enum
	{
		CHAT_SERVER = 0,
		CHAT_HIGHLIGHT,
		CHAT_CLIENT,
		CHAT_NUM,
	};
	int GetChatSound(int ChatType);

	int m_Mode;
	int m_WhisperTarget;
	int m_LastWhisperFrom;
	bool m_Show;
	int m_BacklogPage;
	int m_CompletionChosen;
	int m_CompletionFav;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;
	bool m_FirstMap;
	float m_CurrentLineWidth;

	int m_ChatBufferMode;
	char m_aChatBuffer[MAX_LINE_LENGTH];
	char m_aChatCmdBuffer[1024];

	void ClearInput();
	void ClearChatBuffer();

	bool IsClientIgnored(int ClientID);

	struct CHistoryEntry
	{
		int m_Mode;
		char m_aText[1];
	};
	CHistoryEntry *m_pHistoryEntry;
	TStaticRingBuffer<CHistoryEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
	int m_PendingChatCounter;
	int64 m_LastChatSend;
	int64 m_aLastSoundPlayed[CHAT_NUM];

	// chat commands
	bool m_IgnoreCommand;
	int m_SelectedCommand;
	int m_CommandStart;

	array<bool> m_aFilter;
	int m_FilteredCount;
	int FilterChatCommands(const char *pLine);
	int GetFirstActiveCommand();
	int NextActiveCommand(int *pIndex);
	int PreviousActiveCommand(int *pIndex);
	int GetActiveCountRange(int i, int j);

	CCommandManager m_CommandManager;
	bool IsTypingCommand() const;
	void HandleCommands(float x, float y, float w);
	bool ExecuteCommand();
	bool CompleteCommand();
	const char *GetModeName(int Mode) const;

	static void Com_All(IConsole::IResult *pResult, void *pContext);
	static void Com_Team(IConsole::IResult *pResult, void *pContext);
	static void Com_Reply(IConsole::IResult *pResult, void *pContext);
	static void Com_Whisper(IConsole::IResult *pResult, void *pContext);
	static void Com_Mute(IConsole::IResult *pResult, void *pContext);
	static void Com_Befriend(IConsole::IResult *pResult, void *pContext);

	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSaySelf(IConsole::IResult *pResult, void *pUserData);
	static void ConWhisper(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);
	static void ConChatCommand(IConsole::IResult *pResult, void *pUserData);
	static void ServerCommandCallback(IConsole::IResult *pResult, void *pContext);

public:
	// client IDs for special messages
	enum
	{
		CLIENT_MSG = -2,
		SERVER_MSG = -1,
	};
	// Mode defined by the CHAT_* constants in protocol.h

	bool IsActive() const { return m_Mode != CHAT_NONE; }
	void AddLine(const char *pLine, int ClientID = SERVER_MSG, int Mode = CHAT_NONE, int TargetID = -1);
	void Disable();
	void EnableMode(int Mode, const char *pText = 0x0);
	void Say(int Mode, const char *pLine);

	CChat();
	virtual void OnInit();
	virtual void OnReset();
	virtual void OnMapLoad();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);
};
#endif
