/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "SDL.h"

#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>

#include "input.h"


// this header is protected so you don't include it from anywere
#define KEYS_INCLUDE
#include "keynames.h"
#undef KEYS_INCLUDE

// support older SDL version (pre 2.0.6)
#ifndef SDL_JOYSTICK_AXIS_MIN
	#define SDL_JOYSTICK_AXIS_MIN -32768
#endif
#ifndef SDL_JOYSTICK_AXIS_MAX
	#define SDL_JOYSTICK_AXIS_MAX 32767
#endif

#if defined(CONF_FAMILY_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#endif

// for platform specific features that aren't available or are broken in SDL
#include "SDL_syswm.h"

void CInput::AddEvent(char *pText, int Key, int Flags)
{
	if(m_NumEvents != INPUT_BUFFER_SIZE)
	{
		m_aInputEvents[m_NumEvents].m_Key = Key;
		m_aInputEvents[m_NumEvents].m_Flags = Flags;
		if(!pText)
			m_aInputEvents[m_NumEvents].m_aText[0] = 0;
		else
			str_copy(m_aInputEvents[m_NumEvents].m_aText, pText, sizeof(m_aInputEvents[m_NumEvents].m_aText));
		m_aInputEvents[m_NumEvents].m_InputCount = m_InputCounter;
		m_NumEvents++;
	}
}

CInput::CInput()
{
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
	mem_zero(m_aInputState, sizeof(m_aInputState));

	m_pConfig = 0;
	m_pConsole = 0;
	m_pGraphics = 0;

	m_InputCounter = 1;
	m_MouseInputRelative = false;
	m_pClipboardText = 0;

	m_pActiveJoystick = 0x0;

	m_MouseDoubleClick = false;

	m_NumEvents = 0;

	m_CompositionLength = COMP_LENGTH_INACTIVE;
	m_CompositionCursor = 0;
	m_CompositionSelectedLength = 0;
	m_CandidateCount = 0;
	m_CandidateSelectedIndex = -1;
}

void CInput::Init()
{
	StopTextInput();

	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pConfig = Kernel()->RequestInterface<IConfigManager>()->Values();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	MouseModeRelative();

	InitJoysticks();
}

void CInput::Shutdown()
{
	SDL_free(m_pClipboardText);
	CloseJoysticks();
}

void CInput::InitJoysticks()
{
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			dbg_msg("joystick", "Unable to init SDL joystick system: %s", SDL_GetError());
			return;
		}
	}

	int NumJoysticks = SDL_NumJoysticks();
	if(NumJoysticks > 0)
	{
		dbg_msg("joystick", "%d joystick(s) found", NumJoysticks);
		int ActualIndex = 0;
		for(int i = 0; i < NumJoysticks; i++)
		{
			SDL_Joystick *pJoystick = SDL_JoystickOpen(i);
			if(!pJoystick)
			{
				dbg_msg("joystick", "Could not open joystick %d: '%s'", i, SDL_GetError());
				continue;
			}
			CJoystick Joystick(this, ActualIndex, pJoystick);
			m_aJoysticks.add(Joystick);
			dbg_msg("joystick", "Opened Joystick %d '%s' (%d axes, %d buttons, %d balls, %d hats)", i, Joystick.GetName(),
					Joystick.GetNumAxes(), Joystick.GetNumButtons(), Joystick.GetNumBalls(), Joystick.GetNumHats());
			ActualIndex++;
		}
		if(ActualIndex > 0)
		{
			UpdateActiveJoystick();
			Console()->Chain("joystick_guid", ConchainJoystickGuidChanged, this);
		}
	}
	else
	{
		dbg_msg("joystick", "No joysticks found");
	}
}

void CInput::UpdateActiveJoystick()
{
	if(m_aJoysticks.size() == 0)
		return;
	m_pActiveJoystick = 0x0;
	for(array<CJoystick>::range r = m_aJoysticks.all(); !r.empty(); r.pop_front())
	{
		CJoystick &Joystick = r.front();
		if(str_comp(Joystick.GetGUID(), Config()->m_JoystickGUID) == 0)
		{
			m_pActiveJoystick = &Joystick;
			return;
		}
	}
	// Fall back to first joystick if no match was found
	if(!m_pActiveJoystick)
		m_pActiveJoystick = &m_aJoysticks[0];
}

