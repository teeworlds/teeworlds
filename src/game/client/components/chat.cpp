/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/localization.h>

#include <game/client/components/scoreboard.h>
#include <game/client/components/sounds.h>

#include "menus.h"
#include "chat.h"


CChat::CChat()
{
	// init chat commands (must be in alphabetical order)
	static CChatCommand s_aapCommands[] = {
		{"/all", " - Switch to all chat", &Com_All},
		{"/friend", " <player name>", &Com_Befriend},
		{"/m", " <player name>", &Com_Mute},
		{"/mute", " <player name>", &Com_Mute},
		{"/r", " - Reply to a whisper", &Com_Reply},
		{"/team", " - Switch to team chat", &Com_Team},
		{"/w", " <player name>", &Com_Whisper},
		{"/whisper", " <player name>", &Com_Whisper},
	};
	const int CommandsCount = sizeof(s_aapCommands) / sizeof(CChatCommand);
	m_pCommands = new CChatCommands(s_aapCommands, CommandsCount);

	OnReset();
}

CChat::~CChat()
{
	delete m_pCommands;
}

void CChat::OnReset()
{
	for(int i = 0; i < MAX_LINES; i++)
	{
		m_aLines[i].m_Time = 0;
		m_aLines[i].m_aText[0] = 0;
		m_aLines[i].m_aName[0] = 0;
	}

	m_Mode = CHAT_NONE;
	// m_WhisperTarget = -1;
	m_LastWhisperFrom = -1;
	m_ReverseCompletion = false;
	m_Show = false;
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
	m_pCommands->Reset();

	for(int i = 0; i < CHAT_NUM; ++i)
		m_aLastSoundPlayed[i] = 0;
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
	Console()->Register("whisper", "ir", CFGFLAG_CLIENT, ConWhisper, this, "Whisper to a client in chat");
	Console()->Register("chat", "s?i", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team/whisper mode");
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat");
}

bool CChat::OnInput(IInput::CEvent Event)
{
	if(m_Mode == CHAT_NONE)
		return false;

	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
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
		if(IsTypingCommand() && ExecuteCommand())
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
		if (m_Mode == CHAT_WHISPER)
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
					if(str_comp_nocase_num(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer, str_length(m_aCompletionBuffer)) &&
						str_find_nocase(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer))
						Found = true;
				}
				else if(!str_comp_nocase_num(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer, str_length(m_aCompletionBuffer)))
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
			m_pCommands->SelectPreviousCommand();
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
			m_pCommands->SelectNextCommand();
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
}

void CChat::ClearInput()
{
	m_Input.Clear();
	Input()->Clear();
	m_CompletionChosen = -1;

	m_IgnoreCommand = false;
	m_pCommands->Reset();
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		AddLine(pMsg->m_ClientID, pMsg->m_Mode, pMsg->m_pMessage, pMsg->m_TargetID);
	}
}

