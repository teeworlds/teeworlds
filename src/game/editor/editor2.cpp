// LordSk
#include "editor2.h"

#include <zlib.h> // crc32
#include <stdio.h> // sscanf
#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

//#include <intrin.h>

// TODO:
// - Easily know if we're clicking on UI or elsewhere
// ---- what event gets handled where should be VERY clear

// - Allow brush to go in eraser mode, automapper mode
// - Binds
// - Smooth zoom
// - Envelope offset

// - Replace a lot of the static count arrays with dynamic ones (with a stack base)
// - Localize everything

// - Stability is very important (crashes should be easy to catch)
// --- fix layer id / image id handling
// - Input should be handled well (careful of input-locks https://github.com/teeworlds/teeworlds/issues/828)
// ! Do not use DoButton_SpriteClean (dunno why)

static char s_aEdMsg[256];
#define ed_log(...)\
	str_format(s_aEdMsg, sizeof(s_aEdMsg), ##__VA_ARGS__);\
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", s_aEdMsg);

#ifdef CONF_DEBUG
	#define ed_dbg(...)\
		str_format(s_aEdMsg, sizeof(s_aEdMsg), ##__VA_ARGS__);\
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", s_aEdMsg);
#else
	#define ed_dbg(...)
	#undef dbg_assert
	#define dbg_assert(test,msg)
#endif

const vec4 StyleColorBg(0.03, 0, 0.085, 1);
const vec4 StyleColorButton(0.062, 0, 0.19, 1);
const vec4 StyleColorButtonBorder(0.18, 0.00, 0.56, 1);
const vec4 StyleColorButtonHover(0.28, 0.10, 0.64, 1);
const vec4 StyleColorButtonPressed(0.13, 0, 0.40, 1);
const vec4 StyleColorTileSelection(0.0, 0.31, 1, 0.4);
const vec4 StyleColorTileHover(1, 1, 1, 0.25);
const vec4 StyleColorTileHoverBorder(0.0, 0.31, 1, 1);

inline float fract(float f)
{
	return f - (int)f;
}

inline int floor(float f)
{
	return f < 0 ? (int)f-1 : (int)f;
}

inline bool IsInsideRect(vec2 Pos, CUIRect Rect)
{
	return (Pos.x >= Rect.x && Pos.x < (Rect.x+Rect.w) &&
			Pos.y >= Rect.y && Pos.y < (Rect.y+Rect.h));
}

// hash
inline u32 fnv1a32(const void* data, u32 dataSize)
{
	u32 hash = 2166136261;
	for(u32 i = 0; i < dataSize; ++i)
		hash = (hash ^ ((const char*)data)[i]) * 16777619;
	return hash;
}

void CEditorMap::Init(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole)
{
	m_pGraphics = pGraphics;
	m_pStorage = pStorage;
	m_pConsole = pConsole;

	m_TileDispenser.Init(2000000, 100);
	m_QuadDispenser.Init(10000, 5);
	m_EnvPointDispenser.Init(100000, 10);
	m_LayerDispenser.Init(1000);
	m_GroupDispenser.Init(200);
	m_EnvelopeDispenser.Init(1000);
	ed_log("m_TileDispenser.AllocatedSize=%lldMb", m_TileDispenser.AllocatedSize()/(1024*1024));
	ed_log("m_QuadDispenser.AllocatedSize=%lldMb", m_QuadDispenser.AllocatedSize()/(1024*1024));
	ed_log("m_EnvPointDispenser.AllocatedSize=%lldMb", m_EnvPointDispenser.AllocatedSize()/(1024*1024));
	ed_log("m_LayerDispenser.AllocatedSize=%lldMb", m_LayerDispenser.AllocatedSize()/(1024*1024));
	ed_log("m_GroupDispenser.AllocatedSize=%lldMb", m_GroupDispenser.AllocatedSize()/(1024*1024));
	ed_log("m_EnvelopeDispenser.AllocatedSize=%lldMb", m_EnvelopeDispenser.AllocatedSize()/(1024*1024));

	m_aEnvPoints.Init(&m_EnvPointDispenser);
	m_aLayers.Init(&m_LayerDispenser);
	m_aGroups.Init(&m_GroupDispenser);
	m_aEnvelopes.Init(&m_EnvelopeDispenser);

	// speedtest
#if 0
	CChainAllocator<CTile> TestDispenser;
	TestDispenser.Init(1000000, 10);

	const int TestLoopCount = 1000000;
	uint64_t StartCycles = __rdtsc();
	uint64_t MallocCycles, ChainCycles;

	int DummyVar = 0;
	for(int i = 0; i < TestLoopCount; i++)
	{
		CTile* pBuff = (CTile*)malloc(sizeof(CTile) * ((i%1000)+1));
		mem_zero(pBuff, sizeof(CTile) * ((i%1000)+1));
		DummyVar += pBuff[0].m_Index;
		free(pBuff);
	}

	MallocCycles = __rdtsc() - StartCycles;
	StartCycles = __rdtsc();

	for(int i = 0; i < TestLoopCount; i++)
	{
		CMemBlock<CTile> Block = TestDispenser.Alloc((i%1000)+1);
		DummyVar += Block.Get()[0].m_Index;
		TestDispenser.Dealloc(&Block);
	}

	ChainCycles = __rdtsc() - StartCycles;

	ed_log("DummyVar=%d MallocAvgCycles=%llu ChainAvgCycles=%llu", DummyVar, MallocCycles/TestLoopCount,
		   ChainCycles/TestLoopCount);

	const int TestLoopCount2 = 1000;
	array<CTile> BaseTileArray;
	CArray<CTile> OurTileArray;
	OurTileArray.Init(&TestDispenser, 2);
	StartCycles = __rdtsc();

	for(int i = 0; i < TestLoopCount2; i++)
	{
		CTile t;
		t.m_Index = i;
		BaseTileArray.add(t);
		DummyVar += BaseTileArray.size();
	}

	MallocCycles = __rdtsc() - StartCycles;
	StartCycles = __rdtsc();

	for(int i = 0; i < TestLoopCount2; i++)
	{
		CTile t;
		t.m_Index = i;
		OurTileArray.Add(t);
		DummyVar += OurTileArray.Count();
	}

	ChainCycles = __rdtsc() - StartCycles;

	ed_log("ARRAYS DummyVar=%d MallocAvgCycles=%llu ChainAvgCycles=%llu", DummyVar,
		   MallocCycles/TestLoopCount2,
		   ChainCycles/TestLoopCount2);

#endif

	// test
#if 0
	CChainAllocator<CTile, 100, 8> TestDispenser;
	TestDispenser.Init();
	CMemBlock<CTile> Block = TestDispenser.Alloc(57);
	TestDispenser.Dealloc(Block);

	Block = TestDispenser.Alloc(57);
	CMemBlock<CTile> Block2 = TestDispenser.Alloc(18);
	TestDispenser.Dealloc(Block);
	Block = TestDispenser.Alloc(57);
	TestDispenser.Dealloc(Block2);
#endif
}

bool CEditorMap::Save(const char* pFileName)
{
	return false;
}

bool CEditorMap::Load(const char* pFileName)
{
	CDataFileReader File;
	if(!File.Open(m_pStorage, pFileName, IStorage::TYPE_ALL))
		return false;
	// check version
	CMapItemVersion *pItem = (CMapItemVersion *)File.FindItem(MAPITEMTYPE_VERSION, 0);
	if(!pItem || pItem->m_Version != CMapItemVersion::CURRENT_VERSION)
		return false;

	const int FilenameLen = str_length(pFileName);
	mem_copy(m_aPath, pFileName, FilenameLen);
	m_aPath[FilenameLen] = 0;

	Clear();
	AssetsClearEmbeddedFiles();

	int GroupsStart, GroupsNum;
	int LayersStart, LayersNum;
	File.GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
	File.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);

	ed_dbg("GroupsStart=%d GroupsNum=%d LayersStart=%d LayersNum=%d",
			GroupsStart, GroupsNum, LayersStart, LayersNum);

	for(int gi = 0; gi < GroupsNum; gi++)
	{
		CMapItemGroup* pGroup = (CMapItemGroup*)File.GetItem(GroupsStart+gi, 0, 0);
		ed_dbg("Group#%d NumLayers=%d Offset=(%d, %d)", gi, pGroup->m_NumLayers,
				pGroup->m_OffsetX, pGroup->m_OffsetY);
		const int GroupLayerCount = pGroup->m_NumLayers;
		const int GroupLayerStart = pGroup->m_StartLayer;
		CEditorMap::CGroup Group;
		Group.m_ParallaxX = pGroup->m_ParallaxX;
		Group.m_ParallaxY = pGroup->m_ParallaxY;
		Group.m_OffsetX = pGroup->m_OffsetX;
		Group.m_OffsetY = pGroup->m_OffsetY;
		Group.m_LayerCount = 0;

		for(int li = 0; li < GroupLayerCount; li++)
		{
			CMapItemLayer* pLayer = (CMapItemLayer*)File.GetItem(LayersStart+GroupLayerStart+li, 0, 0);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				const CMapItemLayerTilemap& Tilemap = *(CMapItemLayerTilemap*)pLayer;
				ed_dbg("Group#%d Layer=%d (w=%d, h=%d)", gi, li,
						Tilemap.m_Width, Tilemap.m_Height);

				m_MapMaxWidth = max(m_MapMaxWidth, Tilemap.m_Width);
				m_MapMaxHeight = max(m_MapMaxHeight, Tilemap.m_Height);

				CLayer LayerTile;
				LayerTile.m_Type = LAYERTYPE_TILES;
				LayerTile.m_ImageID = Tilemap.m_Image;
				LayerTile.m_Width = Tilemap.m_Width;
				LayerTile.m_Height = Tilemap.m_Height;
				LayerTile.m_ColorEnvelopeID = Tilemap.m_ColorEnv;
				LayerTile.m_Color = vec4(Tilemap.m_Color.r/255.f, Tilemap.m_Color.g/255.f,
										 Tilemap.m_Color.b/255.f, Tilemap.m_Color.a/255.f);
				LayerTile.m_aTiles = NewTileArray();

				CTile *pTiles = (CTile *)File.GetData(Tilemap.m_Data);
				int TileCount = Tilemap.m_Width*Tilemap.m_Height;
				for(int ti = 0; ti < TileCount; ti++)
				{
					CTile Tile = pTiles[ti];
					const int Skips = Tile.m_Skip;
					Tile.m_Skip = 0;

					for(int s = 0; s < Skips; s++)
						LayerTile.m_aTiles.Add(Tile);
					LayerTile.m_aTiles.Add(Tile);

					TileCount -= Skips;
				}

				const int LayerID = m_aLayers.Count();
				m_aLayers.Add(LayerTile);

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;

				if(Tilemap.m_Flags&TILESLAYERFLAG_GAME)
				{
					m_GameLayerID = LayerID;
					m_GameGroupID = m_aGroups.Count();
				}
			}
			else if(pLayer->m_Type == LAYERTYPE_QUADS)
			{
				const CMapItemLayerQuads& ItemQuadLayer = *(CMapItemLayerQuads*)pLayer;

				CLayer LayerQuad;
				LayerQuad.m_Type = LAYERTYPE_QUADS;
				LayerQuad.m_ImageID = ItemQuadLayer.m_Image;

				CQuad *pQuads = (CQuad *)File.GetData(ItemQuadLayer.m_Data);
				LayerQuad.m_aQuads = NewQuadArray();
				LayerQuad.m_aQuads.Add(pQuads, ItemQuadLayer.m_NumQuads);

				const int LayerID = m_aLayers.Count();
				m_aLayers.Add(LayerQuad);

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;
			}
		}

		m_aGroups.Add(Group);
	}

	dbg_assert(m_GameLayerID >= 0, "Game layer not found");
	dbg_assert(m_GameGroupID >= 0, "Game group not found");

	// load envelopes
	int EnvelopeStart, EnvelopeCount;
	int EnvPointStart, EnvPointCount;
	File.GetType(MAPITEMTYPE_ENVELOPE, &EnvelopeStart, &EnvelopeCount);
	File.GetType(MAPITEMTYPE_ENVPOINTS, &EnvPointStart, &EnvPointCount);
	// FIXME: EnvPointCount is always 1?

	ed_dbg("EnvelopeStart=%d EnvelopeCount=%d EnvPointStart=%d EnvPointCount=%d",
			EnvelopeStart, EnvelopeCount, EnvPointStart, EnvPointCount);

	CEnvPoint* pEnvPoints = (CEnvPoint*)File.GetItem(EnvPointStart, 0, 0);

	// envelopes
	for(int ei = 0; ei < EnvelopeCount; ei++)
	{
		CMapItemEnvelope *pItem = (CMapItemEnvelope *)File.GetItem(EnvelopeStart+ei, 0, 0);
		const CMapItemEnvelope Env = *pItem;
		m_aEnvelopes.Add(Env);

		if(Env.m_Version >= 3)
		{
			m_aEnvPoints.Add(&pEnvPoints[Env.m_StartPoint], Env.m_NumPoints);
		}
		else
		{
			// backwards compatibility
			for(int i = 0; i < Env.m_NumPoints; i++)
			{
				// convert CEnvPoint_v1 -> CEnvPoint
				CEnvPoint_v1 *pEnvPoint_v1 = &((CEnvPoint_v1 *)pEnvPoints)[i + Env.m_StartPoint];
				CEnvPoint Point;
				mem_zero(&Point, sizeof(Point));
				mem_copy(&Point, pEnvPoint_v1, sizeof(CEnvPoint_v1));
				m_aEnvPoints.Add(Point);
			}
		}
	}

	// load textures
	int ImagesStart, ImagesCount;
	File.GetType(MAPITEMTYPE_IMAGE, &ImagesStart, &ImagesCount);
	CImageName aImageName[MAX_IMAGES];
	CImageInfo aImageInfo[MAX_IMAGES];
	u32 aImageEmbeddedCrc[MAX_IMAGES];

	for(int i = 0; i < ImagesCount && i < MAX_IMAGES; i++)
	{
		mem_zero(&aImageName[i], sizeof(aImageName[i]));
		CMapItemImage *pImg = (CMapItemImage *)File.GetItem(ImagesStart+i, 0, 0);
		const char *pImgName = (char *)File.GetData(pImg->m_ImageName);
		mem_copy(&aImageName[i], pImgName, min((u64)str_length(pImgName), sizeof(aImageName[i].m_Buff)-1));
		aImageInfo[i].m_pData = nullptr;
		aImageInfo[i].m_Width = pImg->m_Width;
		aImageInfo[i].m_Height = pImg->m_Height;

		if(pImg->m_External)
		{
			aImageEmbeddedCrc[i] = 0; // external
			aImageInfo[i].m_Format = CImageInfo::FORMAT_AUTO;
		}
		else
		{
			unsigned long DataSize = 0;
			void *pData = File.GetData(pImg->m_ImageData, &DataSize);

			// save embedded data
			aImageEmbeddedCrc[i] = AssetsAddEmbeddedData(pData, DataSize);
			aImageInfo[i].m_Format = pImg->m_Version == 1 ? CImageInfo::FORMAT_RGBA : pImg->m_Format;
		}
	}

	AssetsClearAndSetImages(aImageName, aImageInfo, aImageEmbeddedCrc, ImagesCount);

	return true;
}

void CEditorMap::LoadDefault()
{
	Clear();

	CGroup BgGroup;
	BgGroup.m_OffsetX = 0;
	BgGroup.m_OffsetY = 0;
	BgGroup.m_ParallaxX = 0;
	BgGroup.m_ParallaxY = 0;

	CLayer& BgQuadLayer = NewQuadLayer();

	CQuad SkyQuad;
	SkyQuad.m_ColorEnv = -1;
	SkyQuad.m_PosEnv = -1;
	const int Width = i2fx(800);
	const int Height = i2fx(600);
	SkyQuad.m_aPoints[0].x = SkyQuad.m_aPoints[2].x = -Width;
	SkyQuad.m_aPoints[1].x = SkyQuad.m_aPoints[3].x = Width;
	SkyQuad.m_aPoints[0].y = SkyQuad.m_aPoints[1].y = -Height;
	SkyQuad.m_aPoints[2].y = SkyQuad.m_aPoints[3].y = Height;
	SkyQuad.m_aPoints[4].x = SkyQuad.m_aPoints[4].y = 0;
	SkyQuad.m_aColors[0].r = SkyQuad.m_aColors[1].r = 94;
	SkyQuad.m_aColors[0].g = SkyQuad.m_aColors[1].g = 132;
	SkyQuad.m_aColors[0].b = SkyQuad.m_aColors[1].b = 174;
	SkyQuad.m_aColors[2].r = SkyQuad.m_aColors[3].r = 204;
	SkyQuad.m_aColors[2].g = SkyQuad.m_aColors[3].g = 232;
	SkyQuad.m_aColors[2].b = SkyQuad.m_aColors[3].b = 255;
	SkyQuad.m_aColors[0].a = SkyQuad.m_aColors[1].a = 255;
	SkyQuad.m_aColors[2].a = SkyQuad.m_aColors[3].a = 255;
	BgQuadLayer.m_aQuads.Add(SkyQuad);

	BgGroup.m_apLayerIDs[BgGroup.m_LayerCount++] = m_aLayers.Count()-1;
	m_aGroups.Add(BgGroup);

	CGroup GameGroup;
	GameGroup.m_OffsetX = 0;
	GameGroup.m_OffsetY = 0;
	GameGroup.m_ParallaxX = 100;
	GameGroup.m_ParallaxY = 100;

	CLayer& Gamelayer = NewTileLayer(50, 50);

	GameGroup.m_apLayerIDs[GameGroup.m_LayerCount++] = m_aLayers.Count()-1;
	m_aGroups.Add(GameGroup);

	m_GameGroupID = m_aGroups.Count()-1;
	m_GameLayerID = GameGroup.m_apLayerIDs[0];
}