void CInput::ConchainJoystickGuidChanged(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	static_cast<CInput *>(pUserData)->UpdateActiveJoystick();
}

float CInput::GetJoystickDeadzone()
{
	return Config()->m_JoystickTolerance/50.0f;
}

CInput::CJoystick::CJoystick(CInput *pInput, int Index, SDL_Joystick *pDelegate)
{
	m_pInput = pInput;
	m_Index = Index;
	m_pDelegate = pDelegate;
	m_NumAxes = SDL_JoystickNumAxes(pDelegate);
	m_NumButtons = SDL_JoystickNumButtons(pDelegate);
	m_NumBalls = SDL_JoystickNumBalls(pDelegate);
	m_NumHats = SDL_JoystickNumHats(pDelegate);
	str_copy(m_aName, SDL_JoystickName(pDelegate), sizeof(m_aName));
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(pDelegate), m_aGUID, sizeof(m_aGUID));
	m_InstanceID = SDL_JoystickInstanceID(pDelegate);
}

void CInput::CloseJoysticks()
{
	if(SDL_WasInit(SDL_INIT_JOYSTICK))
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	m_aJoysticks.clear();
	m_pActiveJoystick = 0x0;
}

void CInput::SelectNextJoystick()
{
	const int Num = m_aJoysticks.size();
	if(Num > 1)
	{
		m_pActiveJoystick = &m_aJoysticks[(m_pActiveJoystick->GetIndex() + 1) % Num];
		str_copy(Config()->m_JoystickGUID, m_pActiveJoystick->GetGUID(), sizeof(Config()->m_JoystickGUID));
	}
}

float CInput::CJoystick::GetAxisValue(int Axis)
{
	return (SDL_JoystickGetAxis(m_pDelegate, Axis) - SDL_JOYSTICK_AXIS_MIN) / float(SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN) * 2.0f - 1.0f;
}

int CInput::CJoystick::GetJoystickHatKey(int Hat, int HatValue)
{
	switch(HatValue)
	{
		case SDL_HAT_LEFTUP: return KEY_JOY_HAT0_LEFTUP + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_UP: return KEY_JOY_HAT0_UP + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_RIGHTUP: return KEY_JOY_HAT0_RIGHTUP + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_LEFT: return KEY_JOY_HAT0_LEFT + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_RIGHT: return KEY_JOY_HAT0_RIGHT + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_LEFTDOWN: return KEY_JOY_HAT0_LEFTDOWN + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_DOWN: return KEY_JOY_HAT0_DOWN + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
		case SDL_HAT_RIGHTDOWN: return KEY_JOY_HAT0_RIGHTDOWN + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT;
	}
	return -1;
}

int CInput::CJoystick::GetHatValue(int Hat)
{
	return GetJoystickHatKey(Hat, SDL_JoystickGetHat(m_pDelegate, Hat));
}

bool CInput::CJoystick::Relative(float *pX, float *pY)
{
	if(!Input()->Config()->m_JoystickEnable)
		return false;

	const vec2 RawJoystickPos = vec2(GetAxisValue(Input()->Config()->m_JoystickX), GetAxisValue(Input()->Config()->m_JoystickY));
	const float Len = length(RawJoystickPos);
	const float DeadZone = Input()->GetJoystickDeadzone();
	if(Len > DeadZone)
	{
		const float Factor = 0.1f * maximum((Len - DeadZone) / (1 - DeadZone), 0.001f) / Len;
		*pX = RawJoystickPos.x * Factor;
		*pY = RawJoystickPos.y * Factor;
		return true;
	}
	return false;
}

bool CInput::CJoystick::Absolute(float *pX, float *pY)
{
	if(!Input()->m_MouseInputRelative || !Input()->Config()->m_JoystickEnable)
		return false;

	const vec2 RawJoystickPos = vec2(GetAxisValue(Input()->Config()->m_JoystickX), GetAxisValue(Input()->Config()->m_JoystickY));
	const float DeadZone = Input()->GetJoystickDeadzone();
	if(dot(RawJoystickPos, RawJoystickPos) > DeadZone*DeadZone)
	{
		*pX = RawJoystickPos.x;
		*pY = RawJoystickPos.y;
		return true;
	}
	return false;
}