void CChat::AddLine(int ClientID, int Mode, const char *pLine, int TargetID)
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
		if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
			(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
			Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
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
		m_aLines[m_CurrentLine].m_Time = time_get();
		m_aLines[m_CurrentLine].m_Size[0].y = -1.0f;
		m_aLines[m_CurrentLine].m_Size[1].y = -1.0f;
		m_aLines[m_CurrentLine].m_ClientID = ClientID;
		m_aLines[m_CurrentLine].m_TargetID = TargetID;
		m_aLines[m_CurrentLine].m_Mode = Mode;
		m_aLines[m_CurrentLine].m_NameColor = -2;

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

		m_aLines[m_CurrentLine].m_Highlighted =  Highlighted;

		int NameCID = ClientID;
		if(Mode == CHAT_WHISPER && ClientID == m_pClient->m_LocalClientID && TargetID >= 0)
			NameCID = TargetID;

		if(ClientID == SERVER_MSG)
		{
			m_aLines[m_CurrentLine].m_aName[0] = 0;
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "*** %s", pLine);
		}
		else if(ClientID == CLIENT_MSG)
		{
			m_aLines[m_CurrentLine].m_aName[0] = 0;
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "— %s", pLine);
		}
		else
		{
			if(m_pClient->m_aClients[ClientID].m_Team == TEAM_SPECTATORS)
				m_aLines[m_CurrentLine].m_NameColor = TEAM_SPECTATORS;

			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
			{
				if(m_pClient->m_aClients[ClientID].m_Team == TEAM_RED)
					m_aLines[m_CurrentLine].m_NameColor = TEAM_RED;
				else if(m_pClient->m_aClients[ClientID].m_Team == TEAM_BLUE)
					m_aLines[m_CurrentLine].m_NameColor = TEAM_BLUE;
			}

			str_format(m_aLines[m_CurrentLine].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName), "%s", m_pClient->m_aClients[NameCID].m_aName);
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}

		char aBuf[1024];
		char aBufMode[32];
		if(Mode == CHAT_WHISPER)
			str_copy(aBufMode, "whisper", sizeof(aBufMode));
		else if(Mode == CHAT_TEAM)
			str_copy(aBufMode, "teamchat", sizeof(aBufMode));
		else
			str_copy(aBufMode, "chat", sizeof(aBufMode));

		str_format(aBuf, sizeof(aBuf), "%2d: %s: %s", NameCID, m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
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

void CChat::OnRender()
{
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

	// dont render chat if the menu is active
	if(m_pClient->m_pMenus->IsActive())
		return;

	const float Height = 300.0f;
	const float Width = Height*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);
	float x = 12.0f;
	float y = Height-20.0f;
	const int LocalCID = m_pClient->m_LocalClientID;
	const CGameClient::CClientData& LocalClient = m_pClient->m_aClients[LocalCID];
	const int LocalTteam = LocalClient.m_Team;
	// bool showCommands;
	float CategoryWidth = 0;

	if(m_Mode == CHAT_WHISPER && !m_pClient->m_aClients[m_WhisperTarget].m_Active)
		m_Mode = CHAT_NONE;
	else if(m_Mode != CHAT_NONE)
	{
		// calculate category text size
		// TODO: rework TextRender. Writing the same code twice to calculate a simple thing as width is ridiculus
		float CategoryHeight;
		const float IconOffsetX = m_Mode == CHAT_WHISPER ? 6.0f : 0.0f;
		const float CategoryFontSize = 8.0f;
		const float InputFontSize = 8.0f;
		char aCatText[48];

		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, x, y, CategoryFontSize, 0);

			if(m_Mode == CHAT_ALL)
				str_copy(aCatText, Localize("All"), sizeof(aCatText));
			else if(m_Mode == CHAT_TEAM)
			{
				if(LocalTteam == TEAM_SPECTATORS)
					str_copy(aCatText, Localize("Spectators"), sizeof(aCatText));
				else
					str_copy(aCatText, Localize("Team"), sizeof(aCatText));
			}
			else if(m_Mode == CHAT_WHISPER)
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
		const vec4 CRCWhite(1.0f, 1.0f, 1.0f, 0.25f);
		const vec4 CRCTeam(0.4f, 1.0f, 0.4f, 0.4f);
		const vec4 CRCWhisper(0.0f, 0.5f, 1.0f, 0.5f);

		vec4 CatRectColor = CRCWhite;
		if(m_Mode == CHAT_TEAM)
			CatRectColor = CRCTeam;
		else if(m_Mode == CHAT_WHISPER)
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

		if(m_Mode == CHAT_WHISPER)
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

		Graphics()->SetColor(1, 1, 1, 1);
		Graphics()->QuadsDrawTL(&QuadIcon, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();

		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x + IconOffsetX, y, CategoryFontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Width-190.0f;
		Cursor.m_MaxLines = 2;

		if(m_Mode == CHAT_WHISPER)
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

		TextRender()->TextEx(&Cursor, m_Input.GetString()+m_ChatStringOffset, m_Input.GetCursorOffset()-m_ChatStringOffset);
		static float MarkerOffset = TextRender()->TextWidth(0, 8.0f, "|", -1)/3;
		CTextCursor Marker = Cursor;
		Marker.m_X -= MarkerOffset;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, m_Input.GetString()+m_Input.GetCursorOffset(), -1);
	}

	y -= 8.0f;

	// show all chat when typing
	m_Show |= m_Mode != CHAT_NONE;

	int64 Now = time_get();
	const int64 TimeFreq = time_freq();

	// get scoreboard data
	const bool IsScoreboardActive = m_pClient->m_pScoreboard->Active();
	CUIRect ScoreboardRect = m_pClient->m_pScoreboard->GetScoreboardRect();
	const CUIRect ScoreboardScreen = *UI()->Screen();
	CUIRect ScoreboardRectFixed;
	ScoreboardRectFixed.x = ScoreboardRect.x/ScoreboardScreen.w * Width;
	ScoreboardRectFixed.y = ScoreboardRect.y/ScoreboardScreen.h * Height;
	ScoreboardRectFixed.w = ScoreboardRect.w/ScoreboardScreen.w * Width;
	ScoreboardRectFixed.h = ScoreboardRect.h/ScoreboardScreen.h * Height;

	float LineWidth = 200.0f;
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

	float Begin = x;
	float FontSize = 6.0f;
	CTextCursor Cursor;
	int OffsetType = IsScoreboardActive ? 1 : 0;

	// get the y offset (calculate it if we haven't done that yet)
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		CLine& Line = m_aLines[r];

		if(m_aLines[r].m_aText[0] == 0) break;

		if(Line.m_Size[OffsetType].y < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, Begin, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = LineWidth;

			char aBuf[768] = {0};
			if(Line.m_Mode == CHAT_TEAM)
				str_format(aBuf, sizeof(aBuf), "[%s] ", Localize("Team"));
			else if(Line.m_Mode == CHAT_WHISPER)
				str_format(aBuf, sizeof(aBuf), "[%s] ", Localize("Whisper"));

			if(Line.m_ClientID >= 0)
			{
				Cursor.m_X += RenderTools()->GetClientIdRectSize(Cursor.m_FontSize);
				str_append(aBuf, Line.m_aName, sizeof(aBuf));
				str_append(aBuf, ": ", sizeof(aBuf));
			}

			str_append(aBuf, Line.m_aText, sizeof(aBuf));

			TextRender()->TextEx(&Cursor, aBuf, -1);
			// FIXME: sometimes an empty line will pop here when the cursor reaches the end of line
			Line.m_Size[OffsetType].y = Cursor.m_LineCount * Cursor.m_FontSize;
			Line.m_Size[OffsetType].x = Cursor.m_LineCount == 1 ? Cursor.m_X - Cursor.m_StartX : LineWidth;
		}
	}

	if(m_Show)
	{
		CUIRect Rect;
		Rect.x = 0;
		Rect.y = HeightLimit - 2.0f;
		Rect.w = LineWidth + x;
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

	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		CLine& Line = m_aLines[r];

		if(m_aLines[r].m_aText[0] == 0) break;

		if(Now > Line.m_Time+16*TimeFreq && !m_Show)
			break;

		y -= Line.m_Size[OffsetType].y;

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		float Blend = Now > Line.m_Time+14*TimeFreq && !m_Show ? 1.0f-(Now-Line.m_Time-14*TimeFreq)/(2.0f*TimeFreq) : 1.0f;

		const float HlTimeFull = 1.0f;
		const float HlTimeFade = 1.0f;

		float Delta = (Now - Line.m_Time) / (float)TimeFreq;
		const float HighlightBlend = 1.0f - clamp(Delta - HlTimeFull, 0.0f, HlTimeFade) / HlTimeFade;

		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		const vec2 ShadowOffset(0.8f, 1.5f);
		const vec4 ShadowWhisper(0.09f, 0.f, 0.26f, Blend * 0.9f);
		const vec4 ShadowBlack(0, 0, 0, Blend * 0.9f);
		vec4 ShadowColor = ShadowBlack;

		if(Line.m_Mode == CHAT_WHISPER)
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
			mix(Line.m_Mode == CHAT_TEAM ? 0.6f : 0.5f, 1.0f, HighlightBlend));

		vec4 TextColor = ColorAllText;

		if(Line.m_Highlighted && ColorHighlightBg.a > 0.001f)
		{
			CUIRect BgRect;
			BgRect.x = Cursor.m_X;
			BgRect.y = Cursor.m_Y + 2.0f;
			BgRect.w = Line.m_Size[OffsetType].x - 2.0f;
			BgRect.h = Line.m_Size[OffsetType].y;

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
		if(Line.m_Mode == CHAT_WHISPER)
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
			if(Line.m_ClientID == LocalCID && Line.m_TargetID >= 0)
				Graphics()->QuadsSetSubset(1, 0, 0, 1); // To
			else if(Line.m_TargetID == LocalCID)
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
		if(Line.m_ClientID < 0)
			TextColor = ColorSystem;
		else if(Line.m_Mode == CHAT_WHISPER)
			TextColor = ColorWhisper;
		else if(Line.m_Mode == CHAT_TEAM)
			TextColor = ColorTeamPre;
		else if(Line.m_NameColor == TEAM_RED)
			TextColor = ColorRed;
		else if(Line.m_NameColor == TEAM_BLUE)
			TextColor = ColorBlue;
		else if(Line.m_NameColor == TEAM_SPECTATORS)
			TextColor = ColorSpec;
		else
			TextColor = ColorAllPre;

		if(Line.m_ClientID >= 0)
		{
			int NameCID = Line.m_ClientID;
			if(Line.m_Mode == CHAT_WHISPER && Line.m_ClientID == m_pClient->m_LocalClientID && Line.m_TargetID >= 0)
				NameCID = Line.m_TargetID;

			vec4 IdTextColor = vec4(0.1f*Blend, 0.1f*Blend, 0.1f*Blend, 1.0f*Blend);
			vec4 BgIdColor = TextColor;
			BgIdColor.a = 0.5f*Blend;
			RenderTools()->DrawClientID(TextRender(), &Cursor, NameCID, BgIdColor, IdTextColor);
			str_format(aBuf, sizeof(aBuf), "%s: ", Line.m_aName);
			TextRender()->TextShadowed(&Cursor, aBuf, -1, ShadowOffset, ShadowColor, TextColor);
		}

		// render line
		if(Line.m_ClientID < 0)
			TextColor = ColorSystem;
		else if(Line.m_Mode == CHAT_WHISPER)
			TextColor = ColorWhisper;
		else if(Line.m_Mode == CHAT_TEAM)
			TextColor = ColorTeamText;
		else
			TextColor = ColorAllText;

		if(Line.m_Highlighted)
		{
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);

			TextRender()->TextOutlineColor(ColorHighlightOutline.r,
										   ColorHighlightOutline.g,
										   ColorHighlightOutline.b,
										   ColorHighlightOutline.a);

			TextRender()->TextEx(&Cursor, Line.m_aText, -1);
		}
		else
			TextRender()->TextShadowed(&Cursor, Line.m_aText, -1, ShadowOffset, ShadowColor, TextColor);
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

		m_pCommands->Filter(m_Input.GetString()); // flag active commands, update selected command
		const int ActiveCount = m_pCommands->CountActiveCommands();

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
				TextRender()->TextEx(&Cursor, Localize("Press Enter to confirm or Esc to cancel"), -1);				
			}

			// render commands
			for(int i = ActiveCount - 1; i >= 0; i--)
			{
				y -= LineHeight;
				CUIRect HighlightRect = {Rect.x, y, LineWidth, LineHeight-1};

				// retrieve command
				const CChatCommand* pCommand = m_pCommands->GetCommand(i);

				// draw selection box
				if(pCommand == m_pCommands->GetSelectedCommand())
					RenderTools()->DrawUIRect(&HighlightRect,  vec4(0.25f, 0.25f, 0.6f, Alpha), CUI::CORNER_ALL, 2.0f);

				// print command
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Rect.x + 5.0f, y, 5.0f, TEXTFLAG_RENDER);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextEx(&Cursor, pCommand->m_pCommandText, -1);
				TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
				TextRender()->TextEx(&Cursor, pCommand->m_pHelpText, -1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}
}

