/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "SDL.h"

#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>

#include "input.h"

//print >>f, "int inp_key_code(const char *key_name) { int i; if (!strcmp(key_name, \"-?-\")) return -1; else for (i = 0; i < 512; i++) if (!strcmp(key_strings[i], key_name)) return i; return -1; }"

// this header is protected so you don't include it from anywere
#define KEYS_INCLUDE
#include "keynames.h"
#undef KEYS_INCLUDE

void CInput::AddEvent(int Unicode, int Key, int Flags)
{
	if(m_NumEvents != INPUT_BUFFER_SIZE)
	{
		m_aInputEvents[m_NumEvents].m_Unicode = Unicode;
		m_aInputEvents[m_NumEvents].m_Key = Key;
		m_aInputEvents[m_NumEvents].m_Flags = Flags;
		m_NumEvents++;
	}
}

CInput::CInput()
{
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
	mem_zero(m_aInputState, sizeof(m_aInputState));

	m_MouseModes = 0;

	m_InputCurrent = 0;
	m_InputDispatched = false;

	m_LastRelease = 0;
	m_ReleaseDelta = -1;

	m_NumEvents = 0;
}

void CInput::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	SDL_StartTextInput();
}

void CInput::SetMouseModes(int modes)
{
	m_MouseModes = modes;
}

int CInput::GetMouseModes()
{;
	return m_MouseModes;
}

void CInput::GetMousePosition(float *x, float *y)
{
	if(GetMouseModes() & MOUSE_MODE_NO_MOUSE)
	{
		*x = m_LastMousePosX;
		*y = m_LastMousePosY;
		return;
	}

	float Sens = g_Config.m_InpMousesens/100.0f;
	int nx = 0, ny = 0;
	SDL_GetMouseState(&nx, &ny);
	*x = nx * Sens;
	*y = ny * Sens;

	m_LastMousePosX = *x;
	m_LastMousePosY = *y;

	if(GetMouseModes() & MOUSE_MODE_WARP_CENTER)
		m_pGraphics->WarpMouse( Graphics()->ScreenWidth()/2,Graphics()->ScreenHeight()/2);
}

void CInput::GetRelativePosition(float *x, float *y)
{
	*x *= (float)100/g_Config.m_InpMousesens;
	*y *= (float)100/g_Config.m_InpMousesens;
	*x -= Graphics()->ScreenWidth()/2;
	*y -= Graphics()->ScreenHeight()/2;
}

bool CInput::MouseMoved()
{
	int x = 0, y = 0;
	SDL_GetRelativeMouseState(&x, &y);
	return x != 0 || y != 0;
}

#if 0
void CInput::ShowCursor(bool show)
{
	SDL_ShowCursor(show);
}

bool CInput::GetShowCursor()
{
	return SDL_ShowCursor(-1);
}

void CInput::SetMouseMode(CInput::MouseMode mode)
{
	if(m_MouseMode == mode)
		return;

	m_MouseMode = mode;
	if(g_Config.m_InpGrab)
	{
		if(mode == MOUSE_MODE_RELATIVE)
			m_pGraphics->GrabWindow(true);
		else if(mode == MOUSE_MODE_ABSOLUTE)
			m_pGraphics->GrabWindow(false);
	}
}

CInput::MouseMode CInput::GetMouseMode()
{
	return m_MouseMode;
}

void CInput::GetMousePosition(float *x, float *y)
{
	int nx = 0, ny = 0;
	if(m_MouseMode == MOUSE_MODE_RELATIVE)
	{
		float Sens = g_Config.m_InpMousesens/100.0f;
		SDL_GetRelativeMouseState(&nx, &ny);
		nx *= Sens;
		ny *= Sens;
	}
	else if(m_MouseMode == MOUSE_MODE_ABSOLUTE)
	{
		SDL_GetMouseState(&nx, &ny);
	}
	*x = nx;
	*y = ny;
}

bool CInput::MouseMoved()
{
	int x = 0, y = 0;
	SDL_GetRelativeMouseState(&x, &y);
	return x != 0 || y != 0;
}
#endif
#if 0
void CInput::MouseRelative(float *x, float *y)
{
	int nx = 0, ny = 0;
	float Sens = g_Config.m_InpMousesens/100.0f;

	if(g_Config.m_InpGrab)
		SDL_GetRelativeMouseState(&nx, &ny);
	else
	{
		if(m_InputGrabbed)
		{
			SDL_GetMouseState(&nx,&ny);
			m_pGraphics->WarpMouse( Graphics()->ScreenWidth()/2,Graphics()->ScreenHeight()/2);
			nx -= Graphics()->ScreenWidth()/2; ny -= Graphics()->ScreenHeight()/2;
		}
	}

	*x = nx*Sens;
	*y = ny*Sens;
}

void CInput::MouseModeAbsolute()
{
	SDL_ShowCursor(1);
	m_InputGrabbed = 0;
	if(g_Config.m_InpGrab)
		m_pGraphics->GrabWindow(false);
}

void CInput::MouseModeRelative()
{
	SDL_ShowCursor(0);
	m_InputGrabbed = 1;
	if(g_Config.m_InpGrab)
		m_pGraphics->GrabWindow(true);
}
#endif

