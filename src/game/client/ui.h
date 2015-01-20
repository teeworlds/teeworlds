/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_H
#define GAME_CLIENT_UI_H

class CUIRect
{
	// TODO: Refactor: Redo UI scaling
	float Scale() const;
public:
	float x, y, w, h;

	void HSplitMid(CUIRect *pTop, CUIRect *pBottom) const;
	void HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void VSplitMid(CUIRect *pLeft, CUIRect *pRight) const;
	void VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const;
	void VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const;

	void Margin(float Cut, CUIRect *pOtherRect) const;
	void VMargin(float Cut, CUIRect *pOtherRect) const;
	void HMargin(float Cut, CUIRect *pOtherRect) const;

};

class CUI
{
	const void *m_pHotItem;
	const void *m_pActiveItem;
	const void *m_pLastActiveItem;
	const void *m_pBecommingHotItem;
	float m_MouseX, m_MouseY; // in gui space
	float m_MouseWorldX, m_MouseWorldY; // in world space
	unsigned m_MouseButtons;
	unsigned m_LastMouseButtons;

	CUIRect m_Screen;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;

public:
	// TODO: Refactor: Fill this in
	void SetGraphics(class IGraphics *pGraphics, class ITextRender *pTextRender) { m_pGraphics = pGraphics; m_pTextRender = pTextRender;}
	class IGraphics *Graphics() const { return m_pGraphics; }
	class ITextRender *TextRender() const { return m_pTextRender; }

	CUI();

	enum
	{
		CORNER_TL=1,
		CORNER_TR=2,
		CORNER_BL=4,
		CORNER_BR=8,
		CORNER_ITL=16,
		CORNER_ITR=32,
		CORNER_IBL=64,
		CORNER_IBR=128,

		CORNER_T=CORNER_TL|CORNER_TR,
		CORNER_B=CORNER_BL|CORNER_BR,
		CORNER_R=CORNER_TR|CORNER_BR,
		CORNER_L=CORNER_TL|CORNER_BL,

		CORNER_IT=CORNER_ITL|CORNER_ITR,
		CORNER_IB=CORNER_IBL|CORNER_IBR,
		CORNER_IR=CORNER_ITR|CORNER_IBR,
		CORNER_IL=CORNER_ITL|CORNER_IBL,

		CORNER_ALL=CORNER_T|CORNER_B,
		CORNER_INV_ALL=CORNER_IT|CORNER_IB
	};

	enum EAlignment
	{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
	};

	int Update(float mx, float my, float Mwx, float Mwy, int m_Buttons);

	float MouseX() const { return m_MouseX; }
	float MouseY() const { return m_MouseY; }
	float MouseWorldX() const { return m_MouseWorldX; }
	float MouseWorldY() const { return m_MouseWorldY; }
	int MouseButton(int Index) const { return (m_MouseButtons>>Index)&1; }
	int MouseButtonClicked(int Index) const { return MouseButton(Index) && !((m_LastMouseButtons>>Index)&1) ; }

	void SetHotItem(const void *pID) { m_pBecommingHotItem = pID; }
	void SetActiveItem(const void *pID) { m_pActiveItem = pID; if (pID) m_pLastActiveItem = pID; }
	void ClearLastActiveItem() { m_pLastActiveItem = 0; }
	const void *HotItem() const { return m_pHotItem; }
	const void *NextHotItem() const { return m_pBecommingHotItem; }
	const void *ActiveItem() const { return m_pActiveItem; }
	const void *LastActiveItem() const { return m_pLastActiveItem; }

	int MouseInside(const CUIRect *pRect) const;
	void ConvertMouseMove(float *x, float *y) const;

	CUIRect *Screen();
	float PixelSize();
	void ClipEnable(const CUIRect *pRect);
	void ClipDisable();

	// TODO: Refactor: Redo UI scaling
	float Scale() const;

	int DoButtonLogic(const void *pID, const char *pText /* TODO: Refactor: Remove */, int Checked, const CUIRect *pRect);
	int DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY);
	int DoColorSelectionLogic(const CUIRect *pRect, const CUIRect *pButton);

	// TODO: Refactor: Remove this?
	void DoLabel(const CUIRect *pRect, const char *pText, float Size, EAlignment Align, int MaxWidth = -1);
	void DoLabelScaled(const CUIRect *pRect, const char *pText, float Size, EAlignment Align, int MaxWidth = -1);
};


#endif
