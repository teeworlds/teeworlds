/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <engine/map.h>
#include <game/mapitems.h>
#include <game/gamecore.h> // StrToInts, IntsToStr
#include <base/tl/array.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	class IMap *m_pMap;

	void InitGameLayer();
	void InitTilemapSkip();

public:
	CLayers();
	void Init(class IKernel *pKernel, class IMap *pMap=0);
	int NumGroups() const { return m_GroupsNum; }
	int NumLayers() const { return m_LayersNum; }
	class IMap *Map() const { return m_pMap; }
	CMapItemGroup *GameGroup() const { return m_pGameGroup; }
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; }
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;
	bool HasMaterialLayer() { return m_pMaterialLayer != nullptr; }
	CMapItemLayerTilemap *MaterialLayer() const { return m_pMaterialLayer; }
	CMapItemLayerTilemap *GetCustomLayer(const char* LayerName);
private:
	CMapItemLayerTilemap *m_pMaterialLayer;
	array<CMapItemLayerTilemap*> m_apCustomLayers;
};

#endif
