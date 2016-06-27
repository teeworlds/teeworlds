#ifndef MODAPI_CLIENT_GUI_FLOATEDIT_H
#define MODAPI_CLIENT_GUI_FLOATEDIT_H

#include "widget.h"
#include "button.h"

class CModAPI_ClientGui_AbstractFloatEdit : public CModAPI_ClientGui_Widget
{
protected:
	bool RefreshText;
	float m_LastValue;
	char m_aFloatText[64];
	int m_CursorCharPos;
	bool m_UnderMouse;
	bool m_Clicked;
	
protected:
	void ApplyTextEntry();
	
	virtual void SetValue(float v) = 0;
	virtual float GetValue() = 0;
	virtual void ApplyFormat();
	
public:
	CModAPI_ClientGui_AbstractFloatEdit(class CModAPI_ClientGui_Config *pConfig);
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnInputEvent();
};

class CModAPI_ClientGui_FloatEdit : public CModAPI_ClientGui_AbstractFloatEdit
{
protected:
	float m_Value;
	
	virtual void SetValue(float v);
	virtual float GetValue();
	
public:
	CModAPI_ClientGui_FloatEdit(class CModAPI_ClientGui_Config *pConfig, float DefaultValue);
};

class CModAPI_ClientGui_ExternalFloatEdit : public CModAPI_ClientGui_AbstractFloatEdit
{
protected:
	float* m_Memory;
	
	virtual void SetValue(float v);
	virtual float GetValue();
	
public:
	CModAPI_ClientGui_ExternalFloatEdit(class CModAPI_ClientGui_Config *pConfig, float* m_Memory);
};

#endif
