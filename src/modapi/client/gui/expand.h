#ifndef MODAPI_CLIENT_GUI_EXPAND_H
#define MODAPI_CLIENT_GUI_EXPAND_H

#include <base/tl/array.h>

#include "widget.h"

class CModAPI_ClientGui_Expand : public CModAPI_ClientGui_Widget
{	
protected:
	CModAPI_ClientGui_Widget* m_pTitle;
	array<CModAPI_ClientGui_Widget*> m_Childs;

public:
	CModAPI_ClientGui_Expand(class CModAPI_ClientGui_Config *pConfig);
	virtual ~CModAPI_ClientGui_Expand();
	
	virtual void Clear();
	virtual void Add(CModAPI_ClientGui_Widget* pWidget);
	virtual void SetTitle(CModAPI_ClientGui_Widget* pWidget);
	
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnButtonRelease(int Button);
	virtual void OnInputEvent();
};

#endif
