/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <generated/client_data.h>

#include <base/system.h>

#include <engine/shared/ringbuffer.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <game/client/ui.h>

#include <game/version.h>

#include <game/client/lineinput.h>
#include <game/client/render.h>
#include <game/client/components/controls.h>
#include <game/client/components/menus.h>

#include "console.h"

static const char *s_apMapCommands[] = {"sv_map ", "change_map "};
static bool IsMapCommandPrefix(const char *pStr)
{
	for(unsigned i = 0; i < sizeof(s_apMapCommands) / sizeof(char *); i++)
		if(str_startswith_nocase(pStr, s_apMapCommands[i]))
			return true;
	return false;
}

static const char *s_apTuningCommands[] = {"tune ", "tune_reset "};
static bool IsTuningCommandPrefix(const char *pStr)
{
	for(unsigned i = 0; i < sizeof(s_apTuningCommands) / sizeof(char *); i++)
		if(str_startswith_nocase(pStr, s_apTuningCommands[i]))
			return true;
	return false;
}

enum
{
	CONSOLE_CLOSED,
	CONSOLE_OPENING,
	CONSOLE_OPEN,
	CONSOLE_CLOSING,
};

CGameConsole::CInstance::CInstance(int Type)
{
	m_pHistoryEntry = 0x0;

	m_Type = Type;

	if(Type == CGameConsole::CONSOLETYPE_LOCAL)
	{
		m_pName = "local_console";
		m_CompletionFlagmask = CFGFLAG_CLIENT;
	}
	else
	{
		m_pName = "remote_console";
		m_CompletionFlagmask = CFGFLAG_SERVER;
	}

	m_aCompletionBuffer[0] = 0;
	m_CompletionChosen = -1;
	m_aCompletionBufferArgument[0] = 0;
	m_CompletionChosenArgument = -1;
	Reset();

	m_IsCommand = false;
}

void CGameConsole::CInstance::Init(CGameConsole *pGameConsole)
{
	m_pGameConsole = pGameConsole;
};

void CGameConsole::CInstance::ClearBacklog()
{
	m_Backlog.Init();
	m_BacklogActPage = 0;
}

void CGameConsole::CInstance::ClearHistory()
{
	m_History.Init();
	m_pHistoryEntry = 0;
}

void CGameConsole::CInstance::ExecuteLine(const char *pLine)
{
	if(m_Type == CGameConsole::CONSOLETYPE_LOCAL)
		m_pGameConsole->m_pConsole->ExecuteLine(pLine);
	else
	{
		if(m_pGameConsole->Client()->RconAuthed())
			m_pGameConsole->Client()->SendRcon(pLine);
		else
			m_pGameConsole->Client()->SendRconAuth("", pLine);
	}
}

void CGameConsole::CInstance::PossibleCommandsCompleteCallback(int Index, const char *pStr, void *pUser)
{
	CGameConsole::CInstance *pInstance = (CGameConsole::CInstance *)pUser;
	if(pInstance->m_CompletionChosen == Index)
		pInstance->m_Input.Set(pStr);
}

void CGameConsole::CInstance::PossibleArgumentsCompleteCallback(int Index, const char *pStr, void *pUser)
{
	CGameConsole::CInstance *pInstance = (CGameConsole::CInstance *)pUser;
	if(pInstance->m_CompletionChosenArgument == Index)
	{
		// get command
		char aBuf[512] = { 0 };
		const char *pSrc = pInstance->GetString();
		unsigned i = 0;
		for(; i < sizeof(aBuf) - 2 && *pSrc && *pSrc != ' '; i++, pSrc++)
			aBuf[i] = *pSrc;
		aBuf[i++] = ' ';
		aBuf[i] = 0;

		// append argument
		str_append(aBuf, pStr, sizeof(aBuf));
		pInstance->m_Input.Set(aBuf);
	}
}

