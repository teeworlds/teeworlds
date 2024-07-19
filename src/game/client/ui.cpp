/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/config.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/textrender.h>

#include "ui.h"

/********************************************************
 UI
*********************************************************/

const float CUI::ms_ListheaderHeight = 17.0f;
const float CUI::ms_FontmodHeight = 0.8f;

const vec4 CUI::ms_DefaultTextColor(1.0f, 1.0f, 1.0f, 1.0f);
const vec4 CUI::ms_DefaultTextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
const vec4 CUI::ms_HighlightTextColor(0.0f, 0.0f, 0.0f, 1.0f);
const vec4 CUI::ms_HighlightTextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
const vec4 CUI::ms_TransparentTextColor(1.0f, 1.0f, 1.0f, 0.5f);

const CLinearScrollbarScale CUI::ms_LinearScrollbarScale;
const CLogarithmicScrollbarScale CUI::ms_LogarithmicScrollbarScale(25);

CUI *CUIElementBase::s_pUI = 0;

IClient *CUIElementBase::Client() const { return s_pUI->Client(); }
CConfig *CUIElementBase::Config() const { return s_pUI->Config(); }
IGraphics *CUIElementBase::Graphics() const { return s_pUI->Graphics(); }
IInput *CUIElementBase::Input() const { return s_pUI->Input(); }
ITextRender *CUIElementBase::TextRender() const { return s_pUI->TextRender(); }

float CButtonContainer::GetFade(bool Checked, float Seconds)
{
	if(UI()->HotItem() == this || Checked)
	{
		m_FadeStartTime = Client()->LocalTime();
		return 1.0f;
	}

	return maximum(0.0f, m_FadeStartTime -  Client()->LocalTime() + Seconds)/Seconds;
}


CUI::CUI()
{
	m_pHotItem = 0;
	m_pActiveItem = 0;
	m_pLastActiveItem = 0;
	m_pBecommingHotItem = 0;

	m_MouseX = 0;
	m_MouseY = 0;
	m_MouseWorldX = 0;
	m_MouseWorldY = 0;
	m_MouseButtons = 0;
	m_LastMouseButtons = 0;
	m_Enabled = true;

	m_HotkeysPressed = 0;

	m_Screen.x = 0;
	m_Screen.y = 0;

	m_NumClips = 0;

	m_pActiveTooltip = 0;
	m_aTooltipText[0] = '\0';

	m_NumPopupMenus = 0;
}

void CUI::Init(class IKernel *pKernel)
{
	m_pClient = pKernel->RequestInterface<IClient>();
	m_pConfig = pKernel->RequestInterface<IConfigManager>()->Values();
	m_pGraphics = pKernel->RequestInterface<IGraphics>();
	m_pInput = pKernel->RequestInterface<IInput>();
	m_pTextRender = pKernel->RequestInterface<ITextRender>();
	CUIRect::Init(m_pGraphics);
	CLineInput::Init(m_pInput, m_pTextRender, m_pGraphics, m_pClient);
	CUIElementBase::Init(this);
}

void CUI::Update(float MouseX, float MouseY, float MouseWorldX, float MouseWorldY)
{
	unsigned MouseButtons = 0;
	if(Enabled())
	{
		if(Input()->KeyIsPressed(KEY_MOUSE_1)) MouseButtons |= 1;
		if(Input()->KeyIsPressed(KEY_MOUSE_2)) MouseButtons |= 2;
		if(Input()->KeyIsPressed(KEY_MOUSE_3)) MouseButtons |= 4;
	}

	m_MouseX = MouseX;
	m_MouseY = MouseY;
	m_MouseWorldX = MouseWorldX;
	m_MouseWorldY = MouseWorldY;
	m_LastMouseButtons = m_MouseButtons;
	m_MouseButtons = MouseButtons;
	m_pHotItem = m_pBecommingHotItem;
	if(m_pActiveItem)
		m_pHotItem = m_pActiveItem;
	m_pBecommingHotItem = 0;

	if(Enabled())
	{
		CLineInput *pActiveInput = CLineInput::GetActiveInput();
		if(pActiveInput && m_pLastActiveItem && pActiveInput != m_pLastActiveItem)
			pActiveInput->Deactivate();
	}
}

