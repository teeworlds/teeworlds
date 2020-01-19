/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/localization.h>

#include <game/client/components/scoreboard.h>
#include <game/client/components/sounds.h>

#include "menus.h"
#include "chat.h"
#include "binds.h"


void CChat::OnReset()
{
	if(Client()->State() == IClient::STATE_OFFLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		for(int i = 0; i < MAX_LINES; i++)
		{
			m_aLines[i].m_Time = 0;
			m_aLines[i].m_Size.y = -1.0f;
			m_aLines[i].m_aText[0] = 0;
			m_aLines[i].m_aName[0] = 0;
		}

		m_Mode = CHAT_NONE;
		// m_WhisperTarget = -1;
		m_LastWhisperFrom = -1;
		m_ReverseCompletion = false;
		m_Show = false;
		m_BacklogPage = 0;
		m_InputUpdate = false;
		m_ChatStringOffset = 0;
		m_CompletionChosen = -1;
		m_CompletionFav = -1;
		m_aCompletionBuffer[0] = 0;
		m_PlaceholderOffset = 0;
		m_PlaceholderLength = 0;
		m_pHistoryEntry = 0x0;
		m_PendingChatCounter = 0;
		m_LastChatSend = 0;
		m_IgnoreCommand = false;
		m_Commands.Reset();

		for(int i = 0; i < CHAT_NUM; ++i)
			m_aLastSoundPlayed[i] = 0;
	}
	else
	{
		for(int i = 0; i < MAX_LINES; i++)
		{
			m_aLines[i].m_Size.y = -1.0f;
		}
	}

	m_CurrentLineWidth = -1.0f;

	// init chat commands (must be in alphabetical order)
	m_Commands.ClearCommands();
	m_Commands.AddCommand("all", "", "Switch to all chat", &Com_All);
	m_Commands.AddCommand("friend", "p", "Add player as friend", &Com_Befriend);
	m_Commands.AddCommand("m", "p", "Mute a player", &Com_Mute);
	m_Commands.AddCommand("mute", "p", "Mute a player", &Com_Mute);
	m_Commands.AddCommand("r", "", "Reply to a whisper", &Com_Reply);
	m_Commands.AddCommand("team", "", "Switch to team chat", &Com_Team);
	m_Commands.AddCommand("w", "p", "Whisper another player", &Com_Whisper);
	m_Commands.AddCommand("whisper", "p", "Whisper another player", &Com_Whisper);
}

void CChat::OnMapLoad()
{
	if(Client()->State() == IClient::STATE_LOADING)
	{
		if(m_FirstMap)
			m_FirstMap = false;
		else
		{
			// display map rotation marker in chat
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), Localize("Map changed to '%s'"), Client()->GetCurrentMapName());
			AddLine(aBuf, CLIENT_MSG, CHAT_ALL);
		}
	}
}

void CChat::OnRelease()
{
	m_Show = false;
}

void CChat::OnStateChange(int NewState, int OldState)
{
	if(OldState <= IClient::STATE_CONNECTING)
	{
		m_Mode = CHAT_NONE;
		for(int i = 0; i < MAX_LINES; i++)
			m_aLines[i].m_Time = 0;
		m_CurrentLine = 0;
		ClearChatBuffer();
		m_FirstMap = true;
	}
}

void CChat::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	((CChat*)pUserData)->Say(CHAT_ALL, pResult->GetString(0));
}

void CChat::ConSayTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CChat*)pUserData)->Say(CHAT_TEAM, pResult->GetString(0));
}

void CChat::ConSaySelf(IConsole::IResult *pResult, void *pUserData)
{
	((CChat*)pUserData)->AddLine(pResult->GetString(0), CLIENT_MSG);
}

void CChat::ConWhisper(IConsole::IResult *pResult, void *pUserData)
{
	CChat *pChat = (CChat *)pUserData;

	int Target = pResult->GetInteger(0);
	if(Target < 0 || Target >= MAX_CLIENTS || !pChat->m_pClient->m_aClients[Target].m_Active || pChat->m_pClient->m_LocalClientID == Target)
		pChat->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "please enter a valid ClientID");
	else
	{
		pChat->m_WhisperTarget = Target;
		pChat->Say(CHAT_WHISPER, pResult->GetString(1));
	}
}

void CChat::ConChat(IConsole::IResult *pResult, void *pUserData)
{
	CChat *pChat = (CChat *)pUserData;

	const char *pMode = pResult->GetString(0);
	if(str_comp(pMode, "all") == 0)
		pChat->EnableMode(CHAT_ALL);
	else if(str_comp(pMode, "team") == 0)
		pChat->EnableMode(CHAT_TEAM);
	else if(str_comp(pMode, "whisper") == 0)
	{
		int Target = pChat->m_WhisperTarget; // default to ID of last target
		if(pResult->NumArguments() == 2)
			Target = pResult->GetInteger(1);
		else
		{
			// pick next valid player as target
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				int ClientID = (Target + i) % MAX_CLIENTS;
				if(pChat->m_pClient->m_aClients[ClientID].m_Active && pChat->m_pClient->m_LocalClientID != ClientID)
				{
					Target = ClientID;
					break;
				}
			}
		}
		if(Target < 0 || Target >= MAX_CLIENTS || !pChat->m_pClient->m_aClients[Target].m_Active || pChat->m_pClient->m_LocalClientID == Target)
		{
			if(pResult->NumArguments() == 2)
				pChat->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "please enter a valid ClientID");
		}
		else
		{
			pChat->m_WhisperTarget = Target;
			pChat->EnableMode(CHAT_WHISPER);
		}
	}
	else
		((CChat*)pUserData)->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "expected all, team or whisper as mode");
}

void CChat::ConShowChat(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->m_Show = pResult->GetInteger(0) != 0;
}

void CChat::OnInit()
{
	m_Input.Init(Input());
}

