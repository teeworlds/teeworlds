/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BACKGROUND_H
#define GAME_CLIENT_COMPONENTS_BACKGROUND_H
#include <game/client/component.h>

class CBackground : public CComponent
{
	static int m_BackgroundTexture;
public:
	virtual void OnInit();
	virtual void OnRender();
};

#endif
