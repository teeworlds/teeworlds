/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_H
#define GAME_CLIENT_UI_H

#include <engine/textrender.h>
#include "lineinput.h"
#include "ui_rect.h"

class IScrollbarScale
{
public:
	virtual float ToRelative(int AbsoluteValue, int Min, int Max) const = 0;
	virtual int ToAbsolute(float RelativeValue, int Min, int Max) const = 0;
};
class CLinearScrollbarScale : public IScrollbarScale
{
public:
	float ToRelative(int AbsoluteValue, int Min, int Max) const
	{
		return (AbsoluteValue - Min) / (float)(Max - Min);
	}
	int ToAbsolute(float RelativeValue, int Min, int Max) const
	{
		return round_to_int(RelativeValue*(Max - Min) + Min + 0.1f);
	}
};
class CLogarithmicScrollbarScale : public IScrollbarScale
{
private:
	int m_MinAdjustment;
public:
	CLogarithmicScrollbarScale(int MinAdjustment)
	{
		m_MinAdjustment = maximum(MinAdjustment, 1); // must be at least 1 to support Min == 0 with logarithm
	}
	float ToRelative(int AbsoluteValue, int Min, int Max) const
	{
		if(Min < m_MinAdjustment)
		{
			AbsoluteValue += m_MinAdjustment;
			Min += m_MinAdjustment;
			Max += m_MinAdjustment;
		}
		return (log(AbsoluteValue) - log(Min)) / (float)(log(Max) - log(Min));
	}
	int ToAbsolute(float RelativeValue, int Min, int Max) const
	{
		int ResultAdjustment = 0;
		if(Min < m_MinAdjustment)
		{
			Min += m_MinAdjustment;
			Max += m_MinAdjustment;
			ResultAdjustment = -m_MinAdjustment;
		}
		return round_to_int(exp(RelativeValue*(log(Max) - log(Min)) + log(Min))) + ResultAdjustment;
	}
};


class IButtonColorFunction
{
public:
	virtual vec4 GetColor(bool Active, bool Hovered) const = 0;
};
static class CDarkButtonColorFunction : public IButtonColorFunction
{
public:
	vec4 GetColor(bool Active, bool Hovered) const
	{
		if(Active)
			return vec4(0.15f, 0.15f, 0.15f, 0.25f);
		else if(Hovered)
			return vec4(0.5f, 0.5f, 0.5f, 0.25f);
		return vec4(0.0f, 0.0f, 0.0f, 0.25f);
	}
} const DarkButtonColorFunction;
static class CLightButtonColorFunction : public IButtonColorFunction
{
public:
	vec4 GetColor(bool Active, bool Hovered) const
	{
		if(Active)
			return vec4(1.0f, 1.0f, 1.0f, 0.4f);
		else if(Hovered)
			return vec4(1.0f, 1.0f, 1.0f, 0.6f);
		return vec4(1.0f, 1.0f, 1.0f, 0.5f);
	}
} const LightButtonColorFunction;
static class CScrollBarColorFunction : public IButtonColorFunction
{
public:
	vec4 GetColor(bool Active, bool Hovered) const
	{
		if(Active)
			return vec4(0.9f, 0.9f, 0.9f, 1.0f);
		else if(Hovered)
			return vec4(1.0f, 1.0f, 1.0f, 1.0f);
		return vec4(0.8f, 0.8f, 0.8f, 1.0f);
	}
} const ScrollBarColorFunction;


class CUIElementBase
{
private:
	static class CUI *s_pUI;

public:
	static void Init(CUI *pUI) { s_pUI = pUI; }

	class CUI *UI() const { return s_pUI; }
	class IClient *Client() const;
	class CConfig *Config() const;
	class IGraphics *Graphics() const;
	class IInput *Input() const;
	class ITextRender *TextRender() const;
};

class CButtonContainer : public CUIElementBase
{
	bool m_CleanBackground;
	float m_FadeStartTime;
public:
	CButtonContainer(bool CleanBackground = false) : m_FadeStartTime(0.0f) { m_CleanBackground = CleanBackground; }
	float GetFade(bool Checked = false, float Seconds = 0.6f);
	bool IsCleanBackground() const { return m_CleanBackground; }
};