void CChat::OnConsoleInit()
{
	Console()->Register("say", "r", CFGFLAG_CLIENT, ConSay, this, "Say in chat");
	Console()->Register("say_team", "r", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat");
	Console()->Register("say_self", "r", CFGFLAG_CLIENT, ConSaySelf, this, "Say message just for yourself");
	Console()->Register("whisper", "ir", CFGFLAG_CLIENT, ConWhisper, this, "Whisper to a client in chat");
	Console()->Register("chat", "s?i", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team/whisper mode");
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat");
}

void CChat::ClearChatBuffer()
{
	mem_zero(m_ChatBuffer, sizeof(m_ChatBuffer));
	m_ChatBufferMode = CHAT_NONE;
}

bool CChat::OnInput(IInput::CEvent Event)
{
	if(Client()->State() != Client()->STATE_ONLINE)
		return false;

	// chat history scrolling
	if(m_Show && Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_PAGEUP || Event.m_Key == KEY_PAGEDOWN))
	{
		if(Event.m_Key == KEY_PAGEUP)
		{
			++m_BacklogPage;
			if(m_BacklogPage >= MAX_CHAT_PAGES) // will be further capped during rendering
				m_BacklogPage = MAX_CHAT_PAGES-1;
		}
		else if(Event.m_Key == KEY_PAGEDOWN)
		{
			--m_BacklogPage;
			if(m_BacklogPage < 0)
				m_BacklogPage = 0;
		}
		return m_Mode != CHAT_NONE;
	}
	if(m_Mode == CHAT_NONE)
		return false;

	if(Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_ESCAPE || Event.m_Key == KEY_MOUSE_1 || Event.m_Key == KEY_MOUSE_2))
	{
		if(IsTypingCommand())
		{
			m_IgnoreCommand = true;
		}
		else
		{
			m_Mode = CHAT_NONE;
			m_pClient->OnRelease();
		}
	}
	else if(Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER))
	{
		if(IsTypingCommand() && ExecuteCommand(true))
		{
			// everything is handled within
		}
		else
		{
			if(m_Input.GetString()[0])
			{
				bool AddEntry = false;

				if(m_PendingChatCounter == 0 && m_LastChatSend+time_freq() < time_get())
				{
					Say(m_Mode, m_Input.GetString());
					AddEntry = true;
				}
				else if(m_PendingChatCounter < 3)
				{
					++m_PendingChatCounter;
					AddEntry = true;
				}

				if(AddEntry)
				{
					CHistoryEntry *pEntry = m_History.Allocate(sizeof(CHistoryEntry)+m_Input.GetLength());
					pEntry->m_Mode = m_Mode;
					mem_copy(pEntry->m_aText, m_Input.GetString(), m_Input.GetLength()+1);
				}
			}
			m_pHistoryEntry = 0x0;
			m_Mode = CHAT_NONE;
			m_pClient->OnRelease();
		}
	}
	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_TAB)
	{
		if(IsTypingCommand() && ExecuteCommand(false))
		{
			// everything is handled within
		}
		else if (m_Mode == CHAT_WHISPER)
		{
			// change target
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				int ClientID;
				if(Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))
					ClientID = (m_WhisperTarget + MAX_CLIENTS - i) % MAX_CLIENTS; // pick previous player as target
				else
					ClientID = (m_WhisperTarget + i) % MAX_CLIENTS; // pick next player as target
				if (m_pClient->m_aClients[ClientID].m_Active && m_WhisperTarget != ClientID && m_pClient->m_LocalClientID != ClientID)
				{
					m_WhisperTarget = ClientID;
					break;
				}
			}
		}
		else
		{
			// fill the completion buffer
			if(m_CompletionChosen < 0)
			{
				const char *pCursor = m_Input.GetString()+m_Input.GetCursorOffset();
				for(int Count = 0; Count < m_Input.GetCursorOffset() && *(pCursor-1) != ' '; --pCursor, ++Count);
				m_PlaceholderOffset = pCursor-m_Input.GetString();

				for(m_PlaceholderLength = 0; *pCursor && *pCursor != ' '; ++pCursor)
					++m_PlaceholderLength;

				str_truncate(m_aCompletionBuffer, sizeof(m_aCompletionBuffer), m_Input.GetString()+m_PlaceholderOffset, m_PlaceholderLength);
			}

			// find next possible name
			const char *pCompletionString = 0;
			if(m_CompletionChosen < 0 && m_CompletionFav >= 0)
				m_CompletionChosen = m_CompletionFav;
			else
			{
				if (m_ReverseCompletion)
					m_CompletionChosen = (m_CompletionChosen - 1 + 2 * MAX_CLIENTS) % (2 * MAX_CLIENTS);
				else
					m_CompletionChosen = (m_CompletionChosen + 1) % (2 * MAX_CLIENTS);
			}

			for(int i = 0; i < 2*MAX_CLIENTS; ++i)
			{
				int SearchType;
				int Index;

				if(m_ReverseCompletion)
				{
					SearchType = ((m_CompletionChosen-i +2*MAX_CLIENTS)%(2*MAX_CLIENTS))/MAX_CLIENTS;
					Index = (m_CompletionChosen-i + MAX_CLIENTS )%MAX_CLIENTS;
				}
				else
				{
					SearchType = ((m_CompletionChosen+i)%(2*MAX_CLIENTS))/MAX_CLIENTS;
					Index = (m_CompletionChosen+i)%MAX_CLIENTS;
				}

				if(!m_pClient->m_aClients[Index].m_Active)
					continue;

				bool Found = false;
				if(SearchType == 1)
				{
					if(!str_startswith_nocase(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer) &&
						str_find_nocase(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer))
						Found = true;
				}
				else if(str_startswith_nocase(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer))
					Found = true;

				if(Found)
				{
					pCompletionString = m_pClient->m_aClients[Index].m_aName;
					m_CompletionChosen = Index+SearchType*MAX_CLIENTS;
					m_CompletionFav = m_CompletionChosen%MAX_CLIENTS;
					break;
				}
			}

			// insert the name
			if(pCompletionString)
			{
				char aBuf[256];
				// add part before the name
				str_truncate(aBuf, sizeof(aBuf), m_Input.GetString(), m_PlaceholderOffset);

				// add the name
				str_append(aBuf, pCompletionString, sizeof(aBuf));

				// add seperator
				const char *pSeparator = "";
				if(*(m_Input.GetString()+m_PlaceholderOffset+m_PlaceholderLength) != ' ')
					pSeparator = m_PlaceholderOffset == 0 ? ": " : " ";
				else if(m_PlaceholderOffset == 0)
					pSeparator = ":";
				if(*pSeparator)
					str_append(aBuf, pSeparator, sizeof(aBuf));

				// add part after the name
				str_append(aBuf, m_Input.GetString()+m_PlaceholderOffset+m_PlaceholderLength, sizeof(aBuf));

				m_PlaceholderLength = str_length(pSeparator)+str_length(pCompletionString);
				m_OldChatStringLength = m_Input.GetLength();
				m_Input.Set(aBuf);
				m_Input.SetCursorOffset(m_PlaceholderOffset+m_PlaceholderLength);
				m_InputUpdate = true;
			}
		}
	}
	else
	{
		m_OldChatStringLength = m_Input.GetLength();
		if(m_Input.ProcessInput(Event))
		{
			m_InputUpdate = true;

			// reset name completion process
			m_CompletionChosen = -1;
		}
	}

	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_LCTRL)
		m_ReverseCompletion = true;
	else if(Event.m_Flags&IInput::FLAG_RELEASE && Event.m_Key == KEY_LCTRL)
		m_ReverseCompletion = false;

	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_UP)
	{
		if(IsTypingCommand())
		{
			m_Commands.SelectPreviousCommand();
		}
		else
		{
			if(m_pHistoryEntry)
			{
				CHistoryEntry *pTest = m_History.Prev(m_pHistoryEntry);

				if(pTest)
					m_pHistoryEntry = pTest;
			}
			else
				m_pHistoryEntry = m_History.Last();

			if(m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry->m_aText);
		}
	}
	else if (Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_DOWN)
	{
		if(IsTypingCommand())
		{
			m_Commands.SelectNextCommand();
		}
		else
		{
			if(m_pHistoryEntry)
				m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

			if (m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry->m_aText);
			else
				m_Input.Clear();
		}
	}

	//Handle Chat Buffer
	if((Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)) || !m_Input.GetLength())
	{
		ClearChatBuffer();
	}
	else if(Event.m_Key != KEY_MOUSE_1 && Event.m_Key != KEY_MOUSE_2)
	{
		//Save Chat Buffer
		m_ChatBufferMode = m_Mode;
		str_copy(m_ChatBuffer, m_Input.GetString(), sizeof(m_ChatBuffer));
	}
	return true;
}


void CChat::EnableMode(int Mode, const char* pText)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	m_Mode = Mode;
	ClearInput();

	if(pText) // optional text to initalize with
	{
		m_Input.Set(pText);
		m_Input.SetCursorOffset(str_length(pText));
		m_InputUpdate = true;
	}
	else if(m_Mode == m_ChatBufferMode)
	{
		m_Input.Set(m_ChatBuffer);
		m_Input.SetCursorOffset(str_length(m_ChatBuffer));
		m_InputUpdate = true;
	}
}

