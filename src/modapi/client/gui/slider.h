#ifndef MODAPI_CLIENT_GUI_SLIDER_H
#define MODAPI_CLIENT_GUI_SLIDER_H

#include <base/tl/array.h>

#include "widget.h"

class CModAPI_ClientGui_AbstractSlider : public CModAPI_ClientGui_Widget
{
protected:
	float m_Pos;

	CModAPI_ClientGui_Rect m_RailRect;
	CModAPI_ClientGui_Rect m_SliderRect;
	bool m_UnderMouse;
	bool m_ButtonDown;
	
	virtual void OnNewPosition(float Pos) = 0;

public:
	CModAPI_ClientGui_AbstractSlider(class CModAPI_ClientGui_Config *pConfig);
	
	virtual void Render();
	
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonRelease(int Button);
	
	virtual void SetSliderSize(int Size) = 0;
	virtual float GetSliderPos();
	virtual void SetSliderPos(float Pos);
};

class CModAPI_ClientGui_HSlider : public CModAPI_ClientGui_AbstractSlider
{
public:
	CModAPI_ClientGui_HSlider(class CModAPI_ClientGui_Config *pConfig);
	
	virtual void Update();
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	
	virtual void SetSliderSize(int Size);
};

class CModAPI_ClientGui_VSlider : public CModAPI_ClientGui_AbstractSlider
{
public:
	CModAPI_ClientGui_VSlider(class CModAPI_ClientGui_Config *pConfig);
	
	virtual void Update();
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
	virtual void OnButtonClick(int X, int Y, int Button);
	
	virtual void SetSliderSize(int Size);
};

#endif
