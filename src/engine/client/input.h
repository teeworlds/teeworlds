/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_INPUT_H
#define ENGINE_CLIENT_INPUT_H

#include <base/tl/sorted_array.h>

class CInput : public IEngineInput
{
public:
	class CJoystick : public IJoystick
	{
		CInput *m_pInput;
		int m_Index;
		char m_aName[64];
		char m_aGUID[34];
		int m_NumAxes;
		int m_NumButtons;
		int m_NumBalls;
		int m_NumHats;
		SDL_Joystick *m_pDelegate;

		CInput *Input() { return m_pInput; }
	public:
		CJoystick() { /* empty constructor for sorted_array */ }
		CJoystick(CInput *pInput, int Index, SDL_Joystick *pDelegate);
		int GetIndex() const { return m_Index; }
		const char *GetName() const { return m_aName; }
		const char *GetGUID() const { return m_aGUID; }
		int GetNumAxes() const { return m_NumAxes; }
		int GetNumButtons() const { return m_NumButtons; }
		int GetNumBalls() const { return m_NumBalls; }
		int GetNumHats() const { return m_NumHats; }
		float GetAxisValue(int Axis);
		bool Relative(float *pX, float *pY);
		bool Absolute(float *pX, float *pY);
		void Close();
	};
private:
	IEngineGraphics *m_pGraphics;
	CConfig *m_pConfig;
	IConsole *m_pConsole;

	IEngineGraphics *Graphics() { return m_pGraphics; }
	CConfig *Config() { return m_pConfig; }
	IConsole *Console() { return m_pConsole; }

	// joystick
	array<CJoystick> m_aJoysticks;
	CJoystick *m_pActiveJoystick;
	void InitJoysticks();
	void CloseJoysticks();
	void UpdateActiveJoystick();
	static void ConchainJoystickGuidChanged(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	float GetJoystickDeadzone();

	bool m_MouseInputRelative;
	char *m_pClipboardText;

	int m_PreviousHat;

	bool m_MouseDoubleClick;

	void AddEvent(char *pText, int Key, int Flags);
	void Clear();
	bool IsEventValid(CEvent *pEvent) const { return pEvent->m_InputCount == m_InputCounter; }

	//quick access to input
	unsigned short m_aInputCount[g_MaxKeys];	// tw-KEY
	unsigned char m_aInputState[g_MaxKeys];	// SDL_SCANCODE
	int m_InputCounter;
	void UpdateMouseState();
	void UpdateJoystickState();

	void ClearKeyStates();
	bool KeyState(int Key) const;
public:
	CInput();
	~CInput();

	void Init();
	int Update();

	bool KeyIsPressed(int Key) const { return KeyState(Key); }
	bool KeyPress(int Key, bool CheckCounter) const { return CheckCounter ? (m_aInputCount[Key] == m_InputCounter) : m_aInputCount[Key]; }

	int NumJoysticks() const { return m_aJoysticks.size(); }
	IJoystick *GetActiveJoystick() { return m_pActiveJoystick; }
	void SelectNextJoystick();

	void MouseModeRelative();
	void MouseModeAbsolute();
	bool MouseDoubleClick();
	bool MouseRelative(float *pX, float *pY);

	const char *GetClipboardText();
	void SetClipboardText(const char *pText);
};

#endif
