/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_INPUT_H
#define ENGINE_CLIENT_INPUT_H

class CInput : public IEngineInput
{
	IEngineGraphics *m_pGraphics;

	int m_InputGrabbed;

	int64 m_LastRelease;
	int64 m_ReleaseDelta;

	void AddEvent(char *pText, int Key, int Flags);
	void ClearEvents()
	{
		IInput::ClearEvents();
		m_InputDispatched = true;
	}

	//quick access to input
	struct
	{
		unsigned char m_Presses;
		unsigned char m_Releases;
	} m_aInputCount[2][g_MaxKeys];	// tw-KEY

	unsigned char m_aInputState[2][g_MaxKeys];	// SDL_SCANCODE
	int m_InputCurrent;
	bool m_InputDispatched;

	void ClearKeyStates();
	bool KeyState(int Key) const;

	IEngineGraphics *Graphics() { return m_pGraphics; }

public:
	CInput();

	virtual void Init();

	bool KeyIsPressed(int Key) const { return KeyState(Key); }
	bool KeyRelease(int Key) const { return m_aInputCount[m_InputCurrent][Key].m_Releases; }
	bool KeyPress(int Key) const { return m_aInputCount[m_InputCurrent][Key].m_Presses; }

	virtual void MouseRelative(float *x, float *y);
	virtual void MouseModeAbsolute();
	virtual void MouseModeRelative();
	virtual int MouseDoubleClick();

	virtual int Update();
};

#endif
