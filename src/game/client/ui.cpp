/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/input.h>
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
	m_pActiveInput = 0;

	m_Screen.x = 0;
	m_Screen.y = 0;

	m_NumClips = 0;
}

void CUI::Init(class CConfig *pConfig, class IGraphics *pGraphics, class IInput *pInput, class ITextRender *pTextRender)
{
	m_pConfig = pConfig;
	m_pGraphics = pGraphics;
	m_pInput = pInput;
	m_pTextRender = pTextRender;
	CUIRect::Init(pGraphics);
	CLineInput::Init(pInput);
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
	if(m_pActiveInput != m_pLastActiveItem)
		m_pActiveInput = 0;
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

	if(m_pActiveInput && m_pActiveInput->ProcessInput(e))
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

void CUI::DoLabel(const CUIRect *pRect, const char *pText, float FontSize, EAlignment Align, float LineWidth, bool MultiLine)
{
	// TODO: FIX ME!!!!
	// Graphics()->BlendNormal();

	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxLines = MultiLine ? -1 : 1;
	s_Cursor.m_MaxWidth = LineWidth;
	s_Cursor.m_Align = Align;

	switch(Align)
	{
	case CUI::ALIGN_LEFT:
		s_Cursor.m_Align = TEXTALIGN_LEFT;
		s_Cursor.MoveTo(pRect->x, pRect->y);
		break;
	case CUI::ALIGN_CENTER:
		s_Cursor.m_Align = TEXTALIGN_CENTER;
		s_Cursor.MoveTo(pRect->x + pRect->w / 2.0f, pRect->y);
		break;
	case CUI::ALIGN_RIGHT:
		s_Cursor.m_Align = TEXTALIGN_RIGHT;
		s_Cursor.MoveTo(pRect->x + pRect->w, pRect->y);
		break;
	}
	TextRender()->TextOutlined(&s_Cursor, pText, -1);
}

void CUI::DoLabelHighlighted(const CUIRect *pRect, const char *pText, const char *pHighlighted, float FontSize, const vec4 &TextColor, const vec4 &HighlightColor)
{
	static CTextCursor s_Cursor;
	s_Cursor.Reset();
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_MaxWidth = pRect->w;
	s_Cursor.MoveTo(pRect->x, pRect->y);

	TextRender()->TextColor(TextColor);
	const char *pMatch = pHighlighted && pHighlighted[0] ? str_find_nocase(pText, pHighlighted) : 0;
	if(pMatch)
	{
		TextRender()->TextDeferred(&s_Cursor, pText, (int)(pMatch - pText));
		TextRender()->TextColor(HighlightColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch, str_length(pHighlighted));
		TextRender()->TextColor(TextColor);
		TextRender()->TextDeferred(&s_Cursor, pMatch + str_length(pHighlighted), -1);
	}
	else
		TextRender()->TextDeferred(&s_Cursor, pText, -1);

	TextRender()->DrawTextOutlined(&s_Cursor);
}

bool CUI::DoEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, bool Hidden, int Corners, const IButtonColorFunction *pColorFunction)
{
	const bool Inside = MouseHovered(pRect);
	const int Len = pLineInput->GetLength();
	const bool Changed = pLineInput->WasChanged();

	bool UpdateOffset = false;
	float ScrollOffset = pLineInput->GetScrollOffset();

	static bool s_DoScroll = false;

	if(LastActiveItem() == pLineInput)
	{
		static float s_ScrollStart = 0.0f;

		if(Inside && MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = MouseX();
			int MxRel = (int)(MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(TextRender()->TextWidth(FontSize, pLineInput->GetString(), i) - ScrollOffset > MxRel)
				{
					pLineInput->SetCursorOffset(i - 1);
					break;
				}

				if(i == Len)
					pLineInput->SetCursorOffset(Len);
			}
		}
		else if(!MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(MouseX() < pRect->x && s_ScrollStart-MouseX() > 10.0f)
			{
				pLineInput->SetCursorOffset(pLineInput->GetCursorOffset()-1);
				s_ScrollStart = MouseX();
				UpdateOffset = true;
			}
			else if(MouseX() > pRect->x+pRect->w && MouseX()-s_ScrollStart > 10.0f)
			{
				pLineInput->SetCursorOffset(pLineInput->GetCursorOffset()+1);
				s_ScrollStart = MouseX();
				UpdateOffset = true;
			}
		}
		else if(!Inside && MouseButton(0))
		{
			s_DoScroll = false;
			SetActiveItem(0);
			ClearLastActiveItem();
		}

		m_pActiveInput = pLineInput;
	}

	bool JustGotActive = false;

	if(CheckActiveItem(pLineInput))
	{
		if(!MouseButton(0))
		{
			s_DoScroll = false;
			SetActiveItem(0);
		}
	}
	else if(HotItem() == pLineInput)
	{
		if(MouseButton(0))
		{
			if(LastActiveItem() != pLineInput)
				JustGotActive = true;
			SetActiveItem(pLineInput);
		}
	}

	if(Inside)
		SetHotItem(pLineInput);

	const float Spacing = 2.0f;
	CUIRect Textbox = *pRect;
	Textbox.Draw(pColorFunction->GetColor(LastActiveItem() == pLineInput, Inside), 5.0f, Corners);
	Textbox.VMargin(Spacing, &Textbox);
	Textbox.HMargin((Textbox.h-FontSize/ms_FontmodHeight)/2, &Textbox);

	const char *pDisplayStr = pLineInput->GetString();
	char aStars[128];

	if(Hidden)
	{
		unsigned s = Len;
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(LastActiveItem() == pLineInput && !JustGotActive && (UpdateOffset || Changed))
	{
		float w = TextRender()->TextWidth(FontSize, pDisplayStr, pLineInput->GetCursorOffset());
		if(w-ScrollOffset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(FontSize, pDisplayStr, -1);
			do
			{
				ScrollOffset += minimum(wt-ScrollOffset-Textbox.w, Textbox.w/3);
			}
			while(w-ScrollOffset > Textbox.w);
		}
		else if(w-ScrollOffset < 0.0f)
		{
			// move to the right
			do
			{
				ScrollOffset = maximum(0.0f, ScrollOffset-Textbox.w/3);
			}
			while(w-ScrollOffset < 0.0f);
		}
	}
	ClipEnable(pRect);
	Textbox.x -= ScrollOffset;

	DoLabel(&Textbox, pDisplayStr, FontSize, CUI::ALIGN_LEFT);

	// render the cursor
	if(LastActiveItem() == pLineInput && !JustGotActive)
	{
		if((2*time_get()/time_freq()) % 2)	// make it blink
		{
			float TextWidth = TextRender()->TextWidth(FontSize, pDisplayStr, pLineInput->GetCursorOffset());
			Textbox = *pRect;
			Textbox.VSplitLeft(Spacing, 0, &Textbox);
			Textbox.x += TextWidth - ScrollOffset - TextRender()->TextWidth(FontSize, "|", -1)/2;
			DoLabel(&Textbox, "|", FontSize, CUI::ALIGN_LEFT);
		}
	}
	ClipDisable();

	pLineInput->SetScrollOffset(ScrollOffset);

	return Changed;
}

void CUI::DoEditBoxOption(CLineInput *pLineInput, const CUIRect *pRect, const char *pStr, float VSplitVal, bool Hidden)
{
	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, EditBox;
	pRect->VSplitLeft(VSplitVal, &Label, &EditBox);

	const float FontSize = pRect->h*ms_FontmodHeight*0.8f;
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pStr);
	Label.y += 2.0f;
	DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_CENTER);

	DoEditBox(pLineInput, &EditBox, FontSize, Hidden);
}

