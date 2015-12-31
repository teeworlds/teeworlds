/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_ITEMS_H
#define GAME_CLIENT_COMPONENTS_ITEMS_H
#include <game/client/component.h>

class CItems : public CComponent
{
	void RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID);
	void RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent);
	void RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameDataFlag *pPrevGameDataFlag, const CNetObj_GameDataFlag *pCurGameDataFlag);
	void RenderLaser(const struct CNetObj_Laser *pCurrent);
	void RenderModAPISprite(const CNetObj_ModAPI_Sprite *pPrev, const struct CNetObj_ModAPI_Sprite *pCurrent);
	void RenderModAPILine(const struct CNetObj_ModAPI_Line *pCurrent);

public:
	virtual void OnRender();
};

#endif