void CChat::ClearInput()
{
	m_Input.Clear();
	Input()->Clear();
	m_CompletionChosen = -1;

	m_IgnoreCommand = false;
	m_Commands.Reset();
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		AddLine(pMsg->m_pMessage, pMsg->m_ClientID, pMsg->m_Mode, pMsg->m_TargetID);
	}
	else if(MsgType == NETMSGTYPE_SV_COMMANDINFO)
	{
		CNetMsg_Sv_CommandInfo *pMsg = (CNetMsg_Sv_CommandInfo *)pRawMsg;
		if(!m_Commands.GetCommandByName(pMsg->m_pName))
		{
			dbg_msg("chat_commands", "adding server chat command: name='%s' args='%s' help='%s'", pMsg->m_pName, pMsg->m_ArgsFormat, pMsg->m_HelpText);
			m_Commands.AddCommand(pMsg->m_pName, pMsg->m_ArgsFormat, pMsg->m_HelpText, 0);
		}
	}
	else if(MsgType == NETMSGTYPE_SV_COMMANDINFOREMOVE)
	{
		CNetMsg_Sv_CommandInfoRemove *pMsg = (CNetMsg_Sv_CommandInfoRemove *)pRawMsg;
		
		CChatCommand *pCommand = m_Commands.GetCommandByName(pMsg->m_pName);

		if(pCommand)
		{
			mem_zero(pCommand, sizeof(CChatCommand));
			dbg_msg("chat_commands", "removed chat command: name='%s'", pMsg->m_pName);
		}
	}
}

void CChat::AddLine(const char *pLine, int ClientID, int Mode, int TargetID)
{
	if(*pLine == 0 || (ClientID >= 0 && (!g_Config.m_ClShowsocial || !m_pClient->m_aClients[ClientID].m_Active || // unknown client
		m_pClient->m_aClients[ClientID].m_ChatIgnore ||
		g_Config.m_ClFilterchat == 2 ||
		(m_pClient->m_LocalClientID != ClientID && g_Config.m_ClFilterchat == 1 && !m_pClient->m_aClients[ClientID].m_Friend))))
		return;

	if(Mode == CHAT_WHISPER)
	{
		// unknown client
		if(ClientID < 0 || !m_pClient->m_aClients[ClientID].m_Active || TargetID < 0 || !m_pClient->m_aClients[TargetID].m_Active)
			return;
		// should be sender or receiver
		if(ClientID != m_pClient->m_LocalClientID && TargetID != m_pClient->m_LocalClientID)
			return;
		// ignore and chat filter
		if(m_pClient->m_aClients[TargetID].m_ChatIgnore || g_Config.m_ClFilterchat == 2 ||
			(m_pClient->m_LocalClientID != TargetID && g_Config.m_ClFilterchat == 1 && !m_pClient->m_aClients[TargetID].m_Friend))
			return;
	}

	// trim right and set maximum length to 128 utf8-characters
	int Length = 0;
	const char *pStr = pLine;
	const char *pEnd = 0;
	while(*pStr)
	{
		const char *pStrOld = pStr;
		int Code = str_utf8_decode(&pStr);

		// check if unicode is not empty
		if(!str_utf8_is_whitespace(Code))
		{
			pEnd = 0;
		}
		else if(pEnd == 0)
			pEnd = pStrOld;

		if(++Length >= 127)
		{
			*(const_cast<char *>(pStr)) = 0;
			break;
		}
	}
	if(pEnd != 0)
		*(const_cast<char *>(pEnd)) = 0;

	bool Highlighted = false;
	char *p = const_cast<char*>(pLine);
	while(*p)
	{
		pLine = p;
		// find line seperator and strip multiline
		while(*p)
		{
			if(*p++ == '\n')
			{
				*(p-1) = 0;
				break;
			}
		}

		m_CurrentLine = (m_CurrentLine+1)%MAX_LINES;
		CLine *pCurLine = &m_aLines[m_CurrentLine];

		pCurLine->m_Time = time_get();
		pCurLine->m_Size.y = -1.0f;
		pCurLine->m_ClientID = ClientID;
		pCurLine->m_TargetID = TargetID;
		pCurLine->m_Mode = Mode;
		pCurLine->m_NameColor = -2;

		// check for highlighted name
		Highlighted = false;
		// do not highlight our own messages, whispers and system messages
		if(Mode != CHAT_WHISPER && ClientID >= 0 && ClientID != m_pClient->m_LocalClientID)
		{
			const char *pHL = str_find_nocase(pLine, m_pClient->m_aClients[m_pClient->m_LocalClientID].m_aName);
			if(pHL)
			{
				int Length = str_length(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_aName);
				if((pLine == pHL || pHL[-1] == ' ')) // "" or " " before
				{
					if((pHL[Length] == 0 || pHL[Length] == ' ')) // "" or " " after
						Highlighted = true;
					if(pHL[Length] == ':' && (pHL[Length+1] == 0 || pHL[Length+1] == ' ')) // ":" or ": " after
						Highlighted = true;
				}
				m_CompletionFav = ClientID;
			}
		}

		pCurLine->m_Highlighted =  Highlighted;

		int NameCID = ClientID;
		if(Mode == CHAT_WHISPER && ClientID == m_pClient->m_LocalClientID && TargetID >= 0)
			NameCID = TargetID;

		if(ClientID == SERVER_MSG)
		{
			pCurLine->m_aName[0] = 0;
			str_format(pCurLine->m_aText, sizeof(pCurLine->m_aText), "*** %s", pLine);
		}
		else if(ClientID == CLIENT_MSG)
		{
			pCurLine->m_aName[0] = 0;
			str_format(pCurLine->m_aText, sizeof(pCurLine->m_aText), "— %s", pLine);
		}
		else
		{
			if(m_pClient->m_aClients[ClientID].m_Team == TEAM_SPECTATORS)
				pCurLine->m_NameColor = TEAM_SPECTATORS;

			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
			{
				if(m_pClient->m_aClients[ClientID].m_Team == TEAM_RED)
					pCurLine->m_NameColor = TEAM_RED;
				else if(m_pClient->m_aClients[ClientID].m_Team == TEAM_BLUE)
					pCurLine->m_NameColor = TEAM_BLUE;
			}

			str_format(pCurLine->m_aName, sizeof(pCurLine->m_aName), "%s", m_pClient->m_aClients[NameCID].m_aName);
			str_format(pCurLine->m_aText, sizeof(pCurLine->m_aText), "%s", pLine);
		}

		char aBuf[1024];
		char aBufMode[32];
		if(Mode == CHAT_WHISPER)
			str_copy(aBufMode, "whisper", sizeof(aBufMode));
		else if(Mode == CHAT_TEAM)
			str_copy(aBufMode, "teamchat", sizeof(aBufMode));
		else
			str_copy(aBufMode, "chat", sizeof(aBufMode));

		str_format(aBuf, sizeof(aBuf), "%2d: %s: %s", NameCID, pCurLine->m_aName, pCurLine->m_aText);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, aBufMode, aBuf, Highlighted || Mode == CHAT_WHISPER);
	}

	if(Mode == CHAT_WHISPER && m_pClient->m_LocalClientID != ClientID)
		m_LastWhisperFrom = ClientID; // we received a a whisper

	// play sound
	int64 Now = time_get();
	if(ClientID < 0)
	{
		if(Now-m_aLastSoundPlayed[CHAT_SERVER] >= time_freq()*3/10)
		{
			m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0);
			m_aLastSoundPlayed[CHAT_SERVER] = Now;
		}
	}
	else if(Highlighted || Mode == CHAT_WHISPER)
	{
		if(Now-m_aLastSoundPlayed[CHAT_HIGHLIGHT] >= time_freq()*3/10)
		{
			m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_HIGHLIGHT, 0);
			m_aLastSoundPlayed[CHAT_HIGHLIGHT] = Now;
		}
	}
	else
	{
		if(Now-m_aLastSoundPlayed[CHAT_CLIENT] >= time_freq()*3/10)
		{
			m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_CLIENT, 0);
			m_aLastSoundPlayed[CHAT_CLIENT] = Now;
		}
	}
}