bool CInput::MouseRelative(float *pX, float *pY)
{
	if(!m_MouseInputRelative)
		return false;

	int MouseX = 0, MouseY = 0;
	SDL_GetRelativeMouseState(&MouseX, &MouseY);
	if(MouseX || MouseY)
	{
		*pX = MouseX;
		*pY = MouseY;
		return true;
	}
	return false;
}

void CInput::MouseModeAbsolute()
{
	if(m_MouseInputRelative)
	{
		m_MouseInputRelative = false;
		SDL_ShowCursor(SDL_ENABLE);
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void CInput::MouseModeRelative()
{
	if(!m_MouseInputRelative)
	{
		m_MouseInputRelative = true;
		SDL_ShowCursor(SDL_DISABLE);
		if(SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, Config()->m_InpGrab ? "0" : "1", SDL_HINT_OVERRIDE) == SDL_FALSE)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "input", "unable to switch relative mouse mode");
		}
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_GetRelativeMouseState(NULL, NULL);
	}
}

bool CInput::MouseDoubleClick()
{
	if(m_MouseDoubleClick)
	{
		m_MouseDoubleClick = false;
		return true;
	}
	return false;
}

const char *CInput::GetClipboardText()
{
	if(m_pClipboardText)
		SDL_free(m_pClipboardText);
	m_pClipboardText = SDL_GetClipboardText();
	if(m_pClipboardText)
		str_sanitize_cc(m_pClipboardText);
	return m_pClipboardText;
}

void CInput::SetClipboardText(const char *pText)
{
	SDL_SetClipboardText(pText);
}

void CInput::StartTextInput()
{
	// enable system messages for ime
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	SDL_StartTextInput();
}

void CInput::StopTextInput()
{
	SDL_StopTextInput();
	// disable system messages for performance
	SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
	m_CompositionLength = COMP_LENGTH_INACTIVE;
	m_CompositionCursor = 0;
	m_aComposition[0] = 0;
	m_CompositionSelectedLength = 0;
	m_CandidateCount = 0;
}

void CInput::Clear()
{
	mem_zero(m_aInputState, sizeof(m_aInputState));
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
	m_NumEvents = 0;
}

bool CInput::KeyState(int Key) const
{
	return Key >= KEY_FIRST
		&& Key < KEY_LAST
		&& m_aInputState[Key>=KEY_MOUSE_1 ? Key : SDL_GetScancodeFromKey(KeyToKeycode(Key))];
}

void CInput::UpdateMouseState()
{
	int MouseState = SDL_GetMouseState(NULL, NULL);
	if(MouseState&SDL_BUTTON(SDL_BUTTON_LEFT)) m_aInputState[KEY_MOUSE_1] = true;
	if(MouseState&SDL_BUTTON(SDL_BUTTON_RIGHT)) m_aInputState[KEY_MOUSE_2] = true;
	if(MouseState&SDL_BUTTON(SDL_BUTTON_MIDDLE)) m_aInputState[KEY_MOUSE_3] = true;
	if(MouseState&SDL_BUTTON(SDL_BUTTON_X1)) m_aInputState[KEY_MOUSE_4] = true;
	if(MouseState&SDL_BUTTON(SDL_BUTTON_X2)) m_aInputState[KEY_MOUSE_5] = true;
	if(MouseState&SDL_BUTTON(6)) m_aInputState[KEY_MOUSE_6] = true;
	if(MouseState&SDL_BUTTON(7)) m_aInputState[KEY_MOUSE_7] = true;
	if(MouseState&SDL_BUTTON(8)) m_aInputState[KEY_MOUSE_8] = true;
	if(MouseState&SDL_BUTTON(9)) m_aInputState[KEY_MOUSE_9] = true;
}

void CInput::UpdateJoystickState()
{
	if(!Config()->m_JoystickEnable)
		return;
	IJoystick *pJoystick = GetActiveJoystick();
	if(!pJoystick)
		return;

	const float DeadZone = GetJoystickDeadzone();
	for(int Axis = 0; Axis < pJoystick->GetNumAxes(); Axis++)
	{
		const float Value = pJoystick->GetAxisValue(Axis);
		const int LeftKey = KEY_JOY_AXIS_0_LEFT + 2 * Axis;
		const int RightKey = LeftKey + 1;
		m_aInputState[LeftKey] = Value <= -DeadZone;
		m_aInputState[RightKey] = Value >= DeadZone;
	}

	for(int Hat = 0; Hat < pJoystick->GetNumHats(); Hat++)
	{
		const int HatState = pJoystick->GetHatValue(Hat);
		for(int Key = KEY_JOY_HAT0_LEFTUP + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT; Key <= KEY_JOY_HAT0_RIGHTDOWN + Hat * NUM_JOYSTICK_BUTTONS_PER_HAT; Key++)
			m_aInputState[Key] = Key == HatState;
	}
}

