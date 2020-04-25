#pragma once
#include <base/vmath.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/lineinput.h>

#include "ed2_common.h"

struct CUIButton
{
	u8 m_Hovered;
	u8 m_Pressed;
	u8 m_Clicked;

	CUIButton()
	{
		m_Hovered = false;
		m_Pressed = false;
		m_Clicked = false;
	}
};

struct CUITextInput
{
	CUIButton m_Button;
	u8 m_Selected;
	CLineInput m_LineInput;
	int m_CursorPos;

	CUITextInput()
	{
		m_Selected = false;
	}
};

struct CUIIntegerInput
{
	CUITextInput m_TextInput;
	char m_aIntBuff[10]; // 10 clamps the int between -99999999 and 999999999, which should be more than enough. Do a 64bit version if it is not.

	CUIIntegerInput()
	{
		m_aIntBuff[0] = 0;
	}
};

struct CUIMouseDrag
{
	vec2 m_StartDragPos;
	vec2 m_EndDragPos;
	bool m_IsDragging;
	CUIButton m_Button;

	CUIMouseDrag()
	{
		m_IsDragging = false;
	}
};

struct CUICheckboxYesNo
{
	CUIButton m_YesBut;
	CUIButton m_NoBut;
};

struct CUIGrabHandle: CUIMouseDrag
{
	bool m_IsGrabbed;
};

struct CUIListBox
{
	enum {
		MAX_COLUMNS = 10,
		COLTYPE_TEXT = 0,
		COLTYPE_DATE,
		COLTYPE_ICON,
	};

	struct ColData{
		int m_Type;
		int m_Width;
		const char *m_pName;
	};

	struct Entry {
		int m_Id;
		const void *m_paData[10];
	};

	int m_Hovering;
	int m_Selected;

	int m_SortCol;
	int m_SortDir;

	CUIListBox()
	{
		m_Hovering = m_Selected = m_SortCol = m_SortDir = -1;
	}
};

struct CEditor2Ui
{
	ITextRender *m_pTextRender;
	IGraphics* m_pGraphics;
	CRenderTools m_RenderTools;
	IInput *m_pInput;
	CUI m_UI;

	float m_LocalTime;

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline ITextRender *TextRender() { return m_pTextRender; };
	inline CRenderTools *RenderTools() { return &m_RenderTools; }
	inline IInput *Input() { return m_pInput; };
	inline CUI *UI() { return &m_UI; }

