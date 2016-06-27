#ifndef MODAPI_CLIENT_GUI_TABS_H
#define MODAPI_CLIENT_GUI_TABS_H

#include <base/tl/array.h>

#include "widget.h"

class CModAPI_ClientGui_Tabs : public CModAPI_ClientGui_Widget
{
public:
	struct CTab
	{
		int m_IconId;
		char m_aHint[128];
		CModAPI_ClientGui_Widget* m_pWidget;
		CModAPI_ClientGui_Rect m_Rect;
	};
	
protected:
	CModAPI_ClientGui_Rect m_ContentRect;
	array<CTab> m_Tabs;
	int m_SelectedTab;
	
public:
	CModAPI_ClientGui_Tabs(class CModAPI_ClientGui_Config *pConfig);
	virtual ~CModAPI_ClientGui_Tabs();
	
	virtual void Update();
	virtual void Render();
	
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnButtonRelease(int Button);
	virtual void OnInputEvent();
	
	void AddTab(CModAPI_ClientGui_Widget* pWidget, int IconId, const char* pHint);
	void Clear();
};

#endif
