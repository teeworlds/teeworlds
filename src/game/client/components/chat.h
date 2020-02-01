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
	CLineInput m_Input;

	enum
	{
		MAX_LINES = 250,
		MAX_CHAT_PAGES = 10,
		MAX_LINE_LENGTH = 512,
	};

	struct CLine
	{
		int64 m_Time;
		vec2 m_Size;
		int m_ClientID;
		int m_TargetID;
		int m_Mode;
		int m_NameColor;
		char m_aName[MAX_NAME_LENGTH];
		char m_aText[MAX_LINE_LENGTH];
		bool m_Highlighted;
	};

	CLine m_aLines[MAX_LINES];
	int m_CurrentLine;

	// chat sounds
	enum
	{
		CHAT_SERVER=0,
		CHAT_HIGHLIGHT,
		CHAT_CLIENT,
		CHAT_NUM,
	};

	int m_Mode;
	int m_WhisperTarget;
	int m_LastWhisperFrom;
	bool m_Show;
	int m_BacklogPage;
	bool m_InputUpdate;
	int m_ChatStringOffset;
	int m_OldChatStringLength;
	int m_CompletionChosen;
	int m_CompletionFav;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;
	bool m_ReverseCompletion;
	bool m_FirstMap;
	float m_CurrentLineWidth;

	int m_ChatBufferMode;
	char m_ChatBuffer[MAX_LINE_LENGTH];
	char m_ChatCmdBuffer[1024];

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

	typedef void (*COMMAND_CALLBACK)(CChat *pChatData, const char *pArgs);

	// chat commands
	bool m_IgnoreCommand;
	int m_SelectedCommand;

	bool *m_pFilter;
	int m_FilteredCount;
	int FilterChatCommands(const char *pLine);
	int GetFirstActiveCommand();
	void NextActiveCommand(int *Index);
	void PreviousActiveCommand(int *Index);

	CCommandManager m_CommandManager;
	bool IsTypingCommand() const;
	void HandleCommands(float x, float y, float w);
	bool ExecuteCommand(bool Execute);

	static void Com_All(IConsole::IResult *pResult, void *pContext);
	static void Com_Team(IConsole::IResult *pResult, void *pContext);
	static void Com_Reply(IConsole::IResult *pResult, void *pContext);
	static void Com_Whisper(IConsole::IResult *pResult, void *pContext);
	static void Com_Mute(IConsole::IResult *pResult, void *pContext);
	static void Com_Befriend(IConsole::IResult *pResult, void *pContext);

	void ClearInput();

	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSaySelf(IConsole::IResult *pResult, void *pUserData);
	static void ConWhisper(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);
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
	void EnableMode(int Mode, const char* pText = NULL);
	void Say(int Mode, const char *pLine);
	void ClearChatBuffer();
	const char* GetCommandName(int Mode) const;

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