void CGameConsole::CInstance::OnInput(IInput::CEvent Event)
{
	bool Handled = false;

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		if(Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)
		{
			if(m_Input.GetString()[0])
			{
				if(m_Type == CONSOLETYPE_LOCAL || m_pGameConsole->Client()->RconAuthed())
				{
					char *pEntry = m_History.Allocate(m_Input.GetLength()+1);
					mem_copy(pEntry, m_Input.GetString(), m_Input.GetLength()+1);
					// print out the user's commands before they get run
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "> %s", m_Input.GetString());
					PrintLine(aBuf, false);
				}
				ExecuteLine(m_Input.GetString());
				m_Input.Clear();
				m_pHistoryEntry = 0x0;
			}

			Handled = true;
		}
		else if(Event.m_Key == KEY_UP)
		{
			if(m_pHistoryEntry)
			{
				char *pTest = m_History.Prev(m_pHistoryEntry);

				if(pTest)
					m_pHistoryEntry = pTest;
			}
			else
				m_pHistoryEntry = m_History.Last();

			if(m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry);
			Handled = true;
		}
		else if(Event.m_Key == KEY_DOWN)
		{
			if(m_pHistoryEntry)
				m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

			if(m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry);
			else
				m_Input.Clear();
			Handled = true;
		}
		else if(Event.m_Key == KEY_TAB)
		{
			const int Direction = m_pGameConsole->m_pClient->Input()->KeyIsPressed(KEY_LSHIFT) || m_pGameConsole->m_pClient->Input()->KeyIsPressed(KEY_RSHIFT) ? -1 : 1;

			// command completion
			if(m_Type == CGameConsole::CONSOLETYPE_LOCAL || m_pGameConsole->Client()->RconAuthed())
			{
				const bool UseTempCommands = m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands();
				const int CompletionEnumerationCount = m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, UseTempCommands);
				if(CompletionEnumerationCount)
				{
					if(m_CompletionChosen == -1 && Direction < 0)
						m_CompletionChosen = 0;
					m_CompletionChosen = (m_CompletionChosen + Direction + CompletionEnumerationCount) % CompletionEnumerationCount;
					m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, UseTempCommands, PossibleCommandsCompleteCallback, this);
				}
				else if(m_CompletionChosen != -1)
				{
					m_CompletionChosen = -1;
					Reset();
				}
			}

			// argument completion (map, tuning, ...)
			if(m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed())
			{
				const bool MapCompletion = IsMapCommandPrefix(GetString());
				const bool TuningCompletion = IsTuningCommandPrefix(GetString());
				if(MapCompletion || TuningCompletion)
				{
					int CompletionEnumerationCount = 0;
					if(MapCompletion)
						CompletionEnumerationCount = m_pGameConsole->m_pConsole->PossibleMaps(m_aCompletionBufferArgument);
					else if(TuningCompletion)
						CompletionEnumerationCount = m_pGameConsole->m_pClient->m_Tuning.PossibleTunings(m_aCompletionBufferArgument);

					if(CompletionEnumerationCount)
					{
						if(m_CompletionChosenArgument == -1 && Direction < 0)
							m_CompletionChosenArgument = 0;
						m_CompletionChosenArgument = (m_CompletionChosenArgument + Direction + CompletionEnumerationCount) % CompletionEnumerationCount;
						if(MapCompletion)
							m_pGameConsole->m_pConsole->PossibleMaps(m_aCompletionBufferArgument, PossibleArgumentsCompleteCallback, this);
						else if(TuningCompletion)
							m_pGameConsole->m_pClient->m_Tuning.PossibleTunings(m_aCompletionBufferArgument, PossibleArgumentsCompleteCallback, this);
					}
					else if(m_CompletionChosenArgument != -1)
					{
						m_CompletionChosenArgument = -1;
						Reset();
					}
				}
			}
		}
		else if(Event.m_Key == KEY_PAGEUP)
		{
			++m_BacklogActPage;
		}
		else if(Event.m_Key == KEY_PAGEDOWN)
		{
			--m_BacklogActPage;
			if(m_BacklogActPage < 0)
				m_BacklogActPage = 0;
		}
	}

	if(!Handled)
		m_Input.ProcessInput(Event);

	if(Event.m_Flags&(IInput::FLAG_PRESS|IInput::FLAG_TEXT))
	{
		if(Event.m_Key != KEY_TAB && Event.m_Key != KEY_LSHIFT && Event.m_Key != KEY_RSHIFT)
		{
			m_CompletionChosen = -1;
			str_copy(m_aCompletionBuffer, m_Input.GetString(), sizeof(m_aCompletionBuffer));

			for(unsigned i = 0; i < sizeof(s_apMapCommands) / sizeof(char *); i++)
			{
				if(str_startswith_nocase(m_Input.GetString(), s_apMapCommands[i]))
				{
					m_CompletionChosenArgument = -1;
					str_copy(m_aCompletionBufferArgument, &m_Input.GetString()[str_length(s_apMapCommands[i])], sizeof(m_aCompletionBufferArgument));
				}
			}

			for(unsigned i = 0; i < sizeof(s_apTuningCommands) / sizeof(char *); i++)
			{
				if(str_startswith_nocase(m_Input.GetString(), s_apTuningCommands[i]))
				{
					m_CompletionChosenArgument = -1;
					str_copy(m_aCompletionBufferArgument, &m_Input.GetString()[str_length(s_apTuningCommands[i])], sizeof(m_aCompletionBufferArgument));
				}
			}

			Reset();
		}

		// find the current command
		{
			char aBuf[sizeof(m_aCommandName)];
			const char *pSrc = GetString();
			int i = 0;
			for(; i < (int)sizeof(aBuf)-1 && *pSrc && *pSrc != ' '; i++, pSrc++)
				aBuf[i] = *pSrc;
			aBuf[i] = 0;

			const IConsole::CCommandInfo *pCommand = m_pGameConsole->m_pConsole->GetCommandInfo(aBuf, m_CompletionFlagmask,
				m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands());
			if(pCommand)
			{
				m_IsCommand = true;
				str_copy(m_aCommandName, pCommand->m_pName, sizeof(m_aCommandName));
				str_copy(m_aCommandHelp, pCommand->m_pHelp, sizeof(m_aCommandHelp));
				str_copy(m_aCommandParams, pCommand->m_pParams, sizeof(m_aCommandParams));
			}
			else
				m_IsCommand = false;
		}
	}
}