const char* CChat::GetCommandName(int Mode) const
{
	switch(Mode)
	{
		case CHAT_ALL: return "chat all";
		case CHAT_WHISPER: return "chat whisper";
		case CHAT_TEAM: return "chat team";
		default: return "";
	}
}

void CChat::OnRender()
{
	if(Client()->State() <= Client()->STATE_LOADING)
		return;

	// send pending chat messages
	if(m_PendingChatCounter > 0 && m_LastChatSend+time_freq() < time_get())
	{
		CHistoryEntry *pEntry = m_History.Last();
		for(int i = m_PendingChatCounter-1; pEntry; --i, pEntry = m_History.Prev(pEntry))
		{
			if(i == 0)
			{
				Say(pEntry->m_Mode, pEntry->m_aText);
				break;
			}
		}
		--m_PendingChatCounter;
	}

	const float Height = 300.0f;
	const float Width = Height*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);
	float x = 12.0f;
	float y = Height-20.0f;
	float LineWidth = 200.0f;

	// bool showCommands;
	float CategoryWidth = 0;

	if(m_Mode == CHAT_WHISPER && !m_pClient->m_aClients[m_WhisperTarget].m_Active)
		m_Mode = CHAT_NONE;
	else if(m_Mode != CHAT_NONE || m_ChatBufferMode != CHAT_NONE)
	{
		//Set ChatMode and alpha blend for buffered chat
		int ChatMode = m_Mode;
		float Blend = 1.0f;
		if(m_Mode == CHAT_NONE)
		{
			ChatMode = m_ChatBufferMode;
			Blend = 0.5f;
		}

		// calculate category text size
		// TODO: rework TextRender. Writing the same code twice to calculate a simple thing as width is ridiculus
		float CategoryHeight;
		const float IconOffsetX = ChatMode == CHAT_WHISPER ? 6.0f : 0.0f;
		const float CategoryFontSize = 8.0f;
		const float InputFontSize = 8.0f;
		char aCatText[48];

		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, x, y, CategoryFontSize, 0);

			if(ChatMode == CHAT_ALL)
				str_copy(aCatText, Localize("All"), sizeof(aCatText));
			else if(ChatMode == CHAT_TEAM)
			{
				const int LocalCID = m_pClient->m_LocalClientID;
				const CGameClient::CClientData& LocalClient = m_pClient->m_aClients[LocalCID];
				const int LocalTteam = LocalClient.m_Team;

				if(LocalTteam == TEAM_SPECTATORS)
					str_copy(aCatText, Localize("Spectators"), sizeof(aCatText));
				else
					str_copy(aCatText, Localize("Team"), sizeof(aCatText));
			}
			else if(ChatMode == CHAT_WHISPER)
			{
				CategoryWidth += RenderTools()->GetClientIdRectSize(CategoryFontSize);
				str_format(aCatText, sizeof(aCatText), "%s",m_pClient->m_aClients[m_WhisperTarget].m_aName);
			}
			else
				str_copy(aCatText, Localize("Chat"), sizeof(aCatText));

			TextRender()->TextEx(&Cursor, aCatText, -1);

			CategoryWidth += Cursor.m_X - Cursor.m_StartX;
			CategoryHeight = Cursor.m_FontSize;
		}

		// draw a background box
		const vec4 CRCWhite(1.0f, 1.0f, 1.0f, 0.25f*Blend);
		const vec4 CRCTeam(0.4f, 1.0f, 0.4f, 0.4f*Blend);
		const vec4 CRCWhisper(0.0f, 0.5f, 1.0f, 0.5f*Blend);

		vec4 CatRectColor = CRCWhite;
		if(ChatMode == CHAT_TEAM)
			CatRectColor = CRCTeam;
		else if(ChatMode == CHAT_WHISPER)
			CatRectColor = CRCWhisper;

		CUIRect CatRect;
		CatRect.x = 0;
		CatRect.y = y;
		CatRect.w = CategoryWidth + x + 2.0f + IconOffsetX;
		CatRect.h = CategoryHeight + 4.0f;
		RenderTools()->DrawUIRect(&CatRect, CatRectColor, CUI::CORNER_R, 2.0f);

		// draw chat icon
		Graphics()->WrapClamp();
		IGraphics::CQuadItem QuadIcon;

		if(ChatMode == CHAT_WHISPER)
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CHATWHISPER].m_Id);
			Graphics()->QuadsBegin();
			Graphics()->QuadsSetSubset(1, 0, 0, 1);
			QuadIcon = IGraphics::CQuadItem(1.5f, y + (CatRect.h - 8.0f) * 0.5f, 16.f, 8.0f);
		}
		else
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(SPRITE_DOTDOT);
			QuadIcon = IGraphics::CQuadItem(1.0f, y, 10.f, 10.0f);
		}

		Graphics()->SetColor(1, 1, 1, 1.0f*Blend);
		Graphics()->QuadsDrawTL(&QuadIcon, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();

		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x + IconOffsetX, y, CategoryFontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Width-190.0f;
		Cursor.m_MaxLines = 2;

		//make buffered chat name transparent
		TextRender()->TextColor(1, 1, 1, Blend);

		if(ChatMode == CHAT_WHISPER)
			RenderTools()->DrawClientID(TextRender(), &Cursor, m_WhisperTarget);
		TextRender()->TextEx(&Cursor, aCatText, -1);

		Cursor.m_X += 4.0f;
		Cursor.m_Y -= (InputFontSize-CategoryFontSize) * 0.5f;
		Cursor.m_StartX = Cursor.m_X;
		Cursor.m_FontSize = InputFontSize;

		// check if the visible text has to be moved
		if(m_InputUpdate)
		{
			if(m_ChatStringOffset > 0 && m_Input.GetLength() < m_OldChatStringLength)
				m_ChatStringOffset = max(0, m_ChatStringOffset-(m_OldChatStringLength-m_Input.GetLength()));

			if(m_ChatStringOffset > m_Input.GetCursorOffset())
				m_ChatStringOffset -= m_ChatStringOffset-m_Input.GetCursorOffset();
			else
			{
				CTextCursor Temp = Cursor;
				Temp.m_Flags = 0;

				TextRender()->TextEx(&Temp, m_Input.GetString()+m_ChatStringOffset, m_Input.GetCursorOffset()-m_ChatStringOffset);
				TextRender()->TextEx(&Temp, "|", -1);
				while(Temp.m_LineCount > 2)
				{
					++m_ChatStringOffset;
					Temp = Cursor;
					Temp.m_Flags = 0;
					TextRender()->TextEx(&Temp, m_Input.GetString()+m_ChatStringOffset, m_Input.GetCursorOffset()-m_ChatStringOffset);
					TextRender()->TextEx(&Temp, "|", -1);
				}
			}
			m_InputUpdate = false;
		}

		//render buffered text
		if(m_Mode == CHAT_NONE)
		{
			//calculate WidthLimit
			float WidthLimit = LineWidth + x + 3.0f - Cursor.m_X;
			float TextWidth = TextRender()->TextWidth(0, Cursor.m_FontSize, m_Input.GetString(), -1, -1);

			//add dots when string excesses length
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);
			if(TextWidth > WidthLimit)
			{
				const static float DotWidth = TextRender()->TextWidth(0, Cursor.m_FontSize, "...", -1, -1);

				Cursor.m_Flags|=TEXTFLAG_STOP_AT_END;

				//Limit the line width to append three dots
				Cursor.m_LineWidth = WidthLimit-DotWidth;

				TextRender()->TextEx(&Cursor, m_Input.GetString(), -1);

				//Change line width back to default
				Cursor.m_LineWidth = LineWidth;
				TextRender()->TextEx(&Cursor, "...", -1);
			}
			else
				TextRender()->TextEx(&Cursor, m_Input.GetString(), -1);

			//render helper annotation
			CTextCursor InfoCursor;
			TextRender()->SetCursor(&InfoCursor, 2.0f, y+12.0f, CategoryFontSize*0.75, TEXTFLAG_RENDER);

			//Check if key exists with bind
			int KeyID, Modifier;
			m_pClient->m_pBinds->GetKeyID(GetCommandName(m_ChatBufferMode), KeyID, Modifier);
			
			if(KeyID < KEY_LAST)
			{
				//find keyname and format text
				char aKeyName[64];
				m_pClient->m_pBinds->GetKey(GetCommandName(m_ChatBufferMode), aKeyName, sizeof(aKeyName), KeyID, Modifier);

				char aInfoText[128];
				str_format(aInfoText, sizeof(aInfoText), Localize("Press %s to resume chatting"), aKeyName);
				TextRender()->TextEx(&InfoCursor, aInfoText, -1);
			}
		}
		else
		{
			//Render normal text
			TextRender()->TextEx(&Cursor, m_Input.GetString()+m_ChatStringOffset, m_Input.GetCursorOffset()-m_ChatStringOffset);
			static float MarkerOffset = TextRender()->TextWidth(0, 8.0f, "|", -1, -1.0f)/3;
			CTextCursor Marker = Cursor;
			Marker.m_X -= MarkerOffset;

			TextRender()->TextEx(&Marker, "|", -1);
			TextRender()->TextEx(&Cursor, m_Input.GetString()+m_Input.GetCursorOffset(), -1);

			//Render command autocomplete option hint
			if(IsTypingCommand() && m_Commands.CountActiveCommands())
				if(const CChat::CChatCommand* pCommand = m_Commands.GetSelectedCommand())
					if(str_length(pCommand->m_aName)+1 > str_length(m_Input.GetString()))
					{
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
						TextRender()->TextEx(&Cursor, pCommand->m_aName + str_length(m_Input.GetString())-1, -1);
					}

			if(ChatMode == CHAT_WHISPER)
			{
				//render helper annotation
				CTextCursor InfoCursor;
				TextRender()->SetCursor(&InfoCursor, 2.0f, y+12.0f, CategoryFontSize*0.75, TEXTFLAG_RENDER);

				char aInfoText[128];
				str_format(aInfoText, sizeof(aInfoText), Localize("Press Tab to cycle chat recipients"));
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
				TextRender()->TextEx(&InfoCursor, aInfoText, -1);
			}
		}
	}

	y -= 8.0f;

	// show all chat when typing
	m_Show |= m_Mode != CHAT_NONE;

	int64 Now = time_get();
	const int64 TimeFreq = time_freq();

	// get scoreboard data
	const bool IsScoreboardActive = m_pClient->m_pScoreboard->IsActive();
	CUIRect ScoreboardRect = m_pClient->m_pScoreboard->GetScoreboardRect();
	const CUIRect ScoreboardScreen = *UI()->Screen();
	CUIRect ScoreboardRectFixed;
	ScoreboardRectFixed.x = ScoreboardRect.x/ScoreboardScreen.w * Width;
	ScoreboardRectFixed.y = ScoreboardRect.y/ScoreboardScreen.h * Height;
	ScoreboardRectFixed.w = ScoreboardRect.w/ScoreboardScreen.w * Width;
	ScoreboardRectFixed.h = ScoreboardRect.h/ScoreboardScreen.h * Height;

	float HeightLimit = m_Show ? 90.0f : 200.0f;

	if(IsScoreboardActive)
	{
		// calculate chat area (height gets a penalty as long lines are better to read)
		float ReducedLineWidth = min(ScoreboardRectFixed.x - 5.0f - x, LineWidth);
		float ReducedHeightLimit = max(ScoreboardRectFixed.y+ScoreboardRectFixed.h+5.0f, HeightLimit);
		float Area1 = ReducedLineWidth * ((Height-HeightLimit) * 0.5f);
		float Area2 = LineWidth * ((Height-ReducedHeightLimit) * 0.5f);

		if(Area1 >= Area2)
			LineWidth = ReducedLineWidth;
		else
			HeightLimit = ReducedHeightLimit;
	}

	if(m_CurrentLineWidth != LineWidth)
	{
		for(int i = 0; i < MAX_LINES; i++)
		{
			m_aLines[i].m_Size.y = -1.0f;
		}
		m_CurrentLineWidth = LineWidth;
	}

	float Begin = x;
	float FontSize = 6.0f;
	CTextCursor Cursor;

	// get the y offset (calculate it if we haven't done that yet)
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		CLine *pLine = &m_aLines[r];

		if(pLine->m_aText[0] == 0) break;

		if(pLine->m_Size.y < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, Begin, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = LineWidth;

			char aBuf[768] = {0};
			if(pLine->m_Mode == CHAT_WHISPER)
			{
				Cursor.m_X += 12.5f;
			}

			if(pLine->m_ClientID >= 0)
			{
				Cursor.m_X += RenderTools()->GetClientIdRectSize(Cursor.m_FontSize);
				str_format(aBuf, sizeof(aBuf), "%s: ", pLine->m_aName);
			}

			str_append(aBuf, pLine->m_aText, sizeof(aBuf));

			TextRender()->TextEx(&Cursor, aBuf, -1);
			pLine->m_Size.y = Cursor.m_LineCount * Cursor.m_FontSize;
			pLine->m_Size.x = Cursor.m_LineCount == 1 ? Cursor.m_X - Cursor.m_StartX : LineWidth;
		}
	}

	if(m_Show)
	{
		CUIRect Rect;
		Rect.x = 0;
		Rect.y = HeightLimit - 2.0f;
		Rect.w = LineWidth + x + 3.0f;
		Rect.h = Height - HeightLimit - 22.f;

		const float LeftAlpha = 0.85f;
		const float RightAlpha = 0.05f;
		RenderTools()->DrawUIRect4(&Rect,
								   vec4(0, 0, 0, LeftAlpha),
								   vec4(0, 0, 0, RightAlpha),
								   vec4(0, 0, 0, LeftAlpha),
								   vec4(0, 0, 0, RightAlpha),
								   CUI::CORNER_R, 3.0f);
	}

	// compute the page index
	int StartLine = 0;
	if(m_Show)
	{
		int Page;
		int l = 0;
		for(Page = 0; Page < MAX_CHAT_PAGES; Page++)
		{
			int PageY = y;
			bool endReached = false;
			for(; l < MAX_LINES; l++)
			{
				int r = ((m_CurrentLine-l)+MAX_LINES)%MAX_LINES;
				const CLine *pLine = &m_aLines[r];

				if(pLine->m_aText[0] == 0)
				{
					endReached = true;
					break;
				}
				if(pLine->m_ClientID >= 0 && m_pClient->m_aClients[pLine->m_ClientID].m_ChatIgnore)
					continue;
				if(PageY < HeightLimit)
					break;
				PageY -= pLine->m_Size.y;
			}
			if(endReached)
				break;
			if(Page < m_BacklogPage)
				StartLine = l - 1;
		}
		if(Page == MAX_CHAT_PAGES)
			Page--;
		if(Page < m_BacklogPage) // cap the page to the last
			m_BacklogPage = Page;

		// render the page count
		if(Page > 0)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), Localize("-Page %d/%d-"), m_BacklogPage+1, Page+1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.6f);
			TextRender()->Text(0, 6.0f, HeightLimit-3.0f, FontSize-1.0f, aBuf, -1.0f);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	for(int i = StartLine; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		const CLine *pLine = &m_aLines[r];

		if(pLine->m_aText[0] == 0)
			break;

		if(pLine->m_ClientID >= 0 && m_pClient->m_aClients[pLine->m_ClientID].m_ChatIgnore)
			continue;

		if(Now > pLine->m_Time+16*TimeFreq && !m_Show)
			break;

		y -= pLine->m_Size.y;

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		float Blend = Now > pLine->m_Time+14*TimeFreq && !m_Show ? 1.0f-(Now-pLine->m_Time-14*TimeFreq)/(2.0f*TimeFreq) : 1.0f;

		const float HlTimeFull = 1.0f;
		const float HlTimeFade = 1.0f;

		float Delta = (Now - pLine->m_Time) / (float)TimeFreq;
		const float HighlightBlend = 1.0f - clamp(Delta - HlTimeFull, 0.0f, HlTimeFade) / HlTimeFade;

		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		const vec2 ShadowOffset(0.8f, 1.5f);
		const vec4 ShadowWhisper(0.09f, 0.f, 0.26f, Blend * 0.9f);
		const vec4 ShadowBlack(0, 0, 0, Blend * 0.9f);
		vec4 ShadowColor = ShadowBlack;

		if(pLine->m_Mode == CHAT_WHISPER)
			ShadowColor = ShadowWhisper;


		const vec4 ColorSystem(1.0f, 1.0f, 0.5f, Blend);
		const vec4 ColorWhisper(0.4f, 1.0f, 1.0f, Blend);
		const vec4 ColorRed(1.0f, 0.5f, 0.5f, Blend);
		const vec4 ColorBlue(0.7f, 0.7f, 1.0f, Blend);
		const vec4 ColorSpec(0.75f, 0.5f, 0.75f, Blend);
		const vec4 ColorAllPre(0.8f, 0.8f, 0.8f, Blend);
		const vec4 ColorAllText(1.0f, 1.0f, 1.0f, Blend);
		const vec4 ColorTeamPre(0.45f, 0.9f, 0.45f, Blend);
		const vec4 ColorTeamText(0.6f, 1.0f, 0.6f, Blend);
		const vec4 ColorHighlightBg(0.0f, 0.27f, 0.9f, 0.5f * HighlightBlend);
		const vec4 ColorHighlightOutline(0.0f, 0.4f, 1.0f,
			mix(pLine->m_Mode == CHAT_TEAM ? 0.6f : 0.5f, 1.0f, HighlightBlend));

		vec4 TextColor = ColorAllText;

		if(pLine->m_Highlighted && ColorHighlightBg.a > 0.001f)
		{
			CUIRect BgRect;
			BgRect.x = Cursor.m_X;
			BgRect.y = Cursor.m_Y + 2.0f;
			BgRect.w = pLine->m_Size.x - 2.0f;
			BgRect.h = pLine->m_Size.y;

			vec4 LeftColor = ColorHighlightBg * ColorHighlightBg.a;
			LeftColor.a = ColorHighlightBg.a;

			vec4 RightColor = ColorHighlightBg;
			RightColor *= 0.1f;

			RenderTools()->DrawUIRect4(&BgRect,
									   LeftColor,
									   RightColor,
									   LeftColor,
									   RightColor,
									   CUI::CORNER_R, 2.0f);
		}

		char aBuf[48];
		if(pLine->m_Mode == CHAT_WHISPER)
		{
			const float LineBaseY = TextRender()->TextGetLineBaseY(&Cursor);

			const float qw = 10.0f;
			const float qh = 5.0f;
			const float qx = Cursor.m_X + 2.0f;
			const float qy = LineBaseY - qh - 0.5f;

			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CHATWHISPER].m_Id);
			Graphics()->WrapClamp();

			Graphics()->QuadsBegin();

			// image orientation
			const int LocalCID = m_pClient->m_LocalClientID;
			if(pLine->m_ClientID == LocalCID && pLine->m_TargetID >= 0)
				Graphics()->QuadsSetSubset(1, 0, 0, 1); // To
			else if(pLine->m_TargetID == LocalCID)
				Graphics()->QuadsSetSubset(0, 0, 1, 1); // From
			else
				dbg_break();


			// shadow pass
			Graphics()->SetColor(ShadowWhisper.r*ShadowWhisper.a*Blend, ShadowWhisper.g*ShadowWhisper.a*Blend,
								 ShadowWhisper.b*ShadowWhisper.a*Blend, ShadowWhisper.a*Blend);
			IGraphics::CQuadItem Quad(qx + 0.2f, qy + 0.5f, qw, qh);
			Graphics()->QuadsDrawTL(&Quad, 1);

			// color pass
			Graphics()->SetColor(ColorWhisper.r*Blend, ColorWhisper.g*Blend, ColorWhisper.b*Blend, Blend);
			Quad = IGraphics::CQuadItem(qx, qy, qw, qh);
			Graphics()->QuadsDrawTL(&Quad, 1);

			Graphics()->QuadsEnd();
			Graphics()->WrapNormal();
			Cursor.m_X += 12.5f;
		}

		// render name
		if(pLine->m_ClientID < 0)
			TextColor = ColorSystem;
		else if(pLine->m_Mode == CHAT_WHISPER)
			TextColor = ColorWhisper;
		else if(pLine->m_Mode == CHAT_TEAM)
			TextColor = ColorTeamPre;
		else if(pLine->m_NameColor == TEAM_RED)
			TextColor = ColorRed;
		else if(pLine->m_NameColor == TEAM_BLUE)
			TextColor = ColorBlue;
		else if(pLine->m_NameColor == TEAM_SPECTATORS)
			TextColor = ColorSpec;
		else
			TextColor = ColorAllPre;

		if(pLine->m_ClientID >= 0)
		{
			int NameCID = pLine->m_ClientID;
			if(pLine->m_Mode == CHAT_WHISPER && pLine->m_ClientID == m_pClient->m_LocalClientID && pLine->m_TargetID >= 0)
				NameCID = pLine->m_TargetID;

			vec4 IdTextColor = vec4(0.1f*Blend, 0.1f*Blend, 0.1f*Blend, 1.0f*Blend);
			vec4 BgIdColor = TextColor;
			BgIdColor.a = 0.5f*Blend;
			RenderTools()->DrawClientID(TextRender(), &Cursor, NameCID, BgIdColor, IdTextColor);
			str_format(aBuf, sizeof(aBuf), "%s: ", pLine->m_aName);
			TextRender()->TextShadowed(&Cursor, aBuf, -1, ShadowOffset, ShadowColor, TextColor);
		}

		// render line
		if(pLine->m_ClientID < 0)
			TextColor = ColorSystem;
		else if(pLine->m_Mode == CHAT_WHISPER)
			TextColor = ColorWhisper;
		else if(pLine->m_Mode == CHAT_TEAM)
			TextColor = ColorTeamText;
		else
			TextColor = ColorAllText;

		if(pLine->m_Highlighted)
		{
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);

			TextRender()->TextOutlineColor(ColorHighlightOutline.r,
										   ColorHighlightOutline.g,
										   ColorHighlightOutline.b,
										   ColorHighlightOutline.a);

			TextRender()->TextEx(&Cursor, pLine->m_aText, -1);
		}
		else
			TextRender()->TextShadowed(&Cursor, pLine->m_aText, -1, ShadowOffset, ShadowColor, TextColor);
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);

	HandleCommands(x+CategoryWidth, Height - 24.f, 200.0f-CategoryWidth);
}