void CEditorMap::Clear()
{
	m_GameLayerID = -1;
	m_GameGroupID = -1;
	m_MapMaxWidth = 0;
	m_MapMaxHeight = 0;

	// release tiles and quads
	const int LayerCount = m_aLayers.Count();
	for(int li = 0; li < LayerCount; li++)
	{
		if(m_aLayers[li].IsTileLayer())
			m_aLayers[li].m_aTiles.Clear();
		else
			m_aLayers[li].m_aQuads.Clear();
	}

	m_aLayers.Clear();
	m_aGroups.Clear();
	m_aEnvelopes.Clear();
	m_aEnvPoints.Clear();
}

void CEditorMap::AssetsClearAndSetImages(CEditorMap::CImageName* aName, CImageInfo* aInfo,
	u32* aImageEmbeddedCrc, int ImageCount)
{
	dbg_assert(ImageCount <= MAX_IMAGES, "ImageCount > MAX_IMAGES");

	IGraphics::CTextureHandle aKeepTexHandle[MAX_IMAGES];
	for(int ni = 0; ni < ImageCount; ni++)
		aKeepTexHandle[ni].Invalidate();

	for(int i = 0; i < m_Assets.m_ImageCount; i++)
	{
		bool Found = false;
		for(int ni = 0; ni < ImageCount; ni++)
		{
			if(m_Assets.m_aImageEmbeddedCrc[i] == aImageEmbeddedCrc[ni] &&
			   mem_comp(m_Assets.m_aImageNames[i].m_Buff, aName[ni].m_Buff, sizeof(aName[ni].m_Buff)) == 0)
			{
				aKeepTexHandle[ni] = m_Assets.m_aTextureHandle[i];
				Found = true;
				break;
			}
		}

		if(!Found)
			m_pGraphics->UnloadTexture(&m_Assets.m_aTextureHandle[i]);
	}

	m_Assets.m_ImageCount = ImageCount;

	memmove(m_Assets.m_aImageNames, aName, sizeof(aName[0]) * ImageCount);
	memmove(m_Assets.m_aTextureInfos, aInfo, sizeof(aInfo[0]) * ImageCount);
	memmove(m_Assets.m_aImageEmbeddedCrc, aImageEmbeddedCrc, sizeof(aImageEmbeddedCrc[0]) * ImageCount);

	for(int i = 0; i < ImageCount && i < MAX_IMAGES; i++)
	{
		const int TextureFlags = IGraphics::TEXLOAD_MULTI_DIMENSION;

		if(aKeepTexHandle[i].IsValid())
		{
			ed_dbg("%s kept", aName[i].m_Buff);
			m_Assets.m_aTextureHandle[i] = aKeepTexHandle[i];
			continue;
		}

		if(m_Assets.m_aImageEmbeddedCrc[i] == 0) // external
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "mapres/%s.png", aName[i].m_Buff);
			m_Assets.m_aTextureHandle[i] = m_pGraphics->LoadTexture(aBuf, IStorage::TYPE_ALL,
				CImageInfo::FORMAT_AUTO, TextureFlags);
			m_Assets.m_aTextureInfos[i].m_Format = CImageInfo::FORMAT_AUTO;
			ed_dbg("%s loaded", aBuf);
		}
		else
		{
			const u32 Crc = aImageEmbeddedCrc[i];
			const int EmbeddedFileCount = m_Assets.m_EmbeddedFileCount;
			CEmbeddedFile* pFile = nullptr;
			for(int f = 0; f < EmbeddedFileCount; f++)
			{
				if(m_Assets.m_aEmbeddedFile[f].m_Crc == Crc)
				{
					pFile = &m_Assets.m_aEmbeddedFile[f];
					break;
				}
			}

			// embedded file should be found
			dbg_assert(pFile != nullptr, "Embedded file not found");

			m_Assets.m_aTextureHandle[i] = m_pGraphics->LoadTextureRaw(aInfo[i].m_Width, aInfo[i].m_Height,
				aInfo[i].m_Format, pFile->m_pData, CImageInfo::FORMAT_RGBA, TextureFlags);
		}

		m_Assets.m_aImageNameHash[i] = fnv1a32(&m_Assets.m_aImageNames[i],
			sizeof(m_Assets.m_aImageNames[i]));

		dbg_assert(m_Assets.m_aTextureHandle[i].IsValid(), "Invalid texture");
	}
}

u32 CEditorMap::AssetsAddEmbeddedData(void* pData, u64 DataSize)
{
	// 10 Mb limit
	dbg_assert(DataSize < (u64)(1024*1024*10), "DataSize is invalid");
	void* pDataCopy = mem_alloc(DataSize, 0);
	memmove(pDataCopy, pData, DataSize);

	CEmbeddedFile EmbeddedFile;
	u32 Crc = crc32(0L, 0x0, 0);
	Crc = crc32(Crc, (u8*)pData, DataSize);
	EmbeddedFile.m_Crc = Crc;
	EmbeddedFile.m_Type = 0;
	EmbeddedFile.m_pData = pDataCopy;
	m_Assets.m_aEmbeddedFile[m_Assets.m_EmbeddedFileCount++] = EmbeddedFile;
	return Crc;
}

void CEditorMap::AssetsClearEmbeddedFiles()
{
	for(int i = 0; i < m_Assets.m_EmbeddedFileCount; i++)
	{
		if(m_Assets.m_aEmbeddedFile[i].m_pData)
		{
			free(m_Assets.m_aEmbeddedFile[i].m_pData);
			m_Assets.m_aEmbeddedFile[i].m_pData = nullptr;
		}
	}
	m_Assets.m_EmbeddedFileCount = 0;
}

bool CEditorMap::AssetsAddAndLoadImage(const char* pFilename)
{
	// TODO: return error ID
	if(m_Assets.m_ImageCount > MAX_IMAGES-1)
		return false;

	const int StrLen = str_length(pFilename);
	const bool EndsWithPng = StrLen > 4 && str_comp_nocase_num(pFilename+StrLen-4, ".png", 4) == 0;
	if(!EndsWithPng)
	{
		// TODO: can we load anything other than png files?
		ed_dbg("ERROR: '%s' image file not supported", pFilename);
		return false;
	}


	CImageName ImgName;
	mem_zero(&ImgName, sizeof(ImgName));
	const int NameLen = min(StrLen-4, (int)sizeof(ImgName.m_Buff)-1);
	memmove(ImgName.m_Buff, pFilename, NameLen);
	ImgName.m_Buff[NameLen] = 0;
	const u32 NameHash = fnv1a32(&ImgName, sizeof(ImgName));

	// find image by name
	const int ImageCount = m_Assets.m_ImageCount;
	for(int i = 0; i < ImageCount; i++)
	{
		if(m_Assets.m_aImageNameHash[i] == NameHash)
		{
			ed_log("'%s' image file already loaded", pFilename);
			return false;
		}
	}

	const int ImgID = m_Assets.m_ImageCount++;
	m_Assets.m_aImageNames[ImgID] = ImgName;
	m_Assets.m_aImageNameHash[ImgID] = NameHash;
	m_Assets.m_aImageEmbeddedCrc[ImgID] = 0;

	char aFilePath[256];
	str_format(aFilePath, sizeof(aFilePath), "mapres/%s", pFilename);

	CImageInfo ImgInfo;
	IGraphics::CTextureHandle TexHnd;
	if(Graphics()->LoadPNG(&ImgInfo, aFilePath, IStorage::TYPE_ALL))
	{
		const int TextureFlags = IGraphics::TEXLOAD_MULTI_DIMENSION;
		ImgInfo.m_Format = CImageInfo::FORMAT_AUTO;
		TexHnd = Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format,
			ImgInfo.m_pData, ImgInfo.m_Format, TextureFlags);
		mem_free(ImgInfo.m_pData); // TODO: keep this?
		ImgInfo.m_pData = NULL;

		if(!TexHnd.IsValid())
		{
			ed_dbg("LoadTextureRaw ERROR: could not load '%s' image file", aFilePath);
			m_Assets.m_ImageCount--;
			return false;
		}
	}
	else
	{
		ed_dbg("LoadPNG ERROR: could not load '%s' image file", aFilePath);
		m_Assets.m_ImageCount--;
		return false;
	}

	m_Assets.m_aTextureHandle[ImgID] = TexHnd;
	m_Assets.m_aTextureInfos[ImgID] = ImgInfo;
	ed_dbg("Image '%s' loaded", aFilePath);
	return true;
}

void CEditorMap::AssetsDeleteImage(int ImgID)
{
	Graphics()->UnloadTexture(&m_Assets.m_aTextureHandle[ImgID]);
	const int SwappedID = m_Assets.m_ImageCount-1;
	swap(m_Assets.m_aImageNames[ImgID], m_Assets.m_aImageNames[SwappedID]);
	swap(m_Assets.m_aImageEmbeddedCrc[ImgID], m_Assets.m_aImageEmbeddedCrc[SwappedID]);
	swap(m_Assets.m_aTextureHandle[ImgID], m_Assets.m_aTextureHandle[SwappedID]);
	swap(m_Assets.m_aTextureInfos[ImgID], m_Assets.m_aTextureInfos[SwappedID]);
	m_Assets.m_ImageCount--;

	const int LayerCount = m_aLayers.Count();
	for(int li = 0; li < LayerCount; li++)
	{
		if(m_aLayers[li].m_ImageID == ImgID)
			m_aLayers[li].m_ImageID = -1;
		if(m_aLayers[li].m_ImageID == SwappedID)
			m_aLayers[li].m_ImageID = ImgID;
	}
}

CEditorMap::CSnapshot* CEditorMap::SaveSnapshot()
{
	// calculate snapshot size
	u64 SnapSize = sizeof(CSnapshot)-1;

	const int GroupCount = m_aGroups.Count();
	SnapSize += sizeof(CGroup) * GroupCount;

	const int LayerCount = m_aLayers.Count();
	SnapSize += sizeof(CMapItemLayer*) * LayerCount;
	for(int li = 0; li < LayerCount; li++)
	{
		const CLayer& Layer = m_aLayers[li];
		if(Layer.IsTileLayer())
		{
			SnapSize += sizeof(CMapItemLayerTilemap);
			SnapSize += sizeof(CTile) * Layer.m_aTiles.Count();
		}
		else
		{
			SnapSize += sizeof(CMapItemLayerQuads);
			SnapSize += sizeof(CQuad) * Layer.m_aQuads.Count();
		}
	}

	const int EnvelopeCount = m_aEnvelopes.Count();
	SnapSize += sizeof(CMapItemEnvelope) * EnvelopeCount;

	for(int ei = 0; ei < EnvelopeCount; ei++)
	{
		SnapSize += sizeof(CEnvPoint) * m_aEnvelopes[ei].m_NumPoints;
	}

	const int ImageCount = m_Assets.m_ImageCount;
	SnapSize += sizeof(CSnapshot::m_aImageNames[0]) * ImageCount; // m_aImageNames
	SnapSize += sizeof(CSnapshot::m_aImageInfos[0]) * ImageCount; // m_aImageInfos
	SnapSize += sizeof(CSnapshot::m_aImageEmbeddedCrc[0]) * ImageCount; // m_aImageEmbeddedCrc

	ed_dbg("Map snapshot size = %lluKo", SnapSize/1024);

	CSnapshot& Snap = *(CSnapshot*)mem_alloc(SnapSize, 0);
	Snap.m_GroupCount = GroupCount;
	Snap.m_LayerCount = LayerCount;
	Snap.m_EnvelopeCount = EnvelopeCount;
	Snap.m_ImageCount = ImageCount;
	Snap.m_GameGroupID = m_GameGroupID;
	Snap.m_GameLayerID = m_GameLayerID;
	Snap.m_aImageNames = (CImageName*)(Snap.m_Data);
	Snap.m_aImageEmbeddedCrc = (u32*)(Snap.m_aImageNames + ImageCount);
	Snap.m_aImageInfos = (CImageInfo*)(Snap.m_aImageEmbeddedCrc + ImageCount);
	Snap.m_aGroups = (CGroup*)(Snap.m_aImageInfos + ImageCount);
	Snap.m_apLayers = (CMapItemLayer**)(Snap.m_aGroups + GroupCount);

	for(int i = 0; i < ImageCount; i++)
	{
		Snap.m_aImageNames[i] = m_Assets.m_aImageNames[i];
	}

	memmove(Snap.m_aImageEmbeddedCrc, m_Assets.m_aImageEmbeddedCrc,
		sizeof(Snap.m_aImageEmbeddedCrc[0]) * ImageCount);

	for(int i = 0; i < ImageCount; i++)
	{
		Snap.m_aImageInfos[i] = m_Assets.m_aTextureInfos[i];
	}

	for(int gi = 0; gi < GroupCount; gi++)
	{
		Snap.m_aGroups[gi] = m_aGroups[gi];
	}

	CMapItemLayer* pCurrentLayerData = (CMapItemLayer*)(Snap.m_apLayers + Snap.m_LayerCount);
	int TileStartID = 0;
	int QuadStartID = 0;

	for(int li = 0; li < LayerCount; li++)
	{
		const CLayer& Layer = m_aLayers[li];
		Snap.m_apLayers[li] = pCurrentLayerData;

		if(Layer.IsTileLayer())
		{
			CMapItemLayerTilemap& Tilemap = *(CMapItemLayerTilemap*)Snap.m_apLayers[li];
			Tilemap.m_Layer.m_Type = LAYERTYPE_TILES;
			if(li == m_GameLayerID)
				Tilemap.m_Layer.m_Type = LAYERTYPE_GAME;

			Tilemap.m_Color.r = (int)(Layer.m_Color.r*255);
			Tilemap.m_Color.g = (int)(Layer.m_Color.g*255);
			Tilemap.m_Color.b = (int)(Layer.m_Color.b*255);
			Tilemap.m_Color.a = (int)(Layer.m_Color.a*255);
			Tilemap.m_ColorEnv = Layer.m_ColorEnvelopeID;
			Tilemap.m_Data = TileStartID;
			TileStartID += Layer.m_Width*Layer.m_Height;
			Tilemap.m_Width = Layer.m_Width;
			Tilemap.m_Height = Layer.m_Height;
			Tilemap.m_Image = Layer.m_ImageID;

			pCurrentLayerData = (CMapItemLayer*)(&Tilemap + 1);
		}
		else
		{
			CMapItemLayerQuads& LayerQuads = *(CMapItemLayerQuads*)Snap.m_apLayers[li];
			LayerQuads.m_Layer.m_Type = LAYERTYPE_QUADS;
			LayerQuads.m_Data = QuadStartID;
			LayerQuads.m_NumQuads = Layer.m_aQuads.Count();
			QuadStartID += LayerQuads.m_NumQuads;
			LayerQuads.m_Image = Layer.m_ImageID;

			pCurrentLayerData = (CMapItemLayer*)(&LayerQuads + 1);
		}
	}

	Snap.m_aEnvelopes = (CMapItemEnvelope*)pCurrentLayerData;
	for(int ei = 0; ei < EnvelopeCount; ei++)
	{
		Snap.m_aEnvelopes[ei] = m_aEnvelopes[ei];
	}

	Snap.m_aTiles = (CTile*)(Snap.m_aEnvelopes + EnvelopeCount);
	CTile* pCurrentTile = Snap.m_aTiles;

	for(int li = 0; li < LayerCount; li++)
	{
		const CLayer& Layer = m_aLayers[li];
		if(Layer.IsTileLayer())
		{
			mem_copy(pCurrentTile, Layer.m_aTiles.Data(), sizeof(CTile)*Layer.m_aTiles.Count());
			pCurrentTile += Layer.m_aTiles.Count();
		}
	}

	Snap.m_aQuads = (CQuad*)pCurrentTile;
	CQuad* pCurrentQuad = Snap.m_aQuads;

	for(int li = 0; li < LayerCount; li++)
	{
		const CLayer& Layer = m_aLayers[li];
		if(Layer.IsQuadLayer())
		{
			mem_copy(pCurrentQuad, Layer.m_aQuads.Data(), sizeof(CQuad)*Layer.m_aQuads.Count());
			pCurrentQuad += Layer.m_aQuads.Count();
		}
	}

	Snap.m_aEnvPoints = (CEnvPoint*)pCurrentQuad;
	CEnvPoint* pCurrentEnvPoint = Snap.m_aEnvPoints;
	mem_copy(pCurrentEnvPoint, m_aEnvPoints.Data(), sizeof(CEnvPoint) * m_aEnvPoints.Count());

#ifdef CONF_DEBUG
	CompareSnapshot(&Snap);
#endif
	return &Snap;
}

void CEditorMap::RestoreSnapshot(const CEditorMap::CSnapshot* pSnapshot)
{
	const CEditorMap::CSnapshot& Snap = *pSnapshot;

	Clear();

	m_GameGroupID = Snap.m_GameGroupID;
	m_GameLayerID = Snap.m_GameLayerID;

	AssetsClearAndSetImages(Snap.m_aImageNames, Snap.m_aImageInfos, Snap.m_aImageEmbeddedCrc,
		Snap.m_ImageCount);

	m_aGroups.Add(Snap.m_aGroups, Snap.m_GroupCount);
	m_aLayers.AddEmpty(Snap.m_LayerCount);

	const CTile* pSnapTiles = Snap.m_aTiles;
	const CQuad* pSnapQuads = Snap.m_aQuads;

	for(int li = 0; li < Snap.m_LayerCount; li++)
	{
		const CMapItemLayer& SnapLayer = *Snap.m_apLayers[li];
		CLayer& Layer = m_aLayers[li];

		if(SnapLayer.m_Type == LAYERTYPE_GAME || SnapLayer.m_Type == LAYERTYPE_TILES)
		{
			const CMapItemLayerTilemap& SnapTilemap = *(CMapItemLayerTilemap*)Snap.m_apLayers[li];
			Layer.m_Type = LAYERTYPE_TILES;
			Layer.m_ImageID = SnapTilemap.m_Image;
			Layer.m_Color.r = SnapTilemap.m_Color.r/255.f;
			Layer.m_Color.g = SnapTilemap.m_Color.g/255.f;
			Layer.m_Color.b = SnapTilemap.m_Color.b/255.f;
			Layer.m_Color.a = SnapTilemap.m_Color.a/255.f;
			Layer.m_ColorEnvelopeID = SnapTilemap.m_ColorEnv;
			Layer.m_Width = SnapTilemap.m_Width;
			Layer.m_Height = SnapTilemap.m_Height;
			Layer.m_aTiles = NewTileArray();
			Layer.m_aTiles.Add(pSnapTiles, Layer.m_Width*Layer.m_Height);
			pSnapTiles += Layer.m_Width*Layer.m_Height;
		}
		else
		{
			const CMapItemLayerQuads& SnapQuadLayer = *(CMapItemLayerQuads*)Snap.m_apLayers[li];
			Layer.m_Type = LAYERTYPE_QUADS;
			Layer.m_ImageID = SnapQuadLayer.m_Image;
			Layer.m_aQuads = NewQuadArray();
			Layer.m_aQuads.Add(pSnapQuads, SnapQuadLayer.m_NumQuads);
			pSnapQuads += SnapQuadLayer.m_NumQuads;
		}
	}

	m_aEnvelopes.Add(Snap.m_aEnvelopes, Snap.m_EnvelopeCount);

	const CEnvPoint* pSnapEnvPoint = Snap.m_aEnvPoints;
	for(int ei = 0; ei < Snap.m_EnvelopeCount; ei++)
	{
		m_aEnvPoints.Add(pSnapEnvPoint, m_aEnvelopes[ei].m_NumPoints);
		pSnapEnvPoint += m_aEnvelopes[ei].m_NumPoints;
	}

	ed_dbg("Map snapshot restored");

#ifdef CONF_DEBUG
	CompareSnapshot(&Snap);
#endif
}

