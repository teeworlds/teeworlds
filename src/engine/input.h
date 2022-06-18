/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "kernel.h"

const int g_MaxKeys = 512;
extern const char g_aaKeyStrings[g_MaxKeys][20];

class IInput : public IInterface
{
	MACRO_INTERFACE("input", 0)
public:
	class CEvent
	{
	public:
		int m_Flags;
		int m_Key;
		char m_aText[32*UTF8_BYTE_LENGTH+1];
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
		FLAG_TEXTEDIT=16,

		CURSOR_NONE = 0,
		CURSOR_MOUSE,
		CURSOR_JOYSTICK,

		MAX_CANDIDATES = 16,
		MAX_CANDIDATE_LENGTH = 16,
		MAX_CANDIDATE_ARRAY_SIZE=MAX_CANDIDATE_LENGTH*UTF8_BYTE_LENGTH+1,
		MAX_COMPOSITION_ARRAY_SIZE = 32, // SDL2 limitation

		COMP_LENGTH_INACTIVE = -1
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
	class IJoystick
	{
	public:
		virtual int GetIndex() const = 0;
		virtual const char *GetName() const = 0;
		virtual int GetNumAxes() const = 0;
		virtual int GetNumButtons() const = 0;
		virtual int GetNumBalls() const = 0;
		virtual int GetNumHats() const = 0;
		virtual float GetAxisValue(int Axis) = 0;
		virtual int GetHatValue(int Hat) = 0;
		virtual bool Relative(float *pX, float *pY) = 0;
		virtual bool Absolute(float *pX, float *pY) = 0;
	};
	virtual int NumJoysticks() const = 0;
	virtual IJoystick *GetActiveJoystick() = 0;
	virtual void SelectNextJoystick() = 0;

	// mouse
	virtual void MouseModeRelative() = 0;
	virtual void MouseModeAbsolute() = 0;
	virtual bool MouseDoubleClick() = 0;
	virtual bool MouseRelative(float *pX, float *pY) = 0;

	// clipboard
	virtual const char *GetClipboardText() = 0;
	virtual void SetClipboardText(const char *pText) = 0;

	// text editing
	virtual void StartTextInput() = 0;
	virtual void StopTextInput() = 0;
	virtual const char *GetComposition() const = 0;
	virtual bool HasComposition() const = 0;
	virtual int GetCompositionCursor() const = 0;
	virtual int GetCompositionSelectedLength() const = 0;
	virtual int GetCompositionLength() const = 0;
	virtual const char *GetCandidate(int Index) const = 0;
	virtual int GetCandidateCount() const = 0;
	virtual int GetCandidateSelectedIndex() const = 0;
	virtual void SetCompositionWindowPosition(float X, float Y, float H) = 0;

	int CursorRelative(float *pX, float *pY)
	{
		if(MouseRelative(pX, pY))
			return CURSOR_MOUSE;
		IJoystick *pJoystick = GetActiveJoystick();
		if(pJoystick && pJoystick->Relative(pX, pY))
			return CURSOR_JOYSTICK;
		return CURSOR_NONE;
	}
};


class IEngineInput : public IInput
{
	MACRO_INTERFACE("engineinput", 0)
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual int Update() = 0;
};

extern IEngineInput *CreateEngineInput();

#endif