void CGameConsole::CInstance::PrintLine(const char *pLine, bool Highlighted)
{
	int Len = str_length(pLine);

	if(Len > 255)
		Len = 255;

	CBacklogEntry *pEntry = m_Backlog.Allocate(sizeof(CBacklogEntry)+Len);
	pEntry->m_YOffset = -1.0f;
	pEntry->m_Highlighted = Highlighted;
	mem_copy(pEntry->m_aText, pLine, Len);
	pEntry->m_aText[Len] = 0;
}

CGameConsole::CGameConsole()
: m_LocalConsole(CONSOLETYPE_LOCAL), m_RemoteConsole(CONSOLETYPE_REMOTE)
{
	m_ConsoleType = CONSOLETYPE_LOCAL;
	m_ConsoleState = CONSOLE_CLOSED;
	m_StateChangeEnd = 0.0f;
	m_StateChangeDuration = 0.1f;
}

float CGameConsole::TimeNow()
{
	static int64 s_TimeStart = time_get();
	return float(time_get()-s_TimeStart)/float(time_freq());
}

CGameConsole::CInstance *CGameConsole::CurrentConsole()
{
	if(m_ConsoleType == CONSOLETYPE_REMOTE)
		return &m_RemoteConsole;
	return &m_LocalConsole;
}

void CGameConsole::OnReset()
{
	m_RemoteConsole.Reset();
}

// only defined for 0<=t<=1
static float ConsoleScaleFunc(float t)
{
	//return t;
	return sinf(acosf(1.0f-t));
}

struct CCompletionOptionRenderInfo
{
	CGameConsole *m_pSelf;
	CTextCursor *m_pCursor;
	const char *m_pCurrentCmd;
	int m_WantedCompletion;
	int m_EnumCount;
	float m_Offset;
	float *m_pOffsetChange;
	float m_Width;
	float m_TotalWidth;
};

