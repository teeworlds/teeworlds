#ifndef MODAPI_CLIENT_GUI_TEXTEDIT_H
#define MODAPI_CLIENT_GUI_TEXTEDIT_H

#include "widget.h"

class CModAPI_ClientGui_AbstractTextEdit : public CModAPI_ClientGui_Widget
{
protected:
	int m_TextStyle;
	int m_TextMaxSize;
	
	int m_CursorCharPos;
	
	bool m_UnderMouse;
	bool m_Clicked;

protected:
	virtual char* GetTextPtr() = 0;

public:
	CModAPI_ClientGui_AbstractTextEdit(class CModAPI_ClientGui_Config *pConfig, int TextMaxChar, int Style = MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL);
	
	virtual void Render();
	
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnInputEvent();
};

#endif
