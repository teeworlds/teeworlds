/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <base/system.h>
#include <base/tl/array.h>
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

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
	struct CChatCommand
	{
		char m_aName[32];
		char m_aHelpText[64];
		char m_aArgsFormat[16];
		// If callback is null, then it's a server-side command.
		COMMAND_CALLBACK m_pfnCallback;
		bool m_aFiltered; // 0 = shown, 1 = hidden
		bool m_Used;
	};

	class CChatCommands
	{
		enum
		{
			// 8 is the number of vanilla commands, 14 the number of commands left to fill the chat.
			MAX_COMMANDS = 8 + 14
		};

		CChatCommand m_aCommands[MAX_COMMANDS];
		CChatCommand *m_pSelectedCommand;

	private:
		int GetActiveIndex(int index) const;
	public:
		CChatCommands();
		~CChatCommands();

		void AddCommand(const char *pName, const char *pArgsFormat, const char *pHelpText, COMMAND_CALLBACK pfnCallback);
		void ClearCommands();
		CChatCommand *GetCommandByName(const char *pName);
		void Reset();
		void Filter(const char* pLine);
		int CountActiveCommands() const;
		const CChatCommand* GetCommand(int index) const;
		const CChatCommand* GetSelectedCommand() const;
		void SelectPreviousCommand();
		void SelectNextCommand();
	};

	CChatCommands m_Commands;
	bool m_IgnoreCommand;
	bool IsTypingCommand() const;
	void HandleCommands(float x, float y, float w);
	bool ExecuteCommand(bool Execute);
	int IdentifyNameParameter(const char* pCommand) const;

	static void Com_All(CChat *pChatData, const char* pCommand);
	static void Com_Team(CChat *pChatData, const char* pCommand);
	static void Com_Reply(CChat *pChatData, const char* pCommand);
	static void Com_Whisper(CChat *pChatData, const char* pCommand);
	static void Com_Mute(CChat *pChatData, const char* pCommand);
	static void Com_Befriend(CChat *pChatData, const char* pCommand);

	void ClearInput();

	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSaySelf(IConsole::IResult *pResult, void *pUserData);
	static void ConWhisper(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);

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
