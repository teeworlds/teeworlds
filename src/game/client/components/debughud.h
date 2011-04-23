/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_DEBUGHUD_H
#define GAME_CLIENT_COMPONENTS_DEBUGHUD_H
#include <game/client/component.h>

class CDebugHud : public CComponent
{
	void RenderNetCorrections();
	void RenderTuning();
public:
	virtual void OnRender();
};

#endif