void CInput::HandleJoystickAxisMotionEvent(const SDL_Event &Event)
{
	if(!Config()->m_JoystickEnable)
		return;
	CJoystick *pJoystick = GetActiveJoystick();
	if(!pJoystick || pJoystick->GetInstanceID() != Event.jaxis.which)
		return;
	if(Event.jaxis.axis >= NUM_JOYSTICK_AXES)
		return;

	const int LeftKey = KEY_JOY_AXIS_0_LEFT + 2 * Event.jaxis.axis;
	const int RightKey = LeftKey + 1;
	const float DeadZone = GetJoystickDeadzone();

	if(Event.jaxis.value <= SDL_JOYSTICK_AXIS_MIN * DeadZone && !m_aInputState[LeftKey])
	{
		m_aInputState[LeftKey] = true;
		m_aInputCount[LeftKey] = m_InputCounter;
		AddEvent(0, LeftKey, IInput::FLAG_PRESS);
	}
	else if(Event.jaxis.value > SDL_JOYSTICK_AXIS_MIN * DeadZone && m_aInputState[LeftKey])
	{
		m_aInputState[LeftKey] = false;
		AddEvent(0, LeftKey, IInput::FLAG_RELEASE);
	}

	if(Event.jaxis.value >= SDL_JOYSTICK_AXIS_MAX * DeadZone && !m_aInputState[RightKey])
	{
		m_aInputState[RightKey] = true;
		m_aInputCount[RightKey] = m_InputCounter;
		AddEvent(0, RightKey, IInput::FLAG_PRESS);
	}
	else if(Event.jaxis.value < SDL_JOYSTICK_AXIS_MAX * DeadZone && m_aInputState[RightKey])
	{
		m_aInputState[RightKey] = false;
		AddEvent(0, RightKey, IInput::FLAG_RELEASE);
	}
}

void CInput::HandleJoystickButtonEvent(const SDL_Event &Event)
{
	if(!Config()->m_JoystickEnable)
		return;
	CJoystick *pJoystick = GetActiveJoystick();
	if(!pJoystick || pJoystick->GetInstanceID() != Event.jbutton.which)
		return;
	if(Event.jbutton.button >= NUM_JOYSTICK_BUTTONS)
		return;

	const int Key = Event.jbutton.button + KEY_JOYSTICK_BUTTON_0;

	if(Event.type == SDL_JOYBUTTONDOWN)
	{
		m_aInputState[Key] = true;
		m_aInputCount[Key] = m_InputCounter;
		AddEvent(0, Key, IInput::FLAG_PRESS);
	}
	else if(Event.type == SDL_JOYBUTTONUP)
	{
		m_aInputState[Key] = false;
		AddEvent(0, Key, IInput::FLAG_RELEASE);
	}
}

void CInput::HandleJoystickHatMotionEvent(const SDL_Event &Event)
{
	if(!Config()->m_JoystickEnable)
		return;
	CJoystick *pJoystick = GetActiveJoystick();
	if(!pJoystick || pJoystick->GetInstanceID() != Event.jhat.which)
		return;
	if(Event.jhat.hat >= NUM_JOYSTICK_HATS)
		return;

	const int CurrentKey = CJoystick::GetJoystickHatKey(Event.jhat.hat, Event.jhat.value);

	for(int Key = KEY_JOY_HAT0_LEFTUP + Event.jhat.hat * NUM_JOYSTICK_BUTTONS_PER_HAT; Key <= KEY_JOY_HAT0_RIGHTDOWN + Event.jhat.hat * NUM_JOYSTICK_BUTTONS_PER_HAT; Key++)
	{
		if(Key != CurrentKey && m_aInputState[Key])
		{
			m_aInputState[Key] = false;
			AddEvent(0, Key, IInput::FLAG_RELEASE);
		}
	}

	if(CurrentKey >= 0)
	{
		m_aInputState[CurrentKey] = true;
		m_aInputCount[CurrentKey] = m_InputCounter;
		AddEvent(0, CurrentKey, IInput::FLAG_PRESS);
	}
}