void CGameConsole::PossibleCommandsRenderCallback(int Index, const char *pStr, void *pUser)
{
	CCompletionOptionRenderInfo *pInfo = static_cast<CCompletionOptionRenderInfo *>(pUser);

	if(pInfo->m_EnumCount == pInfo->m_WantedCompletion)
	{
		pInfo->m_pSelf->TextRender()->TextColor(0.05f, 0.05f, 0.05f,1);
		const float BeginX = pInfo->m_pCursor->AdvancePosition().x + pInfo->m_Offset;
		pInfo->m_pSelf->TextRender()->TextDeferred(pInfo->m_pCursor, pStr, -1);
		CTextBoundingBox Box = pInfo->m_pCursor->BoundingBox();
		CUIRect Rect = {Box.x - 5 + BeginX, Box.y, Box.w + 8 - BeginX, Box.h};

		Rect.Draw(vec4(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,0.85f), pInfo->m_pCursor->m_FontSize/3);

		// scroll when out of sight
		const bool MoveLeft = Rect.x - *pInfo->m_pOffsetChange < 0.0f;
		const bool MoveRight = Rect.x + Rect.w - *pInfo->m_pOffsetChange > pInfo->m_Width;
		if(MoveLeft && !MoveRight)
			*pInfo->m_pOffsetChange -= -Rect.x + pInfo->m_Width/4.0f;
		else if(!MoveLeft && MoveRight)
			*pInfo->m_pOffsetChange += Rect.x + Rect.w - pInfo->m_Width + pInfo->m_Width/4.0f;
	}
	else
	{
		const char *pMatchStart = str_find_nocase(pStr, pInfo->m_pCurrentCmd);
		if(pMatchStart)
		{
			pInfo->m_pSelf->TextRender()->TextColor(0.5f,0.5f,0.5f,1);
			pInfo->m_pSelf->TextRender()->TextDeferred(pInfo->m_pCursor, pStr, pMatchStart-pStr);
			pInfo->m_pSelf->TextRender()->TextColor(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,1);
			pInfo->m_pSelf->TextRender()->TextDeferred(pInfo->m_pCursor, pMatchStart, str_length(pInfo->m_pCurrentCmd));
			pInfo->m_pSelf->TextRender()->TextColor(0.5f,0.5f,0.5f,1);
			pInfo->m_pSelf->TextRender()->TextDeferred(pInfo->m_pCursor, pMatchStart+str_length(pInfo->m_pCurrentCmd), -1);
		}
		else
		{
			pInfo->m_pSelf->TextRender()->TextColor(0.75f,0.75f,0.75f,1);
			pInfo->m_pSelf->TextRender()->TextDeferred(pInfo->m_pCursor, pStr, -1);
		}
	}

	pInfo->m_EnumCount++;
	pInfo->m_pSelf->TextRender()->TextAdvance(pInfo->m_pCursor, 7.0f);
	pInfo->m_TotalWidth = pInfo->m_pCursor->AdvancePosition().x + pInfo->m_Offset;
}

