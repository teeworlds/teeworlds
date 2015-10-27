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

	void AddEvent(int Unicode, int Key, int Flags);
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
	int KeyState(int Key) const;
	int KeyStateOld(int Key) const;
	int KeyWasPressed(int Key) const { return KeyStateOld(Key); }

	IEngineGraphics *Graphics() { return m_pGraphics; }

public:
	CInput();

	virtual void Init();

	int KeyPressed(int Key) const { return KeyState(Key); }
	int KeyReleases(int Key) const { return m_aInputCount[m_InputCurrent][Key].m_Releases; }
	int KeyPresses(int Key) const { return m_aInputCount[m_InputCurrent][Key].m_Presses; }
	int KeyDown(int Key) const { return KeyPressed(Key)&&!KeyWasPressed(Key); }

	virtual void MouseRelative(float *x, float *y);
	virtual void MouseModeAbsolute();
	virtual void MouseModeRelative();
	virtual int MouseDoubleClick();

	virtual int Update();
};

#endif
