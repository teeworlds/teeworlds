/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_PLAYERS_H
#define GAME_CLIENT_COMPONENTS_PLAYERS_H
#include <game/client/component.h>

class CPlayers : public CComponent
{
	void RenderPlayer(
		const CNetObj_Character *pPrevChar,
		const CNetObj_Character *pPlayerChar,
		const CNetObj_PlayerInfo *pPlayerInfo,
		const CTeeRenderInfo *pRenderInfo,
		int ClientID
	) const;
	void RenderHook(
		const CNetObj_Character *pPrevChar,
		const CNetObj_Character *pPlayerChar,
		const CTeeRenderInfo *pRenderInfo,
		int ClientID
	) const;
	void RenderHarpoon(
		CNetObj_Character* pOwnerChar,
		CNetObj_Character* pOwnerPrev,
		CNetObj_Character* pVictimChar,
		CNetObj_Character* pVictimPrev,
		const CNetObj_HarpoonDragPlayer* DragInfo
	) const;
	void RenderEntityHarpoon(
		CNetObj_Character* pOwnerChar,
		CNetObj_Character* pOwnerPrev,
		const CNetObj_HarpoonDragPlayer* DragInfo
	) const;

public:
	virtual void OnRender();
	vec2 HarpoonMouthAlignment(vec2 PlayerPos, float Angle) const;
};

#endif