bool CUI::KeyPress(int Key) const
{
	return Enabled() && Input()->KeyPress(Key);
}

bool CUI::KeyIsPressed(int Key) const
{
	return Enabled() && Input()->KeyIsPressed(Key);
}

bool CUI::ConsumeHotkey(unsigned Hotkey)
{
	bool Pressed = m_HotkeysPressed&Hotkey;
	m_HotkeysPressed &= ~Hotkey;
	return Pressed;
}

bool CUI::OnInput(const IInput::CEvent &e)
{
	if(!Enabled())
		return false;

	CLineInput *pActiveInput = CLineInput::GetActiveInput();
	if(pActiveInput && pActiveInput->ProcessInput(e))
		return true;

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		unsigned LastHotkeysPressed = m_HotkeysPressed;
		if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
			m_HotkeysPressed |= HOTKEY_ENTER;
		else if(e.m_Key == KEY_ESCAPE)
			m_HotkeysPressed |= HOTKEY_ESCAPE;
		else if(e.m_Key == KEY_TAB && !Input()->KeyIsPressed(KEY_LALT) && !Input()->KeyIsPressed(KEY_RALT))
			m_HotkeysPressed |= HOTKEY_TAB;
		else if(e.m_Key == KEY_DELETE)
			m_HotkeysPressed |= HOTKEY_DELETE;
		else if(e.m_Key == KEY_UP)
			m_HotkeysPressed |= HOTKEY_UP;
		else if(e.m_Key == KEY_DOWN)
			m_HotkeysPressed |= HOTKEY_DOWN;
		return LastHotkeysPressed != m_HotkeysPressed;
	}
	return false;
}

void CUI::ConvertCursorMove(float *pX, float *pY, int CursorType) const
{
	float Factor = 1.0f;
	switch(CursorType)
	{
		case IInput::CURSOR_MOUSE:
			Factor = Config()->m_UiMousesens/100.0f;
			break;
		case IInput::CURSOR_JOYSTICK:
			Factor = Config()->m_UiJoystickSens/100.0f;
			break;
	}
	*pX *= Factor;
	*pY *= Factor;
}

const CUIRect *CUI::Screen()
{
	m_Screen.h = 600;
	m_Screen.w = Graphics()->ScreenAspect() * m_Screen.h;
	return &m_Screen;
}

float CUI::PixelSize()
{
	return Screen()->w/Graphics()->ScreenWidth();
}

void CUI::MapScreen()
{
	const CUIRect *pScreen = Screen();
	Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);
}

void CUI::ClipEnable(const CUIRect *pRect)
{
	if(IsClipped())
	{
		dbg_assert(m_NumClips < MAX_CLIP_NESTING_DEPTH, "max clip nesting depth exceeded");
		const CUIRect *pOldRect = ClipArea();
		CUIRect Intersection;
		Intersection.x = maximum(pRect->x, pOldRect->x);
		Intersection.y = maximum(pRect->y, pOldRect->y);
		Intersection.w = minimum(pRect->x+pRect->w, pOldRect->x+pOldRect->w) - pRect->x;
		Intersection.h = minimum(pRect->y+pRect->h, pOldRect->y+pOldRect->h) - pRect->y;
		m_aClips[m_NumClips] = Intersection;
	}
	else
	{
		m_aClips[m_NumClips] = *pRect;
	}
	m_NumClips++;
	UpdateClipping();
}

void CUI::ClipDisable()
{
	dbg_assert(m_NumClips > 0, "no clip region");
	m_NumClips--;
	UpdateClipping();
}

const CUIRect *CUI::ClipArea() const
{
	dbg_assert(m_NumClips > 0, "no clip region");
	return &m_aClips[m_NumClips - 1];
}