#ifdef CONF_DEBUG
void CEditorMap::CompareSnapshot(const CEditorMap::CSnapshot* pSnapshot)
{
	const CEditorMap::CSnapshot& Snap = *pSnapshot;
	dbg_assert(Snap.m_GroupCount == m_aGroups.Count(), "");
	dbg_assert(Snap.m_LayerCount == m_aLayers.Count(), "");
	dbg_assert(Snap.m_EnvelopeCount == m_aEnvelopes.Count(), "");
	dbg_assert(Snap.m_ImageCount == m_Assets.m_ImageCount, "");
	dbg_assert(Snap.m_GameGroupID == m_GameGroupID, "");
	dbg_assert(Snap.m_GameLayerID == m_GameLayerID, "");

	for(int i = 0; i < Snap.m_ImageCount; i++)
	{
		dbg_assert(mem_comp(&Snap.m_aImageEmbeddedCrc[i], &m_Assets.m_aImageEmbeddedCrc[i],
			sizeof(Snap.m_aImageEmbeddedCrc[i])) == 0, "");
		dbg_assert(mem_comp(&Snap.m_aImageNames[i], &m_Assets.m_aImageNames[i],
			sizeof(Snap.m_aImageNames[i])) == 0, "");
		dbg_assert(mem_comp(&Snap.m_aImageInfos[i], &m_Assets.m_aTextureInfos[i],
			sizeof(Snap.m_aImageInfos[i])) == 0, "");
	}

	for(int gi = 0; gi < Snap.m_GroupCount; gi++)
	{
		const CGroup& SnapGroup = Snap.m_aGroups[gi];
		const CGroup Group = m_aGroups[gi];
		dbg_assert(mem_comp(&SnapGroup, &Group, sizeof(CGroup)) == 0, "Groups don't match");
	}

	const CTile* pSnapTiles = Snap.m_aTiles;
	const CQuad* pSnapQuads = Snap.m_aQuads;

	for(int li = 0; li < Snap.m_LayerCount; li++)
	{
		const CMapItemLayer& SnapLayer = *Snap.m_apLayers[li];
		const CLayer& Layer = m_aLayers[li];

		if(SnapLayer.m_Type == LAYERTYPE_GAME)
			dbg_assert(Layer.m_Type == LAYERTYPE_TILES && li == m_GameLayerID, "");
		else
			dbg_assert(SnapLayer.m_Type == LAYERTYPE_GAME || SnapLayer.m_Type == Layer.m_Type, "");

		if(Layer.IsTileLayer())
		{
			const CMapItemLayerTilemap& SnapTilemap = *(CMapItemLayerTilemap*)Snap.m_apLayers[li];
			dbg_assert(SnapTilemap.m_Image == Layer.m_ImageID, "");
			dbg_assert(SnapTilemap.m_ColorEnv == Layer.m_ColorEnvelopeID, "");
			dbg_assert(SnapTilemap.m_Color.r == (int)(Layer.m_Color.r*255), "");
			dbg_assert(SnapTilemap.m_Color.g == (int)(Layer.m_Color.g*255), "");
			dbg_assert(SnapTilemap.m_Color.b == (int)(Layer.m_Color.b*255), "");
			dbg_assert(SnapTilemap.m_Color.a == (int)(Layer.m_Color.a*255), "");
			dbg_assert(SnapTilemap.m_Width == Layer.m_Width, "");
			dbg_assert(SnapTilemap.m_Height == Layer.m_Height, "");
			dbg_assert(mem_comp(pSnapTiles, Layer.m_aTiles.Data(),
				Layer.m_Width*Layer.m_Height*sizeof(CTile)) == 0, "Layer tiles don't match");
			pSnapTiles += Layer.m_Width*Layer.m_Height;
		}
		else if(Layer.IsQuadLayer())
		{
			const CMapItemLayerQuads& SnapQuadLayer = *(CMapItemLayerQuads*)Snap.m_apLayers[li];
			dbg_assert(SnapQuadLayer.m_Image == Layer.m_ImageID, "");
			dbg_assert(SnapQuadLayer.m_NumQuads == Layer.m_aQuads.Count(), "");
			dbg_assert(mem_comp(pSnapQuads, Layer.m_aQuads.Data(),
				SnapQuadLayer.m_NumQuads*sizeof(CQuad)) == 0, "Quads don't match");
			pSnapQuads += SnapQuadLayer.m_NumQuads;
		}
	}

	const CEnvPoint* pSnapEnvPoint = Snap.m_aEnvPoints;
	for(int ei = 0; ei < Snap.m_EnvelopeCount; ei++)
	{
		dbg_assert(mem_comp(&Snap.m_aEnvelopes[ei], &m_aEnvelopes[ei], sizeof(CMapItemEnvelope)) == 0, "");
		dbg_assert(mem_comp(pSnapEnvPoint, &m_aEnvPoints[m_aEnvelopes[ei].m_StartPoint],
			sizeof(CEnvPoint)*Snap.m_aEnvelopes[ei].m_NumPoints) == 0, "");
		pSnapEnvPoint += Snap.m_aEnvelopes[ei].m_NumPoints;
	}

	ed_dbg("[✔] Map snapshot matches current map data");
}
#endif

CEditorMap::CLayer& CEditorMap::NewTileLayer(int Width, int Height)
{
	CLayer TileLayer;
	TileLayer.m_Type = LAYERTYPE_TILES;
	TileLayer.m_ImageID = -1;
	TileLayer.m_Width = Width;
	TileLayer.m_Height = Height;
	TileLayer.m_aTiles = NewTileArray();
	TileLayer.m_aTiles.AddEmpty(TileLayer.m_Width * TileLayer.m_Height);
	return m_aLayers.Add(TileLayer);
}

CEditorMap::CLayer&CEditorMap::NewQuadLayer()
{
	CLayer QuadLayer;
	QuadLayer.m_Type = LAYERTYPE_QUADS;
	QuadLayer.m_ImageID = -1;
	QuadLayer.m_aQuads = NewQuadArray();
	return m_aLayers.Add(QuadLayer);
}

/*CEditorAssets::CEditorAssets()
{
	Clear();
}

void CEditorAssets::Init(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
}

void CEditorAssets::Clear()
{
	m_ImageCount = 0;
	mem_zero(m_aImagePathHash, sizeof(m_aImagePathHash));
	mem_zero(m_aImageDataHash, sizeof(m_aImageDataHash));

	for(int i = 0; i < MAX_IMAGES; i++)
		m_aImageTextureHandle[i].Invalidate();
}

int CEditorAssets::LoadQuadImage(const char* aPath)
{
	const int PathLen = str_length(aPath);
	const u32 Hash = fnv1a32(aPath, PathLen);



	IGraphics::CTextureHandle TexHnd = Graphics()->LoadTexture(aPath, IStorage::TYPE_ALL,
		CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);
	return -1;
}*/

IEditor *CreateEditor() { return new CEditor; }

CEditor::CEditor()
{

}

CEditor::~CEditor()
{

}

void CEditor::Init()
{
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pConsole = CreateConsole(CFGFLAG_EDITOR);
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_RenderTools.m_pGraphics = m_pGraphics;
	m_RenderTools.m_pUI = &m_UI;
	m_UI.SetGraphics(m_pGraphics, m_pTextRender);

	m_MousePos = vec2(Graphics()->ScreenWidth() * 0.5f, Graphics()->ScreenHeight() * 0.5f);
	m_UiMousePos = vec2(0, 0);
	m_UiMouseDelta = vec2(0, 0);
	m_MapUiPosOffset = vec2(0,0);

	m_CheckerTexture = Graphics()->LoadTexture("editor/checker.png",
		IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_CursorTexture = Graphics()->LoadTexture("editor/cursor.png",
		IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_EntitiesTexture = Graphics()->LoadTexture("editor/entities.png",
		IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);
	m_GameTexture = Graphics()->LoadTexture("game.png",
		IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

	m_pConsole->Register("load", "r", CFGFLAG_EDITOR, ConLoad, this, "Load map");
	m_pConsole->Register("+show_palette", "", CFGFLAG_EDITOR, ConShowPalette, this, "Show palette");
	m_pConsole->Register("game_view", "i", CFGFLAG_EDITOR, ConGameView, this, "Toggle game view");
	m_pConsole->Register("show_grid", "i", CFGFLAG_EDITOR, ConShowGrid, this, "Toggle grid");
	m_pConsole->Register("undo", "", CFGFLAG_EDITOR, ConUndo, this, "Undo");
	m_pConsole->Register("redo", "", CFGFLAG_EDITOR, ConRedo, this, "Redo");
	m_pConsole->Register("delete_image", "i", CFGFLAG_EDITOR, ConDeleteImage, this, "Delete image");
	m_InputConsole.Init(m_pConsole, m_pGraphics, &m_UI, m_pTextRender);

	// grenade pickup
	{
		const float SpriteW = g_pData->m_aSprites[SPRITE_PICKUP_GRENADE].m_W;
		const float SpriteH = g_pData->m_aSprites[SPRITE_PICKUP_GRENADE].m_H;
		const float ScaleFactor = sqrt(SpriteW*SpriteW+SpriteH*SpriteH);
		const int VisualSize = g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_VisualSize;
		m_RenderGrenadePickupSize = vec2(VisualSize * (SpriteW/ScaleFactor),
										 VisualSize * (SpriteH/ScaleFactor));
	}
	// shotgun pickup
	{
		const float SpriteW = g_pData->m_aSprites[SPRITE_PICKUP_SHOTGUN].m_W;
		const float SpriteH = g_pData->m_aSprites[SPRITE_PICKUP_SHOTGUN].m_H;
		const float ScaleFactor = sqrt(SpriteW*SpriteW+SpriteH*SpriteH);
		const int VisualSize = g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_VisualSize;
		m_RenderShotgunPickupSize =vec2(VisualSize * (SpriteW/ScaleFactor),
										VisualSize * (SpriteH/ScaleFactor));
	}
	// laser pickup
	{
		const float SpriteW = g_pData->m_aSprites[SPRITE_PICKUP_LASER].m_W;
		const float SpriteH = g_pData->m_aSprites[SPRITE_PICKUP_LASER].m_H;
		const float ScaleFactor = sqrt(SpriteW*SpriteW+SpriteH*SpriteH);
		const int VisualSize = g_pData->m_Weapons.m_aId[WEAPON_LASER].m_VisualSize;
		m_RenderLaserPickupSize = vec2(VisualSize * (SpriteW/ScaleFactor),
									   VisualSize * (SpriteH/ScaleFactor));
	}

	m_Map.Init(m_pStorage, m_pGraphics, m_pConsole);
	m_Brush.m_aTiles = m_Map.NewTileArray();

	m_HistoryEntryDispenser.Init(MAX_HISTORY, 1);
	m_pHistoryEntryCurrent = nullptr;

	/*
	m_Map.LoadDefault();
	OnMapLoaded();
	*/
	if(!LoadMap("maps/ctf7.map")) {
		dbg_break();
	}
}

bool CEditor::HasUnsavedData() const
{
	return false;
}

void CEditor::OnInput(IInput::CEvent Event)
{
	if(m_InputConsole.IsOpen())
	{
		m_InputConsole.OnInput(Event);
		return;
	}
}

void CEditor::Update()
{
	m_LocalTime = Client()->LocalTime();

	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);
		// FIXME: this doesn't work with limitfps or asyncrender 1 (when a frame gets skipped)
		// since we update only when we render
		// in practice this isn't a big issue since the input stack gets cleared at the end of Render()
		/*if(!Input()->IsEventValid(&e))
			continue;*/
		OnInput(e);
	}

	UI()->StartCheck();
	const CUIRect UiScreenRect = *UI()->Screen();

	// mouse input
	float rx = 0, ry = 0;
	Input()->MouseRelative(&rx, &ry);
	UI()->ConvertMouseMove(&rx, &ry);

	m_MousePos.x = clamp(m_MousePos.x + rx, 0.0f, (float)Graphics()->ScreenWidth());
	m_MousePos.y = clamp(m_MousePos.y + ry, 0.0f, (float)Graphics()->ScreenHeight());
	float NewUiMousePosX = (m_MousePos.x / (float)Graphics()->ScreenWidth()) * UiScreenRect.w;
	float NewUiMousePosY = (m_MousePos.y / (float)Graphics()->ScreenHeight()) * UiScreenRect.h;
	m_UiMouseDelta.x = NewUiMousePosX-m_UiMousePos.x;
	m_UiMouseDelta.y = NewUiMousePosY-m_UiMousePos.y;
	m_UiMousePos.x = NewUiMousePosX;
	m_UiMousePos.y = NewUiMousePosY;

	enum
	{
		MOUSE_LEFT=1,
		MOUSE_RIGHT=2,
		MOUSE_MIDDLE=4,
	};

	int MouseButtons = 0;
	if(Input()->KeyIsPressed(KEY_MOUSE_1)) MouseButtons |= MOUSE_LEFT;
	if(Input()->KeyIsPressed(KEY_MOUSE_2)) MouseButtons |= MOUSE_RIGHT;
	if(Input()->KeyIsPressed(KEY_MOUSE_3)) MouseButtons |= MOUSE_MIDDLE;
	UI()->Update(m_UiMousePos.x, m_UiMousePos.y, 0, 0, MouseButtons);

	if(Input()->KeyPress(KEY_F1))
	{
		m_InputConsole.ToggleOpen();
	}

	if(!m_InputConsole.IsOpen())
	{
		if(m_UiCurrentPopupID == POPUP_NONE)
		{
			// move view
			if(MouseButtons&MOUSE_RIGHT)
			{
				m_MapUiPosOffset -= m_UiMouseDelta;
			}

			// zoom with mouse wheel
			if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
				ChangeZoom(m_Zoom * 0.9f);
			else if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
				ChangeZoom(m_Zoom * 1.1f);

			if(Input()->KeyPress(KEY_HOME))
			{
				ResetCamera();
			}

			// undo / redo
			const bool IsCtrlPressed = Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL);
			if(IsCtrlPressed && Input()->KeyPress(KEY_Z))
				HistoryUndo();
			else if(IsCtrlPressed && Input()->KeyPress(KEY_Y))
				HistoryRedo();

			// TODO: remove
			if(IsCtrlPressed && Input()->KeyPress(KEY_A))
				ChangePage((m_Page+1) % PAGE_COUNT_);
		}

		if(Input()->KeyIsPressed(KEY_SPACE) && m_UiCurrentPopupID != POPUP_BRUSH_PALETTE)
			m_UiCurrentPopupID = POPUP_BRUSH_PALETTE;
		else if(!Input()->KeyIsPressed(KEY_SPACE) && m_UiCurrentPopupID == POPUP_BRUSH_PALETTE)
			m_UiCurrentPopupID = POPUP_NONE;
	}
}

void CEditor::Render()
{
	const CUIRect UiScreenRect = *UI()->Screen();
	m_UiScreenRect = UiScreenRect;
	m_GfxScreenWidth = Graphics()->ScreenWidth();
	m_GfxScreenHeight = Graphics()->ScreenHeight();

	// basic start
	Graphics()->Clear(0.3f, 0.3f, 0.3f);

	if(m_Page == PAGE_MAP_EDITOR)
		RenderMapView();
	else if(m_Page == PAGE_ASSET_MANAGER)
		RenderAssetManager();

	// console
	m_InputConsole.Render();

	// render mouse cursor
	{
		Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);
		Graphics()->TextureSet(m_CursorTexture);
		Graphics()->WrapClamp();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1,0,1,1);
		/*if(UI()->HotItem())
			Graphics()->SetColor(1,0.5,1,1);*/
		IGraphics::CQuadItem QuadItem(m_UiMousePos.x, m_UiMousePos.y, 16.0f, 16.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();
	}

	UI()->FinishCheck();
	Input()->Clear();
}

