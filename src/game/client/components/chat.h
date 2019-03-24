/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <base/system.h>
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

class CChat : public CComponent
{
	CLineInput m_Input;

	enum
	{
		MAX_LINES = 50,
	};

	struct CLine
	{
		int64 m_Time;
		vec2 m_Size[2];
		int m_ClientID;
		int m_TargetID;
		int m_Mode;
		int m_NameColor;
		char m_aName[64];
		char m_aText[512];
		bool m_Highlighted;
	};

	// client IDs for special messages
	enum 
	{
		CLIENT_MSG = -2,
		SERVER_MSG = -1,
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
	bool m_InputUpdate;
	int m_ChatStringOffset;
	int m_OldChatStringLength;
	int m_CompletionChosen;
	int m_CompletionFav;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;
	bool m_ReverseCompletion;

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
	struct CChatCommand
	{
		const char* m_pCommandText;
		const char* m_pHelpText;
		void (*m_pfnFunc)(CChat *pChatData, const char* pCommand);
		bool m_aFiltered; // 0 = shown, 1 = hidden
	};
		
	class CChatCommands
	{
		CChatCommand *m_apCommands;
		int m_Count;
		CChatCommand *m_pSelectedCommand;

	private:
		int GetActiveIndex(int index) const;
	public:
		CChatCommands(CChatCommand apCommands[], int Count);
		~CChatCommands();
		void Reset();
		void Filter(const char* pLine);
		int CountActiveCommands() const;
		const CChatCommand* GetCommand(int index) const;
		const CChatCommand* GetSelectedCommand() const;
		void SelectPreviousCommand();
		void SelectNextCommand();
	};

	CChatCommands *m_pCommands;
	bool m_IgnoreCommand;
	bool IsTypingCommand() const;
	void HandleCommands(float x, float y, float w);
	bool ExecuteCommand();
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
	static void ConWhisper(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);

public:
	CChat();
	~CChat();

	bool IsActive() const { return m_Mode != CHAT_NONE; }

	void AddLine(int ClientID, int Team, const char *pLine, int TargetID = -1);

	void EnableMode(int Team, const char* pText = NULL);

	void Say(int Team, const char *pLine);

	virtual void OnInit();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);
};
#endif