void CUI::UpdateClipping()
{
	if(IsClipped())
	{
		const CUIRect *pRect = ClipArea();
		const float XScale = Graphics()->ScreenWidth()/Screen()->w;
		const float YScale = Graphics()->ScreenHeight()/Screen()->h;
		Graphics()->ClipEnable((int)(pRect->x*XScale), (int)(pRect->y*YScale), (int)(pRect->w*XScale), (int)(pRect->h*YScale));
	}
	else
	{
		Graphics()->ClipDisable();
	}
}

bool CUI::DoButtonLogic(const void *pID, const CUIRect *pRect, int Button)
{
	// logic
	bool Clicked = false;
	static int s_LastButton = -1;
	const bool Hovered = MouseHovered(pRect);

	if(CheckActiveItem(pID))
	{
		if(s_LastButton == Button && !MouseButton(s_LastButton))
		{
			if(Hovered)
				Clicked = true;
			SetActiveItem(0);
			s_LastButton = -1;
		}
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(Button))
		{
			SetActiveItem(pID);
			s_LastButton = Button;
		}
	}

	if(Hovered && !MouseButton(Button))
		SetHotItem(pID);

	return Clicked;
}

bool CUI::DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY)
{
	if(CheckActiveItem(pID))
	{
		if(!MouseButton(0))
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
			SetActiveItem(pID);
	}

	if(MouseHovered(pRect))
		SetHotItem(pID);

	if(!CheckActiveItem(pID))
		return false;

	if(pX)
		*pX = clamp(m_MouseX - pRect->x, 0.0f, pRect->w);
	if(pY)
		*pY = clamp(m_MouseY - pRect->y, 0.0f, pRect->h);

	return true;
}

void CUI::DoSmoothScrollLogic(float *pScrollOffset, float *pScrollOffsetChange, float ViewPortSize, float TotalSize, float ScrollSpeed)
{
	// instant scrolling if distance too long
	if(absolute(*pScrollOffsetChange) > ViewPortSize)
	{
		*pScrollOffset += *pScrollOffsetChange;
		*pScrollOffsetChange = 0.0f;
	}
	// smooth scrolling
	if(*pScrollOffsetChange)
	{
		const float Delta = *pScrollOffsetChange * clamp(Client()->RenderFrameTime() * ScrollSpeed, 0.0f, 1.0f);
		*pScrollOffset += Delta;
		*pScrollOffsetChange -= Delta;
	}
	// clamp to first item
	if(*pScrollOffset < 0.0f)
	{
		*pScrollOffset = 0.0f;
		*pScrollOffsetChange = 0.0f;
	}
	// clamp to last item
	if(TotalSize > ViewPortSize && *pScrollOffset > TotalSize - ViewPortSize)
	{
		*pScrollOffset = TotalSize - ViewPortSize;
		*pScrollOffsetChange = 0.0f;
	}
}

