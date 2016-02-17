/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/serverbrowser.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/teecomp.h>

#include <game/client/components/scoreboard.h>
#include <game/client/components/sounds.h>
#include <game/localization.h>

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

	m_Mode = MODE_NONE;
	m_Show = false;
	m_InputUpdate = false;
	m_ChatStringOffset = 0;
	m_CompletionChosen = -1;
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
		m_Mode = MODE_NONE;
		for(int i = 0; i < MAX_LINES; i++)
			m_aLines[i].m_Time = 0;
		m_CurrentLine = 0;
	}
}

void CChat::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	((CChat*)pUserData)->Say(0, pResult->GetString(0));
}

void CChat::ConSayTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CChat*)pUserData)->Say(1, pResult->GetString(0));
}

void CChat::ConChat(IConsole::IResult *pResult, void *pUserData)
{
	const char *pMode = pResult->GetString(0);
	if(str_comp(pMode, "all") == 0)
		((CChat*)pUserData)->EnableMode(0);
	else if(str_comp(pMode, "team") == 0)
		((CChat*)pUserData)->EnableMode(1);
	else
		((CChat*)pUserData)->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "expected all or team as mode");
}

void CChat::ConShowChat(IConsole::IResult *pResult, void *pUserData)
{
	((CChat *)pUserData)->m_Show = pResult->GetInteger(0) != 0;
}

void CChat::OnConsoleInit()
{
	Console()->Register("say", "r", CFGFLAG_CLIENT, ConSay, this, "Say in chat");
	Console()->Register("say_team", "r", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat");
	Console()->Register("chat", "s", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team mode");
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat");
}

bool CChat::OnInput(IInput::CEvent Event)
{
	if(m_Mode == MODE_NONE)
		return false;

	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		m_Mode = MODE_NONE;
		m_pClient->OnRelease();
	}
	else if(Event.m_Flags&IInput::FLAG_PRESS && (Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER))
	{
		if(m_Input.GetString()[0])
		{
			bool AddEntry = false;

			if(m_LastChatSend+time_freq() < time_get())
			{
				Say(m_Mode == MODE_ALL ? 0 : 1, m_Input.GetString());
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
				pEntry->m_Team = m_Mode == MODE_ALL ? 0 : 1;
				mem_copy(pEntry->m_aText, m_Input.GetString(), m_Input.GetLength()+1);
			}
		}
		m_pHistoryEntry = 0x0;
		m_Mode = MODE_NONE;
		m_pClient->OnRelease();
	}
	if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_TAB)
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
		m_CompletionChosen = (m_CompletionChosen+1)%(2*MAX_CLIENTS);
		for(int i = 0; i < 2*MAX_CLIENTS; ++i)
		{
			int SearchType = ((m_CompletionChosen+i)%(2*MAX_CLIENTS))/MAX_CLIENTS;
			int Index = (m_CompletionChosen+i)%MAX_CLIENTS;
			if(!m_pClient->m_Snap.m_paPlayerInfos[Index])
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
	else
	{
		// reset name completion process
		if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key != KEY_TAB)
			m_CompletionChosen = -1;

		m_OldChatStringLength = m_Input.GetLength();
		m_Input.ProcessInput(Event);
		m_InputUpdate = true;
	}
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


void CChat::EnableMode(int Team)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(m_Mode == MODE_NONE)
	{
		if(Team)
			m_Mode = MODE_TEAM;
		else
			m_Mode = MODE_ALL;

		m_Input.Clear();
		Input()->ClearEvents();
		m_CompletionChosen = -1;
	}
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		// dont show "finished in" message when race
		if(m_pClient->m_IsRace && pMsg->m_ClientID < 0 && (str_find(pMsg->m_pMessage, " finished in: ") || str_find(pMsg->m_pMessage, "New record: ")))
			return;

		const char *pMessage = pMsg->m_pMessage;
		
		// save last message for each player
		m_Spam = false;
		
		if(!str_comp(m_aaLastMsg[pMsg->m_ClientID+1], pMessage))
			m_Spam = true;
		
		str_copy(m_aaLastMsg[pMsg->m_ClientID+1], pMessage, sizeof(m_aaLastMsg[pMsg->m_ClientID+1]));
		
		// check if message should be marked
		char aBuf[256];
		str_copy(aBuf, g_Config.m_ClSearchName, sizeof(aBuf));

		if(aBuf[0])
		{
			CSplit Sp2 = Split(aBuf, ' '); 
			
			m_ContainsName = false;
			
			int i = 0;
			while (i < Sp2.m_Count)
			{
				if(str_find_nocase(pMessage, Sp2.m_aPointers[i]))
				{
					m_ContainsName = true;
					break;
				}
				else
					i++;
			}
		}
		
		AddLine(pMsg->m_ClientID, pMsg->m_Team, pMsg->m_pMessage);
	}
}

