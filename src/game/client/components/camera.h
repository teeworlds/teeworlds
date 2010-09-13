// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef GAME_CLIENT_COMPONENTS_CAMERA_H
#define GAME_CLIENT_COMPONENTS_CAMERA_H
#include <base/vmath.h>
#include <game/client/component.h>

class CCamera : public CComponent
{	
public:
	vec2 m_Center;
	float m_Zoom;

	CCamera();
	virtual void OnRender();
};

#endif