void CChat::Say(int Mode, const char *pLine)
{
	m_LastChatSend = time_get();

	// send chat message
	CNetMsg_Cl_Say Msg;
	Msg.m_Mode = Mode;
	Msg.m_Target = Mode==CHAT_WHISPER ? m_WhisperTarget : -1;
	Msg.m_pMessage = pLine;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

bool CChat::IsTypingCommand() const
{
	return m_Input.GetString()[0] == '/' && !m_IgnoreCommand;
}

// chat commands handlers
void CChat::HandleCommands(float x, float y, float w)
{
	// render commands menu
	if(m_Mode != CHAT_NONE && IsTypingCommand())
	{
		const float Alpha = 0.90f;
		const float LineWidth = w;
		const float LineHeight = 8.0f;

		m_Commands.Filter(m_Input.GetString()); // flag active commands, update selected command
		const int ActiveCount = m_Commands.CountActiveCommands();

		if(ActiveCount > 0) // at least one command to display
		{
			CUIRect Rect = {x, y-(ActiveCount+1)*LineHeight, LineWidth, (ActiveCount+1)*LineHeight};
			RenderTools()->DrawUIRect(&Rect,  vec4(0.125f, 0.125f, 0.125f, Alpha), CUI::CORNER_ALL, 3.0f);

			// render notification
			{
				y -= LineHeight;
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Rect.x + 5.0f, y, 5.0f, TEXTFLAG_RENDER);
				TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
				if(m_Commands.GetSelectedCommand() && str_startswith(m_Input.GetString() + 1, m_Commands.GetSelectedCommand()->m_aName))
					TextRender()->TextEx(&Cursor, Localize("Press Enter to confirm or Esc to cancel"), -1);
				else
					TextRender()->TextEx(&Cursor, Localize("Press Tab to select or Esc to cancel"), -1);
			}

			// render commands
			for(int i = ActiveCount - 1; i >= 0; i--)
			{
				y -= LineHeight;
				CUIRect HighlightRect = {Rect.x, y, LineWidth, LineHeight-1};

				// retrieve command
				const CChatCommand* pCommand = m_Commands.GetCommand(i);

				if(!pCommand->m_pfnCallback)
					RenderTools()->DrawUIRect(&HighlightRect,  vec4(0.0f, 0.6f, 0.6f, 0.2f), CUI::CORNER_ALL, 0);

				// draw selection box
				if(pCommand == m_Commands.GetSelectedCommand())
					RenderTools()->DrawUIRect(&HighlightRect,  vec4(0.25f, 0.25f, 0.6f, Alpha), CUI::CORNER_ALL, 2.0f);

				// print command
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Rect.x + 5.0f, y, 5.0f, TEXTFLAG_RENDER);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextEx(&Cursor, pCommand->m_aName, -1);
				TextRender()->TextEx(&Cursor, " ", -1);

				TextRender()->TextColor(0.0f, 0.5f, 0.5f, 1.0f);
				for(const char *c = pCommand->m_aArgsFormat; *c; c++)
				{
					// Integer argument
					if(*c == 'i')
					{
						TextRender()->TextEx(&Cursor, "<number> ", -1);
					}
					// Player name argument
					else if(*c == 'p')
					{
						TextRender()->TextEx(&Cursor, "<playername> ", -1);
					}
					// String argument
					else if(*c == 's')
					{
						TextRender()->TextEx(&Cursor, "<text> ", -1);
					}
					// Subcommand argument
					else if(*c == 'c')
					{
						TextRender()->TextEx(&Cursor, "<subcommand> ", -1);
					}
				}
				TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
				TextRender()->TextEx(&Cursor, pCommand->m_aHelpText, -1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}
}

bool CChat::ExecuteCommand(bool Execute)
{
	if(m_Commands.CountActiveCommands() == 0)
		return false;

	const char* pCommandStr = m_Input.GetString();
	const CChatCommand* pCommand = m_Commands.GetSelectedCommand();
	dbg_assert(pCommand != 0, "selected command does not exist");
	bool IsFullMatch = str_find(pCommandStr + 1, pCommand->m_aName); // if the command text is fully inside pCommandStr (aka, not a shortcut)

	if(IsFullMatch && Execute)
	{
		// execute command
		if(pCommand->m_pfnCallback)
			pCommand->m_pfnCallback(this, pCommandStr);
		else
		{
			// Send server-side command.
			CNetMsg_Cl_Command Msg;
			Msg.m_Name = pCommand->m_aName;
			Msg.m_Arguments = pCommandStr;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
			m_Mode = CHAT_NONE;
			m_pClient->OnRelease();
		}
		return true;
	}
	else if(!IsFullMatch && !Execute)
	{
		// autocomplete command
		char aBuf[128];
		str_copy(aBuf, "/", sizeof(aBuf));
		str_append(aBuf, pCommand->m_aName, sizeof(aBuf));
		str_append(aBuf, " ", sizeof(aBuf));

		m_Input.Set(aBuf);
		m_Input.SetCursorOffset(str_length(aBuf));
		m_InputUpdate = true;
		return true;
	}
	return false;
}

// returns -1 if not found or duplicate
int CChat::IdentifyNameParameter(const char* pCommand) const
{
	// retrieve name parameter
	const char* pParameter = str_skip_to_whitespace_const(pCommand);
	if(!pParameter)
		return -1;

	// do not count leading and trailing whitespaces
	char aName[MAX_NAME_LENGTH];
	str_copy(aName, pParameter+1, sizeof(aName));
	str_clean_whitespaces_simple(aName);

	int TargetID = -1;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_aClients[i].m_Active || i == m_pClient->m_LocalClientID) // skip local user
			continue;
		if(str_length(m_pClient->m_aClients[i].m_aName) == str_length(aName) && str_comp(m_pClient->m_aClients[i].m_aName, aName) == 0)
		{
			// name strictly matches
			if(TargetID != -1)
			{
				// duplicate; be conservative
				dbg_msg("chat", "name duplicate found, aborting command");
				return -1;
			}
			TargetID = i;
		}
	}
	return TargetID;
}


