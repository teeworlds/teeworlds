#ifndef MODAPI_CLIENT_GUI_LABEL_H
#define MODAPI_CLIENT_GUI_LABEL_H

#include "widget.h"

class CModAPI_ClientGui_Label : public CModAPI_ClientGui_Widget
{
protected:
	int m_TextStyle;
	char m_aText[128];
	
	int m_TextWidth;
	int m_TextHeight;
	
public:
	CModAPI_ClientGui_Label(class CModAPI_ClientGui_Config *pConfig, const char* pText, int Style = MODAPI_CLIENTGUI_TEXTSTYLE_NORMAL);
	virtual void Render();
	
	void SetText(const char* pText);
};

#endif
