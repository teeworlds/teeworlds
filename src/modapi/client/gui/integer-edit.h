#ifndef MODAPI_CLIENT_GUI_INTEGEREDIT_H
#define MODAPI_CLIENT_GUI_INTEGEREDIT_H

#include "widget.h"
#include "button.h"

class CModAPI_ClientGui_AbstractIntegerEdit : public CModAPI_ClientGui_Widget
{
	class CIncreaseButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_ClientGui_AbstractIntegerEdit* m_pIntegerEdit;
		
		virtual void MouseClickAction();
		
	public:
		CIncreaseButton(CModAPI_ClientGui_AbstractIntegerEdit *pIntegerEdit);
	};
	
	class CDecreaseButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_ClientGui_AbstractIntegerEdit* m_pIntegerEdit;
		
		virtual void MouseClickAction();
		
	public:
		CDecreaseButton(CModAPI_ClientGui_AbstractIntegerEdit *pIntegerEdit);
	};
	
protected:	
	CModAPI_ClientGui_Rect m_IntegerRect;
	
	CIncreaseButton* m_pIncreaseButton;
	CDecreaseButton* m_pDecreaseButton;
	
	virtual void SetValue(int v) = 0;
	virtual int GetValue() = 0;
	
public:
	CModAPI_ClientGui_AbstractIntegerEdit(class CModAPI_ClientGui_Config *pConfig);
	virtual ~CModAPI_ClientGui_AbstractIntegerEdit();
	virtual void Update();
	virtual void Render();
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
};

class CModAPI_ClientGui_IntegerEdit : public CModAPI_ClientGui_AbstractIntegerEdit
{
protected:
	int m_Value;
	
	virtual void SetValue(int v);
	virtual int GetValue();
	
public:
	CModAPI_ClientGui_IntegerEdit(class CModAPI_ClientGui_Config *pConfig, int DefaultValue);
};

class CModAPI_ClientGui_ExternalIntegerEdit : public CModAPI_ClientGui_AbstractIntegerEdit
{
protected:
	int* m_Memory;
	
	virtual void SetValue(int v);
	virtual int GetValue();
	
public:
	CModAPI_ClientGui_ExternalIntegerEdit(class CModAPI_ClientGui_Config *pConfig, int* m_Memory);
};

#endif
