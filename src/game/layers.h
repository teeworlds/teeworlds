/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <engine/map.h>
#include <game/mapitems.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	CMapItemLayerTilemap *m_pTeleLayer;
	CMapItemLayerTilemap *m_pSpeedupLayer;
	CMapItemLayerTilemap *m_pFrontLayer;
	CMapItemLayerTilemap *m_pSwitchLayer;
	class IMap *m_pMap;

public:
	CLayers();
	void Dest();
	void Init(class IKernel *pKernel);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemLayerTilemap *TeleLayer() const { return m_pTeleLayer; };
	CMapItemLayerTilemap *SpeedupLayer() const { return m_pSpeedupLayer; };
	CMapItemLayerTilemap *FrontLayer() const { return m_pFrontLayer; };
	CMapItemLayerTilemap *SwitchLayer() const { return m_pSwitchLayer; };
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;	
};

#endif
