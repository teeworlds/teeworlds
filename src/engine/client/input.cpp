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
#ifndef SDL_JOYSTICK_AXIS_MAX
	#define SDL_JOYSTICK_AXIS_MAX 32767
#endif

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
	m_InputGrabbed = 0;
	m_pClipboardText = 0;

	m_SelectedJoystickIndex = -1;
	m_aSelectedJoystickGUID[0] = '\0';

	m_PreviousHat = 0;

	m_MouseDoubleClick = false;

	m_NumEvents = 0;
}

CInput::~CInput()
{
	if(m_pClipboardText)
	{
		SDL_free(m_pClipboardText);
	}
	CloseJoysticks();
}

void CInput::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pConfig = Kernel()->RequestInterface<IConfigManager>()->Values();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	// FIXME: unicode handling: use SDL_StartTextInput/SDL_StopTextInput on inputs

	MouseModeRelative();

	InitJoysticks();
}

void CInput::InitJoysticks()
{
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			dbg_msg("joystick", "unable to init SDL joystick: %s", SDL_GetError());
			return;
		}
	}

	int NumJoysticks = SDL_NumJoysticks();
	if(NumJoysticks > 0)
	{
		dbg_msg("joystick", "%d joystick(s) found", NumJoysticks);

		for(int i = 0; i < NumJoysticks; i++)
		{
			SDL_Joystick *pJoystick = SDL_JoystickOpen(i);

			if(!pJoystick) {
				dbg_msg("joystick", "Could not open joystick %d: %s", i, SDL_GetError());
				return;
			}
			m_apJoysticks.add(pJoystick);

			dbg_msg("joystick", "Opened Joystick %d", i);
			dbg_msg("joystick", "Name: %s", SDL_JoystickNameForIndex(i));
			dbg_msg("joystick", "Number of Axes: %d", SDL_JoystickNumAxes(pJoystick));
			dbg_msg("joystick", "Number of Buttons: %d", SDL_JoystickNumButtons(pJoystick));
			dbg_msg("joystick", "Number of Balls: %d", SDL_JoystickNumBalls(pJoystick));
		}
	}
	else
	{
		dbg_msg("joystick", "No joysticks found");
	}
}

SDL_Joystick* CInput::GetActiveJoystick()
{
	if(m_apJoysticks.size() == 0)
	{
		return NULL;
	}
	if(m_aSelectedJoystickGUID[0] && str_comp(m_aSelectedJoystickGUID, m_pConfig->m_JoystickGUID) != 0)
	{
		// Refresh if cached GUID differs from configured GUID
		m_SelectedJoystickIndex = -1;
	}
	if(m_SelectedJoystickIndex == -1)
	{
		for(int i = 0; i < m_apJoysticks.size(); i++)
		{
			char aGUID[sizeof(m_aSelectedJoystickGUID)];
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(m_apJoysticks[i]), aGUID, sizeof(aGUID));
			if(str_comp(m_pConfig->m_JoystickGUID, aGUID) == 0)
			{
				m_SelectedJoystickIndex = i;
				str_copy(m_aSelectedJoystickGUID, m_pConfig->m_JoystickGUID, sizeof(m_aSelectedJoystickGUID));
				break;
			}
		}
		// could not find configured joystick, falling back to first available
		if(m_SelectedJoystickIndex == -1)
		{
			m_SelectedJoystickIndex = 0;
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(m_apJoysticks[0]), m_pConfig->m_JoystickGUID, sizeof(m_pConfig->m_JoystickGUID));
			str_copy(m_aSelectedJoystickGUID, m_pConfig->m_JoystickGUID, sizeof(m_aSelectedJoystickGUID));
		}
	}
	return m_apJoysticks[m_SelectedJoystickIndex];
}

void CInput::CloseJoysticks()
{
	for(sorted_array<SDL_Joystick*>::range r = m_apJoysticks.all(); !r.empty(); r.pop_front())
	{
		if (SDL_JoystickGetAttached(r.front()))
		{
			SDL_JoystickClose(r.front());
		}
	}
	m_apJoysticks.clear();
}

void CInput::SelectNextJoystick()
{
	const int Num = m_apJoysticks.size();
	if(Num > 1)
	{
		const int NextIndex = (m_SelectedJoystickIndex + 1) % Num;
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(m_apJoysticks[NextIndex]), m_pConfig->m_JoystickGUID, sizeof(m_pConfig->m_JoystickGUID));
	}
}

const char* CInput::GetJoystickName()
{
	SDL_Joystick* pJoystick = GetActiveJoystick();
	dbg_assert((bool)pJoystick, "Requesting joystick name, but no joysticks were initialized");
	return SDL_JoystickName(pJoystick);
}