void CUI::ApplyCursorAlign(class CTextCursor *pCursor, const CUIRect *pRect, int Align)
{
	pCursor->m_Align = Align;

	float x = pRect->x;
	if((Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_CENTER)
		x += pRect->w / 2.0f;
	else if((Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_RIGHT)
		x += pRect->w;

	float y = pRect->y;
	if((Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_MIDDLE)
		y += pRect->h / 2.0f;
	else if((Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_BOTTOM)
		y += pRect->h;

	pCursor->MoveTo(x, y);
}

void CUI::DoLabel(const CUIRect *pRect, const char *pText, float FontSize, int Align, float LineWidth, bool MultiLine)
{
	// TODO: FIX ME!!!!
	// Graphics()->BlendNormal();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxLines = MultiLine ? -1 : 1;
	s_Cursor.m_MaxWidth = LineWidth;
	s_Cursor.m_Flags = MultiLine ? TEXTFLAG_WORD_WRAP : 0;
	ApplyCursorAlign(&s_Cursor, pRect, Align);

	TextRender()->TextOutlined(&s_Cursor, pText, -1);
}

void CUI::DoLabelHighlighted(const CUIRect *pRect, const char *pText, const char *pHighlighted, float FontSize, const vec4 &TextColor, const vec4 &HighlightColor, int Align)
{
	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxWidth = pRect->w;
	ApplyCursorAlign(&s_Cursor, pRect, Align);

	TextRender()->TextColor(TextColor);
	const char *pMatch = pHighlighted && pHighlighted[0] ? str_find_nocase(pText, pHighlighted) : 0;
	if(pMatch)
	{
		const int HighlightLength = str_length(pHighlighted);
		TextRender()->TextDeferred(&s_Cursor, pText, (int)(pMatch - pText));
		TextRender()->TextColor(HighlightColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch, HighlightLength);
		TextRender()->TextColor(TextColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch + HighlightLength, -1);
	}
	else
		TextRender()->TextDeferred(&s_Cursor, pText, -1);

	TextRender()->DrawTextOutlined(&s_Cursor);
}

void CUI::DoLabelSelected(const CUIRect *pRect, const char *pText, bool Selected, float FontSize, int Align)
{
	if(Selected)
	{
		TextRender()->TextColor(CUI::ms_HighlightTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_HighlightTextOutlineColor);
	}
	DoLabel(pRect, pText, FontSize, Align);
	if(Selected)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	}
}

bool CUI::DoEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, int Corners, const IButtonColorFunction *pColorFunction)
{
	const bool Inside = MouseHovered(pRect);
	const bool Active = LastActiveItem() == pLineInput;
	const bool Changed = pLineInput->WasChanged();
	const char *pDisplayStr = pLineInput->GetDisplayedString();

	bool UpdateOffset = false;
	float ScrollOffset = pLineInput->GetScrollOffset();
	float ScrollOffsetChange = pLineInput->GetScrollOffsetChange();

	static bool s_DoScroll = false;
	static int s_SelectionStartOffset = -1;

	const float VSpacing = 2.0f;
	CUIRect Textbox;
	pRect->VMargin(VSpacing, &Textbox);

	float ScrollSpeed = 10.0f;
	if(Active)
	{
		int CursorOffset = pLineInput->GetCursorOffset();

		if(Inside && MouseButton(0) && !Changed)
		{
			s_DoScroll = true;
			const float MxRel = MouseX() - Textbox.x;
			float TotalTextWidth = 0.0f;
			for(int i = 1, Offset = 0; i <= pLineInput->GetNumChars(); i++)
			{
				const int PrevOffset = Offset;
				Offset = str_utf8_forward(pDisplayStr, Offset);
				const float AddedTextWidth = TextRender()->TextWidth(FontSize, pDisplayStr + PrevOffset, Offset - PrevOffset);
				if(TotalTextWidth + AddedTextWidth/2.0f - ScrollOffset - ScrollOffsetChange > MxRel)
				{
					CursorOffset = PrevOffset;
					if(s_SelectionStartOffset < 0)
						s_SelectionStartOffset = CursorOffset;
					UpdateOffset = true;
					break;
				}
				TotalTextWidth += AddedTextWidth;

				if(i == pLineInput->GetNumChars())
				{
					CursorOffset = pLineInput->GetLength();
					if(s_SelectionStartOffset < 0)
						s_SelectionStartOffset = CursorOffset;
					UpdateOffset = true;
				}
			}
		}
		else if(!MouseButton(0) || Changed)
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
		}
		else if(s_DoScroll)
		{
			if(absolute(ScrollOffsetChange) < 10.0f)
			{
				if(MouseX() < Textbox.x)
				{
					CursorOffset = str_utf8_rewind(pDisplayStr, CursorOffset);
					ScrollSpeed *= clamp(Textbox.x - MouseX(), 1.0f, Textbox.w / 8.0f);
					UpdateOffset = true;
				}
				else if(MouseX() > Textbox.x + Textbox.w)
				{
					CursorOffset = str_utf8_forward(pDisplayStr, CursorOffset);
					ScrollSpeed *= clamp(MouseX() - Textbox.x - Textbox.w, 1.0f, Textbox.w / 8.0f);
					UpdateOffset = true;
				}
			}
		}
		else if(!Inside && MouseButton(0))
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
			SetActiveItem(0);
			ClearLastActiveItem();
		}

		if(s_SelectionStartOffset >= 0)
		{
			const int ActualCursorOffset = pLineInput->OffsetFromDisplayToActual(CursorOffset);
			pLineInput->SetCursorOffset(ActualCursorOffset);
			pLineInput->SetSelection(pLineInput->OffsetFromDisplayToActual(s_SelectionStartOffset), ActualCursorOffset);
		}
	}

	bool JustGotActive = false;

	if(CheckActiveItem(pLineInput))
	{
		if(MouseButton(0))
		{
			if(pLineInput->IsActive() && (Input()->HasComposition() || Input()->GetCandidateCount()))
			{
				// Clear IME composition/candidates on mouse press
				Input()->StopTextInput();
				Input()->StartTextInput();
			}
		}
		else
		{
			s_DoScroll = false;
			s_SelectionStartOffset = -1;
			SetActiveItem(0);
		}
	}
	else if(HotItem() == pLineInput)
	{
		if(MouseButton(0))
		{
			if(!Active)
				JustGotActive = true;
			SetActiveItem(pLineInput);
		}
	}

	if(Inside)
		SetHotItem(pLineInput);

	if(Enabled() && Active && !JustGotActive)
		pLineInput->Activate(UI);
	else
		pLineInput->Deactivate();

	// render
	CTextCursor *pCursor = pLineInput->GetCursor();
	pCursor->m_FontSize = FontSize;
	pRect->Draw(pColorFunction->GetColor(Active, Inside), 5.0f, Corners);
	ClipEnable(pRect);
	Textbox.x -= ScrollOffset;
	ApplyCursorAlign(pCursor, &Textbox, TEXTALIGN_ML);
	pLineInput->Render(UpdateOffset || Changed);
	ClipDisable();

	// check if the text has to be moved
	if(Active && !JustGotActive && (UpdateOffset || Changed))
	{
		const float CaretX = pLineInput->GetCaretPosition().x - Textbox.x - ScrollOffset - ScrollOffsetChange;
		if(CaretX > Textbox.w)
			ScrollOffsetChange += CaretX - Textbox.w;
		else if(CaretX < 0.0f)
			ScrollOffsetChange += CaretX;
	}

	DoSmoothScrollLogic(&ScrollOffset, &ScrollOffsetChange, Textbox.w, pCursor->Width(), ScrollSpeed);

	pLineInput->SetScrollOffset(ScrollOffset);
	pLineInput->SetScrollOffsetChange(ScrollOffsetChange);

	return Changed;
}

