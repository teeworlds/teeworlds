/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_ITEMS_H
#define GAME_CLIENT_COMPONENTS_ITEMS_H
#include <game/client/component.h>

class CItems : public CComponent
{
	enum
	{
		MAX_EXTRA_PROJECTILES=32,
	};

	CNetObj_Projectile m_aExtraProjectiles[MAX_EXTRA_PROJECTILES];
	int m_NumExtraProjectiles;

	void RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID);
	void RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent);
	void RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameDataFlag *pPrevGameDataFlag, const CNetObj_GameDataFlag *pCurGameDataFlag);
	void RenderLaser(const struct CNetObj_Laser *pCurrent);

public:
	virtual void OnReset();
	virtual void OnRender();

	void AddExtraProjectile(CNetObj_Projectile *pProj);
};

#endif
