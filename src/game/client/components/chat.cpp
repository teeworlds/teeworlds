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
	OnReset();
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
		int Target = -1;
		if(pResult->NumArguments() == 2)
			Target = pResult->GetInteger(1);
		else
		{
			// pick next player as target
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				int ClientID = (pChat->m_pClient->m_LocalClientID + i) % MAX_CLIENTS;
				if(pChat->m_pClient->m_aClients[ClientID].m_Active && pChat->m_pClient->m_LocalClientID != ClientID)
				{
					Target = ClientID;
					break;
				}
			}
		}
		if(Target < 0 || Target >= MAX_CLIENTS || !pChat->m_pClient->m_aClients[Target].m_Active || pChat->m_pClient->m_LocalClientID == Target)
			pChat->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "please enter a valid ClientID");
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
		m_Mode = CHAT_NONE;
		m_pClient->OnRelease();
	}
	else if(Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER))
	{
		if(m_Input.GetString()[0])
		{
			bool AddEntry = false;

			if(m_LastChatSend+time_freq() < time_get())
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
	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_TAB)
	{
		if (m_Mode == CHAT_WHISPER)
		{
			// pick next player as target
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				int ClientID = (m_WhisperTarget + i) % MAX_CLIENTS;
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

				str_copy(m_aCompletionBuffer, m_Input.GetString()+m_PlaceholderOffset, min(static_cast<int>(sizeof(m_aCompletionBuffer)), m_PlaceholderLength+1));
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
					m_CompletionFav = m_CompletionChosen;
					break;
				}
			}

			// insert the name
			if(pCompletionString)
			{
				char aBuf[256];
				// add part before the name
				str_copy(aBuf, m_Input.GetString(), min(static_cast<int>(sizeof(aBuf)), m_PlaceholderOffset+1));

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
	else if (Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_DOWN)
	{
		if(m_pHistoryEntry)
			m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

		if (m_pHistoryEntry)
			m_Input.Set(m_pHistoryEntry->m_aText);
		else
			m_Input.Clear();
	}

	return true;
}


void CChat::EnableMode(int Mode)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(m_Mode == CHAT_NONE)
	{
		m_Mode = Mode;
		m_Input.Clear();
		Input()->Clear();
		m_CompletionChosen = -1;
	}
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		AddLine(pMsg->m_ClientID, pMsg->m_Mode, pMsg->m_pMessage);
	}
}

