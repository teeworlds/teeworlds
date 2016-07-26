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
	
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnInputEvent();
};

class CModAPI_ClientGui_ExternalTextEdit : public CModAPI_ClientGui_AbstractTextEdit
{
protected:
	char* m_pText;
	
protected:
	virtual char* GetTextPtr();

public:
	CModAPI_ClientGui_ExternalTextEdit(class CModAPI_ClientGui_Config *pConfig, char* pText, int TextMaxChar, int Style = MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL);
};

#endif