int CInput::MouseDoubleClick()
{
	if(m_ReleaseDelta >= 0 && m_ReleaseDelta < (time_freq() >> 2))
	{
		m_LastRelease = 0;
		m_ReleaseDelta = -1;
		return 1;
	}
	return 0;
}

void CInput::ClearKeyStates()
{
	mem_zero(m_aInputState, sizeof(m_aInputState));
	mem_zero(m_aInputCount, sizeof(m_aInputCount));
}

int CInput::KeyState(int Key)
{
	return m_aInputState[m_InputCurrent][Key];
}

int CInput::Update()
{
//	if(m_InputGrabbed && !Graphics()->WindowActive())
//		MouseModeAbsolute();

	/*if(!input_grabbed && Graphics()->WindowActive())
		Input()->MouseModeRelative();*/

	if(m_InputDispatched)
	{
		// clear and begin count on the other one
		m_InputCurrent^=1;
		mem_zero(&m_aInputCount[m_InputCurrent], sizeof(m_aInputCount[m_InputCurrent]));
		mem_zero(&m_aInputState[m_InputCurrent], sizeof(m_aInputState[m_InputCurrent]));
		m_InputDispatched = false;
	}

	{
		int i;
		Uint8 *pState = SDL_GetKeyboardState(&i);
		if(i >= KEY_LAST)
			i = KEY_LAST-1;
		mem_copy(m_aInputState[m_InputCurrent], pState, i);
	}

	// these states must always be updated manually because they are not in the GetKeyState from SDL
	int i = SDL_GetMouseState(NULL, NULL);
	if(i&SDL_BUTTON(1)) m_aInputState[m_InputCurrent][KEY_MOUSE_1] = 1; // 1 is left
	if(i&SDL_BUTTON(3)) m_aInputState[m_InputCurrent][KEY_MOUSE_2] = 1; // 3 is right
	if(i&SDL_BUTTON(2)) m_aInputState[m_InputCurrent][KEY_MOUSE_3] = 1; // 2 is middle
	if(i&SDL_BUTTON(4)) m_aInputState[m_InputCurrent][KEY_MOUSE_4] = 1;
	if(i&SDL_BUTTON(5)) m_aInputState[m_InputCurrent][KEY_MOUSE_5] = 1;
	if(i&SDL_BUTTON(6)) m_aInputState[m_InputCurrent][KEY_MOUSE_6] = 1;
	if(i&SDL_BUTTON(7)) m_aInputState[m_InputCurrent][KEY_MOUSE_7] = 1;
	if(i&SDL_BUTTON(8)) m_aInputState[m_InputCurrent][KEY_MOUSE_8] = 1;

	{
		SDL_Event Event;

		while(SDL_PollEvent(&Event))
		{
			int Key = -1;
			int Action = IInput::FLAG_PRESS;
			switch (Event.type)
			{
				case SDL_TEXTINPUT:
				{
					int TextLength, i;
					TextLength = strlen(Event.text.text);
					for(i = 0; i < TextLength; i++)
					{
						AddEvent(Event.text.text[i], 0, 0);
					}
				}
				// handle keys
				case SDL_KEYDOWN:
					Key = SDL_GetScancodeFromName(SDL_GetKeyName(Event.key.keysym.sym));
					break;
				case SDL_KEYUP:
					Action = IInput::FLAG_RELEASE;
					Key = SDL_GetScancodeFromName(SDL_GetKeyName(Event.key.keysym.sym));
					break;

				// handle mouse buttons
				case SDL_MOUSEBUTTONUP:
					Action = IInput::FLAG_RELEASE;

					if(Event.button.button == 1) // ignore_convention
					{
						m_ReleaseDelta = time_get() - m_LastRelease;
						m_LastRelease = time_get();
					}

					// fall through
				case SDL_MOUSEBUTTONDOWN:
					if(Event.button.button == SDL_BUTTON_LEFT) Key = KEY_MOUSE_1; // ignore_convention
					if(Event.button.button == SDL_BUTTON_RIGHT) Key = KEY_MOUSE_2; // ignore_convention
					if(Event.button.button == SDL_BUTTON_MIDDLE) Key = KEY_MOUSE_3; // ignore_convention
					if(Event.button.button == 6) Key = KEY_MOUSE_6; // ignore_convention
					if(Event.button.button == 7) Key = KEY_MOUSE_7; // ignore_convention
					if(Event.button.button == 8) Key = KEY_MOUSE_8; // ignore_convention
					break;

				case SDL_MOUSEWHEEL:
					if(Event.wheel.y > 0) Key = KEY_MOUSE_WHEEL_UP; // ignore_convention
					if(Event.wheel.y < 0) Key = KEY_MOUSE_WHEEL_DOWN; // ignore_convention
					AddEvent(0, Key, Action);
					Action = IInput::FLAG_RELEASE;
					break;

				// other messages
				case SDL_QUIT:
					return 1;
			}

			//
			if(Key != -1)
			{
				m_aInputCount[m_InputCurrent][Key].m_Presses++;
				if(Action == IInput::FLAG_PRESS)
					m_aInputState[m_InputCurrent][Key] = 1;
				AddEvent(0, Key, Action);
			}

		}
	}

	return 0;
}


IEngineInput *CreateEngineInput() { return new CInput; }