float CUI::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	Current = clamp(Current, 0.0f, 1.0f);
	// layout
	CUIRect Handle;
	pRect->HSplitTop(minimum(pRect->h/8.0f, 33.0f), &Handle, 0);
	Handle.y += (pRect->h-Handle.h)*Current;
	Handle.VMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);

	// logic
	static float s_OffsetY;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	float ReturnValue = Current;
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
			s_OffsetY = MouseY()-Handle.y;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetY = Handle.h * 0.5f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->y;
		const float Max = pRect->h-Handle.h;
		const float Cur = MouseY()-s_OffsetY;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.w/2.0f);
	Handle.Draw(ScrollBarColorFunction.GetColor(Grabbed, InsideHandle), Handle.w/2.0f);

	return ReturnValue;
}

float CUI::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	Current = clamp(Current, 0.0f, 1.0f);
	// layout
	CUIRect Handle;
	pRect->VSplitLeft(maximum(minimum(pRect->w/8.0f, 33.0f), pRect->h), &Handle, 0);
	Handle.x += (pRect->w-Handle.w)*Current;
	Handle.HMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);

	// logic
	static float s_OffsetX;
	const bool InsideHandle = MouseHovered(&Handle);
	const bool InsideRail = MouseHovered(&Rail);
	float ReturnValue = Current;
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
			s_OffsetX = MouseX()-Handle.x;
			SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetX = Handle.w * 0.5f;
		SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->x;
		const float Max = pRect->w-Handle.w;
		const float Cur = MouseX()-s_OffsetX;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.h/2.0f);
	Handle.Draw(ScrollBarColorFunction.GetColor(Grabbed, InsideHandle), Handle.h/2.0f);

	return ReturnValue;
}

void CUI::DoScrollbarOption(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, const IScrollbarScale *pScale, bool Infinite)
{
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

	float FontSize = pRect->h*ms_FontmodHeight*0.8f;
	float VSplitVal = maximum(TextRender()->TextWidth(FontSize, aBuf, -1), TextRender()->TextWidth(FontSize, aBufMax, -1));

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+10.0f+VSplitVal, &Label, &ScrollBar);
	Label.VSplitLeft(Label.h+5.0f, 0, &Label);
	Label.y += 2.0f;
	DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_LEFT);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, Min, Max)), Min, Max);
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

	float FontSize = pRect->h*ms_FontmodHeight*0.8f;

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+5.0f, 0, &Label);
	Label.VSplitRight(60.0f, &Label, &ScrollBar);
	Label.y += 2.0f;
	DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_LEFT);

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

float CUI::GetClientIDRectWidth(float FontSize)
{
	if(!m_pConfig->m_ClShowUserId)
		return 0;
	return 1.4f * FontSize + 0.2f * FontSize;
}
