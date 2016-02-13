#ifndef MODAPI_CLIENT_GUI_POPUP_H
#define MODAPI_CLIENT_GUI_POPUP_H

#include <base/tl/array.h>

#include "widget.h"
#include "layout.h"

class CModAPI_ClientGui_Popup : public CModAPI_ClientGui_Widget
{
protected:
	int m_ChildWidth;
	int m_ChildHeight;
	CModAPI_ClientGui_Widget* m_Child;
	CModAPI_ClientGui_HListLayout* m_Toolbar;

	CModAPI_ClientGui_Popup(class CModAPI_ClientGui_Config *pConfig);

public:
	virtual ~CModAPI_ClientGui_Popup();
	
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseMotion(int RelX, int RelY, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
	virtual void OnInputEvent();
	
	virtual void Add(CModAPI_ClientGui_Widget* pWidget);
	virtual void Clear();
};

#endif
