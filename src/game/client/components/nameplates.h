/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_NAMEPLATES_H
#define GAME_CLIENT_COMPONENTS_NAMEPLATES_H
#include <game/client/component.h>

class CNamePlates : public CComponent
{
	void RenderNameplate(
		const CNetObj_Character *pPrevChar,
		const CNetObj_Character *pPlayerChar,
		const CNetObj_PlayerInfo *pPlayerInfo,
		int ClientID
	);

public:
	virtual void OnRender();
};

#endif
