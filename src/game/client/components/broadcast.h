/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BROADCAST_H
#define GAME_CLIENT_COMPONENTS_BROADCAST_H
#include <game/client/component.h>

class CBroadcast : public CComponent
{
	char m_aBroadcastText[128];
	int64 m_BroadcastTime;
	float m_BroadcastRenderOffset;

public:
	CBroadcast();

	void DoBroadcast(const char *pText);

	virtual void OnReset();
	virtual void OnRender();
};

#endif
