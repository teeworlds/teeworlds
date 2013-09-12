/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "SDL.h"

#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
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

	m_FirstWarp = false;
	m_LastMousePosX = 0;
	m_LastMousePosY = 0;

	m_pCursorSurface = NULL;
	m_pCursor = NULL;

	m_LastRelease = 0;
	m_ReleaseDelta = -1;

	m_MouseLeft = false;
	m_MouseEntered = false;

	m_NumEvents = 0;
}

void CInput::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	SDL_StartTextInput();
	ShowCursor(true);
	//m_pGraphics->GrabWindow(true);
}

void CInput::LoadHardwareCursor()
{
	if(m_pCursor != NULL)
		return;

	CImageInfo CursorImg;
	if(!m_pGraphics->LoadPNG(&CursorImg, "gui_cursor_small.png", IStorage::TYPE_ALL))
		return;

	m_pCursorSurface = SDL_CreateRGBSurfaceFrom(
		CursorImg.m_pData, CursorImg.m_Width, CursorImg.m_Height,
		32, 4*CursorImg.m_Width,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if(!m_pCursorSurface)
		return;

	m_pCursor = SDL_CreateColorCursor(m_pCursorSurface, 0, 0);
}

int CInput::ShowCursor(bool show)
{
	if(g_Config.m_InpHWCursor)
	{
		LoadHardwareCursor();
		SDL_SetCursor(m_pCursor);
	}
	return SDL_ShowCursor(show);
}

void CInput::SetMouseModes(int modes)
{
	if((m_MouseModes & MOUSE_MODE_WARP_CENTER) && !(modes & MOUSE_MODE_WARP_CENTER))
		m_pGraphics->WarpMouse(m_LastMousePosX, m_LastMousePosY);
	else if(!(m_MouseModes & MOUSE_MODE_WARP_CENTER) && (modes & MOUSE_MODE_WARP_CENTER))
		m_FirstWarp = true;

	m_MouseModes = modes;
}

int CInput::GetMouseModes()
{
	return m_MouseModes;
}

void CInput::GetMousePosition(float *x, float *y)
{
	if(GetMouseModes() & MOUSE_MODE_NO_MOUSE)
		return;

	float Sens = g_Config.m_InpMousesens/100.0f;
	int nx = 0, ny = 0;
	SDL_GetMouseState(&nx, &ny);

	if(m_FirstWarp)
	{
		m_LastMousePosX = nx;
		m_LastMousePosY = ny;
		m_FirstWarp = false;
	}

	*x = nx * Sens;
	*y = ny * Sens;

	if(GetMouseModes() & MOUSE_MODE_WARP_CENTER)
		m_pGraphics->WarpMouse( Graphics()->ScreenWidth()/2,Graphics()->ScreenHeight()/2);
}

void CInput::GetRelativePosition(float *x, float *y)
{
	if(m_FirstWarp)
	{
		*x = 0;
		*y = 0;
		return;
	}

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

const char* CInput::GetClipboardText()
{
	if(m_pClipboardText)
	{
		free(m_pClipboardText);
	}
	m_pClipboardText = SDL_GetClipboardText();
	return m_pClipboardText;
}

void CInput::SetClipboardText(const char *Text)
{
	SDL_SetClipboardText(Text);
}

bool CInput::MouseLeft()
{
	return m_MouseLeft;
}

bool CInput::MouseEntered()
{
	return m_MouseEntered;
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
		const Uint8 *pState = SDL_GetKeyboardState(&i);
		if(i >= KEY_LAST)
			i = KEY_LAST-1;
		mem_copy(m_aInputState[m_InputCurrent], pState, i);
	}

	m_MouseLeft = false;
	m_MouseEntered = false;

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

				case SDL_WINDOWEVENT:
					if(Event.window.event == SDL_WINDOWEVENT_ENTER) m_MouseEntered = true;
					if(Event.window.event == SDL_WINDOWEVENT_LEAVE) m_MouseLeft = true;
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
