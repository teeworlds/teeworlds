/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CAMERA_H
#define GAME_CLIENT_COMPONENTS_CAMERA_H
#include <base/vmath.h>
#include <game/client/component.h>

class CCamera : public CComponent
{	
	static void ConZoomPlus(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConZoomMinus(IConsole::IResult *pResult, void *pUserData, int ClientID);
	static void ConZoomReset(IConsole::IResult *pResult, void *pUserData, int ClientID);

public:
	vec2 m_Center;
	float m_Zoom;
	bool m_WasSpectator;

	CCamera();
	virtual void OnRender();
	virtual void OnConsoleInit();
};

#endif