void CUI::DoEditBoxOption(CLineInput *pLineInput, const CUIRect *pRect, const char *pStr, float VSplitVal)
{
	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, EditBox;
	pRect->VSplitLeft(VSplitVal, &Label, &EditBox);

	const float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pStr);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_MC);

	DoEditBox(pLineInput, &EditBox, FontSize);
}

float CUI::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	Current = clamp(Current, 0.0f, 1.0f);

	// layout
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);

	CUIRect Handle;
	Rail.HSplitTop(clamp(33.0f, Rail.w, Rail.h / 3.0f), &Handle, 0);
	Handle.y += (Rail.h - Handle.h) * Current;

	// logic
	static float s_OffsetY;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	bool Grabbed = false; // whether to apply the offset

	if(CheckActiveItem(pID))
	{
		if(MouseButton(0))
			Grabbed = true;
		else
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
		{
			s_OffsetY = MouseY() - Handle.y;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetY = Handle.h / 2.0f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	float ReturnValue = Current;
	if(Grabbed)
	{
		const float Min = Rail.y;
		const float Max = Rail.h - Handle.h;
		const float Cur = MouseY() - s_OffsetY;
		ReturnValue = clamp((Cur - Min) / Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.w / 2.0f);
	Handle.Draw(ScrollBarColorFunction.GetColor(Grabbed, InsideHandle), Handle.w / 2.0f);

	return ReturnValue;
}

float CUI::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	Current = clamp(Current, 0.0f, 1.0f);

	// layout
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);

	CUIRect Handle;
	Rail.VSplitLeft(clamp(33.0f, Rail.h, Rail.w / 3.0f), &Handle, 0);
	Handle.x += (Rail.w - Handle.w) * Current;

	// logic
	static float s_OffsetX;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	bool Grabbed = false; // whether to apply the offset

	if(CheckActiveItem(pID))
	{
		if(MouseButton(0))
			Grabbed = true;
		else
			SetActiveItem(0);
	}
	else if(HotItem() == pID)
	{
		if(MouseButton(0))
		{
			s_OffsetX = MouseX() - Handle.x;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetX = Handle.w / 2.0f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	float ReturnValue = Current;
	if(Grabbed)
	{
		const float Min = Rail.x;
		const float Max = Rail.w - Handle.w;
		const float Cur = MouseX() - s_OffsetX;
		ReturnValue = clamp((Cur - Min) / Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.h / 2.0f);
	Handle.Draw(ScrollBarColorFunction.GetColor(Grabbed, InsideHandle), Handle.h / 2.0f);

	return ReturnValue;
}

void CUI::DoScrollbarOption(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, const IScrollbarScale *pScale, unsigned char Options)
{
	const bool Infinite = Options & CUI::SCROLLBAR_OPTION_INFINITE;
	const bool NoClampValue = Options & CUI::SCROLLBAR_OPTION_NOCLAMPVALUE;
	int Value = *pOption;
	if(Infinite)
	{
		Min += 1;
		Max += 1;
		if(Value == 0)
			Value = Max;
	}

	char aBufMax[128];
	str_format(aBufMax, sizeof(aBufMax), "%s: %i", pStr, Max);
	char aBuf[128];
	if(!Infinite || Value != Max)
		str_format(aBuf, sizeof(aBuf), "%s: %i", pStr, Value);
	else
		str_format(aBuf, sizeof(aBuf), "%s: \xe2\x88\x9e", pStr);

	float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;
	float VSplitVal = maximum(TextRender()->TextWidth(FontSize, aBuf, -1), TextRender()->TextWidth(FontSize, aBufMax, -1));

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+10.0f+VSplitVal, &Label, &ScrollBar);
	Label.VSplitLeft(Label.h+5.0f, 0, &Label);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, Min, Max)), Min, Max);
	if (NoClampValue && ((Value == Min && *pOption < Min) || (Value == Max && *pOption > Max)))
	{
		Value = *pOption;
	}
	if(Infinite && Value == Max)
		Value = 0;

	*pOption = Value;
}