float CInput::GetJoystickAxisValue(int Axis)
{
	SDL_Joystick* pJoystick = GetActiveJoystick();
	dbg_assert((bool)pJoystick, "Requesting joystick axis value, but no joysticks were initialized");
	return static_cast<float>(SDL_JoystickGetAxis(pJoystick, Axis)) / (float)(SDL_JOYSTICK_AXIS_MAX+1);
}

int CInput::GetJoystickNumAxes()
{
	SDL_Joystick* pJoystick = GetActiveJoystick();
	dbg_assert((bool)pJoystick, "Requesting joystick axes count, but no joysticks were initialized");
	return SDL_JoystickNumAxes(pJoystick);
}

void CInput::MouseRelative(float *x, float *y)
{
	if(!m_InputGrabbed)
		return;

	int nx = 0, ny = 0;
	float MouseSens = m_pConfig->m_InpMousesens/100.0f;
	float JoystickSens = m_pConfig->m_JoystickSens/100.0f;

	SDL_GetRelativeMouseState(&nx,&ny);

	vec2 j = vec2(0.0f, 0.0f);

	if(m_pConfig->m_JoystickEnable && GetActiveJoystick())
	{
		const float Max = 50.0f;
		j = vec2(GetJoystickAxisValue(m_pConfig->m_JoystickX), GetJoystickAxisValue(m_pConfig->m_JoystickY)) * Max;
		const float Len = length(j);

		if(Len/sqrtf(2.0f) <= m_pConfig->m_JoystickTolerance)
		{
			j = vec2(0.0f, 0.0f);
		}
		else
		{
			const vec2 nj = Len > 0.0f ? j / Len : vec2(0.0f, 0.0f);
			j = nj * min(Len, Max) - nj * m_pConfig->m_JoystickTolerance;
		}
	}

	*x = nx*MouseSens + j.x*JoystickSens;
	*y = ny*MouseSens + j.y*JoystickSens;
}

void CInput::MouseModeAbsolute()
{
	if(m_InputGrabbed)
	{
		m_InputGrabbed = 0;
		SDL_ShowCursor(SDL_ENABLE);
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void CInput::MouseModeRelative()
{
	if(!m_InputGrabbed)
	{
		m_InputGrabbed = 1;
		SDL_ShowCursor(SDL_DISABLE);
		if(SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, m_pConfig->m_InpGrab ? "0" : "1", SDL_HINT_OVERRIDE) == SDL_FALSE)
		{
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "input", "unable to switch relative mouse mode");
		}
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_GetRelativeMouseState(NULL, NULL);
	}
}

int CInput::MouseDoubleClick()
{
	if(m_MouseDoubleClick)
	{
		m_MouseDoubleClick = false;
		return 1;
	}
	return 0;
}

const char *CInput::GetClipboardText()
{
	if(m_pClipboardText)
	{
		SDL_free(m_pClipboardText);
	}
	m_pClipboardText = SDL_GetClipboardText();
	if(m_pClipboardText)
		str_sanitize_cc(m_pClipboardText);
	return m_pClipboardText;
}

void CInput::SetClipboardText(const char *pText)
{
	SDL_SetClipboardText(pText);
}

void CInput::Clear()
{
	mem_zero(m_aInputState, sizeof(m_aInputState));
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
	m_NumEvents = 0;
}

bool CInput::KeyState(int Key) const
{
	return m_aInputState[Key>=KEY_MOUSE_1 ? Key : SDL_GetScancodeFromKey(KeyToKeycode(Key))];
}

