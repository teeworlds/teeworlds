/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "kernel.h"

extern const char g_aaKeyStrings[512][20];

class IInput : public IInterface
{
	MACRO_INTERFACE("input", 0)
public:
	class CEvent
	{
	public:
		int m_Flags;
		int m_Unicode;
		int m_Key;
	};

protected:
	enum
	{
		INPUT_BUFFER_SIZE=32
	};

	// quick access to events
	int m_NumEvents;
	IInput::CEvent m_aInputEvents[INPUT_BUFFER_SIZE];

	//quick access to input
	struct
	{
		unsigned char m_Presses;
		unsigned char m_Releases;
	} m_aInputCount[2][1024];

	unsigned char m_aInputState[2][1024];
	int m_InputCurrent;
	bool m_InputDispatched;

	int KeyWasPressed(int Key) { return m_aInputState[m_InputCurrent^1][Key]; }

public:
	enum
	{
		FLAG_PRESS=1,
		FLAG_RELEASE=2,
		FLAG_REPEAT=4
	};

	// events
	int NumEvents() const { return m_NumEvents; }
	void ClearEvents() 
	{ 
		m_NumEvents = 0;
		m_InputDispatched = true;
	}
	CEvent GetEvent(int Index) const
	{
		if(Index < 0 || Index >= m_NumEvents)
		{
			IInput::CEvent e = {0,0};
			return e;
		}
		return m_aInputEvents[Index];
	}

	// keys
	int KeyPressed(int Key) { return m_aInputState[m_InputCurrent][Key]; }
	int KeyReleases(int Key) { return m_aInputCount[m_InputCurrent][Key].m_Releases; }
	int KeyPresses(int Key) { return m_aInputCount[m_InputCurrent][Key].m_Presses; }
	int KeyDown(int Key) { return KeyPressed(Key)&&!KeyWasPressed(Key); }
	const char *KeyName(int Key) { return (Key >= 0 && Key < 512) ? g_aaKeyStrings[Key] : g_aaKeyStrings[0]; }

	enum MouseMode {
		MOUSE_MODE_RELATIVE,
		MOUSE_MODE_ABSOLUTE,
	};

	//
	virtual void ShowCursor(bool show) = 0;
	virtual bool GetShowCursor() = 0;
	virtual void SetMouseMode(MouseMode mode) = 0;
	virtual MouseMode GetMouseMode() = 0;
	virtual void GetMousePosition(float *x, float *y) = 0;
	virtual bool MouseMoved() = 0;
	virtual int MouseDoubleClick() = 0;
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