// callback functions for commands
void CChat::Com_All(CChat *pChatData, const char* pCommand)
{
	const char* pParameter = str_skip_to_whitespace_const(pCommand);
	pChatData->m_ChatCmdBuffer[0] = 0;
	if(pParameter++ && *pParameter) // skip the first space
	{
		// save the parameter in a buffer before EnableMode clears it
		str_copy(pChatData->m_ChatCmdBuffer, pParameter, sizeof(pChatData->m_ChatCmdBuffer));
	}
	pChatData->EnableMode(CHAT_ALL, pChatData->m_ChatCmdBuffer);
}

void CChat::Com_Team(CChat *pChatData, const char* pCommand)
{
	const char* pParameter = str_skip_to_whitespace_const(pCommand);
	pChatData->m_ChatCmdBuffer[0] = 0;
	if(pParameter++ && *pParameter) // skip the first space
	{
		// save the parameter in a buffer before EnableMode clears it
		str_copy(pChatData->m_ChatCmdBuffer, pParameter, sizeof(pChatData->m_ChatCmdBuffer));
	}
	pChatData->EnableMode(CHAT_TEAM, pChatData->m_ChatCmdBuffer);
}

void CChat::Com_Reply(CChat *pChatData, const char* pCommand)
{
	if(pChatData->m_LastWhisperFrom == -1)
		pChatData->ClearInput(); // just reset the chat
	else
	{
		pChatData->m_WhisperTarget = pChatData->m_LastWhisperFrom;

		const char* pParameter = str_skip_to_whitespace_const(pCommand);
		pChatData->m_ChatCmdBuffer[0] = 0;
		if(pParameter++ && *pParameter) // skip the first space
		{
			// save the parameter in a buffer before EnableMode clears it
			str_copy(pChatData->m_ChatCmdBuffer, pParameter, sizeof(pChatData->m_ChatCmdBuffer));
		}
		pChatData->EnableMode(CHAT_WHISPER, pChatData->m_ChatCmdBuffer);
	}
}

