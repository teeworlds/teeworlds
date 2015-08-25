/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_ITEMS_H
#define GAME_CLIENT_COMPONENTS_ITEMS_H
#include <game/client/component.h>

class CItems : public CComponent
{
	void RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID);
	void RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent);
	void RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameData *pPrevGameData, const CNetObj_GameData *pCurGameData);
	void RenderLaser(const struct CNetObj_Laser *pCurrent);

public:
	virtual void OnRender();
};

#endif
