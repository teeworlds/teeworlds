/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include "layers.h"

CLayers::CLayers()
{
	m_GroupsNum = 0;
	m_GroupsStart = 0;
	m_LayersNum = 0;
	m_LayersStart = 0;
	m_pGameGroup = 0x0;
	m_pGameLayer = 0x0;
	m_pMap = 0x0;
}

void CLayers::Init(class IKernel *pKernel, IMap *pMap)
{
	m_pMap = pMap ? pMap : pKernel->RequestInterface<IMap>();
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);

	InitGameLayer();
	InitTilemapSkip();
}

void CLayers::InitGameLayer()
{
	for(int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		if(!pGroup || pGroup->m_StartLayer >= NumLayers())
			continue;

		for(int l = 0; l < minimum(pGroup->m_NumLayers, NumLayers()); l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if(!pLayer)
				continue;

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
				if(pTilemap->m_Flags & TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if(m_pGameGroup->m_Version >= CMapItemGroup_v2::CURRENT_VERSION)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}

					return; // there can only be one game layer and game group
				}
			}
		}
	}

	// no game layer found
	m_pGameLayer = 0x0;
	m_pGameGroup = 0x0;
}

void CLayers::InitTilemapSkip()
{
	for(int g = 0; g < NumGroups(); g++)
	{
		const CMapItemGroup *pGroup = GetGroup(g);
		if(!pGroup || pGroup->m_StartLayer >= NumLayers())
			continue;

		for(int l = 0; l < minimum(pGroup->m_NumLayers, NumLayers()); l++)
		{
			const CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if(!pLayer)
				continue;

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				const CMapItemLayerTilemap *pTilemap = reinterpret_cast<const CMapItemLayerTilemap *>(pLayer);

				const int NumTiles = m_pMap->GetDataSize(pTilemap->m_Data) / sizeof(CTile);
				if(NumTiles <= 0)
					continue;
				CTile *pTiles = static_cast<CTile *>(m_pMap->GetData(pTilemap->m_Data));
				if(!pTiles)
					continue;

				for(int y = 0; y < pTilemap->m_Height; y++)
				{
					for(int x = 1; x < pTilemap->m_Width; )
					{
						const int TileIndex = y * pTilemap->m_Width + x;
						if(TileIndex >= NumTiles)
						{
							y = pTilemap->m_Height; // break outer loop
							break;
						}

						int SkippedX;
						for(SkippedX = 1; x + SkippedX < pTilemap->m_Width && SkippedX < 255; SkippedX++)
							if(TileIndex + SkippedX >= NumTiles || pTiles[TileIndex + SkippedX].m_Index)
								break;

						pTiles[TileIndex].m_Skip = SkippedX - 1;
						x += SkippedX;
					}
				}
			}
		}
	}
}

CMapItemGroup *CLayers::GetGroup(int Index) const
{
	if(!m_pMap || Index < 0 || Index >= NumGroups())
		return 0x0;
	int Size;
	CMapItemGroup *pGroup = static_cast<CMapItemGroup *>(m_pMap->GetItem(m_GroupsStart + Index, 0x0, 0x0, &Size));
	return CMapItemChecker::IsValid(pGroup, Size) ? pGroup : 0x0;
}

CMapItemLayer *CLayers::GetLayer(int Index) const
{
	if(!m_pMap || Index < 0 || Index >= NumLayers())
		return 0x0;
	int Size;
	CMapItemLayer *pLayer = static_cast<CMapItemLayer *>(m_pMap->GetItem(m_LayersStart + Index, 0x0, 0x0, &Size));
	return CMapItemChecker::IsValid(pLayer, Size) ? pLayer : 0x0;
}
