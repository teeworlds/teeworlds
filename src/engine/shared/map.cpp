/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>

#include <engine/map.h>
#include <engine/storage.h>

#include <game/mapitems.h>

#include "datafile.h"

class CMap : public IEngineMap
{
	CDataFileReader *m_pDataFile;
	char m_aError[128];

public:
	CMap()
	{
		m_pDataFile = new CDataFileReader;
		m_aError[0] = '\0';
	}

	~CMap()
	{
		Unload();
		delete m_pDataFile;
	}

	virtual void *GetData(int Index)
	{
		return m_pDataFile->GetData(Index);
	}

	virtual void *GetDataSwapped(int Index)
	{
		return m_pDataFile->GetDataSwapped(Index);
	}

	virtual bool GetDataString(int Index, char *pBuffer, int BufferSize)
	{
		return m_pDataFile->GetDataString(Index, pBuffer, BufferSize);
	}

	virtual int GetDataSize(int Index) const
	{
		return m_pDataFile->GetDataSize(Index);
	}

	virtual void UnloadData(int Index)
	{
		m_pDataFile->UnloadData(Index);
	}

	virtual void *GetItem(int Index, int *pType, int *pID, int *pSize) const
	{
		return m_pDataFile->GetItem(Index, pType, pID, pSize);
	}

	virtual int GetItemSize(int Index) const
	{
		return m_pDataFile->GetItemSize(Index);
	}

	virtual void GetType(int Type, int *pStart, int *pNum) const
	{
		m_pDataFile->GetType(Type, pStart, pNum);
	}

	virtual void *FindItem(int Type, int ID, int *pIndex, int *pSize) const
	{
		return m_pDataFile->FindItem(Type, ID, pIndex, pSize);
	}

	virtual int NumItems() const
	{
		return m_pDataFile->NumItems();
	}

	virtual int NumData() const
	{
		return m_pDataFile->NumData();
	}

	virtual void Unload()
	{
		m_pDataFile->Close();
	}

	bool LoadImpl(const char *pMapName, IStorage *pStorage, int StorageType)
	{
		if(!pStorage)
			pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
		{
			str_copy(m_aError, "IStorage not available", sizeof(m_aError));
			return false;
		}
		if(!m_pDataFile->Open(pStorage, pMapName, StorageType))
		{
			str_copy(m_aError, m_pDataFile->GetError(), sizeof(m_aError));
			return false;
		}

		// check version
		int VersionItemSize;
		const CMapItemVersion *pItem = static_cast<CMapItemVersion *>(m_pDataFile->FindItem(MAPITEMTYPE_VERSION, 0, 0, &VersionItemSize));
		if(!CMapItemChecker::IsValid(pItem, VersionItemSize))
		{
			str_format(m_aError, sizeof(m_aError), "incompatible map version");
			return false;
		}

		// Validate tile layers dimensions and
		// replace compressed tile layers with uncompressed ones
		int GroupsStart, GroupsNum, LayersStart, LayersNum;
		m_pDataFile->GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
		m_pDataFile->GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
		for(int g = 0; g < GroupsNum; g++)
		{
			int GroupItemSize;
			const CMapItemGroup *pGroup = static_cast<CMapItemGroup *>(m_pDataFile->GetItem(GroupsStart + g, 0x0, 0x0, &GroupItemSize));
			if(!CMapItemChecker::IsValid(pGroup, GroupItemSize) || pGroup->m_StartLayer >= LayersNum)
				continue;

			for(int l = 0; l < minimum(pGroup->m_NumLayers, LayersNum); l++)
			{
				int LayerItemSize;
				const CMapItemLayer *pLayer = static_cast<CMapItemLayer *>(m_pDataFile->GetItem(LayersStart + pGroup->m_StartLayer + l, 0x0, 0x0, &LayerItemSize));
				if(!CMapItemChecker::IsValid(pLayer, LayerItemSize))
					continue;

				if(pLayer->m_Type == LAYERTYPE_TILES)
				{
					const CMapItemLayerTilemap *pTilemap = reinterpret_cast<const CMapItemLayerTilemap *>(pLayer);
					if(!CMapItemChecker::IsValid(pTilemap, LayerItemSize) || pTilemap->m_Version < CMapItemLayerTilemap_v4::CURRENT_VERSION)
						continue;

					const int TilemapCount = pTilemap->m_Width * pTilemap->m_Height;
					const int TilemapSize = TilemapCount * sizeof(CTile);

					// extract original tile data
					const int NumSavedTiles = m_pDataFile->GetDataSize(pTilemap->m_Data) / sizeof(CTile);
					CTile *pSavedTiles = static_cast<CTile *>(m_pDataFile->GetData(pTilemap->m_Data));
					if(NumSavedTiles <= 0 || !pSavedTiles)
						continue;

					CTile *pTiles = static_cast<CTile *>(mem_alloc(TilemapSize));
					if(!pTiles)
						continue;

					for(int i = 0, j = 0; i < TilemapCount && j < NumSavedTiles; )
					{
						for(unsigned Counter = 0; Counter <= pSavedTiles[j].m_Skip && i < TilemapCount && j < NumSavedTiles; Counter++)
						{
							pTiles[i] = pSavedTiles[j];
							pTiles[i].m_Skip = 0;
							i++;
						}
						j++;
					}

					m_pDataFile->ReplaceData(pTilemap->m_Data, reinterpret_cast<char *>(pTiles), TilemapSize);
				}
			}
		}

		m_aError[0] = '\0';
		return true;
	}

	virtual bool Load(const char *pMapName, IStorage *pStorage, int StorageType)
	{
		CDataFileReader *pLastDataFile = m_pDataFile;
		m_pDataFile = new CDataFileReader;
		if(LoadImpl(pMapName, pStorage, StorageType))
		{
			delete pLastDataFile;
			return true;
		}
		else
		{
			delete m_pDataFile;
			m_pDataFile = pLastDataFile;
			return false;
		}
	}

	virtual bool IsLoaded() const
	{
		return m_pDataFile->IsOpen();
	}

	virtual const char *GetError() const
	{
		return m_aError;
	}

	virtual SHA256_DIGEST Sha256() const
	{
		return m_pDataFile->Sha256();
	}

	virtual unsigned Crc() const
	{
		return m_pDataFile->Crc();
	}

	virtual unsigned FileSize() const
	{
		return m_pDataFile->FileSize();
	}
};

extern IEngineMap *CreateEngineMap() { return new CMap; }
