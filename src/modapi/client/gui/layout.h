#ifndef MODAPI_CLIENT_GUI_LAYOUT_H
#define MODAPI_CLIENT_GUI_LAYOUT_H

#include <base/tl/array.h>

#include "widget.h"
#include "slider.h"

enum
{
	MODAPI_CLIENTGUI_LAYOUTSTYLE_NONE=0,
	MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME,
};

enum
{
	MODAPI_CLIENTGUI_LAYOUTFILLING_NONE=0,
	MODAPI_CLIENTGUI_LAYOUTFILLING_FIRST,
	MODAPI_CLIENTGUI_LAYOUTFILLING_LAST,
	MODAPI_CLIENTGUI_LAYOUTFILLING_ALL,
};

class CModAPI_ClientGui_HListLayout : public CModAPI_ClientGui_Widget
{
protected:
	int m_Style;
	int m_Model;
	
	array<CModAPI_ClientGui_Widget*> m_Childs;

protected:
	void Update_FillingNone();
	void Update_FillingFirst();
	void Update_FillingLast();
	void Update_FillingAll();

public:
	CModAPI_ClientGui_HListLayout(class CModAPI_ClientGui_Config *pConfig, int Style = MODAPI_CLIENTGUI_LAYOUTSTYLE_FRAME, int Model = MODAPI_CLIENTGUI_LAYOUTFILLING_NONE);
	virtual ~CModAPI_ClientGui_HListLayout();
	
	virtual void Clear();
	virtual void Add(CModAPI_ClientGui_Widget* pWidget);
	
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseMotion(int RelX, int RelY, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
	virtual void OnInputEvent();
};

class CModAPI_ClientGui_VListLayout : public CModAPI_ClientGui_Widget
{
protected:
	class CSlider : public CModAPI_ClientGui_VSlider
	{
	protected:
		CModAPI_ClientGui_VListLayout* m_pLayout;
		
	protected:
		virtual void OnNewPosition(float Pos);
		
	public:
		CSlider(CModAPI_ClientGui_VListLayout* pLayout);
	};
	
	class CSeparator : public CModAPI_ClientGui_Widget
	{
	public:
		CSeparator(CModAPI_ClientGui_Config* pConfig);
		virtual void Render();
	};
	
protected:	
	CModAPI_ClientGui_VSlider* m_pSlider;
	array<CModAPI_ClientGui_Widget*> m_Childs;
	int m_ChildrenHeight;
	
	bool m_ShowScrollBar;
	int m_ScrollValue;
	
public:
	CModAPI_ClientGui_VListLayout(class CModAPI_ClientGui_Config *pConfig);
	virtual ~CModAPI_ClientGui_VListLayout();
	
	virtual void Clear();
	virtual void Add(CModAPI_ClientGui_Widget* pWidget);
	virtual void AddSeparator();
	
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseMotion(int RelX, int RelY, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
	virtual void OnInputEvent();
	
	virtual void SetScroll(float Pos);
};

#endif