void CEditor::RenderLayerGameEntities(const CEditorMap::CLayer& GameLayer)
{
	const CTile* pTiles = GameLayer.m_aTiles.Data();
	const int LayerWidth = GameLayer.m_Width;
	const int LayerHeight = GameLayer.m_Height;

	Graphics()->TextureSet(m_GameTexture);
	Graphics()->QuadsBegin();

	// TODO: cache sprite base positions?
	struct CEntitySprite
	{
		int m_SpriteID;
		vec2 m_Pos;
		vec2 m_Size;
	};

	CEntitySprite aEntitySprites[2048];
	int EntitySpriteCount = 0;

	const float HealthArmorSize = 64*0.7071067811865475244;
	const vec2 NinjaSize(128*(256/263.87876003953027518857),
						 128*(64/263.87876003953027518857));

	const float TileSize = 32.f;
	const float Time = Client()->LocalTime();

	for(int ty = 0; ty < LayerHeight; ty++)
	{
		for(int tx = 0; tx < LayerWidth; tx++)
		{
			const int tid = ty*LayerWidth+tx;
			const u8 Index = pTiles[tid].m_Index - ENTITY_OFFSET;
			if(!Index)
				continue;

			vec2 BasePos(tx*TileSize, ty*TileSize);
			const float Offset = tx + ty;
			vec2 PickupPos = BasePos;
			PickupPos.x += cosf(Time*2.0f+Offset)*2.5f;
			PickupPos.y += sinf(Time*2.0f+Offset)*2.5f;

			if(Index == ENTITY_HEALTH_1)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_HEALTH,
					PickupPos - vec2((HealthArmorSize-TileSize)*0.5f,(HealthArmorSize-TileSize)*0.5f),
					vec2(HealthArmorSize, HealthArmorSize)
				};
			}
			else if(Index == ENTITY_ARMOR_1)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_ARMOR,
					PickupPos - vec2((HealthArmorSize-TileSize)*0.5f,(HealthArmorSize-TileSize)*0.5f),
					vec2(HealthArmorSize, HealthArmorSize)
				};
			}
			else if(Index == ENTITY_WEAPON_GRENADE)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_GRENADE,
					PickupPos - vec2((m_RenderGrenadePickupSize.x-TileSize)*0.5f,
						(m_RenderGrenadePickupSize.y-TileSize)*0.5f),
					m_RenderGrenadePickupSize
				};
			}
			else if(Index == ENTITY_WEAPON_SHOTGUN)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_SHOTGUN,
					PickupPos - vec2((m_RenderShotgunPickupSize.x-TileSize)*0.5f,
						(m_RenderShotgunPickupSize.y-TileSize)*0.5f),
					m_RenderShotgunPickupSize
				};
			}
			else if(Index == ENTITY_WEAPON_LASER)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_LASER,
					PickupPos - vec2((m_RenderLaserPickupSize.x-TileSize)*0.5f,
						(m_RenderLaserPickupSize.y-TileSize)*0.5f),
					m_RenderLaserPickupSize
				};
			}
			else if(Index == ENTITY_POWERUP_NINJA)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_PICKUP_NINJA,
					PickupPos - vec2((NinjaSize.x-TileSize)*0.5f, (NinjaSize.y-TileSize)*0.5f),
					NinjaSize
				};
			}
			else if(Index == ENTITY_FLAGSTAND_RED)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_FLAG_RED,
					BasePos - vec2(0, 54),
					vec2(42, 84)
				};
			}
			else if(Index == ENTITY_FLAGSTAND_BLUE)
			{
				aEntitySprites[EntitySpriteCount++] = {
					SPRITE_FLAG_BLUE,
					BasePos - vec2(0, 54),
					vec2(42, 84)
				};
			}
		}
	}

	for(int i = 0; i < EntitySpriteCount; i++)
	{
		const CEntitySprite& e = aEntitySprites[i];
		RenderTools()->SelectSprite(e.m_SpriteID);
		IGraphics::CQuadItem Quad(e.m_Pos.x, e.m_Pos.y, e.m_Size.x, e.m_Size.y);
		Graphics()->QuadsDrawTL(&Quad, 1);
	}

	Graphics()->QuadsEnd();
}

inline vec2 CEditor::CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY,
										   float ParallaxX, float ParallaxY)
{
	// we add UiScreenRect.w*0.5 and UiScreenRect.h*0.5 because in the game the view
	// is based on the center of the screen
	const CUIRect UiScreenRect = m_UiScreenRect;
	const float MapOffX = (((m_MapUiPosOffset.x+UiScreenRect.w*0.5) * ParallaxX) - UiScreenRect.w*0.5)/
						  UiScreenRect.w * WorldWidth + PosX;
	const float MapOffY = (((m_MapUiPosOffset.y+UiScreenRect.h*0.5) * ParallaxY) - UiScreenRect.h*0.5)/
						  UiScreenRect.h * WorldHeight + PosY;
	return vec2(MapOffX, MapOffY);
}

inline vec2 CEditor::CalcGroupWorldPosFromUiPos(int GroupID, float WorldWidth, float WorldHeight, vec2 UiPos)
{
	const CEditorMap::CGroup& G = m_Map.m_aGroups[GroupID];
	const float OffX = G.m_OffsetX;
	const float OffY = G.m_OffsetY;
	const float ParaX = G.m_ParallaxX/100.f;
	const float ParaY = G.m_ParallaxY/100.f;
	// we add UiScreenRect.w*0.5 and UiScreenRect.h*0.5 because in the game the view
	// is based on the center of the screen
	const CUIRect UiScreenRect = m_UiScreenRect;
	const float MapOffX = (((m_MapUiPosOffset.x + UiScreenRect.w*0.5) * ParaX) -
		UiScreenRect.w*0.5 + UiPos.x)/ UiScreenRect.w * WorldWidth + OffX;
	const float MapOffY = (((m_MapUiPosOffset.y + UiScreenRect.h*0.5) * ParaY) -
		UiScreenRect.h*0.5 + UiPos.y)/ UiScreenRect.h * WorldHeight + OffY;
	return vec2(MapOffX, MapOffY);
}

void CEditor::StaticEnvelopeEval(float TimeOffset, int EnvID, float* pChannels, void* pUser)
{
	CEditor *pThis = (CEditor *)pUser;
	if(EnvID >= 0)
		pThis->EnvelopeEval(TimeOffset, EnvID, pChannels);
}

void CEditor::EnvelopeEval(float TimeOffset, int EnvID, float* pChannels)
{
	pChannels[0] = 0;
	pChannels[1] = 0;
	pChannels[2] = 0;
	pChannels[3] = 0;

	dbg_assert(EnvID < m_Map.m_aEnvelopes.Count(), "EnvID out of bounds");
	if(EnvID >= m_Map.m_aEnvelopes.Count())
		return;

	const CMapItemEnvelope& Env = m_Map.m_aEnvelopes[EnvID];
	const CEnvPoint* pPoints = &m_Map.m_aEnvPoints[0];

	float Time = Client()->LocalTime();
	RenderTools()->RenderEvalEnvelope(pPoints + Env.m_StartPoint, Env.m_NumPoints, 4,
									  Time+TimeOffset, pChannels);
}

void CEditor::RenderMapView()
{
	// get world view points based on neutral paramters
	float aWorldViewRectPoints[4];
	RenderTools()->MapScreenToWorld(0, 0, 1, 1, 0, 0, Graphics()->ScreenAspect(), 1, aWorldViewRectPoints);

	const float WorldViewWidth = aWorldViewRectPoints[2]-aWorldViewRectPoints[0];
	const float WorldViewHeight = aWorldViewRectPoints[3]-aWorldViewRectPoints[1];
	const float ZoomWorldViewWidth = WorldViewWidth * m_Zoom;
	const float ZoomWorldViewHeight = WorldViewHeight * m_Zoom;
	m_ZoomWorldViewWidth = ZoomWorldViewWidth;
	m_ZoomWorldViewHeight = ZoomWorldViewHeight;

	const float FakeToScreenX = m_GfxScreenWidth/ZoomWorldViewWidth;
	const float FakeToScreenY = m_GfxScreenHeight/ZoomWorldViewHeight;
	const float TileSize = 32;

	float SelectedParallaxX = 1;
	float SelectedParallaxY = 1;
	float SelectedPositionX = 0;
	float SelectedPositionY = 0;
	const int SelectedLayerID = m_UiSelectedLayerID;
	const bool IsSelectedLayerTile = m_Map.m_aLayers[SelectedLayerID].m_Type == LAYERTYPE_TILES;
	const bool IsSelectedLayerQuad = m_Map.m_aLayers[SelectedLayerID].m_Type == LAYERTYPE_QUADS;
	dbg_assert(SelectedLayerID >= 0, "No layer selected");
	dbg_assert(m_UiSelectedGroupID >= 0, "Parent group of selected layer not found");
	const CEditorMap::CGroup& Group = m_Map.m_aGroups[m_UiSelectedGroupID];
	SelectedParallaxX = Group.m_ParallaxX / 100.f;
	SelectedParallaxY = Group.m_ParallaxY / 100.f;
	SelectedPositionX = Group.m_OffsetX;
	SelectedPositionY = Group.m_OffsetY;

	const vec2 SelectedScreenOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight,
		SelectedPositionX, SelectedPositionY, SelectedParallaxX, SelectedParallaxY);

	// background
	{
		Graphics()->MapScreen(0, 0, ZoomWorldViewWidth, ZoomWorldViewHeight);
		Graphics()->TextureSet(m_CheckerTexture);
		Graphics()->BlendNormal();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);

		// align background with grid
		float StartX = fract(SelectedScreenOff.x/(TileSize*2));
		float StartY = fract(SelectedScreenOff.y/(TileSize*2));
		Graphics()->QuadsSetSubset(StartX, StartY,
								   ZoomWorldViewWidth/(TileSize*2)+StartX,
								   ZoomWorldViewHeight/(TileSize*2)+StartY);

		IGraphics::CQuadItem QuadItem(0, 0, ZoomWorldViewWidth, ZoomWorldViewHeight);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	// render map
	const int GroupCount = m_Map.m_aGroups.Count();
	const int BaseTilemapFlags = m_ConfigShowExtendedTilemaps ? TILERENDERFLAG_EXTEND:0;

	for(int gi = 0; gi < GroupCount; gi++)
	{
		if(m_UiGroupHidden[gi])
			continue;

		CEditorMap::CGroup& Group = m_Map.m_aGroups[gi];

		const float ParallaxX = Group.m_ParallaxX / 100.f;
		const float ParallaxY = Group.m_ParallaxY / 100.f;
		const float OffsetX = Group.m_OffsetX;
		const float OffsetY = Group.m_OffsetY;
		const vec2 MapOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight,
												  OffsetX, OffsetY, ParallaxX, ParallaxY);
		CUIRect ScreenRect = { MapOff.x, MapOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };

		Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
							  ScreenRect.y+ScreenRect.h);

		const int LayerCount = Group.m_LayerCount;

		for(int li = 0; li < LayerCount; li++)
		{
			const int LyID = Group.m_apLayerIDs[li];
			if(m_UiLayerHidden[LyID])
				continue;

			const CEditorMap::CLayer& Layer = m_Map.m_aLayers[LyID];

			if(Layer.m_Type == LAYERTYPE_TILES)
			{
				const float LyWidth = Layer.m_Width;
				const float LyHeight = Layer.m_Height;
				vec4 LyColor = Layer.m_Color;
				const CTile *pTiles = Layer.m_aTiles.Data();

				if(LyID == m_Map.m_GameLayerID)
				{
					if(m_ConfigShowGameEntities)
						RenderLayerGameEntities(Layer);
					continue;
				}

				if(m_UiLayerHovered[LyID])
					LyColor = vec4(1, 0, 1, 1);

				/*if(SelectedLayerID >= 0 && SelectedLayerID != LyID)
					LyColor.a = 0.5f;*/

				if(Layer.m_ImageID == -1)
					Graphics()->TextureClear();
				else
					Graphics()->TextureSet(m_Map.m_Assets.m_aTextureHandle[Layer.m_ImageID]);

				Graphics()->BlendNone();
				RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
											 BaseTilemapFlags|LAYERRENDERFLAG_OPAQUE,
											 StaticEnvelopeEval, this, Layer.m_ColorEnvelopeID, 0);

				Graphics()->BlendNormal();
				RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
											 BaseTilemapFlags|LAYERRENDERFLAG_TRANSPARENT,
											 StaticEnvelopeEval, this, Layer.m_ColorEnvelopeID, 0);
			}
			else if(Layer.m_Type == LAYERTYPE_QUADS)
			{
				if(Layer.m_ImageID == -1)
					Graphics()->TextureClear();
				else
					Graphics()->TextureSet(m_Map.m_Assets.m_aTextureHandle[Layer.m_ImageID]);

				Graphics()->BlendNormal();
				if(m_UiLayerHovered[LyID])
					Graphics()->BlendAdditive();

				RenderTools()->RenderQuads(Layer.m_aQuads.Data(), Layer.m_aQuads.Count(),
						LAYERRENDERFLAG_TRANSPARENT, StaticEnvelopeEval, this);
			}
		}
	}

	// game layer
	const int LyID = m_Map.m_GameLayerID;
	if(!m_ConfigShowGameEntities && !m_UiLayerHidden[LyID] && !m_UiGroupHidden[m_Map.m_GameGroupID])
	{
		const vec2 MapOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight, 0, 0, 1, 1);
		CUIRect ScreenRect = { MapOff.x, MapOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };

		Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
							  ScreenRect.y+ScreenRect.h);

		const CEditorMap::CLayer& LayerTile = m_Map.m_aLayers[LyID];
		const float LyWidth = LayerTile.m_Width;
		const float LyHeight = LayerTile.m_Height;
		vec4 LyColor = LayerTile.m_Color;
		const CTile *pTiles = LayerTile.m_aTiles.Data();

		Graphics()->TextureSet(m_EntitiesTexture);
		Graphics()->BlendNone();
		RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
									 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_OPAQUE,
									 0, 0, -1, 0);

		Graphics()->BlendNormal();
		RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
									 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_TRANSPARENT,
									 0, 0, -1, 0);
	}


	Graphics()->BlendNormal();

	// origin and border
	CUIRect ScreenRect = { SelectedScreenOff.x, SelectedScreenOff.y,
						   ZoomWorldViewWidth, ZoomWorldViewHeight };
	Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
						  ScreenRect.y+ScreenRect.h);

	IGraphics::CQuadItem OriginLineX(0, 0, TileSize, 2/FakeToScreenY);
	IGraphics::CQuadItem RecOriginLineYtY(0, 0, 2/FakeToScreenX, TileSize);
	float LayerWidth = m_Map.m_aLayers[SelectedLayerID].m_Width * TileSize;
	float LayerHeight = m_Map.m_aLayers[SelectedLayerID].m_Height * TileSize;

	const float bw = 1.0f / FakeToScreenX;
	const float bh = 1.0f / FakeToScreenY;
	IGraphics::CQuadItem aBorders[4] = {
		IGraphics::CQuadItem(0, 0, LayerWidth, bh),
		IGraphics::CQuadItem(0, LayerHeight, LayerWidth, bh),
		IGraphics::CQuadItem(0, 0, bw, LayerHeight),
		IGraphics::CQuadItem(LayerWidth, 0, bw, LayerHeight)
	};

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	if(IsSelectedLayerTile)
	{
		// grid
		if(m_ConfigShowGrid)
		{
			const float GridAlpha =  0.25f;
			Graphics()->SetColor(1.0f * GridAlpha, 1.0f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
			float StartX = SelectedScreenOff.x - fract(SelectedScreenOff.x/TileSize) * TileSize;
			float StartY = SelectedScreenOff.y - fract(SelectedScreenOff.y/TileSize) * TileSize;
			float EndX = SelectedScreenOff.x+ZoomWorldViewWidth;
			float EndY = SelectedScreenOff.y+ZoomWorldViewHeight;
			for(float x = StartX; x < EndX; x+= TileSize)
			{
				const bool MajorLine = (int)(x/TileSize)%10 == 0 && m_ConfigShowGridMajor;
				if(MajorLine)
				{
					Graphics()->SetColor(0.5f * GridAlpha, 0.5f * GridAlpha, 1.0f * GridAlpha,
						GridAlpha);
				}

				IGraphics::CQuadItem Line(x, SelectedScreenOff.y, bw, ZoomWorldViewHeight);
				Graphics()->QuadsDrawTL(&Line, 1);

				if(MajorLine)
				{
					Graphics()->SetColor(1.0f * GridAlpha, 1.0f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
				}
			}
			for(float y = StartY; y < EndY; y+= TileSize)
			{
				const bool MajorLine = (int)(y/TileSize)%10 == 0 && m_ConfigShowGridMajor;
				if(MajorLine)
				{
					Graphics()->SetColor(1.0f * GridAlpha*2.0f, 1.0f * GridAlpha*2.0f, 1.0f * GridAlpha*2.0f,
						GridAlpha*2.0f);
				}

				IGraphics::CQuadItem Line(SelectedScreenOff.x, y, ZoomWorldViewWidth, bh);
				Graphics()->QuadsDrawTL(&Line, 1);

				if(MajorLine)
				{
					Graphics()->SetColor(1.0f * GridAlpha, 1.0f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
				}
			}
		}

		// borders
		Graphics()->SetColor(1, 1, 1, 1);
		Graphics()->QuadsDrawTL(aBorders, 4);
	}

	Graphics()->SetColor(0, 0, 1, 1);
	Graphics()->QuadsDrawTL(&OriginLineX, 1);
	Graphics()->SetColor(0, 1, 0, 1);
	Graphics()->QuadsDrawTL(&RecOriginLineYtY, 1);

	Graphics()->QuadsEnd();

	// hud
	RenderMapViewHud();

	// user interface
	RenderMapEditorUI();
}

void CEditor::RenderMapViewHud()
{
	// NOTE: we're in selected group world space here

	if(m_UiCurrentPopupID != POPUP_NONE)
		return;

	const float TileSize = 32;
	vec2 MouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID, m_ZoomWorldViewWidth,
		m_ZoomWorldViewHeight, m_UiMousePos);

	const vec2 GridMousePos(floor(MouseWorldPos.x/TileSize)*TileSize,
		floor(MouseWorldPos.y/TileSize)*TileSize);

	static CUIMouseDragState s_MapViewDrag;
	bool FinishedDragging = UiDoMouseDragging(&s_MapViewDrag, m_UiMainViewRect, &s_MapViewDrag);

	dbg_assert(m_UiSelectedLayerID >= 0, "No layer selected");
	const CEditorMap::CLayer& SelectedTileLayer = m_Map.m_aLayers[m_UiSelectedLayerID];

	if(SelectedTileLayer.IsTileLayer())
	{
		if(m_Brush.IsEmpty())
		{
			if(s_MapViewDrag.m_IsDragging)
			{
				const vec2 StartMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
					m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_StartDragPos);
				const vec2 EndWorldPos = MouseWorldPos;

				const int StartTX = floor(StartMouseWorldPos.x/TileSize);
				const int StartTY = floor(StartMouseWorldPos.y/TileSize);
				const int EndTX = floor(EndWorldPos.x/TileSize);
				const int EndTY = floor(EndWorldPos.y/TileSize);

				const int DragStartTX = min(StartTX, EndTX);
				const int DragStartTY = min(StartTY, EndTY);
				const int DragEndTX = max(StartTX, EndTX);
				const int DragEndTY = max(StartTY, EndTY);

				const CUIRect HoverRect = {DragStartTX*TileSize, DragStartTY*TileSize,
					(DragEndTX+1-DragStartTX)*TileSize, (DragEndTY+1-DragStartTY)*TileSize};
				vec4 HoverColor = StyleColorTileHover;
				HoverColor.a += sinf(m_LocalTime * 2.0) * 0.1;
				DrawRectBorderMiddle(HoverRect, HoverColor, 2, StyleColorTileHoverBorder);
			}
			else
			{
				const CUIRect HoverRect = {GridMousePos.x, GridMousePos.y, TileSize, TileSize};
				vec4 HoverColor = StyleColorTileHover;
				HoverColor.a += sinf(m_LocalTime * 2.0) * 0.1;
				DrawRect(HoverRect, HoverColor);
			}

			// TODO: if tool == brush
			// TODO: do this better (ignoring mouse click on UI)
			if(SelectedTileLayer.IsTileLayer() && FinishedDragging)
			{
				const vec2 StartMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
					m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_StartDragPos);
				const vec2 EndMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
					m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_EndDragPos);

				const int StartTX = floor(StartMouseWorldPos.x/TileSize);
				const int StartTY = floor(StartMouseWorldPos.y/TileSize);
				const int EndTX = floor(EndMouseWorldPos.x/TileSize);
				const int EndTY = floor(EndMouseWorldPos.y/TileSize);

				const int SelStartX = clamp(min(StartTX, EndTX), 0, SelectedTileLayer.m_Width-1);
				const int SelStartY = clamp(min(StartTY, EndTY), 0, SelectedTileLayer.m_Height-1);
				const int SelEndX = clamp(max(StartTX, EndTX), 0, SelectedTileLayer.m_Width-1) + 1;
				const int SelEndY = clamp(max(StartTY, EndTY), 0, SelectedTileLayer.m_Height-1) + 1;
				const int Width = SelEndX - SelStartX;
				const int Height = SelEndY - SelStartY;

				CDynArray<CTile> aExtractTiles = m_Map.NewTileArray();
				aExtractTiles.AddEmpty(Width * Height);

				const int LayerWidth = SelectedTileLayer.m_Width;
				const CDynArray<CTile>& aLayerTiles = SelectedTileLayer.m_aTiles;
				const int StartTid = SelStartY * LayerWidth + SelStartX;
				const int LastTid = (SelEndY-1) * LayerWidth + SelEndX;

				for(int ti = StartTid; ti < LastTid; ti++)
				{
					const int tx = (ti % LayerWidth) - SelStartX;
					const int ty = (ti / LayerWidth) - SelStartY;
					if(tx >= 0 && tx < Width && ty >= 0 && ty < Height)
						aExtractTiles[ty * Width + tx] = aLayerTiles[ti];
				}

				SetNewBrush(aExtractTiles.Data(), Width, Height);
			}
		}

		// TODO: if tool == brush
		// draw brush
		RenderBrush(GridMousePos);
	}
}