void CChat::Com_Whisper(CChat *pChatData, const char* pCommand)
{
	int TargetID = pChatData->IdentifyNameParameter(pCommand);
	if(TargetID != -1)
	{
		pChatData->m_WhisperTarget = TargetID;
		pChatData->EnableMode(CHAT_WHISPER);
	}
}

void CChat::Com_Mute(CChat *pChatData, const char* pCommand)
{
	int TargetID = pChatData->IdentifyNameParameter(pCommand);
	if(TargetID != -1)
	{
		bool isMuted = pChatData->m_pClient->m_aClients[TargetID].m_ChatIgnore;
		if(isMuted)
			pChatData->m_pClient->Blacklist()->RemoveIgnoredPlayer(pChatData->m_pClient->m_aClients[TargetID].m_aName, pChatData->m_pClient->m_aClients[TargetID].m_aClan);
		else
			pChatData->m_pClient->Blacklist()->AddIgnoredPlayer(pChatData->m_pClient->m_aClients[TargetID].m_aName, pChatData->m_pClient->m_aClients[TargetID].m_aClan);
		pChatData->m_pClient->m_aClients[TargetID].m_ChatIgnore ^= 1;

		pChatData->ClearInput();

		char aMsg[128];
		str_format(aMsg, sizeof(aMsg), !isMuted ? Localize("'%s' was muted") : Localize("'%s' was unmuted"), pChatData->m_pClient->m_aClients[TargetID].m_aName);
		pChatData->AddLine(aMsg, CLIENT_MSG, CHAT_ALL);
	}
	pChatData->m_Mode = CHAT_NONE;
	pChatData->m_pClient->OnRelease();
}