class CUI
{
	enum
	{
		MAX_CLIP_NESTING_DEPTH = 16,
		MAX_POPUP_MENUS = 8,
	};

	bool m_Enabled;

	const void *m_pHotItem;
	const void *m_pActiveItem;
	const void *m_pLastActiveItem;
	const void *m_pBecommingHotItem;
	bool m_ActiveItemValid;

	float m_MouseX, m_MouseY; // in gui space
	float m_MouseWorldX, m_MouseWorldY; // in world space
	unsigned m_MouseButtons;
	unsigned m_LastMouseButtons;

	unsigned m_HotkeysPressed;

	CUIRect m_Screen;

	CUIRect m_aClips[MAX_CLIP_NESTING_DEPTH];
	unsigned m_NumClips;
	void UpdateClipping();

	const void *m_pActiveTooltip;
	CUIRect m_TooltipAnchor;
	char m_aTooltipText[256];

	class
	{
	public:
		CUIRect m_Rect;
		int m_Corners;
		void *m_pContext;
		bool (*m_pfnFunc)(void *pContext, CUIRect View); // returns true to close popup
		bool m_New;
	} m_aPopupMenus[MAX_POPUP_MENUS];
	unsigned m_NumPopupMenus;

	class IClient *m_pClient;
	class CConfig *m_pConfig;
	class IGraphics *m_pGraphics;
	class IInput *m_pInput;
	class ITextRender *m_pTextRender;

	void ApplyCursorAlign(class CTextCursor *pCursor, const CUIRect *pRect, int Align);

public:
	static const vec4 ms_DefaultTextColor;
	static const vec4 ms_DefaultTextOutlineColor;
	static const vec4 ms_HighlightTextColor;
	static const vec4 ms_HighlightTextOutlineColor;
	static const vec4 ms_TransparentTextColor;

	static const float ms_ListheaderHeight;
	static const float ms_FontmodHeight;

	static const CLinearScrollbarScale ms_LinearScrollbarScale;
	static const CLogarithmicScrollbarScale ms_LogarithmicScrollbarScale;

	void Init(class IKernel *pKernel);
	class IClient *Client() const { return m_pClient; }
	class CConfig *Config() const { return m_pConfig; }
	class IGraphics *Graphics() const { return m_pGraphics; }
	class IInput *Input() const { return m_pInput; }
	class ITextRender *TextRender() const { return m_pTextRender; }

	CUI();

	enum
	{
		HOTKEY_ENTER = 1,
		HOTKEY_ESCAPE = 2,
		HOTKEY_UP = 4,
		HOTKEY_DOWN = 8,
		HOTKEY_DELETE = 16,
		HOTKEY_TAB = 32,
	};

	void SetEnabled(bool Enabled) { m_Enabled = Enabled; }
	bool Enabled() const { return m_Enabled; }
	void Update(float MouseX, float MouseY, float MouseWorldX, float MouseWorldY);

	float MouseX() const { return m_MouseX; }
	float MouseY() const { return m_MouseY; }
	float MouseWorldX() const { return m_MouseWorldX; }
	float MouseWorldY() const { return m_MouseWorldY; }
	bool MouseButton(int Index) const { return (m_MouseButtons>>Index)&1; }
	bool MouseButtonClicked(int Index) const { return MouseButton(Index) && !((m_LastMouseButtons>>Index)&1) ; }

	void SetHotItem(const void *pID) { m_pBecommingHotItem = pID; }
	void SetActiveItem(const void *pID) { m_ActiveItemValid = true; m_pActiveItem = pID; if (pID) m_pLastActiveItem = pID; }
	bool CheckActiveItem(const void *pID) { if(m_pActiveItem == pID) { m_ActiveItemValid = true; return true; } return false; }
	void ClearLastActiveItem() { m_pLastActiveItem = 0; }
	const void *HotItem() const { return m_pHotItem; }
	const void *NextHotItem() const { return m_pBecommingHotItem; }
	const void *GetActiveItem() const { return m_pActiveItem; }
	const void *LastActiveItem() const { return m_pLastActiveItem; }