void CEditor::RenderMapEditorUI()
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect RightPanel;
	UiScreenRect.VSplitRight(150, &m_UiMainViewRect, &RightPanel);

	DrawRect(RightPanel, StyleColorBg);

	CUIRect NavRect, ButtonRect;
	RightPanel.Margin(3.0f, &NavRect);
	NavRect.HSplitTop(20, &ButtonRect, &NavRect);
	NavRect.HSplitTop(2, 0, &NavRect);

	// tabs
	enum
	{
		TAB_GROUPS=0,
		TAB_HISTORY,
	};
	static int s_CurrentTab = TAB_GROUPS;
	CUIRect ButtonRectRight;
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRectRight);

	static CUIButtonState s_ButGroups;
	if(UiButton(ButtonRect, "Groups", &s_ButGroups))
	{
		s_CurrentTab = TAB_GROUPS;
	}

	static CUIButtonState s_ButHistory;
	if(UiButton(ButtonRectRight, "History", &s_ButHistory))
	{
		s_CurrentTab = TAB_HISTORY;
	}

	if(s_CurrentTab == TAB_GROUPS)
	{
		RenderMapEditorUiLayerGroups(NavRect);
	}
	else if(s_CurrentTab == TAB_HISTORY)
	{
		// TODO: scrollable region

		CHistoryEntry* pFirstEntry = m_pHistoryEntryCurrent;
		while(pFirstEntry->m_pPrev)
			pFirstEntry = pFirstEntry->m_pPrev;

		static CUIButtonState s_ButEntry[50];
		const float ButtonHeight = 20.0f;
		const float Spacing = 2.0f;

		CHistoryEntry* pCurrentEntry = pFirstEntry;
		int i = 0;
		while(pCurrentEntry)
		{
			NavRect.HSplitTop(ButtonHeight*2, &ButtonRect, &NavRect);
			NavRect.HSplitTop(Spacing, 0, &NavRect);

			// somewhat hacky
			CUIButtonState& ButState = s_ButEntry[i % (sizeof(s_ButEntry)/sizeof(s_ButEntry[0]))];
			UiDoButtonBehavior(pCurrentEntry, ButtonRect, &ButState);

			// clickety click, restore to this entry
			if(ButState.m_Clicked && pCurrentEntry != m_pHistoryEntryCurrent)
			{
				HistoryRestoreToEntry(pCurrentEntry);
			}

			vec4 ColorButton = StyleColorButton;
			if(ButState.m_Pressed)
				ColorButton = StyleColorButtonPressed;
			else if(ButState.m_Hovered)
				ColorButton = StyleColorButtonHover;

			vec4 ColorBorder = StyleColorButtonBorder;
			if(pCurrentEntry == m_pHistoryEntryCurrent)
				ColorBorder = vec4(1, 0, 0, 1);
			DrawRectBorder(ButtonRect, ColorButton, 1, ColorBorder);

			CUIRect ButTopRect, ButBotRect;
			ButtonRect.HSplitMid(&ButTopRect, &ButBotRect);

			DrawText(ButTopRect, pCurrentEntry->m_aActionStr, 8.0f, vec4(1, 1, 1, 1));
			DrawText(ButBotRect, pCurrentEntry->m_aDescStr, 8.0f, vec4(0, 0.5, 1, 1));

			pCurrentEntry = pCurrentEntry->m_pNext;
			i++;
		}
	}


	if(m_UiCurrentPopupID == POPUP_BRUSH_PALETTE)
		RenderPopupBrushPalette();
}

void CEditor::RenderMapEditorUiLayerGroups(CUIRect NavRect)
{
	CUIRect DetailRect, ButtonRect;

	NavRect.HSplitBottom(200, &NavRect, &DetailRect);
	UI()->ClipEnable(&NavRect);

	const int GroupCount = m_Map.m_aGroups.Count();
	const int TotalLayerCount = m_Map.m_aLayers.Count();

	static CDynArraySB<CUIButtonState, 64> s_UiGroupButState;
	static CDynArraySB<CUIButtonState, 64> s_UiGroupShowButState;
	static CDynArraySB<CUIButtonState, 128> s_UiLayerButState;
	static CDynArraySB<CUIButtonState, 128> s_UiLayerShowButState;

	s_UiGroupButState.set_size_zero(GroupCount);
	s_UiGroupShowButState.set_size_zero(GroupCount);
	s_UiLayerButState.set_size_zero(TotalLayerCount);
	s_UiLayerShowButState.set_size_zero(TotalLayerCount);

	m_UiGroupOpen.set_size_zero(GroupCount);
	m_UiGroupHidden.set_size_zero(GroupCount);
	m_UiLayerHovered.set_size_zero(TotalLayerCount);
	m_UiLayerHidden.set_size_zero(TotalLayerCount);

	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float ShowButtonWidth = 15.0f;

	for(int gi = 0; gi < GroupCount; gi++)
	{
		if(gi != 0)
			NavRect.HSplitTop(Spacing, 0, &NavRect);
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);

		CUIRect ExpandBut, ShowButton;

		// show button
		ButtonRect.VSplitRight(ShowButtonWidth, &ButtonRect, &ShowButton);
		CUIButtonState& ShowButState = s_UiGroupShowButState[gi];
		UiDoButtonBehavior(&ShowButState, ShowButton, &ShowButState);

		if(ShowButState.m_Clicked)
			m_UiGroupHidden[gi] ^= 1;

		const bool IsShown = !m_UiGroupHidden[gi];

		vec4 ShowButColor = StyleColorButton;
		if(ShowButState.m_Hovered)
			ShowButColor = StyleColorButtonHover;
		if(ShowButState.m_Pressed)
			ShowButColor = StyleColorButtonPressed;

		DrawRectBorder(ShowButton, ShowButColor, 1, StyleColorButtonBorder);
		DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

		// group button
		CUIButtonState& ButState = s_UiGroupButState[gi];
		UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

		if(ButState.m_Clicked)
			m_UiGroupOpen[gi] ^= 1;

		const bool IsOpen = m_UiGroupOpen[gi];

		vec4 ButColor = StyleColorButton;
		if(ButState.m_Hovered)
			ButColor = StyleColorButtonHover;
		if(ButState.m_Pressed)
			ButColor = StyleColorButtonPressed;

		if(IsOpen)
			DrawRectBorder(ButtonRect, ButColor, 1, StyleColorButtonBorder);
		else
			DrawRect(ButtonRect, ButColor);


		ButtonRect.VSplitLeft(ButtonRect.h, &ExpandBut, &ButtonRect);
		DrawText(ExpandBut, IsOpen ? "-" : "+", FontSize);

		char aGroupName[64];
		if(m_Map.m_GameGroupID == gi)
			str_format(aGroupName, sizeof(aGroupName), "Game group");
		else
			str_format(aGroupName, sizeof(aGroupName), "Group #%d", gi);
		DrawText(ButtonRect, aGroupName, FontSize);

		if(m_UiGroupOpen[gi])
		{
			const int LayerCount = m_Map.m_aGroups[gi].m_LayerCount;

			for(int li = 0; li < LayerCount; li++)
			{
				const int LyID = m_Map.m_aGroups[gi].m_apLayerIDs[li];
				NavRect.HSplitTop(Spacing, 0, &NavRect);
				NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
				ButtonRect.VSplitLeft(10.0f, 0, &ButtonRect);

				dbg_assert(LyID >= 0 && LyID < m_Map.m_aLayers.Count(), "LayerID out of bounds");

				// check whole line for hover
				CUIButtonState WholeLineState;
				UiDoButtonBehavior(0, ButtonRect, &WholeLineState);
				m_UiLayerHovered[LyID] = WholeLineState.m_Hovered;

				// show button
				ButtonRect.VSplitRight(ShowButtonWidth, &ButtonRect, &ShowButton);
				CUIButtonState& ShowButState = s_UiLayerShowButState[LyID];
				UiDoButtonBehavior(&ShowButState, ShowButton, &ShowButState);

				if(ShowButState.m_Clicked)
					m_UiLayerHidden[LyID] ^= 1;

				const bool IsShown = !m_UiLayerHidden[LyID];

				vec4 ShowButColor = StyleColorButton;
				if(ShowButState.m_Hovered)
					ShowButColor = StyleColorButtonHover;
				if(ShowButState.m_Pressed)
					ShowButColor = StyleColorButtonPressed;

				DrawRectBorder(ShowButton, ShowButColor, 1, StyleColorButtonBorder);
				DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

				// layer button
				CUIButtonState& ButState = s_UiLayerButState[LyID];
				UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

				vec4 ButColor = StyleColorButton;
				if(ButState.m_Hovered)
					ButColor = StyleColorButtonHover;
				if(ButState.m_Pressed)
					ButColor = StyleColorButtonPressed;

				if(ButState.m_Clicked)
				{
					m_UiSelectedLayerID = LyID;
					m_UiSelectedGroupID = gi;
				}

				const bool IsSelected = m_UiSelectedLayerID == LyID;

				if(IsSelected)
					DrawRectBorder(ButtonRect, ButColor, 1, vec4(1, 0, 0, 1));
				else
					DrawRect(ButtonRect, ButColor);

				char aLayerName[64];
				const int ImageID = m_Map.m_aLayers[LyID].m_ImageID;
				if(m_Map.m_GameLayerID == LyID)
					str_format(aLayerName, sizeof(aLayerName), "Game Layer");
				else
					str_format(aLayerName, sizeof(aLayerName), "%s (%s)",
							   GetLayerName(LyID),
							   ImageID >= 0 ? m_Map.m_Assets.m_aImageNames[ImageID].m_Buff : "none");
				DrawText(ButtonRect, aLayerName, FontSize);
			}
		}
	}

	UI()->ClipDisable(); // NavRect

	// Add buttons
	CUIRect ButtonRect2;
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	static CUIButtonState s_ButAddTileLayer, s_ButAddQuadLayer, s_ButAddGroup;
	if(UiButton(ButtonRect2, Localize("New group"), &s_ButAddGroup))
	{
		EditCreateAndAddGroup();
	}

	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	if(UiButton(ButtonRect, Localize("T+"), &s_ButAddTileLayer))
	{
		m_UiSelectedLayerID = EditCreateAndAddTileLayerUnder(m_UiSelectedLayerID, m_UiSelectedGroupID);
	}

	if(UiButton(ButtonRect2, Localize("Q+"), &s_ButAddQuadLayer))
	{
		m_UiSelectedLayerID = EditCreateAndAddQuadLayerUnder(m_UiSelectedLayerID, m_UiSelectedGroupID);
	}


	// GROUP/LAYER DETAILS
	dbg_assert(m_UiSelectedLayerID >= 0, "No layer selected");

	// group
	dbg_assert(m_UiSelectedGroupID >= 0, "Parent group of selected layer not found");
	CEditorMap::CGroup& SelectedGroup = m_Map.m_aGroups[m_UiSelectedGroupID];
	const bool IsGameGroup = m_UiSelectedGroupID == m_Map.m_GameGroupID;
	char aBuff[128];

	// label
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	DrawRect(ButtonRect, StyleColorButtonPressed);

	// delete button
	if(!IsGameGroup)
	{
		CUIRect DelButRect;
		ButtonRect.VSplitRight(15, &ButtonRect, &DelButRect);
		static CUIButtonState s_GroupDeleteButton;
		if(UiButtonEx(DelButRect, "x", &s_GroupDeleteButton, vec4(0.4, 0.04, 0.04, 1),
			vec4(0.96, 0.16, 0.16, 1), vec4(0.31, 0, 0, 1), vec4(0.63, 0.035, 0.035, 1), 10))
		{
			EditDeleteGroup(m_UiSelectedGroupID);
		}
	}

	if(IsGameGroup)
		str_format(aBuff, sizeof(aBuff), Localize("Game Group"));
	else
		str_format(aBuff, sizeof(aBuff), "Group #%d", m_UiSelectedGroupID);
	DrawText(ButtonRect, aBuff, FontSize);

	// parallax
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawText(ButtonRect, Localize("Parallax"), FontSize);

	ButtonRect2.VSplitMid(&ButtonRect, &ButtonRect2);
	static CUIIntegerInputState s_IntInpParallaxX, s_IntInpParallaxY;
	int GroupParallaxX = SelectedGroup.m_ParallaxX;
	int GroupParallaxY = SelectedGroup.m_ParallaxY;
	bool ParallaxChanged = false;
	ParallaxChanged |= UiIntegerInput(ButtonRect, &GroupParallaxX, &s_IntInpParallaxX);
	ParallaxChanged |= UiIntegerInput(ButtonRect2, &GroupParallaxY, &s_IntInpParallaxY);
	if(ParallaxChanged)
		EditGroupChangeParallax(m_UiSelectedGroupID, GroupParallaxX, GroupParallaxY);

	// position
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	DrawRect(ButtonRect, vec4(0,0,0,1));
	str_format(aBuff, sizeof(aBuff), "Offset = (%d, %d)",
			   SelectedGroup.m_OffsetX, SelectedGroup.m_OffsetY);
	DrawText(ButtonRect, aBuff, FontSize);


	// layer
	CEditorMap::CLayer& SelectedLayer = m_Map.m_aLayers[m_UiSelectedLayerID];
	const bool IsGameLayer = m_UiSelectedLayerID == m_Map.m_GameLayerID;

	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);

	// delete button
	if(!IsGameLayer)
	{
		CUIRect DelButRect;
		ButtonRect.VSplitRight(15, &ButtonRect, &DelButRect);
		static CUIButtonState s_LayerDeleteButton;
		if(UiButtonEx(DelButRect, "x", &s_LayerDeleteButton, vec4(0.4, 0.04, 0.04, 1),
			vec4(0.96, 0.16, 0.16, 1), vec4(0.31, 0, 0, 1), vec4(0.63, 0.035, 0.035, 1), 10))
		{
			EditDeleteLayer(m_UiSelectedLayerID, m_UiSelectedGroupID);
		}
	}

	// label
	DrawRect(ButtonRect, StyleColorButtonPressed);
	DrawText(ButtonRect, IsGameLayer ? Localize("Game Layer") : GetLayerName(m_UiSelectedLayerID), FontSize);

	// tile layer
	if(SelectedLayer.IsTileLayer())
	{
		// size
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Width = %d Height = %d",
			SelectedLayer.m_Width, SelectedLayer.m_Height);
		DrawText(ButtonRect, aBuff, FontSize);

		// game layer
		if(IsGameLayer)
		{

		}
		else
		{
			// image
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			static CUIButtonState s_ImageButton;
			const char* pText = SelectedLayer.m_ImageID >= 0 ?
				m_Map.m_Assets.m_aImageNames[SelectedLayer.m_ImageID].m_Buff : Localize("none");
			if(UiButton(ButtonRect, pText, &s_ImageButton, FontSize))
			{
				int ImageID = SelectedLayer.m_ImageID + 1;
				if(ImageID >= m_Map.m_Assets.m_ImageCount)
					ImageID = -1;
				EditLayerChangeImage(m_UiSelectedLayerID, ImageID);
			}

			// color
			CUIRect ColorRect;
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			ButtonRect.VSplitMid(&ButtonRect, &ColorRect);

			DrawRect(ButtonRect, vec4(0,0,0,1));
			DrawText(ButtonRect, "Color", FontSize);
			DrawRect(ColorRect, SelectedLayer.m_Color);
		}
	}
	else if(SelectedLayer.IsQuadLayer())
	{
		// image
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		static CUIButtonState s_ImageButton;
		const char* pText = SelectedLayer.m_ImageID >= 0 ?
			m_Map.m_Assets.m_aImageNames[SelectedLayer.m_ImageID].m_Buff : Localize("none");
		if(UiButton(ButtonRect, pText, &s_ImageButton, FontSize))
		{
			int ImageID = SelectedLayer.m_ImageID + 1;
			if(ImageID >= m_Map.m_Assets.m_ImageCount)
				ImageID = -1;
			EditLayerChangeImage(m_UiSelectedLayerID, ImageID);
		}

		// quad count
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Quads = %d", SelectedLayer.m_aQuads.Count());
		DrawText(ButtonRect, aBuff, FontSize);
	}
}

