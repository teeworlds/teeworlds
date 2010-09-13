// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef GAME_CLIENT_COMPONENTS_ITEMS_H
#define GAME_CLIENT_COMPONENTS_ITEMS_H
#include <game/client/component.h>

class CItems : public CComponent
{	
	void RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemId);
	void RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent);
	void RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent);
	void RenderLaser(const struct CNetObj_Laser *pCurrent);
	
public:
	virtual void OnRender();
};

#endif