void CChat::Com_Befriend(CChat *pChatData, const char* pCommand)
{
	int TargetID = pChatData->IdentifyNameParameter(pCommand);
	if(TargetID != -1)
	{
		bool isFriend = pChatData->m_pClient->m_aClients[TargetID].m_Friend;
		if(isFriend)
			pChatData->m_pClient->Friends()->RemoveFriend(pChatData->m_pClient->m_aClients[TargetID].m_aName, pChatData->m_pClient->m_aClients[TargetID].m_aClan);
		else
			pChatData->m_pClient->Friends()->AddFriend(pChatData->m_pClient->m_aClients[TargetID].m_aName, pChatData->m_pClient->m_aClients[TargetID].m_aClan);
		pChatData->m_pClient->m_aClients[TargetID].m_Friend ^= 1;

		pChatData->ClearInput();

		char aMsg[128];
		str_format(aMsg, sizeof(aMsg), !isFriend ? Localize("'%s' was added as a friend") : Localize("'%s' was removed as a friend"), pChatData->m_pClient->m_aClients[TargetID].m_aName);
		pChatData->AddLine(aMsg, CLIENT_MSG, CHAT_ALL);
	}
	pChatData->m_Mode = CHAT_NONE;
	pChatData->m_pClient->OnRelease();
}


// CChatCommands methods
CChat::CChatCommands::CChatCommands() : m_pSelectedCommand(0)
{
	mem_zero(m_aCommands, sizeof(m_aCommands));
}
CChat::CChatCommands::~CChatCommands() {}

// selection
void CChat::CChatCommands::Reset()
{
	m_pSelectedCommand = 0;
}

void CChat::CChatCommands::AddCommand(const char *pName, const char *pArgsFormat, const char *pHelpText, COMMAND_CALLBACK pfnCallback)
{
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(!m_aCommands[i].m_Used)
		{
			mem_zero(&m_aCommands[i], sizeof(CChatCommand));

			str_copy(m_aCommands[i].m_aName, pName, sizeof(m_aCommands[i].m_aName));
			str_copy(m_aCommands[i].m_aHelpText, pHelpText, sizeof(m_aCommands[i].m_aHelpText));
			str_copy(m_aCommands[i].m_aArgsFormat, pArgsFormat, sizeof(m_aCommands[i].m_aArgsFormat));

			m_aCommands[i].m_pfnCallback = pfnCallback;
			m_aCommands[i].m_aFiltered = false;
			m_aCommands[i].m_Used = true;
			break;
		}
	}
}

void CChat::CChatCommands::ClearCommands()
{
	mem_zero(m_aCommands, sizeof(m_aCommands));
}

// Example: /whisper command will match "/whi", "/whisper" and "/whisper tee"
void CChat::CChatCommands::Filter(const char* pLine)
{
	char aCommandStr[64];

	// Ignore first char "/" when filtering.
	str_copy(aCommandStr, pLine + 1, sizeof(aCommandStr));

	// truncate the string at the first whitespace to get the command
	char* pFirstWhitespace = str_skip_to_whitespace(aCommandStr);
	if(pFirstWhitespace)
		*pFirstWhitespace = 0;

	for(int i = 0; i < MAX_COMMANDS; i++)
		if(m_aCommands[i].m_Used)
			m_aCommands[i].m_aFiltered = (str_find_nocase(m_aCommands[i].m_aName, aCommandStr) != m_aCommands[i].m_aName);

	// also update selected command
	if(!GetSelectedCommand() || GetSelectedCommand()->m_aFiltered)
	{
		if(CountActiveCommands() > 0)
			m_pSelectedCommand = &m_aCommands[GetActiveIndex(0)]; // default to first command
		else
			m_pSelectedCommand = 0;
	}
}

// this will not return a correct value if we are not writing a command (m_Input.GetString()[0] == '/')
int CChat::CChatCommands::CountActiveCommands() const
{
	int n = 0;
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(m_aCommands[i].m_Used && !m_aCommands[i].m_aFiltered)
			n++;
	}
	return n;
}

const CChat::CChatCommand* CChat::CChatCommands::GetCommand(int Index) const
{
	int RealIndex = GetActiveIndex(Index);
	return RealIndex != -1 ? &m_aCommands[RealIndex] : 0;
}

CChat::CChatCommand *CChat::CChatCommands::GetCommandByName(const char *pName)
{
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(m_aCommands[i].m_Used && str_comp(m_aCommands[i].m_aName, pName) == 0)
			return &m_aCommands[i];
	}

	return 0;
}

const CChat::CChatCommand* CChat::CChatCommands::GetSelectedCommand() const
{
	return m_pSelectedCommand;
}

void CChat::CChatCommands::SelectPreviousCommand()
{
	CChatCommand* LastCommand = 0x0;
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(!m_aCommands[i].m_Used || m_aCommands[i].m_aFiltered)
			continue;
		if(&m_aCommands[i] == m_pSelectedCommand)
		{
			m_pSelectedCommand = LastCommand;
			return;
		}
		LastCommand = &m_aCommands[i];
	}
}

void CChat::CChatCommands::SelectNextCommand()
{
	bool FoundSelected = false;
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(!m_aCommands[i].m_Used || m_aCommands[i].m_aFiltered)
			continue;
		if(FoundSelected)
		{
			m_pSelectedCommand = &m_aCommands[i];
			return;
		}
		if(&m_aCommands[i] == m_pSelectedCommand)
			FoundSelected = true;
	}
}

int CChat::CChatCommands::GetActiveIndex(int Index) const
{
	for(int i = 0; i < MAX_COMMANDS; i++)
	{
		if(!m_aCommands[i].m_Used || m_aCommands[i].m_aFiltered)
			Index++;
		if(i == Index)
			return i;
	}
	
	return -1;
}