void CEditor::RenderPopupBrushPalette()
{
	dbg_assert(m_UiSelectedLayerID >= 0 && m_UiSelectedLayerID < m_Map.m_aLayers.Count(),
			   "m_UiSelectedLayerID is out of bounds");

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect MainRect = {0, 0, m_UiMainViewRect.h, m_UiMainViewRect.h};
	MainRect.x += (m_UiMainViewRect.w - MainRect.w) * 0.5;
	MainRect.Margin(50.0f, &MainRect);
	m_UiPopupBrushPaletteRect = MainRect;
	DrawRect(MainRect, StyleColorBg);

	CUIRect TopRow;
	MainRect.HSplitTop(40, &TopRow, &MainRect);

	const CEditorMap::CLayer& SelectedTileLayer = m_Map.m_aLayers[m_UiSelectedLayerID];
	dbg_assert(SelectedTileLayer.IsTileLayer(), "Selected layer is not a tile layer");

	IGraphics::CTextureHandle PaletteTexture;
	if(m_UiSelectedLayerID == m_Map.m_GameLayerID)
		PaletteTexture = m_EntitiesTexture;
	else
		PaletteTexture = m_Map.m_Assets.m_aTextureHandle[SelectedTileLayer.m_ImageID];

	CUIRect ImageRect = MainRect;
	ImageRect.w = min(ImageRect.w, ImageRect.h);
	ImageRect.h = ImageRect.w;
	m_UiPopupBrushPaletteImageRect = ImageRect;

	// checker background
	Graphics()->BlendNone();
	Graphics()->TextureSet(m_CheckerTexture);

	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	Graphics()->QuadsSetSubset(0, 0, 64.f, 64.f);
	IGraphics::CQuadItem BgQuad(ImageRect.x, ImageRect.y, ImageRect.w, ImageRect.h);
	Graphics()->QuadsDrawTL(&BgQuad, 1);
	Graphics()->QuadsEnd();

	// palette image
	Graphics()->BlendNormal();
	Graphics()->TextureSet(PaletteTexture);

	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	Graphics()->QuadsSetSubset(0, 0, 1, 1);
	IGraphics::CQuadItem Quad(ImageRect.x, ImageRect.y, ImageRect.w, ImageRect.h);
	Graphics()->QuadsDrawTL(&Quad, 1);
	Graphics()->QuadsEnd();

	const float TileSize = ImageRect.w / 16.f;

	CUIBrushPaletteState& Bps = m_UiBrushPaletteState;
	u8* aTileSelected = Bps.m_aTileSelected;

	// right click clears brush
	if(UI()->MouseButtonClicked(1))
	{
		mem_zero(Bps.m_aTileSelected, sizeof(Bps.m_aTileSelected));
		ClearBrush();
	}

	// do mouse dragging
	static CUIMouseDragState s_DragState;
	bool FinishedDragging = UiDoMouseDragging(&s_DragState, m_UiPopupBrushPaletteImageRect, &s_DragState);
	// TODO: perhaps allow dragging from outside the popup for convenience

	// finished dragging
	if(FinishedDragging)
	{
		u8* aTileSelected = Bps.m_aTileSelected;
		const float TileSize = m_UiPopupBrushPaletteImageRect.w / 16.f;
		const vec2 RelMouseStartPos = s_DragState.m_StartDragPos -
			vec2(m_UiPopupBrushPaletteImageRect.x, m_UiPopupBrushPaletteImageRect.y);
		const vec2 RelMouseEndPos = s_DragState.m_EndDragPos -
			vec2(m_UiPopupBrushPaletteImageRect.x, m_UiPopupBrushPaletteImageRect.y);
		const int DragStartTileX = clamp(RelMouseStartPos.x / TileSize, 0.f, 15.f);
		const int DragStartTileY = clamp(RelMouseStartPos.y / TileSize, 0.f, 15.f);
		const int DragEndTileX = clamp(RelMouseEndPos.x / TileSize, 0.f, 15.f);
		const int DragEndTileY = clamp(RelMouseEndPos.y / TileSize, 0.f, 15.f);

		const int DragTLX = min(DragStartTileX, DragEndTileX);
		const int DragTLY = min(DragStartTileY, DragEndTileY);
		const int DragBRX = max(DragStartTileX, DragEndTileX);
		const int DragBRY = max(DragStartTileY, DragEndTileY);

		for(int ty = DragTLY; ty <= DragBRY; ty++)
		{
			for(int tx = DragTLX; tx <= DragBRX; tx++)
			{
				aTileSelected[ty * 16 + tx] = 1;
			}
		}

		int StartX = 999;
		int StartY = 999;
		int EndX = -1;
		int EndY = -1;
		for(int ti = 0; ti < 256; ti++)
		{
			if(aTileSelected[ti])
			{
				int tx = (ti & 0xF);
				int ty = (ti / 16);
				StartX = min(StartX, tx);
				StartY = min(StartY, ty);
				EndX = max(EndX, tx);
				EndY = max(EndY, ty);
			}
		}

		EndX++;
		EndY++;

		CDynArray<CTile> aBrushTiles = m_Map.NewTileArray();
		const int Width = EndX - StartX;
		const int Height = EndY - StartY;
		aBrushTiles.AddEmpty(Width * Height);

		const int LastTid = (EndY-1)*16+EndX;
		for(int ti = (StartY*16+StartX); ti < LastTid; ti++)
		{
			if(aTileSelected[ti])
			{
				int tx = (ti & 0xF) - StartX;
				int ty = (ti / 16) - StartY;
				aBrushTiles[ty * Width + tx].m_Index = ti;
			}
		}

		SetNewBrush(aBrushTiles.Data(), Width, Height);
	}

	// selected overlay
	for(int ti = 0; ti < 256; ti++)
	{
		if(aTileSelected[ti])
		{
			const int tx = ti & 0xF;
			const int ty = ti / 16;
			CUIRect TileRect = {
				ImageRect.x + tx*TileSize,
				ImageRect.y + ty*TileSize,
				TileSize, TileSize
			};
			DrawRect(TileRect, StyleColorTileSelection);
		}
	}

	// hover tile
	if(!s_DragState.m_IsDragging && UI()->MouseInside(&ImageRect))
	{
		const vec2 RelMousePos = m_UiMousePos - vec2(ImageRect.x, ImageRect.y);
		const int HoveredTileX = RelMousePos.x / TileSize;
		const int HoveredTileY = RelMousePos.y / TileSize;

		CUIRect HoverTileRect = {
			ImageRect.x + HoveredTileX*TileSize,
			ImageRect.y + HoveredTileY*TileSize,
			TileSize, TileSize
		};
		DrawRectBorderOutside(HoverTileRect, StyleColorTileHover, 2, StyleColorTileHoverBorder);
	}

	// drag rectangle
	if(s_DragState.m_IsDragging/* &&
	   (UI()->MouseInside(&ImageRect) || IsInsideRect(s_DragState.m_StartDragPos, ImageRect))*/)
	{
		const vec2 RelMouseStartPos = s_DragState.m_StartDragPos - vec2(ImageRect.x, ImageRect.y);
		const vec2 RelMouseEndPos = m_UiMousePos - vec2(ImageRect.x, ImageRect.y);
		const int DragStartTileX = clamp(RelMouseStartPos.x / TileSize, 0.f, 15.f);
		const int DragStartTileY = clamp(RelMouseStartPos.y / TileSize, 0.f, 15.f);
		const int DragEndTileX = clamp(RelMouseEndPos.x / TileSize, 0.f, 15.f);
		const int DragEndTileY = clamp(RelMouseEndPos.y / TileSize, 0.f, 15.f);

		const int DragTLX = min(DragStartTileX, DragEndTileX);
		const int DragTLY = min(DragStartTileY, DragEndTileY);
		const int DragBRX = max(DragStartTileX, DragEndTileX);
		const int DragBRY = max(DragStartTileY, DragEndTileY);

		CUIRect DragTileRect = {
			ImageRect.x + DragTLX*TileSize,
			ImageRect.y + DragTLY*TileSize,
			(DragBRX-DragTLX+1)*TileSize,
			(DragBRY-DragTLY+1)*TileSize
		};
		DrawRectBorder(DragTileRect, StyleColorTileHover, 2, StyleColorTileHoverBorder);
	}

	CUIRect ButtonRect;
	TopRow.Margin(3.0f, &TopRow);
	TopRow.VSplitLeft(100, &ButtonRect, &TopRow);

	static CUIButtonState s_ButClear;
	if(UiButton(ButtonRect, Localize("Clear"), &s_ButClear))
	{
		mem_zero(aTileSelected, sizeof(u8)*256);
		ClearBrush();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(100, &ButtonRect, &TopRow);

	static CUIButtonState s_ButEraser;
	if(UiButton(ButtonRect, Localize("Eraser"), &s_ButEraser))
	{
		mem_zero(aTileSelected, sizeof(u8)*256);
		aTileSelected[0] = 1; // TODO: don't show this, go into "eraser" state
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(40, &ButtonRect, &TopRow);
	static CUIButtonState s_ButFlipX;
	if(UiButton(ButtonRect, Localize("X/X"), &s_ButFlipX))
	{
		BrushFlipX();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(40, &ButtonRect, &TopRow);
	static CUIButtonState s_ButFlipY;
	if(UiButton(ButtonRect, Localize("Y/Y"), &s_ButFlipY))
	{
		BrushFlipY();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(50, &ButtonRect, &TopRow);
	static CUIButtonState s_ButRotClockwise;
	if(UiButton(ButtonRect, "90° ⟳", &s_ButRotClockwise))
	{
		BrushRotate90Clockwise();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(50, &ButtonRect, &TopRow);
	static CUIButtonState s_ButRotCounterClockwise;
	if(UiButton(ButtonRect, "90° ⟲", &s_ButRotCounterClockwise))
	{
		BrushRotate90CounterClockwise();
	}

	RenderBrush(m_UiMousePos);
}

void CEditor::RenderBrush(vec2 Pos)
{
	if(m_Brush.IsEmpty())
		return;

	float ScreenX0, ScreenX1, ScreenY0, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->MapScreen(ScreenX0 - Pos.x, ScreenY0 - Pos.y,
		ScreenX1 - Pos.x, ScreenY1 - Pos.y);

	const CEditorMap::CLayer& SelectedTileLayer = m_Map.m_aLayers[m_UiSelectedLayerID];
	dbg_assert(SelectedTileLayer.IsTileLayer(), "Selected layer is not a tile layer");

	const float TileSize = 32;
	const CUIRect BrushRect = {0, 0, m_Brush.m_Width * TileSize, m_Brush.m_Height * TileSize};
	DrawRect(BrushRect, vec4(1, 1, 1, 0.1));

	IGraphics::CTextureHandle LayerTexture;
	if(m_UiSelectedLayerID == m_Map.m_GameLayerID)
		LayerTexture = m_EntitiesTexture;
	else
		LayerTexture = m_Map.m_Assets.m_aTextureHandle[SelectedTileLayer.m_ImageID];

	const vec4 White(1, 1, 1, 1);

	Graphics()->TextureSet(LayerTexture);
	Graphics()->BlendNone();
	RenderTools()->RenderTilemap(m_Brush.m_aTiles.Data(), m_Brush.m_Width, m_Brush.m_Height, 32, White,
								 LAYERRENDERFLAG_OPAQUE, 0, 0, -1, 0);
	Graphics()->BlendNormal();
	RenderTools()->RenderTilemap(m_Brush.m_aTiles.Data(), m_Brush.m_Width, m_Brush.m_Height, 32, White,
								 LAYERRENDERFLAG_TRANSPARENT, 0, 0, -1, 0);

	DrawRectBorder(BrushRect, vec4(0, 0, 0, 0), 1, vec4(1, 1, 1, 1));

	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

struct CImageNameItem
{
	int m_Index;
	CEditorMap::CImageName m_Name;
};

static int CompareImageNameItems(const void* pA, const void* pB)
{
	const CImageNameItem& a = *(CImageNameItem*)pA;
	const CImageNameItem& b = *(CImageNameItem*)pB;
	return str_comp_nocase(a.m_Name.m_Buff, b.m_Name.m_Buff);
}

void CEditor::RenderAssetManager()
{
	CEditorMap::CAssets& Assets = m_Map.m_Assets;

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect RightPanel, MainViewRect;
	UiScreenRect.VSplitRight(150, &MainViewRect, &RightPanel);

	DrawRect(RightPanel, StyleColorBg);

	CUIRect NavRect, ButtonRect, DetailRect;
	RightPanel.Margin(3.0f, &NavRect);

	NavRect.HSplitBottom(200, &NavRect, &DetailRect);
	UI()->ClipEnable(&NavRect);

	static CUIButtonState s_UiImageButState[CEditorMap::MAX_IMAGES] = {};
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float ShowButtonWidth = 15.0f;

	static CImageNameItem s_aImageItems[CEditorMap::MAX_IMAGES];
	const int ImageCount = Assets.m_ImageCount;

	// sort images by name
	static float s_LastSortTime = 0;
	if(m_LocalTime - s_LastSortTime > 0.1f) // this should be very fast but still, limit it
	{
		s_LastSortTime = m_LocalTime;
		for(int i = 0; i < ImageCount; i++)
		{
			s_aImageItems[i].m_Index = i;
			s_aImageItems[i].m_Name = Assets.m_aImageNames[i];
		}

		qsort(s_aImageItems, ImageCount, sizeof(s_aImageItems[0]), CompareImageNameItems);
	}

	for(int i = 0; i < ImageCount; i++)
	{
		if(i != 0)
			NavRect.HSplitTop(Spacing, 0, &NavRect);
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);

		const bool Selected = (m_UiSelectedImageID == s_aImageItems[i].m_Index);
		const vec4 ColBorder = Selected ? vec4(1, 0, 0, 1) : StyleColorButtonBorder;
		if(UiButtonEx(ButtonRect, s_aImageItems[i].m_Name.m_Buff, &s_UiImageButState[i],
			StyleColorButton,StyleColorButtonHover, StyleColorButtonPressed, ColBorder, FontSize))
		{
			m_UiSelectedImageID = s_aImageItems[i].m_Index;
		}
	}

	UI()->ClipDisable(); // NavRect

	if(m_UiSelectedImageID == -1 && Assets.m_ImageCount > 0)
		m_UiSelectedImageID = 0;

	if(m_UiSelectedImageID != -1)
	{
		// display image
		CUIRect ImageRect = MainViewRect;
		ImageRect.w = Assets.m_aTextureInfos[m_UiSelectedImageID].m_Width/m_GfxScreenWidth *
			UiScreenRect.w * (1.0/m_Zoom);
		ImageRect.h = Assets.m_aTextureInfos[m_UiSelectedImageID].m_Height/m_GfxScreenHeight *
			UiScreenRect.h * (1.0/m_Zoom);

		UI()->ClipEnable(&MainViewRect);

		Graphics()->TextureSet(m_CheckerTexture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0, 0, ImageRect.w/16.f, ImageRect.h/16.f);
		IGraphics::CQuadItem BgQuad(ImageRect.x, ImageRect.y, ImageRect.w, ImageRect.h);
		Graphics()->QuadsDrawTL(&BgQuad, 1);
		Graphics()->QuadsEnd();

		Graphics()->WrapClamp();

		Graphics()->TextureSet(Assets.m_aTextureHandle[m_UiSelectedImageID]);
		Graphics()->QuadsBegin();
		IGraphics::CQuadItem Quad(ImageRect.x, ImageRect.y, ImageRect.w, ImageRect.h);
		Graphics()->QuadsDrawTL(&Quad, 1);
		Graphics()->QuadsEnd();

		Graphics()->WrapNormal();
		UI()->ClipDisable();

		// details

		// label
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, StyleColorButtonPressed);
		DrawText(ButtonRect, Localize("Image"), FontSize);

		// name
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		DrawText(ButtonRect, Assets.m_aImageNames[m_UiSelectedImageID].m_Buff, FontSize);

		// size
		char aBuff[128];
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "size = (%d, %d)",
			Assets.m_aTextureInfos[m_UiSelectedImageID].m_Width,
			Assets.m_aTextureInfos[m_UiSelectedImageID].m_Height);
		DrawText(ButtonRect, aBuff, FontSize);

		// size
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Embedded Crc = 0x%X",
			Assets.m_aImageEmbeddedCrc[m_UiSelectedImageID]);
		DrawText(ButtonRect, aBuff, FontSize);

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUIButtonState s_ButDelete = {};
		if(UiButton(ButtonRect, Localize("Delete"), &s_ButDelete))
		{
			EditDeleteImage(m_UiSelectedImageID);
		}

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUITextInputState s_TextInputAdd = {};
		static char aAddPath[256] = "grass_main.png";
		UiTextInput(ButtonRect, aAddPath, sizeof(aAddPath), &s_TextInputAdd);

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUIButtonState s_ButAdd = {};
		if(UiButton(ButtonRect, Localize("Add image"), &s_ButAdd))
		{
			EditAddImage(aAddPath);
		}
	}

	if(m_UiSelectedImageID >= Assets.m_ImageCount)
		m_UiSelectedImageID = -1;
}

inline void CEditor::DrawRect(const CUIRect& Rect, const vec4& Color)
{
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
	Graphics()->QuadsDrawTL(&Quad, 1);
	Graphics()->QuadsEnd();
}

void CEditor::DrawRectBorder(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float FakeToScreenX = m_GfxScreenWidth/(ScreenX1-ScreenX0);
	const float FakeToScreenY = m_GfxScreenHeight/(ScreenY1-ScreenY0);
	const float BorderW = Border/FakeToScreenX;
	const float BorderH = Border/FakeToScreenY;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// border pass
	if(Color.a == 1)
	{
		IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(&Quad, 1);
	}
	else
	{
		// border pass
		IGraphics::CQuadItem Quads[4] = {
			IGraphics::CQuadItem(Rect.x, Rect.y, BorderW, Rect.h),
			IGraphics::CQuadItem(Rect.x+Rect.w-BorderW, Rect.y, BorderW, Rect.h),
			IGraphics::CQuadItem(Rect.x+BorderW, Rect.y, Rect.w-BorderW, BorderH),
			IGraphics::CQuadItem(Rect.x+BorderW, Rect.y+Rect.h-BorderH, Rect.w-BorderW, BorderH)
		};
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(Quads, 4);
	}

	// front pass
	if(Color.a > 0.001)
	{
		IGraphics::CQuadItem QuadCenter(Rect.x + BorderW, Rect.y + BorderH,
										Rect.w - BorderW*2, Rect.h - BorderH*2);
		Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a,
							 Color.b*Color.a, Color.a);
		Graphics()->QuadsDrawTL(&QuadCenter, 1);
	}

	Graphics()->QuadsEnd();
}

void CEditor::DrawRectBorderOutside(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float FakeToScreenX = m_GfxScreenWidth/(ScreenX1-ScreenX0);
	const float FakeToScreenY = m_GfxScreenHeight/(ScreenY1-ScreenY0);
	const float BorderW = Border/FakeToScreenX;
	const float BorderH = Border/FakeToScreenY;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// border pass
	if(Color.a == 1)
	{
		IGraphics::CQuadItem Quad(Rect.x-BorderW, Rect.y-BorderH, Rect.w+BorderW*2, Rect.h+BorderH*2);
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(&Quad, 1);
	}
	else
	{
		// border pass
		IGraphics::CQuadItem Quads[4] = {
			IGraphics::CQuadItem(Rect.x-BorderW, Rect.y-BorderH, BorderW, Rect.h+BorderH*2),
			IGraphics::CQuadItem(Rect.x+Rect.w, Rect.y-BorderH, BorderW, Rect.h+BorderH*2),
			IGraphics::CQuadItem(Rect.x, Rect.y-BorderH, Rect.w, BorderH),
			IGraphics::CQuadItem(Rect.x, Rect.y+Rect.h, Rect.w, BorderH)
		};
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(Quads, 4);
	}

	// front pass
	if(Color.a > 0.001)
	{
		IGraphics::CQuadItem QuadCenter(Rect.x, Rect.y, Rect.w, Rect.h);
		Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a,
							 Color.b*Color.a, Color.a);
		Graphics()->QuadsDrawTL(&QuadCenter, 1);
	}

	Graphics()->QuadsEnd();
}

