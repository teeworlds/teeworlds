#ifndef MODAPI_CLIENT_GUI_BUTTON_H
#define MODAPI_CLIENT_GUI_BUTTON_H

#include "widget.h"

class CModAPI_ClientGui_AbstractButton : public CModAPI_ClientGui_Widget
{
protected:	
	bool m_UnderMouse;
	bool m_Clicked;
	int m_ButtonStyle;
	
protected:
	CModAPI_ClientGui_AbstractButton(class CModAPI_ClientGui_Config *pConfig);
	
	virtual void MouseClickAction() = 0;
	
public:
	virtual void Render();
	
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
	
	void SetButtonStyle(int Style);
};

class CModAPI_ClientGui_IconButton : public CModAPI_ClientGui_AbstractButton
{
protected:
	int m_IconId;
	
protected:
	CModAPI_ClientGui_IconButton(class CModAPI_ClientGui_Config *pConfig, int IconId);
	
public:
	virtual void Render();
	
	void SetIcon(int IconId);
};

class CModAPI_ClientGui_TextButton : public CModAPI_ClientGui_AbstractButton
{
protected:
	char m_aText[128];
	int m_IconId;
	bool m_Centered;
	
protected:
	CModAPI_ClientGui_TextButton(class CModAPI_ClientGui_Config *pConfig, const char* pText = 0, int IconId = -1);
	
	virtual void MouseClickAction() = 0;
	
public:
	virtual void Render();
	
	void SetText(const char* pText);
	void SetIcon(int IconId);
};

class CModAPI_ClientGui_ExternalTextButton : public CModAPI_ClientGui_AbstractButton
{
protected:
	char* m_pText;
	bool m_Centered;
	
protected:
	CModAPI_ClientGui_ExternalTextButton(class CModAPI_ClientGui_Config *pConfig, char* pText);
	
	virtual void MouseClickAction() = 0;
	
public:
	virtual void Render();
};

#endif