int CInput::Update()
{
	// keep the counter between 1..0xFFFF, 0 means not pressed
	m_InputCounter = (m_InputCounter%0xFFFF)+1;

	{
		int i;
		const Uint8 *pState = SDL_GetKeyboardState(&i);
		if(i >= KEY_LAST)
			i = KEY_LAST-1;
		mem_copy(m_aInputState, pState, i);
	}

	// these states must always be updated manually because they are not in the GetKeyState from SDL
	int i = SDL_GetMouseState(NULL, NULL);
	if(i&SDL_BUTTON(1)) m_aInputState[KEY_MOUSE_1] = 1; // 1 is left
	if(i&SDL_BUTTON(3)) m_aInputState[KEY_MOUSE_2] = 1; // 3 is right
	if(i&SDL_BUTTON(2)) m_aInputState[KEY_MOUSE_3] = 1; // 2 is middle
	if(i&SDL_BUTTON(4)) m_aInputState[KEY_MOUSE_4] = 1;
	if(i&SDL_BUTTON(5)) m_aInputState[KEY_MOUSE_5] = 1;
	if(i&SDL_BUTTON(6)) m_aInputState[KEY_MOUSE_6] = 1;
	if(i&SDL_BUTTON(7)) m_aInputState[KEY_MOUSE_7] = 1;
	if(i&SDL_BUTTON(8)) m_aInputState[KEY_MOUSE_8] = 1;
	if(i&SDL_BUTTON(9)) m_aInputState[KEY_MOUSE_9] = 1;

	{
		SDL_Event Event;

		while(SDL_PollEvent(&Event))
		{
			int Key = -1;
			int Scancode = 0;
			int Action = IInput::FLAG_PRESS;
			switch (Event.type)
			{
				case SDL_TEXTINPUT:
					AddEvent(Event.text.text, 0, IInput::FLAG_TEXT);
					break;

				// handle keys
				case SDL_KEYDOWN:
					Key = KeycodeToKey(Event.key.keysym.sym);
					Scancode = Event.key.keysym.scancode;
					break;
				case SDL_KEYUP:
					Action = IInput::FLAG_RELEASE;
					Key = KeycodeToKey(Event.key.keysym.sym);
					Scancode = Event.key.keysym.scancode;
					break;

				// handle the joystick events
				case SDL_JOYBUTTONUP:
					Action = IInput::FLAG_RELEASE;

					// fall through
				case SDL_JOYBUTTONDOWN:
					Key = Event.jbutton.button + KEY_JOYSTICK_BUTTON_0;
					Scancode = Key;
					break;

				case SDL_JOYHATMOTION:
					switch (Event.jhat.value) {
					case SDL_HAT_LEFTUP:
						Key = KEY_JOY_HAT_LEFTUP;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_UP:
						Key = KEY_JOY_HAT_UP;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_RIGHTUP:
						Key = KEY_JOY_HAT_RIGHTUP;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_LEFT:
						Key = KEY_JOY_HAT_LEFT;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_CENTERED:
						Action = IInput::FLAG_RELEASE;
						Key = m_PreviousHat;
						Scancode = m_PreviousHat;
						m_PreviousHat = 0;
						break;
					case SDL_HAT_RIGHT:
						Key = KEY_JOY_HAT_RIGHT;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_LEFTDOWN:
						Key = KEY_JOY_HAT_LEFTDOWN;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_DOWN:
						Key = KEY_JOY_HAT_DOWN;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					case SDL_HAT_RIGHTDOWN:
						Key = KEY_JOY_HAT_RIGHTDOWN;
						Scancode = Key;
						m_PreviousHat = Key;
						break;
					}
					break;

				// handle mouse buttons
				case SDL_MOUSEBUTTONUP:
					Action = IInput::FLAG_RELEASE;

					// fall through
				case SDL_MOUSEBUTTONDOWN:
					if(Event.button.button == SDL_BUTTON_LEFT) Key = KEY_MOUSE_1; // ignore_convention
					if(Event.button.button == SDL_BUTTON_RIGHT) Key = KEY_MOUSE_2; // ignore_convention
					if(Event.button.button == SDL_BUTTON_MIDDLE) Key = KEY_MOUSE_3; // ignore_convention
					if(Event.button.button == 4) Key = KEY_MOUSE_4; // ignore_convention
					if(Event.button.button == 5) Key = KEY_MOUSE_5; // ignore_convention
					if(Event.button.button == 6) Key = KEY_MOUSE_6; // ignore_convention
					if(Event.button.button == 7) Key = KEY_MOUSE_7; // ignore_convention
					if(Event.button.button == 8) Key = KEY_MOUSE_8; // ignore_convention
					if(Event.button.button == 9) Key = KEY_MOUSE_9; // ignore_convention
					if(Event.button.button == SDL_BUTTON_LEFT)
					{
						if(Event.button.clicks%2 == 0)
							m_MouseDoubleClick = true;
						if(Event.button.clicks == 1)
							m_MouseDoubleClick = false;
					}
					Scancode = Key;
					break;

				case SDL_MOUSEWHEEL:
					if(Event.wheel.y > 0) Key = KEY_MOUSE_WHEEL_UP; // ignore_convention
					if(Event.wheel.y < 0) Key = KEY_MOUSE_WHEEL_DOWN; // ignore_convention
					Action |= IInput::FLAG_RELEASE;
					break;

#if defined(CONF_PLATFORM_MACOSX)	// Todo SDL: remove this when fixed (mouse state is faulty on start)
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

			//
			if(Key != -1)
			{
				if(Action&IInput::FLAG_PRESS)
				{
					m_aInputState[Scancode] = 1;
					m_aInputCount[Key] = m_InputCounter;
				}
				AddEvent(0, Key, Action);
			}

		}
	}

	return 0;
}


IEngineInput *CreateEngineInput() { return new CInput; }
