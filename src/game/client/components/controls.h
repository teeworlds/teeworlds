#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H
#include <base/vmath.h>
#include <game/client/component.h>

class CControls : public CComponent
{	
public:
	vec2 m_MousePos;
	vec2 m_TargetPos;

	CNetObj_PlayerInput m_InputData;
	int m_InputDirectionLeft;
	int m_InputDirectionRight;

	CControls();
	
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
	
	int SnapInput(int *pData);
};
#endif