void CGameConsole::OnRender()
{
	CUIRect Screen = *UI()->Screen();
	float ConsoleMaxHeight = Screen.h*3/5.0f;
	float ConsoleHeight;

	const float Now = TimeNow();
	float Progress = (Now - (m_StateChangeEnd - m_StateChangeDuration)) / float(m_StateChangeDuration);

	if(Progress >= 1.0f)
	{
		if(m_ConsoleState == CONSOLE_CLOSING)
		{
			m_ConsoleState = CONSOLE_CLOSED;
			CurrentConsole()->m_Input.Deactivate();
		}
		else if(m_ConsoleState == CONSOLE_OPENING)
		{
			m_ConsoleState = CONSOLE_OPEN;
			CurrentConsole()->m_Input.Activate(CONSOLE);
		}

		Progress = 1.0f;
	}

	if(m_ConsoleState == CONSOLE_OPEN && Config()->m_ClEditor)
		Toggle(CONSOLETYPE_LOCAL);

	if(m_ConsoleState == CONSOLE_CLOSED)
		return;

	if(m_ConsoleState == CONSOLE_OPEN)
		Input()->MouseModeAbsolute();

	float ConsoleHeightScale;

	if(m_ConsoleState == CONSOLE_OPENING)
		ConsoleHeightScale = ConsoleScaleFunc(Progress);
	else if(m_ConsoleState == CONSOLE_CLOSING)
		ConsoleHeightScale = ConsoleScaleFunc(1.0f-Progress);
	else //if(console_state == CONSOLE_OPEN)
		ConsoleHeightScale = ConsoleScaleFunc(1.0f);

	ConsoleHeight = ConsoleHeightScale*ConsoleMaxHeight;

	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	// do console shadow
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	IGraphics::CColorVertex Array[4] = {
		IGraphics::CColorVertex(0, 0,0,0, 0.5f),
		IGraphics::CColorVertex(1, 0,0,0, 0.5f),
		IGraphics::CColorVertex(2, 0,0,0, 0.0f),
		IGraphics::CColorVertex(3, 0,0,0, 0.0f)};
	Graphics()->SetColorVertex(Array, 4);
	IGraphics::CQuadItem QuadItem(0, ConsoleHeight, Screen.w, 10.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// do background
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BG].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.2f, 0.2f, 0.2f,0.9f);
	if(m_ConsoleType == CONSOLETYPE_REMOTE)
		Graphics()->SetColor(0.4f, 0.2f, 0.2f,0.9f);
	Graphics()->QuadsSetSubset(0,-ConsoleHeight*0.075f,Screen.w*0.075f*0.5f,0);
	QuadItem = IGraphics::CQuadItem(0, 0, Screen.w, ConsoleHeight);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// do small bar shadow
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Array[0] = IGraphics::CColorVertex(0, 0,0,0, 0.0f);
	Array[1] = IGraphics::CColorVertex(1, 0,0,0, 0.0f);
	Array[2] = IGraphics::CColorVertex(2, 0,0,0, 0.25f);
	Array[3] = IGraphics::CColorVertex(3, 0,0,0, 0.25f);
	Graphics()->SetColorVertex(Array, 4);
	QuadItem = IGraphics::CQuadItem(0, ConsoleHeight-20, Screen.w, 10);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// do the lower bar
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BAR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.9f);
	Graphics()->QuadsSetSubset(0,0.1f,Screen.w*0.015f,1-0.1f);
	QuadItem = IGraphics::CQuadItem(0,ConsoleHeight-10.0f,Screen.w,10.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	ConsoleHeight -= 36.0f;

	CInstance *pConsole = CurrentConsole();

	{
		float FontSize = 10.0f;
		float RowHeight = FontSize*1.25f;
		float x = 3;
		float y = ConsoleHeight - RowHeight - 5.0f;

		static CTextCursor s_CompletionOptionsCursor;
		s_CompletionOptionsCursor.Reset();
		s_CompletionOptionsCursor.MoveTo(x - pConsole->m_CompletionRenderOffset, y + RowHeight + 2.0f);
		s_CompletionOptionsCursor.m_FontSize = FontSize;
		s_CompletionOptionsCursor.m_MaxWidth = -1.0f;

		static CTextCursor s_InfoCursor;
		s_InfoCursor.Reset();
		s_InfoCursor.MoveTo(x, y + 2.0f * RowHeight + 4.0f);
		s_InfoCursor.m_FontSize = FontSize;
		s_InfoCursor.m_MaxWidth = -1.0f;

		// render prompt
		static CTextCursor s_PromptCursor(FontSize);
		s_PromptCursor.MoveTo(x, y);
		const char *pPrompt = "> ";
		if(m_ConsoleType == CONSOLETYPE_REMOTE)
		{
			if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_LOADING)
			{
				if(Client()->RconAuthed())
					pPrompt = "rcon> ";
				else
					pPrompt = "ENTER PASSWORD> ";
			}
			else
				pPrompt = "NOT CONNECTED> ";
		}
		s_PromptCursor.Reset((int64)pPrompt);
		TextRender()->TextOutlined(&s_PromptCursor, pPrompt, -1);

		x = s_PromptCursor.AdvancePosition().x;

		// render console input (wrap line)
		pConsole->m_Input.SetHidden(m_ConsoleType == CONSOLETYPE_REMOTE && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_LOADING) && !Client()->RconAuthed());
		CTextCursor *pInputCursor = pConsole->m_Input.GetCursor();
		pInputCursor->m_Align = TEXTALIGN_BL;
		pInputCursor->m_MaxLines = -1;
		pInputCursor->m_FontSize = FontSize;
		pInputCursor->m_MaxWidth = Screen.w - 10.0f - x;
		pInputCursor->MoveTo(x, y + FontSize * 1.35f);

		pConsole->m_Input.Activate(CONSOLE); // ensure the input is active
		pConsole->m_Input.Render(pConsole->m_Input.WasChanged());

		y -= (pInputCursor->LineCount() - 1) * FontSize;

		// render possible commands
		if((m_ConsoleType == CONSOLETYPE_LOCAL || Client()->RconAuthed()) && pConsole->m_Input.GetString()[0])
		{
			CCompletionOptionRenderInfo Info;
			Info.m_pSelf = this;
			Info.m_WantedCompletion = pConsole->m_CompletionChosen;
			Info.m_EnumCount = 0;
			Info.m_Offset = pConsole->m_CompletionRenderOffset;
			Info.m_pOffsetChange = &pConsole->m_CompletionRenderOffsetChange;
			Info.m_Width = Screen.w;
			Info.m_TotalWidth = 0.0f;
			Info.m_pCurrentCmd = pConsole->m_aCompletionBuffer;
			Info.m_pCursor = &s_CompletionOptionsCursor;
			m_pConsole->PossibleCommands(Info.m_pCurrentCmd, pConsole->m_CompletionFlagmask, m_ConsoleType != CGameConsole::CONSOLETYPE_LOCAL &&
				Client()->RconAuthed() && Client()->UseTempRconCommands(), PossibleCommandsRenderCallback, &Info);

			if(pConsole->m_IsCommand)
			{
				// argument completion
				if(Info.m_EnumCount <= 0)
				{
					const bool MapCompletion = IsMapCommandPrefix(Info.m_pCurrentCmd);
					const bool TuningCompletion = IsTuningCommandPrefix(Info.m_pCurrentCmd);
					if(MapCompletion || TuningCompletion)
					{
						Info.m_WantedCompletion = pConsole->m_CompletionChosenArgument;
						Info.m_EnumCount = 0;
						Info.m_TotalWidth = 0.0f;
						Info.m_pCurrentCmd = pConsole->m_aCompletionBufferArgument;
						if(MapCompletion)
							m_pConsole->PossibleMaps(Info.m_pCurrentCmd, PossibleCommandsRenderCallback, &Info);
						else if(TuningCompletion)
							m_pClient->m_Tuning.PossibleTunings(Info.m_pCurrentCmd, PossibleCommandsRenderCallback, &Info);
					}
				}

				char aBuf[512];
				TextRender()->TextColor(0.9f, 0.9f, 0.9f, 1.0f);
				str_format(aBuf, sizeof(aBuf), "Help: %s    ", pConsole->m_aCommandHelp);
				TextRender()->TextDeferred(&s_InfoCursor, aBuf, -1);
				TextRender()->TextColor(0.7f, 0.7f, 0.7f, 1.0f);
				str_format(aBuf, sizeof(aBuf), "Syntax: %s %s", pConsole->m_aCommandName, pConsole->m_aCommandParams);
				TextRender()->TextDeferred(&s_InfoCursor, aBuf, -1);
			}

			UI()->DoSmoothScrollLogic(&pConsole->m_CompletionRenderOffset, &pConsole->m_CompletionRenderOffsetChange, Info.m_Width, Info.m_TotalWidth);
		}

		TextRender()->DrawTextOutlined(&s_CompletionOptionsCursor);
		TextRender()->DrawTextOutlined(&s_InfoCursor);

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		// render log (actual page, wrap lines)
		static CTextCursor s_Cursor;
		s_Cursor.Reset();
		s_Cursor.MoveTo(x, y);
		s_Cursor.m_FontSize = FontSize;
		s_Cursor.m_MaxLines = -1;
		s_Cursor.m_MaxWidth = Screen.w - 10.0f;

		CInstance::CBacklogEntry *pEntry = pConsole->m_Backlog.Last();
		float OffsetY = 0.0f;
		const float LineOffset = 1.0f;
		static int s_LastActivePage = pConsole->m_BacklogActPage;
		int TotalPages = 1;
		for(int Page = 0; Page <= s_LastActivePage; ++Page, OffsetY = 0.0f)
		{
			while(pEntry)
			{
				s_Cursor.Reset();
				if(pEntry->m_Highlighted)
					TextRender()->TextColor(1.0f, 0.75f, 0.75f, 1.0f);
				TextRender()->TextDeferred(&s_Cursor, pEntry->m_aText, -1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

				// get y offset (calculate it if we haven't yet)
				if(pEntry->m_YOffset < 0.0f)
					pEntry->m_YOffset = s_Cursor.BaseLineY() + LineOffset;
				OffsetY += pEntry->m_YOffset;

				// next page when lines reach the top
				if(y - OffsetY <= RowHeight)
					break;

				// just render output from actual backlog page (render bottom up)
				if(Page == s_LastActivePage)
				{
					s_Cursor.MoveTo(0.0f, y - OffsetY);
					TextRender()->DrawTextOutlined(&s_Cursor);
				}

				pEntry = pConsole->m_Backlog.Prev(pEntry);
			}
			if(!pEntry)
				break;
			TotalPages++;
		}
		pConsole->m_BacklogActPage = clamp(pConsole->m_BacklogActPage, 0, TotalPages - 1);
		s_LastActivePage = pConsole->m_BacklogActPage;

		s_Cursor.Reset();
		s_Cursor.m_MaxWidth = -1;
		// render page
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), Localize("-Page %d-"), pConsole->m_BacklogActPage+1);
		s_Cursor.MoveTo(10.0f, 0.0f);
		TextRender()->TextOutlined(&s_Cursor, aBuf, -1.0f);

		// render version
		str_format(aBuf, sizeof(aBuf), "v%s", GAME_RELEASE_VERSION);
		s_Cursor.Reset();
		s_Cursor.MoveTo(Screen.w-10.0f, 0.0f);
		s_Cursor.m_Align = TEXTALIGN_RIGHT;
		TextRender()->TextOutlined(&s_Cursor, aBuf, -1.0f);
		s_Cursor.m_Align = TEXTALIGN_LEFT;
	}
}