void CUI::DoScrollbarOptionLabeled(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, const char* aLabels[], int NumLabels, const IScrollbarScale *pScale)
{
	int Value = clamp(*pOption, 0, NumLabels - 1);
	const int Max = NumLabels - 1;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s: %s", pStr, aLabels[Value]);

	float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+5.0f, 0, &Label);
	Label.VSplitRight(60.0f, &Label, &ScrollBar);
	DoLabel(&Label, aBuf, FontSize, TEXTALIGN_ML);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, 0, Max)), 0, Max);

	if(HotItem() != pID && !CheckActiveItem(pID) && MouseHovered(pRect) && MouseButtonClicked(0))
		Value = (Value + 1) % NumLabels;

	*pOption = clamp(Value, 0, Max);
}

float CUI::DrawClientID(float FontSize, vec2 CursorPosition, int ID, const vec4& BgColor, const vec4& TextColor)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;

	char aBuf[4];
	str_format(aBuf, sizeof(aBuf), "%d", ID);

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_Align = TEXTALIGN_CENTER;

	vec4 OldColor = TextRender()->GetColor();
	TextRender()->TextColor(TextColor);
	TextRender()->TextDeferred(&s_Cursor, aBuf, -1);
	TextRender()->TextColor(OldColor);

	const float LinebaseY = CursorPosition.y + s_Cursor.BaseLineY();
	const float Width = 1.4f * FontSize;

	CUIRect Rect;
	Rect.x = CursorPosition.x;
	Rect.y = LinebaseY - FontSize + 0.15f * FontSize;
	Rect.w = Width;
	Rect.h = FontSize;
	Rect.Draw(BgColor, 0.25f * FontSize);

	s_Cursor.MoveTo(Rect.x + Rect.w / 2.0f, CursorPosition.y);
	TextRender()->DrawTextPlain(&s_Cursor);

	return Width + 0.2f * FontSize;
}