void CEditor::DrawRectBorderMiddle(const CUIRect& Rect, const vec4& Color, float Border, const vec4 BorderColor)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	const float FakeToScreenX = m_GfxScreenWidth/(ScreenX1-ScreenX0);
	const float FakeToScreenY = m_GfxScreenHeight/(ScreenY1-ScreenY0);
	const float BorderW = Border/FakeToScreenX;
	const float BorderH = Border/FakeToScreenY;
	const float BorderWhalf = BorderW/2.f;
	const float BorderHhalf = BorderH/2.f;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// border pass
	if(Color.a == 1)
	{
		IGraphics::CQuadItem Quad(Rect.x-BorderWhalf, Rect.y-BorderHhalf, Rect.w+BorderW, Rect.h+BorderH);
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(&Quad, 1);
	}
	else
	{
		// border pass
		IGraphics::CQuadItem Quads[4] = {
			IGraphics::CQuadItem(Rect.x-BorderWhalf, Rect.y-BorderHhalf, BorderW, Rect.h+BorderH),
			IGraphics::CQuadItem(Rect.x+Rect.w-BorderHhalf, Rect.y-BorderHhalf, BorderW, Rect.h+BorderH),
			IGraphics::CQuadItem(Rect.x, Rect.y-BorderHhalf, Rect.w, BorderH),
			IGraphics::CQuadItem(Rect.x, Rect.y+Rect.h-BorderHhalf, Rect.w, BorderH)
		};
		Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
							 BorderColor.b*BorderColor.a, BorderColor.a);
		Graphics()->QuadsDrawTL(Quads, 4);
	}

	// front pass
	if(Color.a > 0.001)
	{
		IGraphics::CQuadItem QuadCenter(Rect.x+BorderWhalf, Rect.y+BorderHhalf,
			Rect.w-BorderW, Rect.h-BorderH);
		Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a,
							 Color.b*Color.a, Color.a);
		Graphics()->QuadsDrawTL(&QuadCenter, 1);
	}

	Graphics()->QuadsEnd();
}

inline void CEditor::DrawText(const CUIRect& Rect, const char* pText, float FontSize, vec4 Color)
{
	const float OffY = (Rect.h - FontSize - 3.0f) * 0.5f;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, Rect.x + OffY, Rect.y + OffY, FontSize, TEXTFLAG_RENDER);
	TextRender()->TextShadowed(&Cursor, pText, -1, vec2(0,0), vec4(0,0,0,0), Color);
}

void CEditor::UiDoButtonBehavior(const void* pID, const CUIRect& Rect, CUIButtonState* pButState)
{
	pButState->m_Clicked = false;

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
		{
			pButState->m_Clicked = true;
			UI()->SetActiveItem(0);
		}
	}

	pButState->m_Hovered = false;
	pButState->m_Pressed = false;

	if(UI()->MouseInside(&Rect))
	{
		pButState->m_Hovered = true;
		if(pID)
			UI()->SetHotItem(pID);

		if(UI()->MouseButton(0))
		{
			pButState->m_Pressed = true;
			if(pID && UI()->MouseButtonClicked(0))
				UI()->SetActiveItem(pID);
		}
	}
	else if(pID && UI()->HotItem() == pID)
	{
		UI()->SetHotItem(0);
	}

}

bool CEditor::UiDoMouseDragging(const void* pID, const CUIRect& Rect, CUIMouseDragState* pDragState)
{
	bool Return = false;
	CUIButtonState ButState;
	UiDoButtonBehavior(pID, Rect, &ButState);

	if(UI()->CheckActiveItem(pID) && UI()->MouseButton(0))
	{
		if(!pDragState->m_IsDragging && UI()->MouseButtonClicked(0))
		{
			pDragState->m_StartDragPos = m_UiMousePos;
			pDragState->m_IsDragging = true;
		}
	}
	else
	{
		if(pDragState->m_IsDragging)
		{
			pDragState->m_EndDragPos = m_UiMousePos;
			Return = true; // finished dragging
		}
		pDragState->m_IsDragging = false;
	}

	return Return;
}

bool CEditor::UiButton(const CUIRect& Rect, const char* pText, CUIButtonState* pButState, float FontSize)
{
	return UiButtonEx(Rect, pText, pButState, StyleColorButton, StyleColorButtonHover,
		StyleColorButtonPressed, StyleColorButtonBorder, FontSize);
}

bool CEditor::UiButtonEx(const CUIRect& Rect, const char* pText, CUIButtonState* pButState, vec4 ColNormal,
	vec4 ColHover, vec4 ColPress, vec4 ColBorder, float FontSize)
{
	UiDoButtonBehavior(pButState, Rect, pButState);

	vec4 ShowButColor = ColNormal;
	if(pButState->m_Hovered)
		ShowButColor = ColHover;
	if(pButState->m_Pressed)
		ShowButColor = ColPress;

	DrawRectBorder(Rect, ShowButColor, 1, ColBorder);
	DrawText(Rect, pText, FontSize);
	return pButState->m_Clicked;
}

bool CEditor::UiTextInput(const CUIRect& Rect, char* pText, int TextMaxLength, CUITextInputState* pInputState)
{
	UiDoButtonBehavior(pInputState, Rect, &pInputState->m_Button);
	if(pInputState->m_Button.m_Clicked)
	{
		UI()->SetActiveItem(pInputState);
	}
	else if(UI()->CheckActiveItem(pInputState) && !pInputState->m_Button.m_Hovered &&
		UI()->MouseButtonClicked(0))
	{
		UI()->SetActiveItem(0);
	}

	const bool PrevSelected = pInputState->m_Selected;
	pInputState->m_Selected = UI()->CheckActiveItem(pInputState);

	// just got selected
	if(!PrevSelected && pInputState->m_Selected)
	{
		pInputState->m_CursorPos = str_length(pText);
	}

	static float s_StartBlinkTime = 0;
	const int OldCursorPos = pInputState->m_CursorPos;

	bool Changed = false;
	if(pInputState->m_Selected)
	{
		m_UiTextInputConsumeKeyboardEvents = true;
		for(int i = 0; i < Input()->NumEvents(); i++)
		{
			int Len = str_length(pText);
			int NumChars = Len;
			Changed |= CLineInput::Manipulate(Input()->GetEvent(i), pText, TextMaxLength, TextMaxLength,
				&Len, &pInputState->m_CursorPos, &NumChars, Input());
		}
	}

	const float FontSize = 8.0f;

	DrawRectBorder(Rect, vec4(0, 0, 0, 1), 1,
		pInputState->m_Selected ? vec4(0,0.2,1,1) : StyleColorButtonBorder);
	DrawText(Rect, pText, FontSize);

	// cursor
	if(OldCursorPos != pInputState->m_CursorPos)
		s_StartBlinkTime = m_LocalTime;

	if(pInputState->m_Selected && fmod(m_LocalTime - s_StartBlinkTime, 1.0f) < 0.5f)
	{
		const float OffY = (Rect.h - FontSize - 3.0f) * 0.5f; // see DrawText
		float w = TextRender()->TextWidth(0, FontSize, pText, pInputState->m_CursorPos);
		CUIRect CursorRect = Rect;
		CursorRect.x += w + OffY;
		CursorRect.y += 2;
		CursorRect.w = m_UiScreenRect.w / m_GfxScreenWidth; // 1px
		CursorRect.h -= 4;
		DrawRect(CursorRect, vec4(1,1,1,1));
	}

	return Changed;
}

bool CEditor::UiIntegerInput(const CUIRect& Rect, int* pInteger, CUIIntegerInputState* pInputState)
{
	const int OldInteger = *pInteger;
	const u8 OldSelected = pInputState->m_TextInput.m_Selected;
	UiTextInput(Rect, pInputState->m_aIntBuff, sizeof(pInputState->m_aIntBuff), &pInputState->m_TextInput);

	// string format, when empty or on select/deselect
	if((pInputState->m_aIntBuff[0] == 0 && !OldSelected) || (OldSelected != pInputState->m_TextInput.m_Selected))
	{
		str_format(pInputState->m_aIntBuff, sizeof(pInputState->m_aIntBuff), "%d", *pInteger);
		pInputState->m_TextInput.m_CursorPos = str_length(pInputState->m_aIntBuff);
	}

	// string parse, on return key press
	if(Input()->KeyPress(KEY_RETURN) || Input()->KeyPress(KEY_KP_ENTER))
	{
		if(sscanf(pInputState->m_aIntBuff, "%d", pInteger) == 0)
			*pInteger = 0;
		str_format(pInputState->m_aIntBuff, sizeof(pInputState->m_aIntBuff), "%d", *pInteger);
		pInputState->m_TextInput.m_CursorPos = str_length(pInputState->m_aIntBuff);
	}

	return OldInteger != *pInteger;
}

void CEditor::Reset()
{

}

void CEditor::ResetCamera()
{
	m_Zoom = 1;
	m_MapUiPosOffset = vec2(0,0);
}

void CEditor::ChangeZoom(float Zoom)
{
	// zoom centered on mouse
	const float WorldWidth = m_ZoomWorldViewWidth/m_Zoom;
	const float WorldHeight = m_ZoomWorldViewHeight/m_Zoom;
	const float MuiX = (m_UiMousePos.x+m_MapUiPosOffset.x)/m_UiScreenRect.w;
	const float MuiY = (m_UiMousePos.y+m_MapUiPosOffset.y)/m_UiScreenRect.h;
	const float RelMouseX = MuiX * m_ZoomWorldViewWidth;
	const float RelMouseY = MuiY * m_ZoomWorldViewHeight;
	const float NewZoomWorldViewWidth = WorldWidth * Zoom;
	const float NewZoomWorldViewHeight = WorldHeight * Zoom;
	const float NewRelMouseX = MuiX * NewZoomWorldViewWidth;
	const float NewRelMouseY = MuiY * NewZoomWorldViewHeight;
	m_MapUiPosOffset.x -= (NewRelMouseX-RelMouseX)/NewZoomWorldViewWidth*m_UiScreenRect.w;
	m_MapUiPosOffset.y -= (NewRelMouseY-RelMouseY)/NewZoomWorldViewHeight*m_UiScreenRect.h;
	m_Zoom = Zoom;
}

void CEditor::ChangePage(int Page)
{
	UI()->SetHotItem(0);
	UI()->SetActiveItem(0);
	m_Page = Page;
}

void CEditor::SetNewBrush(CTile* aTiles, int Width, int Height)
{
	dbg_assert(Width > 0 && Height > 0, "Brush: wrong dimensions");
	m_Brush.m_Width = Width;
	m_Brush.m_Height = Height;
	m_Brush.m_aTiles.Clear();
	m_Brush.m_aTiles.Add(aTiles, Width*Height);
}

void CEditor::ClearBrush()
{
	m_Brush.m_Width = 0;
	m_Brush.m_Height = 0;
	m_Brush.m_aTiles.Clear();
}

void CEditor::BrushFlipX()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	CDynArray<CTile>& aTiles = m_Brush.m_aTiles;
	CDynArray<CTile> aTilesCopy = m_Map.NewTileArray();
	aTilesCopy.Add(aTiles.Data(), aTiles.Count());

	for(int ty = 0; ty < BrushHeight; ty++)
	{
		for(int tx = 0; tx < BrushWidth; tx++)
		{
			const int tid = ty * BrushWidth + tx;
			aTiles[tid] = aTilesCopy[ty * BrushWidth + (BrushWidth-tx-1)];
			aTiles[tid].m_Flags ^= aTiles[tid].m_Flags&TILEFLAG_ROTATE ? TILEFLAG_HFLIP:TILEFLAG_VFLIP;
		}
	}
}

void CEditor::BrushFlipY()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	CDynArray<CTile>& aTiles = m_Brush.m_aTiles;
	CDynArray<CTile> aTilesCopy = m_Map.NewTileArray();
	aTilesCopy.Add(aTiles.Data(), aTiles.Count());

	for(int ty = 0; ty < BrushHeight; ty++)
	{
		for(int tx = 0; tx < BrushWidth; tx++)
		{
			const int tid = ty * BrushWidth + tx;
			aTiles[tid] = aTilesCopy[(BrushHeight-ty-1) * BrushWidth + tx];
			aTiles[tid].m_Flags ^= aTiles[tid].m_Flags&TILEFLAG_ROTATE ? TILEFLAG_VFLIP:TILEFLAG_HFLIP;
		}
	}
}

void CEditor::BrushRotate90Clockwise()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	CDynArray<CTile>& aTiles = m_Brush.m_aTiles;
	CDynArray<CTile> aTilesCopy = m_Map.NewTileArray();
	aTilesCopy.Add(aTiles.Data(), aTiles.Count());

	for(int ty = 0; ty < BrushHeight; ty++)
	{
		for(int tx = 0; tx < BrushWidth; tx++)
		{
			const int tid = tx * BrushHeight + (BrushHeight-1-ty);
			aTiles[tid] = aTilesCopy[ty * BrushWidth + tx];
			if(aTiles[tid].m_Flags&TILEFLAG_ROTATE)
				aTiles[tid].m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP);
			aTiles[tid].m_Flags ^= TILEFLAG_ROTATE;
		}
	}

	m_Brush.m_Width = BrushHeight;
	m_Brush.m_Height = BrushWidth;
}

void CEditor::BrushRotate90CounterClockwise()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	CDynArray<CTile>& aTiles = m_Brush.m_aTiles;
	CDynArray<CTile> aTilesCopy = m_Map.NewTileArray();
	aTilesCopy.Add(aTiles.Data(), aTiles.Count());

	for(int ty = 0; ty < BrushHeight; ty++)
	{
		for(int tx = 0; tx < BrushWidth; tx++)
		{
			const int tid = (BrushWidth-1-tx) * BrushHeight + ty;
			aTiles[tid] = aTilesCopy[ty * BrushWidth + tx];
			if(!(aTiles[tid].m_Flags&TILEFLAG_ROTATE))
				aTiles[tid].m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP);
			aTiles[tid].m_Flags ^= TILEFLAG_ROTATE;
		}
	}

	m_Brush.m_Width = BrushHeight;
	m_Brush.m_Height = BrushWidth;
}

bool CEditor::LoadMap(const char* pFileName)
{
	if(m_Map.Load(pFileName))
	{
		OnMapLoaded();
		ed_log("map '%s' sucessfully loaded.", pFileName);
		return true;
	}
	ed_log("failed to load map '%s'", pFileName);
	return false;
}