bool CGameConsole::OnInput(IInput::CEvent Event)
{
	if(m_ConsoleState == CONSOLE_CLOSED)
		return false;
	if((Event.m_Key >= KEY_F1 && Event.m_Key <= KEY_F12) || (Event.m_Key >= KEY_F13 && Event.m_Key <= KEY_F24))
		return false;

	if(Event.m_Key == KEY_ESCAPE && (Event.m_Flags&IInput::FLAG_PRESS))
		Toggle(m_ConsoleType);
	else
		CurrentConsole()->OnInput(Event);

	return true;
}

void CGameConsole::Toggle(int Type)
{
	if(m_ConsoleType != Type && (m_ConsoleState == CONSOLE_OPEN || m_ConsoleState == CONSOLE_OPENING))
	{
		// don't toggle console, just switch what console to use
		m_ConsoleType = Type;
		CurrentConsole()->m_Input.Activate(CONSOLE);
	}
	else
	{
		m_ConsoleType = Type;
		if(m_ConsoleState == CONSOLE_CLOSED || m_ConsoleState == CONSOLE_OPEN)
		{
			m_StateChangeEnd = TimeNow()+m_StateChangeDuration;
		}
		else
		{
			float Progress = m_StateChangeEnd-TimeNow();
			float ReversedProgress = m_StateChangeDuration-Progress;

			m_StateChangeEnd = TimeNow()+ReversedProgress;
		}

		if(m_ConsoleState == CONSOLE_CLOSED || m_ConsoleState == CONSOLE_CLOSING)
		{
			Input()->MouseModeAbsolute();
			UI()->SetEnabled(false);
			CurrentConsole()->m_Input.Deactivate();
			m_ConsoleState = CONSOLE_OPENING;
			// reset controls
			m_pClient->m_pControls->OnReset();
		}
		else
		{
			Input()->MouseModeRelative();
			UI()->SetEnabled(true);
			CurrentConsole()->m_Input.Activate(CONSOLE);
			m_pClient->OnRelease();
			m_ConsoleState = CONSOLE_CLOSING;
		}
	}
}