void CUI::DoTooltip(const void *pID, const CUIRect *pRect, const char *pText)
{
	if(MouseHovered(pRect))
	{
		m_pActiveTooltip = pID;
		m_TooltipAnchor = *pRect;
		str_copy(m_aTooltipText, pText, sizeof(m_aTooltipText));
	}
	else if(m_pActiveTooltip == pID)
	{
		m_pActiveTooltip = 0;
		m_aTooltipText[0] = '\0';
	}
}

void CUI::RenderTooltip()
{
	static const void *s_pLastTooltip = 0;
	if(!m_pActiveTooltip || !m_aTooltipText[0])
	{
		s_pLastTooltip = 0;
		return;
	}
	const int64 Now = time_get();
	static int64 s_TooltipActivationStart = 0;
	if(s_pLastTooltip != m_pActiveTooltip)
	{
		// Reset the fade in timer if a new tooltip got active
		s_TooltipActivationStart = Now;
		s_pLastTooltip = m_pActiveTooltip;
	}

	const float SecondsBeforeFadeIn = 0.75f;
	const float SecondsSinceActivation = (Now - s_TooltipActivationStart)/(float)time_freq();
	if(SecondsSinceActivation < SecondsBeforeFadeIn)
		return;
	const float SecondsFadeIn = 0.25f;
	const float AlphaFactor = SecondsSinceActivation < SecondsBeforeFadeIn+SecondsFadeIn ? (SecondsSinceActivation-SecondsBeforeFadeIn)/SecondsFadeIn : 1.0f;
	const CUIRect *pScreen = Screen();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = 10.0f;
	s_Cursor.m_MaxLines = -1;
	s_Cursor.m_MaxWidth = pScreen->w/4.0f;
	s_Cursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE|TEXTFLAG_WORD_WRAP;
	vec4 OldTextColor = TextRender()->GetColor();
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, AlphaFactor);
	TextRender()->TextDeferred(&s_Cursor, m_aTooltipText, -1);
	TextRender()->TextColor(OldTextColor);

	// Position tooltip below hovered rect, to the right of the mouse cursor.
	// If the tooltip would overflow the screen, position it to the top and/or left instead.
	CTextBoundingBox BoundingBox = s_Cursor.BoundingBox();
	const float Rounding = 4.0f;
	const float Spacing = 4.0f;
	const float Border = 1.0f;
	CUIRect Tooltip = { MouseX(), m_TooltipAnchor.y + m_TooltipAnchor.h + Spacing, BoundingBox.w + 2 * (Spacing+Border), BoundingBox.h + 2 * (Spacing+Border) };
	if(Tooltip.x + Tooltip.w > pScreen->w)
		Tooltip.x -= Tooltip.w;
	if(Tooltip.y + Tooltip.h > pScreen->h)
		Tooltip.y -= Tooltip.h + 2 * Spacing + m_TooltipAnchor.h;
	Tooltip.Draw(vec4(0.0f, 0.0f, 0.0f, 0.4f * AlphaFactor), Rounding);
	Tooltip.Margin(Border, &Tooltip);
	Tooltip.Draw(vec4(0.5f, 0.5f, 0.5f, 0.8f * AlphaFactor), Rounding);
	Tooltip.Margin(Spacing, &Tooltip);
	ApplyCursorAlign(&s_Cursor, &Tooltip, TEXTALIGN_ML);
	TextRender()->DrawTextOutlined(&s_Cursor);

	// Clear active tooltip. DoTooltip must be called each render call.
	m_pActiveTooltip = 0;
	m_aTooltipText[0] = '\0';
}

