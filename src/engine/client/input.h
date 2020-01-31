/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_INPUT_H
#define ENGINE_CLIENT_INPUT_H

#include <base/tl/sorted_array.h>

class CInput : public IEngineInput
{
	IEngineGraphics *m_pGraphics;
	IConfig *m_pConfig;
	IConsole *m_pConsole;

	sorted_array<SDL_Joystick*> m_apJoysticks;
	int m_SelectedJoystickIndex;
	char m_aSelectedJoystickGUID[34];
	SDL_Joystick* GetActiveJoystick();
	void InitJoysticks();
	void CloseJoysticks();

	int m_InputGrabbed;
	char *m_pClipboardText;

	int m_PreviousHat;

	bool m_MouseDoubleClick;

	void AddEvent(char *pText, int Key, int Flags);
	void Clear();
	bool IsEventValid(CEvent *pEvent) const { return pEvent->m_InputCount == m_InputCounter; };

	//quick access to input
	unsigned short m_aInputCount[g_MaxKeys];	// tw-KEY
	unsigned char m_aInputState[g_MaxKeys];	// SDL_SCANCODE
	int m_InputCounter;

	void ClearKeyStates();
	bool KeyState(int Key) const;

	IEngineGraphics *Graphics() { return m_pGraphics; }

public:
	CInput();
	~CInput();

	virtual void Init();

	bool KeyIsPressed(int Key) const { return KeyState(Key); }
	bool KeyPress(int Key, bool CheckCounter) const { return CheckCounter ? (m_aInputCount[Key] == m_InputCounter) : m_aInputCount[Key]; }

	int NumJoysticks() const { return m_apJoysticks.size(); }
	int GetJoystickIndex() const { return m_SelectedJoystickIndex; };
	void SelectNextJoystick();
	const char* GetJoystickName();
	int GetJoystickNumAxes();
	float GetJoystickAxisValue(int Axis);

	virtual void MouseRelative(float *x, float *y);
	virtual void MouseModeAbsolute();
	virtual void MouseModeRelative();
	virtual int MouseDoubleClick();
	virtual const char *GetClipboardText();
	virtual void SetClipboardText(const char *pText);

	virtual int Update();
};

#endif
