/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "kernel.h"

const int g_MaxKeys = 512;
extern const char g_aaKeyStrings[g_MaxKeys][20];
const int g_MaxJoystickAxes = 12;

class IInput : public IInterface
{
	MACRO_INTERFACE("input", 0)
public:
	class CEvent
	{
	public:
		int m_Flags;
		int m_Key;
		char m_aText[32];
		int m_InputCount;
	};

protected:
	enum
	{
		INPUT_BUFFER_SIZE=32
	};

	// quick access to events
	int m_NumEvents;
	IInput::CEvent m_aInputEvents[INPUT_BUFFER_SIZE];

public:
	enum
	{
		FLAG_PRESS=1,
		FLAG_RELEASE=2,
		FLAG_REPEAT=4,
		FLAG_TEXT=8,

		CURSOR_NONE = 0,
		CURSOR_MOUSE,
		CURSOR_JOYSTICK
	};

	// events
	int NumEvents() const { return m_NumEvents; }
	virtual bool IsEventValid(CEvent *pEvent) const = 0;
	CEvent GetEvent(int Index) const
	{
		if(Index < 0 || Index >= m_NumEvents)
		{
			IInput::CEvent e = {0,0};
			return e;
		}
		return m_aInputEvents[Index];
	}
	virtual void Clear() = 0;

	// keys
	virtual bool KeyIsPressed(int Key) const = 0;
	virtual bool KeyPress(int Key, bool CheckCounter=false) const = 0;
	const char *KeyName(int Key) const { return (Key >= 0 && Key < g_MaxKeys) ? g_aaKeyStrings[Key] : g_aaKeyStrings[0]; }

	// joystick
	virtual int NumJoysticks() const = 0;
	virtual int GetJoystickIndex() const = 0;
	virtual void SelectNextJoystick() = 0;
	virtual const char* GetJoystickName() = 0;
	virtual int GetJoystickNumAxes() = 0;
	virtual float GetJoystickAxisValue(int Axis) = 0;
	virtual bool JoystickRelative(float *pX, float *pY) = 0;
	virtual bool JoystickAbsolute(float *pX, float *pY) = 0;

	// mouse
	virtual void MouseModeRelative() = 0;
	virtual void MouseModeAbsolute() = 0;
	virtual int MouseDoubleClick() = 0;
	virtual bool MouseRelative(float *pX, float *pY) = 0;

	// clipboard
	virtual const char* GetClipboardText() = 0;
	virtual void SetClipboardText(const char *pText) = 0;

	int CursorRelative(float *pX, float *pY)
	{
		if(MouseRelative(pX, pY))
			return CURSOR_MOUSE;
		if(JoystickRelative(pX, pY))
			return CURSOR_JOYSTICK;
		return CURSOR_NONE;
	}
};


class IEngineInput : public IInput
{
	MACRO_INTERFACE("engineinput", 0)
public:
	virtual void Init() = 0;
	virtual int Update() = 0;
};

extern IEngineInput *CreateEngineInput();

#endif