void CEditor::OnMapLoaded()
{
	m_UiSelectedLayerID = m_Map.m_GameLayerID;
	m_UiSelectedGroupID = m_Map.m_GameGroupID;
	mem_zero(m_UiGroupHidden.base_ptr(), sizeof(m_UiGroupHidden[0]) * m_UiGroupHidden.capacity());
	m_UiGroupOpen.set_size(m_Map.m_aGroups.Count());
	mem_zero(m_UiGroupOpen.base_ptr(), sizeof(m_UiGroupOpen[0]) * m_UiGroupOpen.capacity());
	m_UiGroupOpen[m_Map.m_GameGroupID] = true;
	mem_zero(m_UiLayerHidden.base_ptr(), sizeof(m_UiLayerHidden[0]) * m_UiLayerHidden.capacity());
	mem_zero(m_UiLayerHovered.base_ptr(), sizeof(m_UiLayerHovered[0]) * m_UiLayerHovered.capacity());
	mem_zero(&m_UiBrushPaletteState, sizeof(m_UiBrushPaletteState));
	ResetCamera();
	ClearBrush();

	// clear history
	if(m_pHistoryEntryCurrent)
	{
		CHistoryEntry* pCurrent = m_pHistoryEntryCurrent->m_pNext;
		while(pCurrent)
		{
			free(pCurrent->m_pSnap);
			pCurrent = pCurrent->m_pNext;
		}

		pCurrent = m_pHistoryEntryCurrent;
		while(pCurrent)
		{
			free(pCurrent->m_pSnap);
			pCurrent = pCurrent->m_pPrev;
		}
		m_HistoryEntryDispenser.Clear();
		m_pHistoryEntryCurrent = nullptr;
	}

	// initialize history
	m_pHistoryEntryCurrent = m_HistoryEntryDispenser.AllocOne();
	m_pHistoryEntryCurrent->m_pSnap = m_Map.SaveSnapshot();
	m_pHistoryEntryCurrent->SetAction("Map loaded");
	m_pHistoryEntryCurrent->SetDescription(m_Map.m_aPath);
}

void CEditor::EditDeleteLayer(int LyID, int ParentGroupID)
{
	dbg_assert(LyID >= 0 && LyID < m_Map.m_aLayers.Count(), "LyID out of bounds");
	dbg_assert(ParentGroupID >= 0 && ParentGroupID < m_Map.m_aGroups.Count(), "ParentGroupID out of bounds");
	dbg_assert(LyID != m_Map.m_GameLayerID, "Can't delete game layer");

	dbg_assert(m_Map.m_aLayers.Count() > 0, "There should be at least a game layer");

	const int SwappedLyID = m_Map.m_aLayers.Count()-1; // see RemoveByIndex
	CEditorMap::CGroup& ParentGroup = m_Map.m_aGroups[ParentGroupID];
	const int ParentGroupLayerCount = ParentGroup.m_LayerCount;
	dbg_assert(ParentGroupLayerCount > 0, "Parent group layer count is zero?");
	const int GroupCount = m_Map.m_aGroups.Count();

	// remove layer id from parent group
	int GroupLayerID = -1;
	int GroupSelectedLayerPos = -1;
	for(int li = 0; li < ParentGroupLayerCount && GroupLayerID == -1 && GroupSelectedLayerPos == -1; li++)
	{
		if(ParentGroup.m_apLayerIDs[li] == LyID)
			GroupLayerID = li;
		if(ParentGroup.m_apLayerIDs[li] == m_UiSelectedLayerID)
			GroupSelectedLayerPos = li;
	}
	dbg_assert(GroupLayerID != -1, "Layer not found inside parent group");

	memmove(&ParentGroup.m_apLayerIDs[GroupLayerID], &ParentGroup.m_apLayerIDs[GroupLayerID+1],
			(ParentGroup.m_LayerCount-GroupLayerID) * sizeof(ParentGroup.m_apLayerIDs[0]));
	ParentGroup.m_LayerCount--;

	// delete actual layer (swap with last)
	m_Map.m_aLayers.RemoveByIndex(LyID);

	// GamelayerID: swap id if needed
	dbg_assert(m_Map.m_GameLayerID != LyID, "Can't delete game layer");
	if(m_Map.m_GameLayerID == SwappedLyID)
		m_Map.m_GameLayerID = LyID;

	// Groups: swap last layer id with deleted id
	for(int gi = 0; gi < GroupCount; gi++)
	{
		CEditorMap::CGroup& Group = m_Map.m_aGroups[gi];
		const int LayerCount = Group.m_LayerCount;
		for(int li = 0; li < LayerCount; li++)
		{
			if(Group.m_apLayerIDs[li] == SwappedLyID)
			{
				Group.m_apLayerIDs[li] = LyID;
				break;
			}
		}
	}

	// UiSelectedLayerID: try to stay at the same selected position
	if(m_UiSelectedLayerID == LyID)
	{
		if(ParentGroup.m_LayerCount > 0 && GroupSelectedLayerPos != -1 &&
		   GroupSelectedLayerPos < ParentGroup.m_LayerCount)
		{
			m_UiSelectedLayerID = ParentGroup.m_apLayerIDs[GroupSelectedLayerPos];
		}
		else
		{
			m_UiSelectedLayerID = m_Map.m_GameLayerID;
			m_UiSelectedGroupID = m_Map.m_GameGroupID;
		}
	}

	// validation checks
#ifdef CONF_DEBUG
	dbg_assert(m_UiSelectedLayerID >= 0 && m_UiSelectedLayerID < m_Map.m_aLayers.Count(),
		"m_UiSelectedLayerID is invalid");
	dbg_assert(m_UiSelectedGroupID >= 0 && m_UiSelectedGroupID < m_Map.m_aGroups.Count(),
		"m_UiSelectedGroupID is invalid");
	dbg_assert(m_Map.m_GameLayerID >= 0 && m_Map.m_GameLayerID < m_Map.m_aLayers.Count(),
		"m_Map.m_GameLayerID is invalid");
	dbg_assert(m_Map.m_GameGroupID >= 0 && m_Map.m_GameGroupID < m_Map.m_aGroups.Count(),
		"m_UiSelectedGroupID is invalid");

	for(int gi = 0; gi < GroupCount; gi++)
	{
		CEditorMap::CGroup& Group = m_Map.m_aGroups[gi];
		const int LayerCount = Group.m_LayerCount;
		for(int li = 0; li < LayerCount; li++)
		{
			dbg_assert(Group.m_apLayerIDs[li] >= 0 && Group.m_apLayerIDs[li] < m_Map.m_aLayers.Count(),
				"Group.m_apLayerIDs[li] is invalid");
		}
	}
#endif

	// history entry
	HistoryNewEntry("Deleted layer", GetLayerName(LyID));
}

void CEditor::EditDeleteGroup(int GroupID)
{
	dbg_assert(GroupID >= 0 && GroupID < m_Map.m_aGroups.Count(), "GroupID out of bounds");
	dbg_assert(GroupID != m_Map.m_GameGroupID, "Can't delete game group");

	CEditorMap::CGroup& Group = m_Map.m_aGroups[GroupID];
	const int LayerCount = m_Map.m_aGroups[GroupID].m_LayerCount;
	while(Group.m_LayerCount > 0)
	{
		EditDeleteLayer(Group.m_apLayerIDs[0], GroupID);
	}

	if(m_Map.m_GameGroupID > GroupID)
		m_Map.m_GameGroupID--; // see RemoveByIndexSlide

	m_Map.m_aGroups.RemoveByIndexSlide(GroupID);

	m_UiSelectedGroupID = m_Map.m_GameGroupID;
	m_UiSelectedLayerID = m_Map.m_GameLayerID;

	// history entry
	char aBuff[64];
	str_format(aBuff, sizeof(aBuff), "Group %d", GroupID);
	HistoryNewEntry("Deleted group", aBuff);
}

void CEditor::EditDeleteImage(int ImgID)
{
	dbg_assert(ImgID >= 0 && ImgID <= m_Map.m_Assets.m_ImageCount, "ImgID out of bounds");

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s", m_Map.m_Assets.m_aImageNames[ImgID]);

	m_Map.AssetsDeleteImage(ImgID);

	// history entry
	HistoryNewEntry("Deleted image", aHistoryEntryDesc);
}

void CEditor::EditAddImage(const char* pFilename)
{
	bool Success = m_Map.AssetsAddAndLoadImage(pFilename);

	// history entry
	if(Success)
	{
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s", pFilename);
		HistoryNewEntry("Added image", aHistoryEntryDesc);
	}
}

void CEditor::EditCreateAndAddGroup()
{
	CEditorMap::CGroup Group;
	Group.m_OffsetX = 0;
	Group.m_OffsetY = 0;
	Group.m_ParallaxX = 100;
	Group.m_ParallaxY = 100;
	Group.m_LayerCount = 0;
	m_Map.m_aGroups.Add(Group);

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Group %d", m_Map.m_aGroups.Count()-1);
	HistoryNewEntry("New group", aHistoryEntryDesc);
}

int CEditor::EditCreateAndAddTileLayerUnder(int UnderLyID, int GroupID)
{
	dbg_assert(UnderLyID >= 0 && UnderLyID < m_Map.m_aLayers.Count(), "LyID out of bounds");
	dbg_assert(GroupID >= 0 && GroupID < m_Map.m_aGroups.Count(), "GroupID out of bounds");

	// base width and height on given layer if it is a tilelayer, else base on game layer
	const CEditorMap::CLayer& TopLayer = m_Map.m_aLayers[UnderLyID];
	int LyWidth = m_Map.m_aLayers[m_Map.m_GameLayerID].m_Width;
	int LyHeight = m_Map.m_aLayers[m_Map.m_GameLayerID].m_Height;
	if(TopLayer.IsTileLayer())
	{
		LyWidth = TopLayer.m_Width;
		LyHeight = TopLayer.m_Height;
	}

	CEditorMap::CLayer& Layer = m_Map.NewTileLayer(LyWidth, LyHeight);
	CEditorMap::CGroup& Group = m_Map.m_aGroups[GroupID];

	int UnderGrpLyID = -1;
	const int ParentGroupLayerCount = Group.m_LayerCount;
	for(int li = 0; li < ParentGroupLayerCount; li++)
	{
		if(Group.m_apLayerIDs[li] == UnderLyID)
		{
			UnderGrpLyID = li;
			break;
		}
	}

	dbg_assert(UnderGrpLyID != -1, "Layer not found in parent group");
	dbg_assert(Group.m_LayerCount < CEditorMap::MAX_GROUP_LAYERS, "Group is full of layers");

	const int GrpLyID = UnderGrpLyID+1;
	memmove(&Group.m_apLayerIDs[GrpLyID+1], &Group.m_apLayerIDs[GrpLyID],
		(Group.m_LayerCount-GrpLyID) * sizeof(Group.m_apLayerIDs[0]));
	Group.m_LayerCount++;
	const int LyID = m_Map.m_aLayers.Count()-1;
	Group.m_apLayerIDs[GrpLyID] = LyID;

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Tile %d", LyID);
	HistoryNewEntry(Localize("New tile layer"), aHistoryEntryDesc);
	return LyID;
}

int CEditor::EditCreateAndAddQuadLayerUnder(int UnderLyID, int GroupID)
{
	dbg_assert(UnderLyID >= 0 && UnderLyID < m_Map.m_aLayers.Count(), "LyID out of bounds");
	dbg_assert(GroupID >= 0 && GroupID < m_Map.m_aGroups.Count(), "GroupID out of bounds");

	CEditorMap::CLayer& Layer = m_Map.NewQuadLayer();
	CEditorMap::CGroup& Group = m_Map.m_aGroups[GroupID];

	int UnderGrpLyID = -1;
	const int ParentGroupLayerCount = Group.m_LayerCount;
	for(int li = 0; li < ParentGroupLayerCount; li++)
	{
		if(Group.m_apLayerIDs[li] == UnderLyID)
		{
			UnderGrpLyID = li;
			break;
		}
	}

	dbg_assert(UnderGrpLyID != -1, "Layer not found in parent group");
	dbg_assert(Group.m_LayerCount < CEditorMap::MAX_GROUP_LAYERS, "Group is full of layers");

	const int GrpLyID = UnderGrpLyID+1;
	memmove(&Group.m_apLayerIDs[GrpLyID+1], &Group.m_apLayerIDs[GrpLyID],
		(Group.m_LayerCount-GrpLyID) * sizeof(Group.m_apLayerIDs[0]));
	Group.m_LayerCount++;
	const int LyID = m_Map.m_aLayers.Count()-1;
	Group.m_apLayerIDs[GrpLyID] = LyID;

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Quad %d", LyID);
	HistoryNewEntry(Localize("New Quad layer"), aHistoryEntryDesc);
	return LyID;
}

void CEditor::EditLayerChangeImage(int LayerID, int NewImageID)
{
	dbg_assert(LayerID >= 0 && LayerID < m_Map.m_aLayers.Count(), "LayerID out of bounds");
	dbg_assert(NewImageID >= -1 && NewImageID < m_Map.m_Assets.m_ImageCount, "NewImageID out of bounds");

	const int OldImageID = m_Map.m_aLayers[LayerID].m_ImageID;
	if(OldImageID == NewImageID)
		return;

	m_Map.m_aLayers[LayerID].m_ImageID = NewImageID;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("%s: changed image"),
		GetLayerName(LayerID));
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s > %s",
		OldImageID < 0 ? Localize("none") : m_Map.m_Assets.m_aImageNames[OldImageID].m_Buff,
		NewImageID < 0 ? Localize("none") : m_Map.m_Assets.m_aImageNames[NewImageID].m_Buff);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor::EditGroupChangeParallax(int GroupID, int NewParallaxX, int NewParallaxY)
{
	dbg_assert(GroupID >= 0 && GroupID < m_Map.m_aGroups.Count(), "GroupID out of bounds");

	CEditorMap::CGroup& Group = m_Map.m_aGroups[GroupID];
	if(NewParallaxX == Group.m_ParallaxX && NewParallaxY == Group.m_ParallaxY)
		return;

	const int OldParallaxX = Group.m_ParallaxX;
	const int OldParallaxY = Group.m_ParallaxY;
	Group.m_ParallaxX = NewParallaxX;
	Group.m_ParallaxY = NewParallaxY;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed parallax"),
		GroupID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
		OldParallaxX, OldParallaxY,
		NewParallaxX, NewParallaxY);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor::HistoryNewEntry(const char* pActionStr, const char* pDescStr)
{
	CHistoryEntry* pEntry;
	pEntry = m_HistoryEntryDispenser.AllocOne();
	pEntry->m_pSnap = m_Map.SaveSnapshot();
	pEntry->SetAction(pActionStr);
	pEntry->SetDescription(pDescStr);

	// delete all the next entries from current
	CHistoryEntry* pCur = m_pHistoryEntryCurrent->m_pNext;
	while(pCur)
	{
		CHistoryEntry* pToDelete = pCur;
		pCur = pCur->m_pNext;

		dbg_assert(pToDelete->m_pSnap != nullptr, "Snapshot is null");
		free(pToDelete->m_pSnap);
		m_HistoryEntryDispenser.DeallocOne(pToDelete);
	}

	m_pHistoryEntryCurrent->m_pNext = pEntry;
	pEntry->m_pPrev = m_pHistoryEntryCurrent;
	m_pHistoryEntryCurrent = pEntry;
}

void CEditor::HistoryRestoreToEntry(CHistoryEntry* pEntry)
{
	dbg_assert(pEntry && pEntry->m_pSnap, "History entry or snapshot invalid");
	m_Map.RestoreSnapshot(pEntry->m_pSnap);
	m_pHistoryEntryCurrent = pEntry;

	// TODO: on map restore
	m_UiSelectedLayerID = m_Map.m_GameLayerID;
	m_UiSelectedGroupID = m_Map.m_GameGroupID;
}

void CEditor::HistoryUndo()
{
	dbg_assert(m_pHistoryEntryCurrent != nullptr, "Current history entry is null");
	if(m_pHistoryEntryCurrent->m_pPrev)
		HistoryRestoreToEntry(m_pHistoryEntryCurrent->m_pPrev);
}

void CEditor::HistoryRedo()
{
	dbg_assert(m_pHistoryEntryCurrent != nullptr, "Current history entry is null");
	if(m_pHistoryEntryCurrent->m_pNext)
		HistoryRestoreToEntry(m_pHistoryEntryCurrent->m_pNext);
}

const char* CEditor::GetLayerName(int LayerID)
{
	static char aBuff[64];
	const CEditorMap::CLayer& Layer = m_Map.m_aLayers[LayerID];
	if(Layer.IsTileLayer())
		str_format(aBuff, sizeof(aBuff), Localize("Tile %d"), LayerID);
	else if(Layer.IsQuadLayer())
		str_format(aBuff, sizeof(aBuff), Localize("Quad %d"), LayerID);
	else
		dbg_break();
	return aBuff;
}

void CEditor::ConLoad(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;

	const int InputTextLen = str_length(pResult->GetString(0));

	char aMapPath[256];
	bool AddMapPath = str_comp_nocase_num(pResult->GetString(0), "maps/", 5) != 0;
	bool AddMapExtension = InputTextLen <= 4 ||
		str_comp_nocase_num(pResult->GetString(0)+InputTextLen-4, ".map", 4) != 0;
	str_format(aMapPath, sizeof(aMapPath), "%s%s%s", AddMapPath ? "maps/":"", pResult->GetString(0),
			   AddMapExtension ? ".map":"");

	dbg_msg("editor", "ConLoad(%s)", aMapPath);
	pSelf->LoadMap(aMapPath);
}

void CEditor::ConShowPalette(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	dbg_assert(0, "Implement this");
}

void CEditor::ConGameView(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	pSelf->m_ConfigShowGameEntities = (pResult->GetInteger(0) > 0);
	pSelf->m_ConfigShowExtendedTilemaps = pSelf->m_ConfigShowGameEntities;
}

void CEditor::ConShowGrid(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	pSelf->m_ConfigShowGrid = (pResult->GetInteger(0) > 0);
}

void CEditor::ConUndo(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	pSelf->HistoryUndo();
}

void CEditor::ConRedo(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	pSelf->HistoryRedo();
}

void CEditor::ConDeleteImage(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;
	pSelf->EditDeleteImage(pResult->GetInteger(0));
}