void CInput::SetCompositionWindowPosition(float X, float Y, float H)
{
	SDL_Rect Rect;
	Rect.x = X / m_pGraphics->ScreenHiDPIScale();
	Rect.y = Y / m_pGraphics->ScreenHiDPIScale();
	Rect.h = H / m_pGraphics->ScreenHiDPIScale();
	Rect.w = 0;
	SDL_SetTextInputRect(&Rect);
}

int CInput::Update()
{
	// keep the counter between 1..0xFFFF, 0 means not pressed
	m_InputCounter = (m_InputCounter%0xFFFF)+1;

	int NumKeyStates;
	const Uint8 *pState = SDL_GetKeyboardState(&NumKeyStates);
	if(NumKeyStates >= KEY_MOUSE_1)
		NumKeyStates = KEY_MOUSE_1;
	mem_copy(m_aInputState, pState, NumKeyStates);
	mem_zero(m_aInputState+NumKeyStates, KEY_LAST-NumKeyStates);

	// these states must always be updated manually because they are not in the SDL_GetKeyboardState from SDL
	UpdateMouseState();
	UpdateJoystickState();

	SDL_Event Event;
	while(SDL_PollEvent(&Event))
	{
		int Key = -1;
		int Scancode = -1;
		int Action = IInput::FLAG_PRESS;
		switch(Event.type)
		{
			// handle text editing candidate
			case SDL_SYSWMEVENT:
				ProcessSystemMessage(Event.syswm.msg);
				break;

			// handle on the spot text editing
			case SDL_TEXTEDITING:
			{
				m_CompositionLength = str_length(Event.edit.text);
				if(m_CompositionLength)
				{
					str_copy(m_aComposition, Event.edit.text, sizeof(m_aComposition));
					m_CompositionCursor = 0;
					for(int i = 0; i < Event.edit.start; i++)
						m_CompositionCursor = str_utf8_forward(m_aComposition, m_CompositionCursor);
					int CompositionEnd = m_CompositionCursor;
					for(int i = 0; i < Event.edit.length; i++)
						CompositionEnd = str_utf8_forward(m_aComposition, CompositionEnd);
					m_CompositionSelectedLength = CompositionEnd - m_CompositionCursor;
					AddEvent(0, 0, IInput::FLAG_TEXT);
				}
				else
				{
					m_aComposition[0] = '\0';
					m_CompositionLength = 0;
					m_CompositionCursor = 0;
					m_CompositionSelectedLength = 0;
				}
				break;
			}
			case SDL_TEXTINPUT:
				m_aComposition[0] = 0;
				m_CompositionLength = COMP_LENGTH_INACTIVE;
				m_CompositionCursor = 0;
				m_CompositionSelectedLength = 0;
				AddEvent(Event.text.text, 0, IInput::FLAG_TEXT);
				break;

			// handle keys
			case SDL_KEYUP:
				Action = IInput::FLAG_RELEASE;

				// fall through
			case SDL_KEYDOWN:
				Key = KeycodeToKey(Event.key.keysym.sym);
				Scancode = Event.key.keysym.scancode;
				break;

			// handle the joystick events
			case SDL_JOYAXISMOTION:
				HandleJoystickAxisMotionEvent(Event);
				break;

			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				HandleJoystickButtonEvent(Event);
				break;

			case SDL_JOYHATMOTION:
				HandleJoystickHatMotionEvent(Event);
				break;

			// handle mouse buttons
			case SDL_MOUSEBUTTONUP:
				Action = IInput::FLAG_RELEASE;

				// fall through
			case SDL_MOUSEBUTTONDOWN:
				if(Event.button.button == SDL_BUTTON_LEFT)
				{
					Key = KEY_MOUSE_1;
					if(Event.button.clicks%2 == 0)
						m_MouseDoubleClick = true;
					else if(Event.button.clicks == 1)
						m_MouseDoubleClick = false;
				}
				else if(Event.button.button == SDL_BUTTON_RIGHT) Key = KEY_MOUSE_2;
				else if(Event.button.button == SDL_BUTTON_MIDDLE) Key = KEY_MOUSE_3;
				else if(Event.button.button == SDL_BUTTON_X1) Key = KEY_MOUSE_4;
				else if(Event.button.button == SDL_BUTTON_X2) Key = KEY_MOUSE_5;
				else if(Event.button.button == 6) Key = KEY_MOUSE_6;
				else if(Event.button.button == 7) Key = KEY_MOUSE_7;
				else if(Event.button.button == 8) Key = KEY_MOUSE_8;
				else if(Event.button.button == 9) Key = KEY_MOUSE_9;
				Scancode = Key;
				break;

			case SDL_MOUSEWHEEL:
				if(Event.wheel.y > 0) Key = KEY_MOUSE_WHEEL_UP;
				else if(Event.wheel.y < 0) Key = KEY_MOUSE_WHEEL_DOWN;
				else break;
				Action |= IInput::FLAG_RELEASE;
				Scancode = Key;
				break;

#if defined(CONF_PLATFORM_MACOS)	// Todo SDL: remove this when fixed (mouse state is faulty on start)
			case SDL_WINDOWEVENT:
				if(Event.window.event == SDL_WINDOWEVENT_MAXIMIZED)
				{
					MouseModeAbsolute();
					MouseModeRelative();
				}
				break;
#endif

			// other messages
			case SDL_QUIT:
				return 1;
		}

		if(Key >= 0 && !HasComposition())
		{
			if((Action&IInput::FLAG_PRESS) && Key < g_MaxKeys && Scancode >= 0 && Scancode < g_MaxKeys)
			{
				m_aInputState[Scancode] = true;
				m_aInputCount[Key] = m_InputCounter;
			}
			AddEvent(0, Key, Action);
		}
	}

	if(m_CompositionLength == 0)
		m_CompositionLength = COMP_LENGTH_INACTIVE;

	return 0;
}