void CChat::AddLine(int ClientID, int Team, const char *pLine)
{
	if(*pLine == 0 || (ClientID != -1 && (m_pClient->m_aClients[ClientID].m_aName[0] == '\0' || // unknown client
		m_pClient->m_aClients[ClientID].m_ChatIgnore ||
		(m_pClient->m_Snap.m_LocalClientID != ClientID && g_Config.m_ClShowChatFriends && !m_pClient->m_aClients[ClientID].m_Friend))))
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
		m_aLines[m_CurrentLine].m_Team = Team;
		m_aLines[m_CurrentLine].m_NameColor = -2;

		// check for highlighted name
		const char *pHL = str_find_nocase(pLine, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName);
		if(pHL)
		{
			int Length = str_length(m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName);
			if((pLine == pHL || pHL[-1] == ' ') && (pHL[Length] == 0 || pHL[Length] == ' ' || (pHL[Length] == ':' && pHL[Length+1] == ' ')))
				Highlighted = true;
		}
		m_aLines[m_CurrentLine].m_Highlighted =  Highlighted;
		m_aLines[m_CurrentLine].m_Highlighted = m_ContainsName || (str_find_nocase(pLine, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) != 0);
			
		if(g_Config.m_ClAntiSpam && m_Spam)
			m_aLines[m_CurrentLine].m_Spam = 1;
		else
			m_aLines[m_CurrentLine].m_Spam = 0;
		if(m_aLines[m_CurrentLine].m_Highlighted)
			Highlighted = true;

		if(ClientID == -1) // server message
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "*** ", sizeof(m_aLines[m_CurrentLine].m_aName));
			
			// get server info
			CServerInfo CurrentServerInfo;
			Client()->GetServerInfo(&CurrentServerInfo);
				
			// match team names team colors
			if(str_find(pLine, "entered and joined the red team"))
			{
				pLine++;
				
				int Num = 0;
				const char* pTmp = pLine;
				while(str_comp_num(pTmp, " entered", 8))
				{
					pTmp++;
					Num++;
					if(!pTmp[0])
						break;
				}
			
				char aName[MAX_NAME_LENGTH];
				str_copy(aName, pLine, Num);
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' entered and joined the %s team", aName, (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1));
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' entered and joined the %s team", aName, CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1));
			}
			else if(str_find(pLine, "entered and joined the blue team"))
			{
				pLine++;
				
				int Num = 0;
				const char* pTmp = pLine;
				while(str_comp_num(pTmp, " entered", 8))
				{
					pTmp++;
					Num++;
					if(!pTmp[0])
						break;
				}
			
				char aName[MAX_NAME_LENGTH];
				str_copy(aName, pLine, Num);
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' entered and joined the %s team", aName, (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2));
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' entered and joined the %s team", aName, CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2));
			}
			else if(str_find(pLine, "joined the red team"))
			{
				pLine++;
				
				int Num = 0;
				const char* pTmp = pLine;
				while(str_comp_num(pTmp, " joined", 7))
				{
					pTmp++;
					Num++;
					if(!pTmp[0])
						break;
				}
			
				char aName[MAX_NAME_LENGTH];
				str_copy(aName, pLine, Num);
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' joined the %s team", aName, (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1));
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' joined the %s team", aName, CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1));
			}
			else if(str_find(pLine, "joined the blue team"))
			{
				pLine++;
				
				int Num = 0;
				const char* pTmp = pLine;
				while(str_comp_num(pTmp, " joined", 7))
				{
					pTmp++;
					Num++;
					if(!pTmp[0])
						break;
				}
			
				char aName[MAX_NAME_LENGTH];
				str_copy(aName, pLine, Num);
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' joined the %s team", aName, (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2));
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "'%s' joined the %s team", aName, CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2));
			}
			else if(g_Config.m_TcColoredFlags && str_find(pLine, "red flag was"))
			{
				while(str_comp_num(pLine, "flag was", 8))
					pLine++;
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "The %s %s", (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1), pLine);
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "The %s %s", CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1), pLine);
			}
			else if(g_Config.m_TcColoredFlags && str_find(pLine, "blue flag was"))
			{
				while(str_comp_num(pLine, "flag was", 8))
					pLine++;
				
				if(m_pClient->m_Snap.m_pLocalInfo)
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "The %s %s", (g_Config.m_TcColoredTeesMethod && m_pClient->m_Snap.m_pLocalInfo->m_Team == TEAM_BLUE)?CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam1):CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2), pLine);
				else
					str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "The %s %s", CTeecompUtils::TeamColorToName(g_Config.m_TcColoredTeesTeam2), pLine);
			}
			else
				str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}
		else
		{
			if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
			{
				m_aLines[m_CurrentLine].m_NameColor = m_pClient->m_aClients[ClientID].m_Team;
 			}
			else
			{
				if(m_pClient->m_aClients[ClientID].m_Team == TEAM_SPECTATORS)
					m_aLines[m_CurrentLine].m_NameColor = -1;
				else
					m_aLines[m_CurrentLine].m_NameColor = -2;
			}

			str_copy(m_aLines[m_CurrentLine].m_aName, m_pClient->m_aClients[ClientID].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}

		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, m_aLines[m_CurrentLine].m_Team?"teamchat":"chat", aBuf);
	}

	// play sound
	int64 Now = time_get();
	if(!g_Config.m_ClAntiSpam || !m_Spam)
	{
		if(ClientID == -1)
		{
			if(g_Config.m_ClServermsgsound && Now-m_aLastSoundPlayed[CHAT_SERVER] >= time_freq()*3/10)
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0);
				m_aLastSoundPlayed[CHAT_SERVER] = Now;
			}
		}
		else if(Highlighted)
		{
			if(g_Config.m_ClChatsound && Now-m_aLastSoundPlayed[CHAT_HIGHLIGHT] >= time_freq()*3/10)
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_HIGHLIGHT, 0);
				m_aLastSoundPlayed[CHAT_HIGHLIGHT] = Now;
			}
		}
		else
		{
			if(g_Config.m_ClChatsound && Now-m_aLastSoundPlayed[CHAT_CLIENT] >= time_freq()*3/10)
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_CLIENT, 0);
				m_aLastSoundPlayed[CHAT_CLIENT] = Now;
			}
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
				Say(pEntry->m_Team, pEntry->m_aText);
				break;
			}
		}
		--m_PendingChatCounter;
	}
	
	TextRender()->BatchBegin();

	float Width = 300.0f*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, Width, 300.0f);
	float x = 5.0f;
	float y = 300.0f-20.0f;
	if(m_Mode != MODE_NONE)
	{
		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Width-190.0f;
		Cursor.m_MaxLines = 2;

		if(m_Mode == MODE_ALL)
			TextRender()->TextEx(&Cursor, Localize("All"), -1);
		else if(m_Mode == MODE_TEAM)
			TextRender()->TextEx(&Cursor, Localize("Team"), -1);
		else
			TextRender()->TextEx(&Cursor, Localize("Chat"), -1);

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

	// get position of scoreboard
	vec4 ScoreboardPosition = m_pClient->m_pScoreboard->GetScoreboardPosition();

	int64 Now = time_get();
	float HeightLimit = (m_Show || (m_Mode != MODE_NONE && g_Config.m_ClChatHistoryOnInput)) ? 50.0f : 200.0f;
	if(m_pClient->m_pScoreboard->Active() && ScoreboardPosition.x < 270.0f)
			HeightLimit = (ScoreboardPosition.y+ScoreboardPosition.w+20.0f)/4;
	float Begin = x;
	float FontSize = 6.0f;
	CTextCursor Cursor;
	int OffsetType = m_pClient->m_pScoreboard->Active() ? 1 : 0;
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		if(Now > m_aLines[r].m_Time+16*time_freq() && !m_Show && (m_Mode == MODE_NONE || !g_Config.m_ClChatHistoryOnInput))
			break;

		float LineWidth = 200.0f;
		if(m_pClient->m_pScoreboard->Active() && ScoreboardPosition.x < 800.0f && y < (ScoreboardPosition.y+ScoreboardPosition.w+20.0f)/4)
			LineWidth = (ScoreboardPosition.x-40.0f)/4;

		// get the y offset (calculate it if we haven't done that yet)
		if(OffsetType == 1 || m_aLines[r].m_YOffset[OffsetType] < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, Begin, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = LineWidth;
			TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
			TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
			m_aLines[r].m_YOffset[OffsetType] = Cursor.m_Y + Cursor.m_FontSize;
		}
		
		if(!m_aLines[r].m_Spam)
		{
			if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID != -1)
				|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID == -1)
				|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
				y -= m_aLines[r].m_YOffset[OffsetType];
		}

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		float Blend = Now > m_aLines[r].m_Time+14*time_freq() && !m_Show && (m_Mode == MODE_NONE || !g_Config.m_ClChatHistoryOnInput) ? 1.0f-(Now-m_aLines[r].m_Time-14*time_freq())/(2.0f*time_freq()) : 1.0f;
		
		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		// render name		
		if(!g_Config.m_ClClearAll)
		{
			vec3 TColor;
			if(m_aLines[r].m_ClientID == -1)
				TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
			else if(m_aLines[r].m_Team)
				TextRender()->TextColor(0.45f, 0.9f, 0.45f, Blend); // team message
			else if(m_aLines[r].m_NameColor == TEAM_RED)
			{
				if(CTeecompUtils::GetForceDmColors(TEAM_RED, m_pClient->m_Snap.m_pLocalInfo ? m_pClient->m_Snap.m_pLocalInfo->m_Team : TEAM_RED))
					TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend);// red
				else
				{
					TColor = CTeecompUtils::GetTeamColor(TEAM_RED, m_pClient->m_Snap.m_pLocalInfo ? m_pClient->m_Snap.m_pLocalInfo->m_Team : TEAM_RED, g_Config.m_TcColoredTeesTeam1,
						g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
					TextRender()->TextColor(TColor.r, TColor.g, TColor.b, Blend);
				}
			}
			else if(m_aLines[r].m_NameColor == TEAM_BLUE)
			{
				if(CTeecompUtils::GetForceDmColors(TEAM_BLUE, m_pClient->m_Snap.m_pLocalInfo ? m_pClient->m_Snap.m_pLocalInfo->m_Team : TEAM_RED))
					TextRender()->TextColor(0.7f, 0.7f, 1.0f, Blend); // blue
				else
				{
					TColor = CTeecompUtils::GetTeamColor(TEAM_BLUE, m_pClient->m_Snap.m_pLocalInfo ? m_pClient->m_Snap.m_pLocalInfo->m_Team : TEAM_RED, g_Config.m_TcColoredTeesTeam1,
							g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
					TextRender()->TextColor(TColor.r, TColor.g, TColor.b, Blend);
				}	
			}
			else if(m_aLines[r].m_NameColor == TEAM_SPECTATORS)
				TextRender()->TextColor(0.75f, 0.5f, 0.75f, Blend); // spectator
			else
				TextRender()->TextColor(0.8f, 0.8f, 0.8f, Blend);

			if(!m_aLines[r].m_Spam)
			{
				if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID != -1)
					|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID == -1)
					|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
					TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
			}

			// render line
			if(m_aLines[r].m_ClientID == -1)
				TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
			else if(m_aLines[r].m_Highlighted)
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // highlighted
			else if(m_aLines[r].m_Team)
				TextRender()->TextColor(0.65f, 1.0f, 0.65f, Blend); // team message
			else
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);

			if(!m_aLines[r].m_Spam)
			{
				if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID != -1)
					|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientID == -1)
					|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
					TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
			}
		}
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	TextRender()->BatchEnd();
}

void CChat::Say(int Team, const char *pLine)
{
	m_LastChatSend = time_get();

	// send chat message
	CNetMsg_Cl_Say Msg;
	Msg.m_Team = Team;
	Msg.m_pMessage = pLine;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

CChat::CSplit CChat::Split(char *pIn, char Delim)
{
	CSplit Sp;
	Sp.m_Count = 1;
	Sp.m_aPointers[0] = pIn;

	while (*++pIn)
	{
		if (*pIn == Delim)
		{
			*pIn = 0;
			Sp.m_aPointers[Sp.m_Count++] = pIn+1;
		}
	}
	return Sp;
}