void CChat::AddLine(int ClientID, int Mode, const char *pLine)
{
	if(*pLine == 0 || (ClientID != -1 && (!g_Config.m_ClShowsocial || m_pClient->m_aClients[ClientID].m_aName[0] == '\0' || // unknown client
		m_pClient->m_aClients[ClientID].m_ChatIgnore ||
		(m_pClient->m_LocalClientID != ClientID && g_Config.m_ClShowChatFriends && !m_pClient->m_aClients[ClientID].m_Friend))))
		return;

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
		Highlighted = false;
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
		m_aLines[m_CurrentLine].m_YOffset[0] = -1.0f;
		m_aLines[m_CurrentLine].m_YOffset[1] = -1.0f;
		m_aLines[m_CurrentLine].m_ClientID = ClientID;
		m_aLines[m_CurrentLine].m_Mode = Mode;
		m_aLines[m_CurrentLine].m_NameColor = -2;

		if(ClientID != m_pClient->m_LocalClientID) // do not highlight our own messages
		{
			// check for highlighted name
			const char *pHL = str_find_nocase(pLine, m_pClient->m_aClients[m_pClient->m_LocalClientID].m_aName);
			if(pHL)
			{
				int Length = str_length(m_pClient->m_aClients[m_pClient->m_LocalClientID].m_aName);
				if((pLine == pHL || pHL[-1] == ' ') // space or nothing before
					&& (((pHL[Length] == 0 || pHL[Length] == ' ') || pHL[Length] == ':') && (pHL[Length+1] == 0 || pHL[Length+1] == ' '))) // space or nothing after, allowing a colon
				{
					Highlighted = true;
				}
				m_CompletionFav = ClientID;
			}
		}

		m_aLines[m_CurrentLine].m_Highlighted =  Highlighted || Mode == CHAT_WHISPER;

		if(ClientID == -1) // server message
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "*** ", sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
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

			str_format(m_aLines[m_CurrentLine].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName), "%2d: %s", ClientID, m_pClient->m_aClients[ClientID].m_aName);
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}

		char aBuf[1024];
		char aBufMode[32];
		if(Mode == CHAT_WHISPER)
			str_copy(aBufMode, "whisper", sizeof(aBufMode));
		else if(Mode == CHAT_TEAM)
			str_copy(aBufMode, "teamchat", sizeof(aBufMode));
		else
			str_copy(aBufMode, "chat", sizeof(aBufMode));
		str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, aBufMode, aBuf, Highlighted);
	}

	// play sound
	int64 Now = time_get();
	if(ClientID == -1)
	{
		if(Now-m_aLastSoundPlayed[CHAT_SERVER] >= time_freq()*3/10)
		{
			m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0);
			m_aLastSoundPlayed[CHAT_SERVER] = Now;
		}
	}
	else if(Highlighted)
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

	float Width = 300.0f*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, Width, 300.0f);
	float x = 5.0f;
	float y = 300.0f-20.0f;
	if(m_Mode != CHAT_NONE)
	{
		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Width-190.0f;
		Cursor.m_MaxLines = 2;

		char aBuf[32];
		if(m_Mode == CHAT_ALL)
			str_copy(aBuf, Localize("All"), sizeof(aBuf));
		else if(m_Mode == CHAT_TEAM)
			str_copy(aBuf, Localize("Team"), sizeof(aBuf));
		else if(m_Mode == CHAT_WHISPER)
			str_format(aBuf, sizeof(aBuf), "%s %2d: %s", Localize("To"), m_WhisperTarget, m_pClient->m_aClients[m_WhisperTarget].m_aName);
		else
			str_copy(aBuf, Localize("Chat"), sizeof(aBuf));
			
		TextRender()->TextEx(&Cursor, aBuf, -1);
		TextRender()->TextEx(&Cursor, ": ", -1);

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

	int64 Now = time_get();
	float LineWidth = m_pClient->m_pScoreboard->Active() ? 90.0f : 200.0f;
	float HeightLimit = m_pClient->m_pScoreboard->Active() ? 230.0f : m_Show ? 50.0f : 200.0f;
	float Begin = x;
	float FontSize = 6.0f;
	CTextCursor Cursor;
	int OffsetType = m_pClient->m_pScoreboard->Active() ? 1 : 0;
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		if(Now > m_aLines[r].m_Time+16*time_freq() && !m_Show)
			break;

		// get the y offset (calculate it if we haven't done that yet)
		if(m_aLines[r].m_YOffset[OffsetType] < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, Begin, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = LineWidth;
			TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
			TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
			m_aLines[r].m_YOffset[OffsetType] = Cursor.m_Y + Cursor.m_FontSize;
		}
		y -= m_aLines[r].m_YOffset[OffsetType];

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		float Blend = Now > m_aLines[r].m_Time+14*time_freq() && !m_Show ? 1.0f-(Now-m_aLines[r].m_Time-14*time_freq())/(2.0f*time_freq()) : 1.0f;

		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		// render name
		if(m_aLines[r].m_ClientID == -1)
			TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
		else if(m_aLines[r].m_Mode == CHAT_TEAM)
			TextRender()->TextColor(0.45f, 0.9f, 0.45f, Blend); // team message
		else if(m_aLines[r].m_NameColor == TEAM_RED)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // red
		else if(m_aLines[r].m_NameColor == TEAM_BLUE)
			TextRender()->TextColor(0.7f, 0.7f, 1.0f, Blend); // blue
		else if(m_aLines[r].m_NameColor == TEAM_SPECTATORS)
			TextRender()->TextColor(0.75f, 0.5f, 0.75f, Blend); // spectator
		else
			TextRender()->TextColor(0.8f, 0.8f, 0.8f, Blend);

		TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);

		// render line
		if(m_aLines[r].m_ClientID == -1)
			TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
		else if(m_aLines[r].m_Highlighted)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // highlighted
		else if(m_aLines[r].m_Mode == CHAT_TEAM)
			TextRender()->TextColor(0.65f, 1.0f, 0.65f, Blend); // team message
		else
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);

		TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
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