void CGameConsole::Dump(int Type)
{
	CInstance *pConsole = Type == CONSOLETYPE_REMOTE ? &m_RemoteConsole : &m_LocalConsole;
	char aBuf[256];
	char aFilename[128];
	str_timestamp(aBuf, sizeof(aBuf));
	str_format(aFilename, sizeof(aFilename), "dumps/%s_dump_%s.txt", pConsole->m_pName, aBuf);
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(File)
	{
		for(CInstance::CBacklogEntry *pEntry = pConsole->m_Backlog.First(); pEntry; pEntry = pConsole->m_Backlog.Next(pEntry))
		{
			io_write(File, pEntry->m_aText, str_length(pEntry->m_aText));
			io_write_newline(File);
		}
		io_close(File);
		str_format(aBuf, sizeof(aBuf), "%s contents were written to '%s'", pConsole->m_pName, aFilename);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Failed to open '%s'", aFilename);
	}
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
}

void CGameConsole::ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Toggle(CONSOLETYPE_LOCAL);
}

void CGameConsole::ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Toggle(CONSOLETYPE_REMOTE);
}

void CGameConsole::ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->m_LocalConsole.ClearBacklog();
}

void CGameConsole::ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->m_RemoteConsole.ClearBacklog();
}

void CGameConsole::ConDumpLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Dump(CONSOLETYPE_LOCAL);
}

void CGameConsole::ConDumpRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Dump(CONSOLETYPE_REMOTE);
}

#ifdef CONF_DEBUG
struct CDumpCommandsCallbackInfo
{
	CGameConsole *m_pConsole;
	int m_FlagMask;
	IOHANDLE m_File;
	bool m_Temp;
};

