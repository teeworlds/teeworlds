// LordSk
#include "input_console.h"

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/textrender.h>
#include <generated/client_data.h>
#include <game/client/ui.h>

void CEditorInputConsole::StaticConsolePrintCallback(const char* pLine, void* pUser, bool Highlighted)
{
	((CEditorInputConsole*)pUser)->ConsolePrintCallback(pLine, Highlighted);
}

void CEditorInputConsole::ConsolePrintCallback(const char* pLine, bool Highlighted)
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

void CEditorInputConsole::Init(IConsole* pConsole, IGraphics* pGraphics, CUI* pUI, ITextRender* pTextRender, IInput* pInput)
{
	m_pConsole = pConsole;
	m_pGraphics = pGraphics;
	m_pTextRender = pTextRender;
	m_pUI = pUI;
	m_LineInput.Init(pInput);

#ifdef CONF_DEBUG
	m_pConsole->RegisterPrintCallback(IConsole::OUTPUT_LEVEL_DEBUG, StaticConsolePrintCallback, this);
#else
	m_pConsole->RegisterPrintCallback(IConsole::OUTPUT_LEVEL_STANDARD, StaticConsolePrintCallback, this);
#endif
}

void CEditorInputConsole::Render()
{
	if(!m_IsOpen)
		return;

	const CUIRect ScreenRect = *m_pUI->Screen();
	Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x + ScreenRect.w,
		ScreenRect.y + ScreenRect.h);

	const float ConsoleWidth = ScreenRect.w;
	const float ConsoleHeight = ScreenRect.h / 3;

	const float FontSize = 10.0f;
	const float Spacing = 2.0f;
	const float InputMargin = 5.0f;
	const float ConsoleMargin = 3.0f;

	CUIRect InputRect, ConsoleRect;
	ScreenRect.HSplitTop(ConsoleHeight, &ConsoleRect, 0);
	ConsoleRect.HSplitBottom(FontSize + InputMargin * 2, &ConsoleRect, &InputRect);

	// background
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BG].m_Id);
	Graphics()->QuadsBegin();

	Graphics()->SetColor(0.1, 0.1, 0.8, 0.95f);
	Graphics()->QuadsSetSubset(0,-ConsoleHeight*0.075f,ConsoleWidth*0.075f*0.5f,0);
	IGraphics::CQuadItem QuadBg(0, 0, ConsoleWidth, ConsoleHeight);
	Graphics()->QuadsDrawTL(&QuadBg, 1);

	Graphics()->QuadsEnd();

	// input line
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0, 0, 0.2, 0.5f);
	IGraphics::CQuadItem QuadInput(InputRect.x, InputRect.y, InputRect.w, InputRect.h);
	Graphics()->QuadsDrawTL(&QuadInput, 1);
	Graphics()->QuadsEnd();

	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, InputRect.x+InputMargin, InputRect.y-2+InputMargin, FontSize, TEXTFLAG_RENDER);
	TextRender()->TextShadowed(&Cursor, "> ", 2, vec2(0, 0), vec4(0, 0, 0, 0),
							   vec4(1, 1, 1, 1));

	Cursor.m_LineWidth = InputRect.w - InputMargin*2;
	// TODO: replace with a simple text function (no shadow)
	TextRender()->TextShadowed(&Cursor, m_LineInput.GetString(), -1, vec2(0, 0), vec4(0, 0, 0, 0),
							   vec4(1, 1, 1, 1));

	// cursor line |
	float w = TextRender()->TextWidth(0, FontSize, m_LineInput.GetString(), m_LineInput.GetCursorOffset()) +
		TextRender()->TextWidth(0, FontSize, "> ", 2);
	CUIRect CursorRect = InputRect;
	CursorRect.x += w + InputMargin;
	CursorRect.y += 2;
	CursorRect.w = ScreenRect.w / Graphics()->ScreenWidth(); // 1px
	CursorRect.h -= 4;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadCursor(CursorRect.x, CursorRect.y, CursorRect.w, CursorRect.h);
	Graphics()->QuadsDrawTL(&QuadCursor, 1);
	Graphics()->QuadsEnd();

	//	render log (actual page, wrap lines)
	CBacklogEntry *pEntry = m_Backlog.Last();
	float OffsetY = 0.0f;
	float LineOffset = 1.0f;
	float y = InputRect.y - ConsoleMargin - 2;
	while(pEntry)
	{
		// get y offset (calculate it if we haven't yet)
		if(pEntry->m_YOffset < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, 0.0f, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = ConsoleRect.w - ConsoleMargin*2;
			TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
			pEntry->m_YOffset = Cursor.m_Y+Cursor.m_FontSize+LineOffset;
		}
		OffsetY += pEntry->m_YOffset;

		//	next page when lines reach the top
		if(y-OffsetY <= ConsoleRect.x-5)
			break;

		//	just render output from actual backlog page (render bottom up)
		TextRender()->SetCursor(&Cursor, ConsoleMargin, y-OffsetY, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = ConsoleRect.w - ConsoleMargin*2;

		vec4 TextColor(0.8, 0.8, 1, 1);
		if(pEntry->m_Highlighted)
			TextColor = vec4(1,0.75,0.75,1);

		TextRender()->TextShadowed(&Cursor, pEntry->m_aText, -1, vec2(0, 0), vec4(0, 0, 0, 0),
								   TextColor);
		pEntry = m_Backlog.Prev(pEntry);
	}
}

