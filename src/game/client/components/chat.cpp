/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>

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
	
	m_Show = false;
	m_InputUpdate = false;
	m_ChatStringOffset = 0;
	m_CompletionChosen = -1;
	m_aCompletionBuffer[0] = 0;
	m_PlaceholderOffset = 0;
	m_PlaceholderLength = 0;
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

void CChat::ConSay(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	((CChat*)pUserData)->Say(0, pResult->GetString(0));
}

void CChat::ConSayTeam(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	((CChat*)pUserData)->Say(1, pResult->GetString(0));
}

void CChat::ConChat(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	const char *pMode = pResult->GetString(0);
	if(str_comp(pMode, "all") == 0)
		((CChat*)pUserData)->EnableMode(0);
	else if(str_comp(pMode, "team") == 0)
		((CChat*)pUserData)->EnableMode(1);
	else
		((CChat*)pUserData)->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "expected all or team as mode");
}

void CChat::ConShowChat(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	((CChat *)pUserData)->m_Show = pResult->GetInteger(0) != 0;
}

void CChat::OnConsoleInit()
{
	Console()->Register("say", "r", CFGFLAG_CLIENT, ConSay, this, "Say in chat", 0);
	Console()->Register("say_team", "r", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat", 0);
	Console()->Register("chat", "s", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team mode", 0);
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat", 0);
}

bool CChat::OnInput(IInput::CEvent e)
{
	if(m_Mode == MODE_NONE)
		return false;

	if(e.m_Flags&IInput::FLAG_PRESS && e.m_Key == KEY_ESCAPE)
	{
		m_Mode = MODE_NONE;
		m_pClient->OnRelease();
	}
	else if(e.m_Flags&IInput::FLAG_PRESS && (e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER))
	{
		if(m_Input.GetString()[0])
			Say(m_Mode == MODE_ALL ? 0 : 1, m_Input.GetString());
		m_Mode = MODE_NONE;
		m_pClient->OnRelease();
	}
	if(e.m_Flags&IInput::FLAG_PRESS && e.m_Key == KEY_TAB)
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
		m_CompletionChosen = (m_CompletionChosen+1)%MAX_CLIENTS;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			int Index = (m_CompletionChosen+i)%MAX_CLIENTS;
			if(!m_pClient->m_Snap.m_paPlayerInfos[Index])
				continue;

			if(str_find_nocase(m_pClient->m_aClients[Index].m_aName, m_aCompletionBuffer))
			{
				pCompletionString = m_pClient->m_aClients[Index].m_aName;
				m_CompletionChosen = Index;
				break;
			}
		}

		// insert the name
		if(pCompletionString)
		{
			char aBuf[256];
			str_copy(aBuf, m_Input.GetString(), min(static_cast<int>(sizeof(aBuf)), m_PlaceholderOffset+1));
			str_append(aBuf, pCompletionString, sizeof(aBuf));
			str_append(aBuf, m_Input.GetString()+m_PlaceholderOffset+m_PlaceholderLength, sizeof(aBuf));
			m_PlaceholderLength = str_length(pCompletionString);
			m_OldChatStringLength = m_Input.GetLength();
			m_Input.Set(aBuf);
			m_Input.SetCursorOffset(m_PlaceholderOffset+m_PlaceholderLength);
			m_InputUpdate = true;
		}
	}
	else
	{
		// reset name completion process
		if(e.m_Flags&IInput::FLAG_PRESS && e.m_Key != KEY_TAB)
			m_CompletionChosen = -1;

		m_OldChatStringLength = m_Input.GetLength();
		m_Input.ProcessInput(e);
		m_InputUpdate = true;
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
		AddLine(pMsg->m_Cid, pMsg->m_Team, pMsg->m_pMessage);
	}
}

void CChat::AddLine(int ClientId, int Team, const char *pLine)
{
	if(ClientId != -1 && m_pClient->m_aClients[ClientId].m_aName[0] == '\0') // unknown client
		return;
	
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
		m_aLines[m_CurrentLine].m_YOffset[0] = -1.0f;
		m_aLines[m_CurrentLine].m_YOffset[1] = -1.0f;
		m_aLines[m_CurrentLine].m_ClientId = ClientId;
		m_aLines[m_CurrentLine].m_Team = Team;
		m_aLines[m_CurrentLine].m_NameColor = -2;
		m_aLines[m_CurrentLine].m_Highlighted = str_find_nocase(pLine, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalCid].m_aName) != 0;

		if(ClientId == -1) // server message
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "*** ", sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}
		else
		{
			if(m_pClient->m_aClients[ClientId].m_Team == -1)
				m_aLines[m_CurrentLine].m_NameColor = -1;

			if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS)
			{
				if(m_pClient->m_aClients[ClientId].m_Team == 0)
					m_aLines[m_CurrentLine].m_NameColor = 0;
				else if(m_pClient->m_aClients[ClientId].m_Team == 1)
					m_aLines[m_CurrentLine].m_NameColor = 1;
			}
			
			str_copy(m_aLines[m_CurrentLine].m_aName, m_pClient->m_aClients[ClientId].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}
		
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chat", aBuf);
	}

	// play sound
	if(ClientId >= 0)
		m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_CLIENT, 0, vec2(0,0));
	else
		m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0, vec2(0,0));
}

void CChat::OnRender()
{
	Graphics()->MapScreen(0,0,300*Graphics()->ScreenAspect(),300);
	float x = 10.0f;
	float y = 300.0f-20.0f;
	if(m_Mode != MODE_NONE)
	{
		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = 200.0f;
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
		CTextCursor Marker = Cursor;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, m_Input.GetString()+m_Input.GetCursorOffset(), -1);
	}

	y -= 8.0f;

	int64 Now = time_get();
	float LineWidth = m_pClient->m_pScoreboard->Active() ? 95.0f : 200.0f;
	float HeightLimit = m_pClient->m_pScoreboard->Active() ? 220.0f : m_Show ? 50.0f : 200.0f;
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
		if(m_aLines[r].m_ClientId == -1)
			TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
		else if(m_aLines[r].m_Team)
			TextRender()->TextColor(0.45f, 0.9f, 0.45f, Blend); // team message
		else if(m_aLines[r].m_NameColor == 0)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // red
		else if(m_aLines[r].m_NameColor == 1)
			TextRender()->TextColor(0.7f, 0.7f, 1.0f, Blend); // blue
		else if(m_aLines[r].m_NameColor == -1)
			TextRender()->TextColor(0.75f, 0.5f, 0.75f, Blend); // spectator
		else
			TextRender()->TextColor(0.8f, 0.8f, 0.8f, Blend);
			
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);

		// render line
		if(m_aLines[r].m_ClientId == -1)
			TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
		else if(m_aLines[r].m_Highlighted)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // highlighted
		else if(m_aLines[r].m_Team)
			TextRender()->TextColor(0.65f, 1.0f, 0.65f, Blend); // team message
		else
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Blend);

		TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CChat::Say(int Team, const char *pLine)
{
	// send chat message
	CNetMsg_Cl_Say Msg;
	Msg.m_Team = Team;
	Msg.m_pMessage = pLine;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