	void StartCheck() { m_ActiveItemValid = false; }
	void FinishCheck() { if(!m_ActiveItemValid && m_pActiveItem != 0) { SetActiveItem(0); m_pHotItem = 0; m_pBecommingHotItem = 0; } }

	bool MouseInside(const CUIRect *pRect) const { return pRect->Inside(m_MouseX, m_MouseY); }
	bool MouseInsideClip() const { return !IsClipped() || MouseInside(ClipArea()); }
	bool MouseHovered(const CUIRect *pRect) const { return MouseInside(pRect) && MouseInsideClip(); }
	void ConvertCursorMove(float *pX, float *pY, int CursorType) const;

	bool KeyPress(int Key) const;
	bool KeyIsPressed(int Key) const;
	bool ConsumeHotkey(unsigned Hotkey);
	void ClearHotkeys() { m_HotkeysPressed = 0; }
	bool OnInput(const IInput::CEvent &e);
	bool IsInputActive() const { return CLineInput::GetActiveInput() != 0; }

	const CUIRect *Screen();
	float PixelSize();
	void MapScreen();

	// clipping
	void ClipEnable(const CUIRect *pRect);
	void ClipDisable();
	const CUIRect *ClipArea() const;
	inline bool IsClipped() const { return m_NumClips > 0; }

	bool DoButtonLogic(const void *pID, const CUIRect *pRect, int Button = 0);
	bool DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY);
	void DoSmoothScrollLogic(float *pScrollOffset, float *pScrollOffsetChange, float ViewPortSize, float TotalSize, float ScrollSpeed = 10.0f);

	// labels
	void DoLabel(const CUIRect *pRect, const char *pText, float FontSize, int Align = TEXTALIGN_TL, float LineWidth = -1.0f, bool MultiLine = true);
	void DoLabelHighlighted(const CUIRect *pRect, const char *pText, const char *pHighlighted, float FontSize, const vec4 &TextColor, const vec4 &HighlightColor, int Align = TEXTALIGN_TL);
	void DoLabelSelected(const CUIRect *pRect, const char *pText, bool Selected, float FontSize, int Align = TEXTALIGN_TL);

	// editboxes
	bool DoEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, int Corners = CUIRect::CORNER_ALL, const IButtonColorFunction *pColorFunction = &DarkButtonColorFunction);
	void DoEditBoxOption(CLineInput *pLineInput, const CUIRect *pRect, const char *pStr, float VSplitVal);

	// scrollbars
	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoScrollbarOption(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, const IScrollbarScale *pScale = &ms_LinearScrollbarScale, unsigned char Options = 0);
	void DoScrollbarOptionLabeled(const void *pID, int *pOption, const CUIRect *pRect, const char *pStr, const char *apLabels[], int NumLabels, const IScrollbarScale *pScale = &ms_LinearScrollbarScale);

	enum
	{
		SCROLLBAR_OPTION_INFINITE = 1 << 0,
		SCROLLBAR_OPTION_NOCLAMPVALUE = 1 << 1,
	};

	// tooltips
	void DoTooltip(const void *pID, const CUIRect *pRect, const char *pText);
	void RenderTooltip();

	// popup menu
	void DoPopupMenu(int X, int Y, int Width, int Height, void *pContext, bool (*pfnFunc)(void *pContext, CUIRect View), int Corners = CUIRect::CORNER_ALL);
	void RenderPopupMenus();
	bool IsPopupActive() const { return m_NumPopupMenus > 0; }

	// client ID
	float DrawClientID(float FontSize, vec2 Position, int ID,
					const vec4& BgColor = vec4(1.0f, 1.0f, 1.0f, 0.5f), const vec4& TextColor = vec4(0.1f, 0.1f, 0.1f, 1.0f));
	float GetClientIDRectWidth(float FontSize);

	float GetListHeaderHeight() const;
	float GetListHeaderHeightFactor() const;
};


#endif
