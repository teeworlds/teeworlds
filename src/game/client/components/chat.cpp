#include <string.h>
#include <stdio.h>

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>

#include <game/client/components/scoreboard.h>
#include <game/client/components/sounds.h>
#include <game/client/components/camera.h>
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
		m_ChatMoving = false;
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
void CChat::ConUpChat(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	((CChat*)pUserData)->m_ChatMoving = true;
	((CChat*)pUserData)->m_RenderLine = (((CChat*)pUserData)->m_RenderLine - 1)%MAX_LINES;
}

void CChat::ConDownChat(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	((CChat*)pUserData)->m_ChatMoving = true;
	((CChat*)pUserData)->m_RenderLine = (((CChat*)pUserData)->m_RenderLine + 1)%MAX_LINES;
}
void CChat::OnConsoleInit()
{
	Console()->Register("say", "r", CFGFLAG_CLIENT, ConSay, this, "Say in chat", 0);
	Console()->Register("say_team", "r", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat", 0);
	Console()->Register("chat", "s", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team mode", 0);
	Console()->Register("chat_up", "", CFGFLAG_CLIENT, ConUpChat, this, "Show early message", 0);
	Console()->Register("chat_down", "", CFGFLAG_CLIENT, ConDownChat, this, "Show last message", 0);
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat", 0);
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
		AddLine(pMsg->m_Cid, pMsg->m_Team, pMsg->m_pMessage);
	}
}

void CChat::AddLine(int ClientId, int Team, const char *pLine)
{
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

		if(ClientId == -1) // server message
		{		
			// Arrows		
			if(g_Config.m_ClArrows && (strstr(pLine, " entered and joined the ") 
					|| strstr(pLine, " has left the game") || strstr(pLine, " joined the ")))
			{
				str_copy(m_aLines[m_CurrentLine].m_aName, "    ", sizeof(m_aLines[m_CurrentLine].m_aName));
			}
			else
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
	if (!m_ChatMoving) {
		m_RenderLine = m_CurrentLine;
	}
	m_ChatMoving = false;
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
		int r = ((m_RenderLine-i)+MAX_LINES)%MAX_LINES;
		if(!m_Show && !m_ChatMoving) {
			if(Now > m_aLines[r].m_Time+15*time_freq())
				break;
		}
	

		float Begin = x;
		float FontSize = (float)g_Config.m_ClTextSize * 0.01f * 6.0f; // Text size factor
		
		// get the y offset
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, Begin, 0, FontSize, 0);
		Cursor.m_LineWidth = LineWidth;
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
		y -= Cursor.m_Y + Cursor.m_FontSize;

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;
		
		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		// render name
		CGameClient::CClientData currentData = m_pClient->m_aClients[m_aLines[r].m_ClientId];
			TextRender()->TextColor(
				(float)(m_PredefinedColors[m_aLines[r].m_ClientId].r)/255.0f,
				(float)(m_PredefinedColors[m_aLines[r].m_ClientId].g)/255.0f,
				(float)(m_PredefinedColors[m_aLines[r].m_ClientId].b)/255.0f,1);
		//TextRender()->TextColor(0.8f,0.8f,0.8f,1);//TODO: add colors at this strigng;
		if(m_aLines[r].m_ClientId == -1)
			TextRender()->TextColor(1,1,0.5f,1); // system
		else if(m_aLines[r].m_Team)
			TextRender()->TextColor(0.45f,0.9f,0.45f,1); // team message
		else if(m_aLines[r].m_NameColor == 0)
			TextRender()->TextColor(1.0f,0.5f,0.5f,1); // red
		else if(m_aLines[r].m_NameColor == 1)
			TextRender()->TextColor(0.7f,0.7f,1.0f,1); // blue
		else if(m_aLines[r].m_NameColor == -1)
			TextRender()->TextColor(0.75f,0.5f,0.75f, 1); // spectator
			
		// render name
		TextRender()->TextEx(&Cursor, m_aLines[r].m_aName, -1);

		// render line
		TextRender()->TextColor(1,1,1,1);
		if(m_aLines[r].m_ClientId == -1)
			TextRender()->TextColor(1,1,0.5f,1); // system
		else if(m_aLines[r].m_Team)
			TextRender()->TextColor(0.65f,1,0.65f,1); // team message
			
		const int txtsize = 12.0f*g_Config.m_ClTextSize/100;

		// Arrows
		if(g_Config.m_ClArrows && m_aLines[r].m_ClientId == -1)
		{		
			if(strstr(m_aLines[r].m_aText, " entered and joined the ")) // Asrrows for entering the game
			{			
				if(strstr(m_aLines[r].m_aText, "blue team"))
					TextRender()->TextColor(0.7f,0.7f,1.0f,1); // blue
					
				else if(strstr(m_aLines[r].m_aText, "red team"))
					TextRender()->TextColor(1.0f,0.5f,0.5f,1); // red
					
				else if(strstr(m_aLines[r].m_aText, "spectators"))
					TextRender()->TextColor(0.80f,0.43f,0.80f, 1); // purple
					
				else if(strstr(m_aLines[r].m_aText, "the game"))
					TextRender()->TextColor(0.8f,0.8f,0.4f,1);
				
				
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ARROWS].m_Id);
				Graphics()->QuadsBegin();
				
				RenderTools()->SelectSprite(SPRITE_ARROW_GREEN);
				
				IGraphics::CQuadItem QuadItem(x+1, y-2+txtsize/2, 10, 10);
				Graphics()->QuadsDraw(&QuadItem, 1);

				Graphics()->QuadsEnd();
				
				char str[256];
				strcpy(str, m_aLines[r].m_aText);
				strcpy(strstr(str , " entered and joined the "), " ");
				TextRender()->TextEx(&Cursor, str, -1);
			}
			else if(strstr(m_aLines[r].m_aText, " has left the game")) // Dunedune arrows for quitting the game
			{
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ARROWS].m_Id);
				TextRender()->TextColor(1.0f,0.57f,0.18f,1);
				
				Graphics()->QuadsBegin();
				RenderTools()->SelectSprite(SPRITE_ARROW_RED);
				
				IGraphics::CQuadItem QuadItem(x+1, y-2+txtsize/2, 10, 10);
				Graphics()->QuadsDraw(&QuadItem, 1);
				Graphics()->QuadsEnd();
				
				char str[256];
				strcpy(str, m_aLines[r].m_aText);
				strcpy(strstr(str , " has left the game"), " ");
				TextRender()->TextEx(&Cursor, str, -1);
			}
			else if(strstr(m_aLines[r].m_aText, " joined the ")) // Dunedune arrows for switching team
			{
				int team = -1;
				if(strstr(m_aLines[r].m_aText, "blue team"))
				{
					TextRender()->TextColor(0.7f,0.7f,1.0f,1);
					team = 1;
				}
				else if(strstr(m_aLines[r].m_aText, "red team"))
				{
					TextRender()->TextColor(1.0f,0.5f,0.5f, 1);
					team = 0;
				}
				else if(strstr(m_aLines[r].m_aText, "spectators"))
				{
					TextRender()->TextColor(0.80f,0.43f,0.80f, 1);
					team = -1;
				}
				else if(strstr(m_aLines[r].m_aText, "the game"))
				{
					TextRender()->TextColor(0.8f,0.8f,0.4f,1);
					team = 2;
				}
				
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ARROWS].m_Id);
				
				Graphics()->QuadsBegin();
				
				if(team == -1)
					RenderTools()->SelectSprite(SPRITE_ARROW_SPEC);
				else
				{
					if(team == 1)
						RenderTools()->SelectSprite(SPRITE_ARROW_RIGHT);
					else if (team == 2)
						RenderTools()->SelectSprite(SPRITE_ARROW_JOIN);
					else if(!team)
						RenderTools()->SelectSprite(SPRITE_ARROW_LEFT);
					else
						RenderTools()->SelectSprite(SPRITE_ARROW_SPEC);
				}
				
				IGraphics::CQuadItem QuadItem(x+1, (team == 0 || team == 1) ? y-1+txtsize : y-2+txtsize/2, 10, 10);
				Graphics()->QuadsDraw(&QuadItem, 1);
					
				Graphics()->QuadsEnd();
				
				char str[256];
				strcpy(str, m_aLines[r].m_aText);
				strcpy(strstr(str , " joined the "), " ");
				TextRender()->TextEx(&Cursor, str, -1);
			}
			else
				TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
		}
		else
			TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);
	}

	TextRender()->TextColor(1,1,1,1);
}

void CChat::Say(int Team, const char *pLine)
{
	if(!str_comp_num(pLine, "/follow", 7) ) {
		int Num;
		if(sscanf(pLine, "/follow %d", &Num) == 1) {
			m_pClient->m_pCamera->SetFollow(Num);
		}
	} else {
		// send chat message
		CNetMsg_Cl_Say Msg;
		Msg.m_Team = Team;
		Msg.m_pMessage = pLine;
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
	}
}