void CUI::DoPopupMenu(int X, int Y, int Width, int Height, void *pContext, bool (*pfnFunc)(void *pContext, CUIRect View), int Corners)
{
	dbg_assert(m_NumPopupMenus < MAX_POPUP_MENUS, "max popup menus exceeded");

	if(X + Width > Screen()->w)
		X = Screen()->w - Width;
	if(Y + Height > Screen()->h)
		Y = Screen()->h - Height;

	m_aPopupMenus[m_NumPopupMenus].m_Rect.x = X;
	m_aPopupMenus[m_NumPopupMenus].m_Rect.y = Y;
	m_aPopupMenus[m_NumPopupMenus].m_Rect.w = Width;
	m_aPopupMenus[m_NumPopupMenus].m_Rect.h = Height;
	m_aPopupMenus[m_NumPopupMenus].m_Corners = Corners;
	m_aPopupMenus[m_NumPopupMenus].m_pContext = pContext;
	m_aPopupMenus[m_NumPopupMenus].m_pfnFunc = pfnFunc;
	m_aPopupMenus[m_NumPopupMenus].m_New = true;
	m_NumPopupMenus++;
}

void CUI::RenderPopupMenus()
{
	const bool MousePressed = MouseButton(0) || MouseButton(1);
	static bool s_MousePressed = MousePressed;

	for(unsigned i = 0; i < m_NumPopupMenus; i++)
	{
		const bool Inside = MouseInside(&m_aPopupMenus[i].m_Rect);
		const bool Active = i == m_NumPopupMenus - 1;

		// prevent activation of UI elements outside of active popup
		if(Active)
			SetHotItem(&m_aPopupMenus[i]);

		CUIRect PopupRect = m_aPopupMenus[i].m_Rect;
		PopupRect.Draw(vec4(0.5f, 0.5f, 0.5f, 0.75f), 3.0f, m_aPopupMenus[i].m_Corners);
		PopupRect.Margin(1.0f, &PopupRect);
		PopupRect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.75f), 3.0f, m_aPopupMenus[i].m_Corners);
		PopupRect.Margin(4.0f, &PopupRect);

		if(m_aPopupMenus[i].m_pfnFunc(m_aPopupMenus[i].m_pContext, PopupRect))
			m_NumPopupMenus = i; // close this popup and all above it
		else if(m_aPopupMenus[i].m_New)
			m_aPopupMenus[i].m_New = false; // prevent popup from being closed by the click that opens it
		else if(Active && ((!Inside && s_MousePressed && !MousePressed) || ConsumeHotkey(HOTKEY_ESCAPE)))
			m_NumPopupMenus--; // close top-most popup by clicking outside and with escape
	}

	s_MousePressed = MousePressed;
}

float CUI::GetClientIDRectWidth(float FontSize)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;
	return 1.4f * FontSize + 0.2f * FontSize;
}

float CUI::GetListHeaderHeight() const
{
	return ms_ListheaderHeight + (m_pConfig->m_UiWideview ? 3.0f : 0.0f);
}

float CUI::GetListHeaderHeightFactor() const
{
	return 1.0f + (m_pConfig->m_UiWideview ? (3.0f/ms_ListheaderHeight) : 0.0f);
}
