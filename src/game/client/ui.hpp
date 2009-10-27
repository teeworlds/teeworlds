/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef FILE_GAME_CLIENT_UI_H
#define FILE_GAME_CLIENT_UI_H

class CUIRect
{
	// TODO: Refactor: Redo UI scaling
	float Scale() const { return 1.0f; }
public:
    float x, y, w, h;
	
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
	float m_MouseX, m_MouseY; /* in gui space */
	float m_MouseWorldX, m_MouseWorldY; /* in world space */
	unsigned m_MouseButtons;
	unsigned m_LastMouseButtons;
	
	CUIRect m_Screen;
	class IGraphics *m_pGraphics;
	
public:
	// TODO: Refactor: Fill this in
	void SetGraphics(class IGraphics *pGraphics) { m_pGraphics = pGraphics; }
	class IGraphics *Graphics() { return m_pGraphics; }

	CUI();

	enum
	{
		CORNER_TL=1,
		CORNER_TR=2,
		CORNER_BL=4,
		CORNER_BR=8,
		
		CORNER_T=CORNER_TL|CORNER_TR,
		CORNER_B=CORNER_BL|CORNER_BR,
		CORNER_R=CORNER_TR|CORNER_BR,
		CORNER_L=CORNER_TL|CORNER_BL,
		
		CORNER_ALL=CORNER_T|CORNER_B
	};

	int Update(float mx, float my, float mwx, float mwy, int buttons);

	float MouseX() const { return m_MouseX; }
	float MouseY() const { return m_MouseY; }
	float MouseWorldX() const { return m_MouseWorldX; }
	float MouseWorldY() const { return m_MouseWorldY; }
	int MouseButton(int Index) const { return (m_MouseButtons>>Index)&1; }
	int MouseButtonClicked(int Index) { return MouseButton(Index) && !((m_LastMouseButtons>>Index)&1) ; }

	void SetHotItem(const void *pID) { m_pBecommingHotItem = pID; }
	void SetActiveItem(const void *pID) { m_pActiveItem = pID; if (pID) m_pLastActiveItem = pID; }
	void ClearLastActiveItem() { m_pLastActiveItem = 0; }
	const void *HotItem() const { return m_pHotItem; }
	const void *NextHotItem() const { return m_pBecommingHotItem; }
	const void *ActiveItem() const { return m_pActiveItem; }
	const void *LastActiveItem() const { return m_pLastActiveItem; }

	int MouseInside(const CUIRect *r);

	CUIRect *Screen();
	void ClipEnable(const CUIRect *r);
	void ClipDisable();
	
	// TODO: Refactor: Redo UI scaling
	void SetScale(float s);
	float Scale() const { return 1.0f; }

	int DoButtonLogic(const void *pID, const char *pText /* TODO: Refactor: Remove */, int Checked, const CUIRect *pRect);
	
	// TODO: Refactor: Remove this?
	void DoLabel(const CUIRect *r, const char *text, float size, int align, int max_width = -1);
};


#endif
