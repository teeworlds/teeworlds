#pragma once
#include <string.h> // memset
#include <base/vmath.h>
#include <engine/graphics.h>
#include <game/mapitems.h>

#include "ed2_common.h"
#include "auto_map2.h"


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
		MAX_LAYERS=1024,
		MAX_IMAGE_NAME_LEN=64,
		MAX_EMBEDDED_FILES=64,
	};

	struct Limits
	{
		static const int GroupParallaxMin = -9999;
		static const int GroupParallaxMax = 9999;
		static const int GroupOffsetMin = -999999;
		static const int GroupOffsetMax = 999999; // this breaks when high enough
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
	CSparseArray<CLayer,MAX_LAYERS> m_aLayers;
	CSparseArray<CGroup,MAX_GROUPS> m_aGroups;
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
	bool AssetsAddAndLoadImage(const char* pFilePath); // also loads automap rules if there are any
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
