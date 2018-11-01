/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <game/mapitems.h>
#include "datafile.h"

class CMap : public IEngineMap
{
	CDataFileReader m_DataFile;
public:
	CMap() {}

	virtual void *GetData(int Index) { return m_DataFile.GetData(Index); }
	virtual void *GetDataSwapped(int Index) { return m_DataFile.GetDataSwapped(Index); }
	virtual void UnloadData(int Index) { m_DataFile.UnloadData(Index); }
	virtual void *GetItem(int Index, int *pType, int *pID) { return m_DataFile.GetItem(Index, pType, pID); }
	virtual void GetType(int Type, int *pStart, int *pNum) { m_DataFile.GetType(Type, pStart, pNum); }
	virtual void *FindItem(int Type, int ID) { return m_DataFile.FindItem(Type, ID); }
	virtual int NumItems() { return m_DataFile.NumItems(); }

	virtual void Unload()
	{
		m_DataFile.Close();
	}

	virtual bool Load(const char *pMapName, IStorage *pStorage)
	{
		if(!pStorage)
			pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
			return false;
		if(!m_DataFile.Open(pStorage, pMapName, IStorage::TYPE_ALL))
			return false;
		// check version
		CMapItemVersion *pItem = (CMapItemVersion *)m_DataFile.FindItem(MAPITEMTYPE_VERSION, 0);
		if(!pItem || pItem->m_Version != CMapItemVersion::CURRENT_VERSION)
			return false;

		// replace compressed tile layers with uncompressed ones
		int GroupsStart, GroupsNum, LayersStart, LayersNum;
		m_DataFile.GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
		m_DataFile.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
		for(int g = 0; g < GroupsNum; g++)
		{
			CMapItemGroup *pGroup = static_cast<CMapItemGroup *>(m_DataFile.GetItem(GroupsStart + g, 0, 0));
			for(int l = 0; l < pGroup->m_NumLayers; l++)
			{
				CMapItemLayer *pLayer = static_cast<CMapItemLayer *>(m_DataFile.GetItem(LayersStart + pGroup->m_StartLayer + l, 0, 0));

				if(pLayer->m_Type == LAYERTYPE_TILES)
				{
					CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
					
					if(pTilemap->m_Version > 3)
					{
						CTile *pTiles = static_cast<CTile *>(mem_alloc(pTilemap->m_Width * pTilemap->m_Height * sizeof(CTile), 1));

						// extract original tile data
						int i = 0;
						CTile *pSavedTiles = static_cast<CTile *>(m_DataFile.GetData(pTilemap->m_Data));
						while(i < pTilemap->m_Width * pTilemap->m_Height)
						{
							for(unsigned Counter = 0; Counter <= pSavedTiles->m_Skip && i < pTilemap->m_Width * pTilemap->m_Height; Counter++)
							{
								pTiles[i] = *pSavedTiles;
								pTiles[i++].m_Skip = 0;
							}

							pSavedTiles++;
						}

						m_DataFile.ReplaceData(pTilemap->m_Data, reinterpret_cast<char *>(pTiles));
					}
				}
			}
			
		}
		
		return true;
	}

	virtual bool IsLoaded()
	{
		return m_DataFile.IsOpen();
	}

	virtual unsigned Crc()
	{
		return m_DataFile.Crc();
	}
};

extern IEngineMap *CreateEngineMap() { return new CMap; }
