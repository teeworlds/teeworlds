// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_CLIENT_INPUT_H
#define ENGINE_CLIENT_INPUT_H

class CInput : public IEngineInput
{
	IEngineGraphics *m_pGraphics;

	int m_InputGrabbed;

	int64 m_LastRelease;
	int64 m_ReleaseDelta;

	void AddEvent(int Unicode, int Key, int Flags);

	IEngineGraphics *Graphics() { return m_pGraphics; }

public:
	CInput();

	virtual void Init();

	virtual void MouseRelative(int *x, int *y);
	virtual void MouseModeAbsolute();
	virtual void MouseModeRelative();
	virtual int MouseDoubleClick();

	void ClearKeyStates();
	int KeyState(int Key);

	int ButtonPressed(int Button) { return m_aInputState[m_InputCurrent][Button]; }

	virtual void Update();
};

#endif
