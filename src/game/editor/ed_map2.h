#pragma once
#include <stdint.h>
#include <base/tl/array.h>
#include <base/vmath.h>
#include <engine/graphics.h>
#include <game/mapitems.h>

#include "auto_map2.h"

// TODO: move this out somewhere at some point
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template<typename T>
struct array2: public array<T>
{
	typedef array<T> ParentT;

	inline T& add(const T& Elt)
	{
		return this->base_ptr()[ParentT::add(Elt)];
	}

	T& add(const T* aElements, int EltCount)
	{
		for(int i = 0; i < EltCount; i++)
			ParentT::add(aElements[i]);
		return *(this->base_ptr()+this->size()-EltCount);
	}

	T& add_empty(int EltCount)
	{
		dbg_assert(EltCount > 0, "Add 0 or more");
		for(int i = 0; i < EltCount; i++)
			ParentT::add(T());
		return *(this->base_ptr()+this->size()-EltCount);
	}

	void clear()
	{
		// don't free buffer on clear
		this->num_elements = 0;
	}

	inline void remove_index_fast(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index_fast(Index);
	}

	// keeps order, way slower
	inline void remove_index(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index(Index);
	}

	inline T& operator[] (int Index)
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}

	inline const T& operator[] (int Index) const
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}
};

// Sparse array
// - Provides indirection between ID and data pointer so IDs are not invalidated
// - Data is packed
template<typename T, u32 MAX_ELEMENTS_>
struct CSparseArray
{
	enum
	{
		MAX_ELEMENTS = MAX_ELEMENTS_,
		INVALID_ID = 0xFFFF
	};

	u16 m_ID[MAX_ELEMENTS];
	u16 m_ReverseID[MAX_ELEMENTS];
	T m_Data[MAX_ELEMENTS];
	int m_Count;

	CSparseArray()
	{
		Clear();
	}

	u32 Push(const T& elt)
	{
		dbg_assert(m_Count < MAX_ELEMENTS, "Array is full");

		for(int i = 0; i < MAX_ELEMENTS; i++)
		{
			if(m_ID[i] == INVALID_ID)
			{
				const u16 dataID = m_Count++;
				m_ID[i] = dataID;
				m_ReverseID[dataID] = i;
				m_Data[dataID] = elt;
				return i;
			}
		}

		dbg_assert(0, "Failed to push element");
		return -1;
	}

	T& Set(u32 SlotID, const T& elt)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] == INVALID_ID) {
			dbg_assert(m_Count < MAX_ELEMENTS, "Array is full"); // should never happen
			const u16 dataID = m_Count++;
			m_ID[SlotID] = dataID;
			m_ReverseID[dataID] = SlotID;
		}
		return m_Data[m_ID[SlotID]] = elt;
	}

	T& Get(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");
		dbg_assert(m_ID[SlotID] != INVALID_ID, "Slot is empty");
		return m_Data[m_ID[SlotID]];
	}

	bool IsValid(u32 SlotID) const
	{
		if(SlotID >= MAX_ELEMENTS) return false;
		if(m_ID[SlotID] == INVALID_ID) return false;
#ifdef CONF_DEBUG
		const u16 dataID = m_ID[SlotID];
		return m_ReverseID[dataID] == SlotID;
#endif
		return true;
	}

	u32 GetIDFromData(const T& elt) const
	{
		int64 dataID = &elt - m_Data;
		dbg_assert(dataID >= 0 && dataID < MAX_ELEMENTS, "Element out of bounds");
		return m_ReverseID[dataID];
	}

	u32 GetDataID(u32 SlotID) const
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");
		dbg_assert(m_ID[SlotID] != INVALID_ID, "Slot is empty");
		return m_ID[SlotID];
	}

	void RemoveByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(m_Count > 0, "Array is empty"); // should never happen
			const u16 dataID = m_ID[SlotID];

			const int lastDataID = m_Count-1;
			const u16 lastID = m_ReverseID[lastDataID];
			m_ID[lastID] = dataID;
			tl_swap(m_ReverseID[dataID], m_ReverseID[lastDataID]);
			tl_swap(m_Data[dataID], m_Data[lastDataID]);
			m_Count--;

			m_ID[SlotID] = INVALID_ID;
		}
	}

	void RemoveKeepOrderByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(m_Count > 0, "Array is empty"); // should never happen


			dbg_assert(0, "Implement me");
		}
	}

	void Clear()
	{
		memset(m_ID, 0xFF, sizeof(m_ID));
		m_Count = 0;
	}

	int Count() const
	{
		return m_Count;
	}

	/*
	inline T& operator [] (u32 ID) const
	{
		return Get(ID);
	}
	*/

	T* Data()
	{
		return m_Data;
	}
};

#define ARR_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))
// --------------------------------------------------------------------------




class IStorage;
class IGraphics;
class IConsole;

struct CEditorMap2
{
	enum
	{
		MAX_IMAGES=128,
		MAX_GROUP_LAYERS=64,
		MAX_GROUPS=128,
		MAX_IMAGE_NAME_LEN=64,
		MAX_EMBEDDED_FILES=64,
	};

	// TODO: split this into CTileLayer and CQuadLayer (make one array of each to allocate them)
	struct CLayer
	{
		char m_aName[12];
		int m_Type;
		int m_ImageID;
		bool m_HighDetail; // TODO: turn into m_Flags ?
		vec4 m_Color;


		// tile
		array2<CTile> m_aTiles;
		int m_Width;
		int m_Height;
		int m_ColorEnvelopeID;
		int m_ColorEnvOffset;


