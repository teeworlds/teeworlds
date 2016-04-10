#ifndef MODAPI_CLIENT_GUI_POPUP_H
#define MODAPI_CLIENT_GUI_POPUP_H

#include <base/tl/array.h>

#include "widget.h"
#include "layout.h"
#include "button.h"

class CModAPI_ClientGui_Popup : public CModAPI_ClientGui_Widget
{
public:
	enum
	{
		ALIGNMENT_LEFT,
		ALIGNMENT_RIGHT,
	};
	
protected:
	int m_ChildWidth;
	int m_ChildHeight;
	CModAPI_ClientGui_Widget* m_Child;
	bool m_IsClosed;

	CModAPI_ClientGui_Popup(class CModAPI_ClientGui_Config *pConfig, const CModAPI_ClientGui_Rect& CreatorRect, int Width, int Height, int Alignment);

public:
	virtual ~CModAPI_ClientGui_Popup();
	
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnButtonRelease(int Button);
	virtual void OnInputEvent();
	
	virtual void Add(CModAPI_ClientGui_Widget* pWidget);
	virtual void Clear();
	
	virtual void Close();
	virtual bool IsClosed();
};

#endif