void CEditorInputConsole::OnInput(IInput::CEvent Event)
{
	if(m_IsOpen && Event.m_Key == KEY_ESCAPE && (Event.m_Flags&IInput::FLAG_PRESS))
	{
		m_IsOpen = false;
		return;
	}

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		if(Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)
		{
			if(m_LineInput.GetString()[0])
			{
				char *pEntry = m_History.Allocate(m_LineInput.GetLength()+1);
				mem_copy(pEntry, m_LineInput.GetString(), m_LineInput.GetLength()+1);
				m_pHistoryEntry = 0x0;
				m_pConsole->ExecuteLine(m_LineInput.GetString());
				m_LineInput.Clear();
				return;
			}
		}
		else if (Event.m_Key == KEY_UP)
		{
			if (m_pHistoryEntry)
			{
				char *pTest = m_History.Prev(m_pHistoryEntry);

				if (pTest)
					m_pHistoryEntry = pTest;
			}
			else
				m_pHistoryEntry = m_History.Last();

			if (m_pHistoryEntry)
				m_LineInput.Set(m_pHistoryEntry);
		}
		else if (Event.m_Key == KEY_DOWN)
		{
			if (m_pHistoryEntry)
				m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

			if (m_pHistoryEntry)
				m_LineInput.Set(m_pHistoryEntry);
			else
				m_LineInput.Clear();
		}
		/*else if(Event.m_Key == KEY_TAB)
		{
			if(m_Type == CGameConsole::CONSOLETYPE_LOCAL || m_pGameConsole->Client()->RconAuthed())
			{
				m_CompletionChosen++;
				m_CompletionEnumerationCount = 0;
				m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, m_Type != CGameConsole::CONSOLETYPE_LOCAL &&
					m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),	PossibleCommandsCompleteCallback, this);

				// handle wrapping
				if(m_CompletionEnumerationCount && m_CompletionChosen >= m_CompletionEnumerationCount)
				{
					m_CompletionChosen %= m_CompletionEnumerationCount;
					m_CompletionEnumerationCount = 0;
					m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, m_Type != CGameConsole::CONSOLETYPE_LOCAL &&
						m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),	PossibleCommandsCompleteCallback, this);
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
		}*/
	}

	if(m_IsOpen)
		m_LineInput.ProcessInput(Event);

	if(Event.m_Flags&(IInput::FLAG_PRESS|IInput::FLAG_TEXT))
	{
		/*if(Event.m_Key != KEY_TAB)
		{
			m_CompletionChosen = -1;
			str_copy(m_aCompletionBuffer, m_Input.GetString(), sizeof(m_aCompletionBuffer));
		}

		// find the current command
		{
			char aBuf[64] = {0};
			const char *pSrc = GetString();
			int i = 0;
			for(; i < (int)sizeof(aBuf)-1 && *pSrc && *pSrc != ' '; i++, pSrc++)
				aBuf[i] = *pSrc;
			aBuf[i] = 0;

			const IConsole::CCommandInfo *pCommand = m_pGameConsole->m_pConsole->GetCommandInfo(aBuf, m_CompletionFlagmask,
				m_Type != CGameConsole::CONSOLETYPE_LOCAL && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands());
			if(pCommand)
			{
				m_IsCommand = true;
				str_copy(m_aCommandName, pCommand->m_pName, IConsole::TEMPCMD_NAME_LENGTH);
				str_copy(m_aCommandHelp, pCommand->m_pHelp, IConsole::TEMPCMD_HELP_LENGTH);
				str_copy(m_aCommandParams, pCommand->m_pParams, IConsole::TEMPCMD_PARAMS_LENGTH);
			}
			else
				m_IsCommand = false;
		}*/
	}
}

void CEditorInputConsole::ToggleOpen()
{
	m_IsOpen ^= 1;
}