	void DrawRect(const CUIRect& Rect, const vec4& Color);
	void DrawRectBorder(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderOutside(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawRectBorderMiddle(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor);
	void DrawText(const CUIRect& Rect, const char* pText, float FontSize, vec4 Color = vec4(1,1,1,1), CUI::EAlignment Align = CUI::ALIGN_LEFT);

	void UiDoButtonBehaviorNoID(const CUIRect& Rect, CUIButton* pButState);
	void UiDoButtonBehavior(const void* pID, const CUIRect& Rect, CUIButton* pOutButState);
	bool UiDoMouseDraggingNoID(const CUIRect& Rect, CUIMouseDrag* pDragState);
	bool UiDoMouseDragging(const CUIRect& Rect, CUIMouseDrag* pDragState);

	bool UiButton(const CUIRect& Rect, const char* pText, CUIButton* pButState, float FontSize = 10, CUI::EAlignment TextAlign = CUI::ALIGN_LEFT);
	bool UiButtonEx(const CUIRect& Rect, const char* pText, CUIButton* pButState,
					vec4 ColNormal, vec4 ColHover, vec4 ColPress, vec4 ColBorder, float FontSize, CUI::EAlignment TextAlign);
	bool UiTextInput(const CUIRect& Rect, char* pText, int TextMaxLength, CUITextInput* pInputState);
	bool UiIntegerInput(const CUIRect& Rect, int* pInteger, CUIIntegerInput* pInputState);
	bool UiSliderInt(const CUIRect& Rect, int* pInteger, int Min, int Max, CUIButton* pInputState);
	bool UiSliderFloat(const CUIRect& Rect, float* pVal, float Min, float Max, CUIButton* pInputState,
		const vec4* pColor = NULL);
	bool UiCheckboxYesNo(const CUIRect& Rect, bool* pVal, CUICheckboxYesNo* pCbyn);
	bool UiButtonSelect(const CUIRect& Rect, const char* pText, CUIButton* pButState, bool Selected,
		float FontSize = 10);
	bool UiGrabHandle(const CUIRect& Rect, CUIGrabHandle* pGrabHandle, const vec4& ColorNormal, const vec4& ColorActive); // Returns pGrabHandle->m_IsDragging
	bool UiListBox(const CUIRect& Rect, const CUIListBox::ColData *pColumns, int ColumnCount,
		CUIListBox::Entry *pEntries, int EntryCount, const char *pFilter, int FilterCol,
		CUIListBox *pListBoxState);

	struct CScrollRegionParams
	{
		float m_ScrollbarWidth;
		float m_ScrollbarMargin;
		float m_SliderMinHeight;
		float m_ScrollSpeed;
		vec4 m_ClipBgColor;
		vec4 m_ScrollbarBgColor;
		vec4 m_RailBgColor;
		vec4 m_SliderColor;
		vec4 m_SliderColorHover;
		vec4 m_SliderColorGrabbed;
		int m_Flags;

		enum {
			FLAG_CONTENT_STATIC_WIDTH = 0x1
		};

		CScrollRegionParams()
		{
			m_ScrollbarWidth = 8;
			m_ScrollbarMargin = 1;
			m_SliderMinHeight = 25;
			m_ScrollSpeed = 5;
			m_ClipBgColor = vec4(0.0f, 0.0f, 0.0f, 0.5f);
			m_ScrollbarBgColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			m_RailBgColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			m_SliderColor = vec4(0.2f, 0.1f, 0.98f, 1.0f);
			m_SliderColorHover = vec4(0.4f, 0.41f, 1.0f, 1.0f);
			m_SliderColorGrabbed = vec4(0.2f, 0.1f, 0.98f, 1.0f);
			m_Flags = 0;
		}
	};

	struct CScrollRegion
	{
		float m_ScrollY;
		float m_ContentH;
		float m_RequestScrollY; // [0, ContentHeight]
		CUIRect m_ClipRect;
		CUIRect m_OldClipRect;
		CUIRect m_RailRect;
		CUIRect m_LastAddedRect; // saved for ScrollHere()
		vec2 m_MouseGrabStart;
		vec2 m_ContentScrollOff;
		bool m_WasClipped;
		CScrollRegionParams m_Params;

		enum {
			SCROLLHERE_KEEP_IN_VIEW=0,
			SCROLLHERE_TOP,
			SCROLLHERE_BOTTOM,
		};

		CScrollRegion()
		{
			m_ScrollY = 0;
			m_ContentH = 0;
			m_RequestScrollY = -1;
			m_ContentScrollOff = vec2(0,0);
			m_WasClipped = false;
			m_Params = CScrollRegionParams();
		}
	};

	void UiBeginScrollRegion(CScrollRegion* pSr, CUIRect* pClipRect, vec2* pOutOffset, const CScrollRegionParams* pParams = 0);
	void UiEndScrollRegion(CScrollRegion* pSr);
	void UiScrollRegionAddRect(CScrollRegion* pSr, CUIRect Rect);
	void UiScrollRegionScrollHere(CScrollRegion* pSr, int Option = CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
	bool UiScrollRegionIsRectClipped(CScrollRegion* pSr, const CUIRect& Rect);
};


// Style colors
const vec4 StyleColorBg(0.03f, 0, 0.085f, 1);
const vec4 StyleColorButton(0.062f, 0, 0.19f, 1);
const vec4 StyleColorButtonBorder(0.18f, 0.00f, 0.56f, 1);
const vec4 StyleColorButtonHover(0.28f, 0.10f, 0.64f, 1);
const vec4 StyleColorButtonPressed(0.13f, 0, 0.40f, 1);
const vec4 StyleColorInputSelected(0,0.2f,1,1);
const vec4 StyleColorTileSelection(0.0, 0.31f, 1, 0.4f);
const vec4 StyleColorTileHover(1, 1, 1, 0.25f);
const vec4 StyleColorTileHoverBorder(0.0, 0.31f, 1, 1);

const float MenuBarHeight = 20;