void CGameConsole::DumpCommandsCallback(int Index, const char *pStr, void *pUser)
{
	CDumpCommandsCallbackInfo *pInfo = (CDumpCommandsCallbackInfo *)pUser;
	const IConsole::CCommandInfo *pCommand = pInfo->m_pConsole->Console()->GetCommandInfo(pStr, pInfo->m_FlagMask, pInfo->m_Temp);
	io_write(pInfo->m_File, pCommand->m_pName, str_length(pCommand->m_pName));
	io_write(pInfo->m_File, ";", 1);
	io_write(pInfo->m_File, pCommand->m_pParams, str_length(pCommand->m_pParams));
	io_write(pInfo->m_File, ";", 1);
	io_write(pInfo->m_File, pCommand->m_pHelp, str_length(pCommand->m_pHelp));
	io_write_newline(pInfo->m_File);
}

void CGameConsole::ConDumpCommands(IConsole::IResult *pResult, void *pUserData)
{
	CGameConsole *pConsole = (CGameConsole *)pUserData;
	char aBuf[128];
	char aFilename[64];
	for(int i = 0; i < 2; i++) // client and server commands separately
	{
		const char *pSide = i == 0 ? "client" : "server";
		str_format(aFilename, sizeof(aFilename), "dumps/%s_commands.csv", pSide);
		IOHANDLE File = pConsole->Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(File)
		{
			CDumpCommandsCallbackInfo Info;
			Info.m_pConsole = pConsole;
			Info.m_FlagMask = i == 0 ? CFGFLAG_CLIENT : CFGFLAG_SERVER;
			Info.m_File = File;
			Info.m_Temp = false;
			int Count = pConsole->Console()->PossibleCommands("", Info.m_FlagMask, Info.m_Temp, DumpCommandsCallback, &Info);
			Info.m_Temp = true;
			Count += pConsole->Console()->PossibleCommands("", Info.m_FlagMask, Info.m_Temp, DumpCommandsCallback, &Info);
			io_close(File);
			str_format(aBuf, sizeof(aBuf), "%d %s commands were written to '%s'", Count, pSide, aFilename);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "Failed to open '%s'", aFilename);
		}
		pConsole->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
	}
}
#endif

void CGameConsole::ClientConsolePrintCallback(const char *pStr, void *pUserData, bool Highlighted)
{
	((CGameConsole *)pUserData)->m_LocalConsole.PrintLine(pStr, Highlighted);
}

void CGameConsole::ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 1)
	{
		CGameConsole *pThis = static_cast<CGameConsole *>(pUserData);
		pThis->Console()->SetPrintOutputLevel(pThis->m_PrintCBIndex, pResult->GetInteger(0));
	}
}

bool CGameConsole::IsConsoleActive()
{
	return m_ConsoleState != CONSOLE_CLOSED;
}

void CGameConsole::PrintLine(int Type, const char *pLine)
{
	if(Type == CONSOLETYPE_LOCAL)
		m_LocalConsole.PrintLine(pLine, false);
	else if(Type == CONSOLETYPE_REMOTE)
		m_RemoteConsole.PrintLine(pLine, false);
}

void CGameConsole::OnConsoleInit()
{
	// init console instances
	m_LocalConsole.Init(this);
	m_RemoteConsole.Init(this);

	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_PrintCBIndex = Console()->RegisterPrintCallback(Config()->m_ConsoleOutputLevel, ClientConsolePrintCallback, this);

	Console()->Register("toggle_local_console", "", CFGFLAG_CLIENT, ConToggleLocalConsole, this, "Toggle local console");
	Console()->Register("toggle_remote_console", "", CFGFLAG_CLIENT, ConToggleRemoteConsole, this, "Toggle remote console");
	Console()->Register("clear_local_console", "", CFGFLAG_CLIENT, ConClearLocalConsole, this, "Clear local console");
	Console()->Register("clear_remote_console", "", CFGFLAG_CLIENT, ConClearRemoteConsole, this, "Clear remote console");
	Console()->Register("dump_local_console", "", CFGFLAG_CLIENT, ConDumpLocalConsole, this, "Write local console contents to a text file");
	Console()->Register("dump_remote_console", "", CFGFLAG_CLIENT, ConDumpRemoteConsole, this, "Write remote console contents to a text file");

#ifdef CONF_DEBUG
	Console()->Register("dump_commands", "", CFGFLAG_CLIENT, ConDumpCommands, this, "Write list of all commands to a text file");
#endif

	Console()->Chain("console_output_level", ConchainConsoleOutputLevelUpdate, this);
}

void CGameConsole::OnStateChange(int NewState, int OldState)
{
	if(NewState == IClient::STATE_OFFLINE)
		m_RemoteConsole.ClearHistory();
}