		// quad
		array2<CQuad> m_aQuads;

		inline bool IsTileLayer() const { return m_Type == LAYERTYPE_TILES; }
		inline bool IsQuadLayer() const { return m_Type == LAYERTYPE_QUADS; }

		CLayer()
		{
			str_copy(m_aName, "unknown", sizeof(m_aName));
			m_Type = LAYERTYPE_INVALID;
			m_ImageID = 0;
			m_HighDetail = false;
			m_Color = vec4(1,1,1,1);
			m_Width = 0;
			m_Height = 0;
			m_ColorEnvelopeID = -1;
			m_ColorEnvOffset = 0;
		}
	};

	struct CGroup
	{
		char m_aName[12];
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount;
		int m_ParallaxX;
		int m_ParallaxY;
		int m_OffsetX;
		int m_OffsetY;
		int m_ClipX;
		int m_ClipY;
		int m_ClipWidth;
		int m_ClipHeight;
		bool m_UseClipping;

		CGroup()
		{
			m_aName[0] = 0;
			m_LayerCount = 0;
			m_ParallaxX = 0;
			m_ParallaxY = 0;
			m_OffsetX = 0;
			m_OffsetY = 0;
			m_ClipX = 0;
			m_ClipY = 0;
			m_ClipWidth = 0;
			m_ClipHeight = 0;
			m_UseClipping = false;
		}
	};

	struct CEnvelope
	{
		int m_Version;
		int m_Channels;
		CEnvPoint* m_aPoints;
		int m_NumPoints;
		//int m_aName[8];
		bool m_Synchronized;
	};

	struct CImageName
	{
		char m_Buff[MAX_IMAGE_NAME_LEN];
	};

	struct CEmbeddedFile
	{
		u32 m_Crc;
		int m_Type; // unused for now (only images)
		void* m_pData;
	};

	struct CAssets
	{
		// TODO: pack some of these together
		CImageName m_aImageNames[MAX_IMAGES];
		u32 m_aImageNameHash[MAX_IMAGES];
		u32 m_aImageEmbeddedCrc[MAX_IMAGES];
		IGraphics::CTextureHandle m_aTextureHandle[MAX_IMAGES];
		CImageInfo m_aTextureInfos[MAX_IMAGES];
		int m_ImageCount;

		CEmbeddedFile m_aEmbeddedFile[MAX_EMBEDDED_FILES];
		int m_EmbeddedFileCount;

		array<u32> m_aAutomapTileHashID;
		array<CTilesetMapper2> m_aAutomapTile;
	};

	// used for undo/redo
	struct CSnapshot
	{
		int m_GroupCount;
		int m_LayerCount;
		int m_EnvelopeCount;
		int m_ImageCount;
		int m_GameLayerID;
		int m_GameGroupID;
		CImageName* m_aImageNames;
		u32* m_aImageNameHash;
		u32* m_aImageEmbeddedCrc;
		CImageInfo* m_aImageInfos;
		CGroup* m_aGroups;
		CMapItemLayer** m_apLayers;
		u32* m_aGroupIDs;
		u32* m_aLayerIDs;
		u32 m_aGroupIDList[MAX_GROUPS];
		int m_GroupIDListCount;
		CMapItemEnvelope* m_aEnvelopes;
		CTile* m_aTiles;
		CQuad* m_aQuads;
		CEnvPoint* m_aEnvPoints;
		u8 m_Data[1];
	};

	int m_MapMaxWidth;
	int m_MapMaxHeight;
	int m_GameLayerID;
	int m_GameGroupID;

	char m_aPath[256];

	array2<CEnvPoint> m_aEnvPoints;
	CSparseArray<CLayer,1024> m_aLayers;
	CSparseArray<CGroup,256> m_aGroups;
	array2<CMapItemEnvelope> m_aEnvelopes;

	u32 m_aGroupIDList[MAX_GROUPS];
	int m_GroupIDListCount;

	CAssets m_Assets;

	IGraphics* m_pGraphics;
	IConsole *m_pConsole;
	IStorage *m_pStorage;

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline IConsole *Console() { return m_pConsole; };
	inline IStorage *Storage() { return m_pStorage; };

	void Init(IStorage *pStorage, IGraphics* pGraphics, IConsole* pConsole);
	bool Save(const char *pFileName);
	bool Load(const char *pFileName);
	void LoadDefault();
	void Clear();

	// loads not-loaded images, clears the rest
	void AssetsClearAndSetImages(CImageName* aName, CImageInfo* aInfo, u32* aImageEmbeddedCrc, int ImageCount);
	u32 AssetsAddEmbeddedData(void* pData, u64 DataSize);
	void AssetsClearEmbeddedFiles();
	bool AssetsAddAndLoadImage(const char* pFilename); // also loads automap rules if there are any
	void AssetsDeleteImage(int ImgID);
	void AssetsLoadAutomapFileForImage(int ImgID);
	void AssetsLoadMissingAutomapFiles();
	CTilesetMapper2* AssetsFindTilesetMapper(int ImgID);

	CSnapshot* SaveSnapshot();
	void RestoreSnapshot(const CSnapshot* pSnapshot);
#ifdef CONF_DEBUG
	void CompareSnapshot(const CSnapshot* pSnapshot);
#endif

	CLayer& NewTileLayer(int Width, int Height, u32* pOutID = 0x0);
	CLayer& NewQuadLayer(u32* pOutID = 0x0);
};