bool CChat::ExecuteCommand()
{
	if(m_pCommands->CountActiveCommands() == 0)
		return false;
	
	const char* pCommandStr = m_Input.GetString();
	const CChatCommand* pCommand = m_pCommands->GetSelectedCommand();
	dbg_assert(pCommand != 0, "selected command does not exist");
	bool IsFullMatch = str_find(pCommandStr, pCommand->m_pCommandText); // if the command text is fully inside pCommandStr (aka, not a shortcut)

	if(IsFullMatch)
	{
		// execute command
		if(pCommand->m_pfnFunc != 0)
			pCommand->m_pfnFunc(this, pCommandStr);
	}
	else
	{
		// autocomplete command
		char aBuf[128];
		str_copy(aBuf, pCommand->m_pCommandText, sizeof(aBuf));
		str_append(aBuf, " ", sizeof(aBuf));

		m_Input.Set(aBuf);
		m_Input.SetCursorOffset(str_length(aBuf));
		m_InputUpdate = true;
	}
	return true;
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
				dbg_msg("chat", "name duplicate found, aborting whisper command");
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
	char *pBuf = 0x0;
	if(pParameter++ && *pParameter) // skip the first space
	{
		// save the parameter in a buffer before EnableMode clears it
		pBuf = (char*)mem_alloc(str_length(pParameter) + 1, 1);
		str_copy(pBuf, pParameter, str_length(pParameter) + 1);
	}
	pChatData->EnableMode(CHAT_ALL, pBuf);
	mem_free(pBuf);
}

