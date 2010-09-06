
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

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

bool CChat::OnInput(IInput::CEvent e)
{
	if(m_Mode == MODE_NONE)
		return false;

	if(e.m_Flags&IInput::FLAG_PRESS && e.m_Key == KEY_ESCAPE)
		m_Mode = MODE_NONE;
	else if(e.m_Flags&IInput::FLAG_PRESS && (e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER))
	{
		if(m_Input.GetString()[0])
			Say(m_Mode == MODE_ALL ? 0 : 1, m_Input.GetString());
		m_Mode = MODE_NONE;
	}
	else
		m_Input.ProcessInput(e);
	
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
	}
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		const char *pMessage = pMsg->m_pMessage;
		
		// save last message for each player
		m_Spam = false;
		
		if(!str_comp(m_aaLastMsg[pMsg->m_Cid+1], pMessage))
			m_Spam = true;
		
		str_copy(m_aaLastMsg[pMsg->m_Cid+1], pMessage, sizeof(m_aaLastMsg[pMsg->m_Cid+1]));
			
		// check if player is ignored
		char aBuf[128];
		str_copy(aBuf, g_Config.m_ClSpammerName, sizeof(aBuf));
		
		CSplit Sp = Split(aBuf, ' '); 
		
		m_IgnorePlayer = false;
		
		if(g_Config.m_ClBlockSpammer)
		{
			int i = 0;
			while (i < Sp.m_Count)
			{
				if(str_find_nocase(m_pClient->m_aClients[pMsg->m_Cid].m_aName, Sp.m_aPointers[i]) != 0)
				{
					m_IgnorePlayer = true;
					break;
				}
				else
					i++;
			}
		}
		
		// check if message should be marked
		str_copy(aBuf, g_Config.m_ClSearchName, sizeof(aBuf));

		CSplit Sp2 = Split(aBuf, ' '); 
		
		m_ContainsName = false;
		
		if(g_Config.m_ClChangeColor || g_Config.m_ClChangeSound)
		{
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
		m_aLines[m_CurrentLine].m_ClientId = ClientId;
		m_aLines[m_CurrentLine].m_Team = Team;
		m_aLines[m_CurrentLine].m_NameColor = -2;
		
		m_aLines[m_CurrentLine].m_ContainsName = 0;
		
		if(g_Config.m_ClBlockSpammer && m_IgnorePlayer)
			m_aLines[m_CurrentLine].m_Ignore = 1;
		else
			m_aLines[m_CurrentLine].m_Ignore = 0;
			
		if(g_Config.m_ClAntiSpam && m_Spam)
			m_aLines[m_CurrentLine].m_Spam = 1;
		else
			m_aLines[m_CurrentLine].m_Spam = 0;

		if(ClientId == -1) // server message
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "*** ", sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}
		else
		{
			if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS)
			{
				m_aLines[m_CurrentLine].m_ContainsName = (int)m_ContainsName;
				m_aLines[m_CurrentLine].m_NameColor = m_pClient->m_aClients[ClientId].m_Team;
 			}
			else
			{
				m_aLines[m_CurrentLine].m_ContainsName = (int)m_ContainsName;
				if(m_pClient->m_aClients[ClientId].m_Team == -1)
					m_aLines[m_CurrentLine].m_NameColor = -1;
				else
					m_aLines[m_CurrentLine].m_NameColor = -2;
			}
			
			str_copy(m_aLines[m_CurrentLine].m_aName, m_pClient->m_aClients[ClientId].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}
		
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chat", aBuf);
	}

	// play sound
	if(!m_Spam && !m_IgnorePlayer)
	{
		if((ClientId >= 0) && g_Config.m_ClChangeSound && m_ContainsName)	
		{
			if(g_Config.m_ClChatsound)
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_MARK, 0, vec2(0,0));
		}
		else if(ClientId >= 0)
		{
			if(g_Config.m_ClChatsound)
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_CLIENT, 0, vec2(0,0));
		}
		else
		{
			if(g_Config.m_ClServermsgsound)
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0, vec2(0,0));
		}
	}
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
		
		if(m_Mode == MODE_ALL)
			TextRender()->TextEx(&Cursor, Localize("All"), -1);
		else if(m_Mode == MODE_TEAM)
			TextRender()->TextEx(&Cursor, Localize("Team"), -1);
		else
			TextRender()->TextEx(&Cursor, Localize("Chat"), -1);

		TextRender()->TextEx(&Cursor, ": ", -1);
			
		TextRender()->TextEx(&Cursor, m_Input.GetString(), m_Input.GetCursorOffset());
		CTextCursor Marker = Cursor;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, m_Input.GetString()+m_Input.GetCursorOffset(), -1);
	}

	y -= 8.0f;

	int64 Now = time_get();
	float LineWidth = m_pClient->m_pScoreboard->Active() ? 95.0f : 200.0f;
	float HeightLimit = m_pClient->m_pScoreboard->Active() ? 220.0f : m_Show ? 50.0f : 200.0f;
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		if(Now > m_aLines[r].m_Time+15*time_freq() && !m_Show)
			break;

		float Begin = x;
		float FontSize = 6.0f;
		
		// get the y offset
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, Begin, 0, FontSize, 0);
		Cursor.m_LineWidth = LineWidth;
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
		if(!m_aLines[r].m_Spam && !m_aLines[r].m_Ignore)
		{
			if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId != -1)
				|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId == -1)
				|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
				y -= Cursor.m_Y + Cursor.m_FontSize;
		}

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;
		
		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		// render name		
		if(!g_Config.m_ClClearAll)
		{
			vec3 TColor;
			TextRender()->TextColor(0.8f,0.8f,0.8f,1);
			if(m_aLines[r].m_ClientId == -1)
				TextRender()->TextColor(1,1,0.5f,1); // system
			else if(m_aLines[r].m_Team)
				TextRender()->TextColor(0.45f,0.9f,0.45f,1); // m_Team message
			else if(m_aLines[r].m_NameColor == 0)
			{
				if(!m_pClient->m_Snap.m_pLocalInfo)
					TextRender()->TextColor(1.0f,0.5f,0.5f,1); // red
				else
				{
					TColor = CTeecompUtils::GetTeamColor(0, m_pClient->m_Snap.m_pLocalInfo->m_Team, g_Config.m_TcColoredTeesTeam1,
						g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
					TextRender()->TextColor(TColor.r, TColor.g, TColor.b, 1);
				}
			}
			else if(m_aLines[r].m_NameColor == 1)
			{
				if(!m_pClient->m_Snap.m_pLocalInfo)
					TextRender()->TextColor(0.7f,0.7f,1.0f,1); // blue
				else
				{
					TColor = CTeecompUtils::GetTeamColor(1, m_pClient->m_Snap.m_pLocalInfo->m_Team, g_Config.m_TcColoredTeesTeam1,
							g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
					TextRender()->TextColor(TColor.r, TColor.g, TColor.b, 1);
				}	
			}
			else if(m_aLines[r].m_NameColor == -1)
				TextRender()->TextColor(0.75f,0.5f,0.75f, 1); // spectator

			// render name
			if(!m_aLines[r].m_Spam && !m_aLines[r].m_Ignore)
			{
				if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId != -1)
					|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId == -1)
					|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
					TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
			}

			// render line
			if(m_aLines[r].m_ContainsName && g_Config.m_ClChangeColor)
				TextRender()->TextColor(0.6f,0.6f,0.6f,1); // standard color if name
			else
				TextRender()->TextColor(1,1,1,1);
			
			if(m_aLines[r].m_ClientId == -1)
				TextRender()->TextColor(1,1,0.5f,1); // system
			else if(m_aLines[r].m_Team && m_aLines[r].m_ContainsName && g_Config.m_ClChangeColor)
				TextRender()->TextColor(0.3f,1,0.3f,1); // team color if name
			else if(m_aLines[r].m_Team)
				TextRender()->TextColor(0.65f,1,0.65f,1); // team message

			if(!m_aLines[r].m_Spam && !m_aLines[r].m_Ignore)
			{
				if((g_Config.m_ClRenderChat && !g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId != -1)
					|| (!g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg && m_aLines[r].m_ClientId == -1)
					|| (g_Config.m_ClRenderChat && g_Config.m_ClRenderServermsg))
					TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
			}
		}
	}

	TextRender()->TextColor(1,1,1,1);
}

void CChat::Say(int Team, const char *pLine)
{
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