void CInput::ProcessSystemMessage(SDL_SysWMmsg *pMsg)
{
#if defined(CONF_FAMILY_WINDOWS)
	// Todo SDL: remove this after SDL2 supports IME candidates
	if(pMsg->subsystem == SDL_SYSWM_WINDOWS)
	{
		if(pMsg->msg.win.msg != WM_IME_NOTIFY)
			return;

		switch(pMsg->msg.win.wParam)
		{
			case IMN_OPENCANDIDATE:
			case IMN_CHANGECANDIDATE:
			{
				HWND WindowHandle = pMsg->msg.win.hwnd;
				HIMC ImeContext = ImmGetContext(WindowHandle);
				DWORD CandidateCount;
				DWORD Size = ImmGetCandidateListCountW(ImeContext, &CandidateCount);
				LPCANDIDATELIST CandidateList = NULL;
				if(Size > 0)
				{
					CandidateList = (LPCANDIDATELIST)mem_alloc(Size);
					Size = ImmGetCandidateListW(ImeContext, 0, CandidateList, Size);
				}
				if(CandidateList && Size > 0)
				{
					m_CandidateCount = 0;
					for(DWORD i = CandidateList->dwPageStart; i < CandidateList->dwCount && m_CandidateCount < (int)CandidateList->dwPageSize; i++)
					{
						LPCWSTR Candidate = (LPCWSTR)((DWORD_PTR)CandidateList + CandidateList->dwOffset[i]);
						WideCharToMultiByte(CP_UTF8, 0, Candidate, -1, m_aaCandidates[m_CandidateCount], MAX_CANDIDATE_ARRAY_SIZE, "?", NULL);
						m_aaCandidates[m_CandidateCount][MAX_CANDIDATE_ARRAY_SIZE - 1] = '\0';
						m_CandidateCount++;
					}
					m_CandidateSelectedIndex = CandidateList->dwSelection - CandidateList->dwPageStart;
				}
				else
				{
					m_CandidateCount = 0;
					m_CandidateSelectedIndex = -1;
				}
				mem_free(CandidateList);
				ImmReleaseContext(WindowHandle, ImeContext);
				break;
			}
			case IMN_CLOSECANDIDATE:
				m_CandidateCount = 0;
				m_CandidateSelectedIndex = -1;
				break;
		}
	}
#endif
}

IEngineInput *CreateEngineInput() { return new CInput; }