void CChat::Com_Team(CChat *pChatData, const char* pCommand)
{
	const char* pParameter = str_skip_to_whitespace_const(pCommand);
	char *pBuf = 0x0;
	if(pParameter++ && *pParameter) // skip the first space
	{
		// save the parameter in a buffer before EnableMode clears it
		pBuf = (char*)mem_alloc(str_length(pParameter) + 1, 1);
		str_copy(pBuf, pParameter, str_length(pParameter) + 1);
	}
	pChatData->EnableMode(CHAT_TEAM, pBuf);
	mem_free(pBuf);
}

void CChat::Com_Reply(CChat *pChatData, const char* pCommand)
{
	if(pChatData->m_LastWhisperFrom == -1)
		pChatData->ClearInput(); // just reset the chat
	else
	{
		pChatData->m_WhisperTarget = pChatData->m_LastWhisperFrom;

		const char* pParameter = str_skip_to_whitespace_const(pCommand);
		char *pBuf = 0x0;
		if(pParameter++ && *pParameter) // skip the first space
		{
			// save the parameter in a buffer before EnableMode clears it
			pBuf = (char*)mem_alloc(str_length(pParameter) + 1, 1);
			str_copy(pBuf, pParameter, sizeof(pBuf));
		}
		pChatData->EnableMode(CHAT_WHISPER, pBuf);
		mem_free(pBuf);
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
		pChatData->m_pClient->m_aClients[TargetID].m_ChatIgnore ^= 1;
		
		pChatData->ClearInput();
		
		char aMsg[128];
		str_format(aMsg, sizeof(aMsg), pChatData->m_pClient->m_aClients[TargetID].m_ChatIgnore ? Localize("'%s' was muted") : Localize("'%s' was unmuted"), pChatData->m_pClient->m_aClients[TargetID].m_aName);
		pChatData->AddLine(CLIENT_MSG, CHAT_ALL, aMsg, -1);
	}
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
		pChatData->AddLine(CLIENT_MSG, CHAT_ALL, aMsg, -1);
	}
}


// CChatCommands methods
CChat::CChatCommands::CChatCommands(CChatCommand apCommands[], int Count) : m_apCommands(apCommands), m_Count(Count), m_pSelectedCommand(0x0) { }
CChat::CChatCommands::~CChatCommands() { }

// selection
void CChat::CChatCommands::Reset()
{
	m_pSelectedCommand = 0x0;
}

// Example: /whisper command will match "/whi", "/whisper" and "/whisper tee"
void CChat::CChatCommands::Filter(const char* pLine)
{
	char aCommandStr[64];
	str_copy(aCommandStr, pLine, sizeof(aCommandStr));
	// truncate the string at the first whitespace to get the command
	char* pFirstWhitespace = str_skip_to_whitespace(aCommandStr);
	if(pFirstWhitespace)
		*pFirstWhitespace = 0;

	for(int i = 0; i < m_Count; i++)
		m_apCommands[i].m_aFiltered = (str_find_nocase(m_apCommands[i].m_pCommandText, aCommandStr) != m_apCommands[i].m_pCommandText);

	// also update selected command
	if(!GetSelectedCommand() || GetSelectedCommand()->m_aFiltered)
	{
		if(CountActiveCommands() > 0)
			m_pSelectedCommand = &m_apCommands[GetActiveIndex(0)]; // default to first command
		else
			m_pSelectedCommand = 0x0;
	}
}

// this will not return a correct value if we are not writing a command (m_Input.GetString()[0] == '/') 
int CChat::CChatCommands::CountActiveCommands() const
{
	int n = m_Count;
	for(int i = 0; i < m_Count; i++)
		n -= m_apCommands[i].m_aFiltered;
	return n;
}

const CChat::CChatCommand* CChat::CChatCommands::GetCommand(int index) const
{
	return &m_apCommands[GetActiveIndex(index)];
}

const CChat::CChatCommand* CChat::CChatCommands::GetSelectedCommand() const
{
	return m_pSelectedCommand;
}

void CChat::CChatCommands::SelectPreviousCommand()
{
	CChatCommand* LastCommand = 0x0;	
	for(int i = 0; i < m_Count; i++)
	{
		if(m_apCommands[i].m_aFiltered)
			continue;
		if(&m_apCommands[i] == m_pSelectedCommand)
		{
			m_pSelectedCommand = LastCommand;
			return;
		}
		LastCommand = &m_apCommands[i];
	}
}

void CChat::CChatCommands::SelectNextCommand()
{
	bool FoundSelected = false;	
	for(int i = 0; i < m_Count; i++)
	{
		if(m_apCommands[i].m_aFiltered)
			continue;
		if(FoundSelected)
		{
			m_pSelectedCommand = &m_apCommands[i];
			return;
		}
		if(&m_apCommands[i] == m_pSelectedCommand)
			FoundSelected = true;
	}
}

int CChat::CChatCommands::GetActiveIndex(int index) const
{
	for(int i = 0; i < m_Count; i++)
	{
		if(m_apCommands[i].m_aFiltered)
			index++;
		if(i == index)
			return i;
	}
	dbg_break();
	return -1;
}
