// LordSk
#include "editor2.h"

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

//#include <intrin.h>

// TODO:
// - Data setup for easy add/remove (tiles, quads, ...)
// - Binds
// - Smooth zoom

static char s_aEdMsg[256];
#define ed_log(...)\
	str_format(s_aEdMsg, sizeof(s_aEdMsg), ##__VA_ARGS__);\
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", s_aEdMsg);

#define ed_dbg(...)\
	str_format(s_aEdMsg, sizeof(s_aEdMsg), ##__VA_ARGS__);\
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", s_aEdMsg);

static vec4 StyleColorBg(0.03, 0, 0.085, 1);
static vec4 StyleColorButton(0.062, 0, 0.19, 1);
static vec4 StyleColorButtonBorder(0.18, 0.00, 0.56, 1);
static vec4 StyleColorButtonHover(0.28, 0.10, 0.64, 1);
static vec4 StyleColorButtonPressed(0.13, 0, 0.40, 1);

inline float fract(float f)
{
	return f - (int)f;
}

inline bool IsInsideRect(vec2 Pos, CUIRect Rect)
{
	return (Pos.x >= Rect.x && Pos.x < (Rect.x+Rect.w) &&
			Pos.y >= Rect.y && Pos.y < (Rect.y+Rect.h));
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

	Clear();

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
	// TODO: move this out?
	int ImagesStart, ImagesCount;
	File.GetType(MAPITEMTYPE_IMAGE, &ImagesStart, &ImagesCount);

	for(int i = 0; i < ImagesCount && i < MAX_TEXTURES; i++)
	{
		int TextureFlags = 0;
		bool FoundQuadLayer = false;
		bool FoundTileLayer = false;
		const int LayerCount = m_aLayers.Count();
		for(int li = 0; li < LayerCount; li++)
		{
			const CLayer& Layer = m_aLayers[li];
			if(!FoundQuadLayer && Layer.m_Type == LAYERTYPE_QUADS && Layer.m_ImageID == i)
				FoundQuadLayer = true;
			if(!FoundTileLayer && Layer.m_Type == LAYERTYPE_TILES && Layer.m_ImageID == i)
				FoundTileLayer = true;
		}
		if(FoundTileLayer)
			TextureFlags = FoundQuadLayer ? IGraphics::TEXLOAD_MULTI_DIMENSION :
											IGraphics::TEXLOAD_ARRAY_256;

		CMapItemImage *pImg = (CMapItemImage *)File.GetItem(ImagesStart+i, 0, 0);
		if(pImg->m_External || (pImg->m_Version > 1 && pImg->m_Format != CImageInfo::FORMAT_RGB &&
								pImg->m_Format != CImageInfo::FORMAT_RGBA))
		{
			char Buf[256];
			char *pName = (char *)File.GetData(pImg->m_ImageName);
			str_format(Buf, sizeof(Buf), "mapres/%s.png", pName);
			m_aTextures[i] = m_pGraphics->LoadTexture(Buf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO,
													TextureFlags);
			if(TextureFlags == IGraphics::TEXLOAD_MULTI_DIMENSION)
				m_aTextures2D[i] = m_aTextures[i];
			else
			{
				m_aTextures2D[i] = m_pGraphics->LoadTexture(Buf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO,
															IGraphics::TEXLOAD_MULTI_DIMENSION);
			}
			ed_dbg("mapres/%s.png loaded", pName);
		}
		else
		{
			void *pData = File.GetData(pImg->m_ImageData);
			m_aTextures[i] = m_pGraphics->LoadTextureRaw(pImg->m_Width, pImg->m_Height,
				pImg->m_Version == 1 ? CImageInfo::FORMAT_RGBA : pImg->m_Format, pData,
				CImageInfo::FORMAT_RGBA, TextureFlags);
			if(TextureFlags == IGraphics::TEXLOAD_MULTI_DIMENSION)
				m_aTextures2D[i] = m_aTextures[i];
			else
			{
				m_aTextures2D[i] = m_pGraphics->LoadTextureRaw(pImg->m_Width, pImg->m_Height,
															   pImg->m_Version == 1 ? CImageInfo::FORMAT_RGBA : pImg->m_Format, pData,
															   CImageInfo::FORMAT_RGBA,
															   IGraphics::TEXLOAD_MULTI_DIMENSION);
			}
		}
		dbg_assert(m_aTextures[i].IsValid(), "Invalid texture");
	}

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

	m_aEnvPoints.Clear();
	m_aLayers.Clear();
	m_aGroups.Clear();
	m_aEnvelopes.Clear();

	for(int i = 0; i < m_TextureCount; i++)
	{
		if(m_aTextures[i].Id() != m_aTextures2D[i].Id())
			m_pGraphics->UnloadTexture(&m_aTextures2D[i]);
		m_pGraphics->UnloadTexture(&m_aTextures[i]);
	}
	m_TextureCount = 0;
}

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
		m_InputConsole.OnInput(Event);

	if(IsPopupBrushPalette())
	{
		CUIBrushPaletteState& Bps = m_UiBrushPaletteState;

		if(Event.m_Flags&IInput::FLAG_PRESS)
		{
			if(Event.m_Key == KEY_MOUSE_1)
			{
				if(!Bps.m_MouseClicked)
				{
					Bps.m_MouseStartDragPos = m_UiMousePos;
					Bps.m_MouseIsDraggingRect = true;
				}
				Bps.m_MouseClicked = true;
			}
		}
		else if(Event.m_Flags&IInput::FLAG_RELEASE)
		{
			if(Event.m_Key == KEY_MOUSE_1)
			{
				if(Bps.m_MouseClicked)
				{
					Bps.m_MouseEndDragPos = m_UiMousePos;
					Bps.m_MouseIsDraggingRect = false;
					Bps.m_DoHandleDragSquare = true;

					// we dragged outside the popup
					if(!IsInsideRect(Bps.m_MouseStartDragPos, m_UiPopupBrushPaletteImageRect) &&
					   !IsInsideRect(Bps.m_MouseEndDragPos, m_UiPopupBrushPaletteImageRect))
					{
						Bps.m_DoHandleDragSquare = false;
					}
				}
				Bps.m_MouseClicked = false;
			}
		}
	}
}

void CEditor::Update()
{
	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);
		if(!Input()->IsEventValid(&e))
			continue;
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
	}

	if(Input()->KeyPress(KEY_HOME))
	{
		ResetCamera();
	}

	if(Input()->KeyPress(KEY_F1))
	{
		m_InputConsole.ToggleOpen();
	}

	//m_UiShowBrushPalette = Input()->KeyIsPressed(KEY_SPACE);

	Input()->Clear();
}

void CEditor::Render()
{
	const CUIRect UiScreenRect = *UI()->Screen();
	m_UiScreenRect = UiScreenRect;

	// basic start
	Graphics()->Clear(0.3f, 0.3f, 0.3f);

	// get world view points based on neutral paramters
	float aWorldViewRectPoints[4];
	RenderTools()->MapScreenToWorld(0, 0, 1, 1, 0, 0, Graphics()->ScreenAspect(), 1, aWorldViewRectPoints);

	const float WorldViewWidth = aWorldViewRectPoints[2]-aWorldViewRectPoints[0];
	const float WorldViewHeight = aWorldViewRectPoints[3]-aWorldViewRectPoints[1];
	const float ZoomWorldViewWidth = WorldViewWidth * m_Zoom;
	const float ZoomWorldViewHeight = WorldViewHeight * m_Zoom;
	m_ZoomWorldViewWidth = ZoomWorldViewWidth;
	m_ZoomWorldViewHeight = ZoomWorldViewHeight;

	m_GfxScreenWidth = Graphics()->ScreenWidth();
	m_GfxScreenHeight = Graphics()->ScreenHeight();
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
	if(SelectedLayerID >= 0)
	{
		dbg_assert(m_UiSelectedGroupID >= 0, "Parent group of selected layer not found");
		const CEditorMap::CGroup& Group = m_Map.m_aGroups[m_UiSelectedGroupID];
		SelectedParallaxX = Group.m_ParallaxX / 100.f;
		SelectedParallaxY = Group.m_ParallaxY / 100.f;
		SelectedPositionX = Group.m_OffsetX;
		SelectedPositionY = Group.m_OffsetY;
	}

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
					Graphics()->TextureSet(m_Map.m_aTextures[Layer.m_ImageID]);

				Graphics()->BlendNone();
				RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
											 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_OPAQUE,
											 0, 0, -1, 0);

				Graphics()->BlendNormal();
				RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
											 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_TRANSPARENT,
											 0, 0, -1, 0);
			}
			else if(Layer.m_Type == LAYERTYPE_QUADS)
			{
				if(Layer.m_ImageID == -1)
					Graphics()->TextureClear();
				else
					Graphics()->TextureSet(m_Map.m_aTextures[Layer.m_ImageID]);

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
	CUIRect ScreenRect = { SelectedScreenOff.x, SelectedScreenOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };
	Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
						  ScreenRect.y+ScreenRect.h);

	IGraphics::CQuadItem OriginLineX(0, 0, TileSize, 2/FakeToScreenY);
	IGraphics::CQuadItem RecOriginLineYtY(0, 0, 2/FakeToScreenX, TileSize);
	float LayerWidth = m_Map.m_MapMaxWidth * TileSize;
	float LayerHeight = m_Map.m_MapMaxHeight * TileSize;
	if(SelectedLayerID)
	{
		LayerWidth = m_Map.m_aLayers[SelectedLayerID].m_Width * TileSize;
		LayerHeight = m_Map.m_aLayers[SelectedLayerID].m_Height * TileSize;
	}

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

	if(SelectedLayerID < 0 || IsSelectedLayerTile)
	{
		// grid
		if(m_ConfigShowGrid)
		{
			const float GridAlpha = 0.25f;
			Graphics()->SetColor(1.0f * GridAlpha, 1.0f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
			float StartX = SelectedScreenOff.x - fract(SelectedScreenOff.x/TileSize) * TileSize;
			float StartY = SelectedScreenOff.y - fract(SelectedScreenOff.y/TileSize) * TileSize;
			float EndX = SelectedScreenOff.x+ZoomWorldViewWidth;
			float EndY = SelectedScreenOff.y+ZoomWorldViewHeight;
			for(float x = StartX; x < EndX; x+= TileSize)
			{
				IGraphics::CQuadItem Line(x, SelectedScreenOff.y, bw, ZoomWorldViewHeight);
				Graphics()->QuadsDrawTL(&Line, 1);
			}
			for(float y = StartY; y < EndY; y+= TileSize)
			{
				IGraphics::CQuadItem Line(SelectedScreenOff.x, y, ZoomWorldViewWidth, bh);
				Graphics()->QuadsDrawTL(&Line, 1);
			}
		}


		Graphics()->SetColor(1, 1, 1, 1);
		Graphics()->QuadsDrawTL(aBorders, 4);
	}

	Graphics()->SetColor(0, 0, 1, 1);
	Graphics()->QuadsDrawTL(&OriginLineX, 1);
	Graphics()->SetColor(0, 1, 0, 1);
	Graphics()->QuadsDrawTL(&RecOriginLineYtY, 1);

	Graphics()->QuadsEnd();

	// user interface
	RenderUI();

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

void CEditor::RenderUI()
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect RightPanel;
	UiScreenRect.VSplitRight(150, &m_UiMainViewRect, &RightPanel);

	DrawRect(RightPanel, vec4(0.03, 0, 0.085, 1));

	CUIRect NavRect, ButtonRect;
	RightPanel.Margin(3.0f, &NavRect);

	static CUIButtonState s_UiGroupButState[MAX_GROUPS] = {};
	static CUIButtonState s_UiGroupShowButState[MAX_GROUPS] = {};
	static CUIButtonState s_UiLayerButState[MAX_LAYERS] = {};
	static CUIButtonState s_UiLayerShowButState[MAX_LAYERS] = {};
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float ShowButtonWidth = 15.0f;

	if(m_UiSelectedLayerID >= 0)
	{
		dbg_assert(m_UiSelectedGroupID >= 0, "Parent group of selected layer not found");
		const CEditorMap::CGroup& SelectedGroup = m_Map.m_aGroups[m_UiSelectedGroupID];
		char aBuff[128];
		CUIRect ButtonRect;

		// parallax
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
		NavRect.HSplitTop(Spacing, 0, &NavRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Parallax = (%d, %d)",
				   SelectedGroup.m_ParallaxX, SelectedGroup.m_ParallaxY);
		DrawText(ButtonRect, aBuff, FontSize);

		// position
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
		NavRect.HSplitTop(Spacing, 0, &NavRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Position = (%d, %d)",
				   SelectedGroup.m_OffsetX, SelectedGroup.m_OffsetY);
		DrawText(ButtonRect, aBuff, FontSize);
	}

	const int GroupCount = m_Map.m_aGroups.Count();
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

				dbg_assert(LyID >= 0 && LyID < MAX_LAYERS, "LayerID out of bounds");

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
				if(m_Map.m_GameLayerID == LyID)
					str_format(aLayerName, sizeof(aLayerName), "Game Layer");
				else
					str_format(aLayerName, sizeof(aLayerName), "%s Layer #%d",
							   m_Map.m_aLayers[LyID].m_Type == LAYERTYPE_TILES ?
								   "Tile" : "Quad",
							   li);
				DrawText(ButtonRect, aLayerName, FontSize);
			}
		}
	}

	if(m_UiCurrentPopupID == POPUP_BRUSH_PALETTE)
		RenderPopupBrushPalette();
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
	DrawRect(MainRect, vec4(1, 0, 0, 1));

	CUIRect TopRow;
	MainRect.HSplitTop(40, &TopRow, &MainRect);
	DrawRect(TopRow, vec4(0.03, 0, 0.085, 1.0));

	const CEditorMap::CLayer& SelectedTileLayer = m_Map.m_aLayers[m_UiSelectedLayerID];
	dbg_assert(SelectedTileLayer.IsTileLayer(), "Selected layer is not a tile layer");

	IGraphics::CTextureHandle PaletteTexture;
	if(m_UiSelectedLayerID == m_Map.m_GameLayerID)
		PaletteTexture = m_EntitiesTexture;
	else
		PaletteTexture = m_Map.m_aTextures2D[SelectedTileLayer.m_ImageID];

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
	u8* aTileSelected = Bps.m_TileSelected;

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
			DrawRect(TileRect, vec4(0.5, 0, 1, 0.5));
		}
	}

	// hover tile
	if(!Bps.m_MouseIsDraggingRect && UI()->MouseInside(&ImageRect))
	{
		const vec2 RelMousePos = m_UiMousePos - vec2(ImageRect.x, ImageRect.y);
		const int HoveredTileX = RelMousePos.x / TileSize;
		const int HoveredTileY = RelMousePos.y / TileSize;

		CUIRect HoverTileRect = {
			ImageRect.x + HoveredTileX*TileSize,
			ImageRect.y + HoveredTileY*TileSize,
			TileSize, TileSize
		};
		DrawRect(HoverTileRect, vec4(0.5, 0, 1, 0.5));
	}

	// drag rectangle
	if(Bps.m_MouseIsDraggingRect &&
	   (UI()->MouseInside(&ImageRect) || IsInsideRect(Bps.m_MouseStartDragPos, ImageRect)))
	{
		const vec2 RelMouseStartPos = Bps.m_MouseStartDragPos - vec2(ImageRect.x, ImageRect.y);
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
		DrawRect(DragTileRect, vec4(0.5, 0, 1, 0.75));
	}

	if(Bps.m_DoHandleDragSquare)
	{
		Bps.m_DoHandleDragSquare = false;
		const vec2 RelMouseStartPos = Bps.m_MouseStartDragPos - vec2(ImageRect.x, ImageRect.y);
		const vec2 RelMouseEndPos = Bps.m_MouseEndDragPos - vec2(ImageRect.x, ImageRect.y);
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
	}

	CUIRect ButtonRect;
	TopRow.Margin(3.0f, &TopRow);
	TopRow.VSplitLeft(100, &ButtonRect, &TopRow);

	static CUIButtonState s_ButClear;
	if(UiButton(ButtonRect, Localize("Clear"), &s_ButClear))
	{
		mem_zero(aTileSelected, sizeof(u8)*256);
	}
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
	const float FakeToScreenX = m_GfxScreenWidth/m_UiScreenRect.w;
	const float FakeToScreenY = m_GfxScreenHeight/m_UiScreenRect.h;
	const float BorderW = Border/FakeToScreenX;
	const float BorderH = Border/FakeToScreenY;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// border pass
	IGraphics::CQuadItem Quad(Rect.x, Rect.y, Rect.w, Rect.h);
	Graphics()->SetColor(BorderColor.r*BorderColor.a, BorderColor.g*BorderColor.a,
						 BorderColor.b*BorderColor.a, BorderColor.a);
	Graphics()->QuadsDrawTL(&Quad, 1);

	// front pass
	Quad.m_X += BorderW;
	Quad.m_Y += BorderH;
	Quad.m_Width -= BorderW*2;
	Quad.m_Height -= BorderH*2;
	Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a,
						 Color.b*Color.a, Color.a);
	Graphics()->QuadsDrawTL(&Quad, 1);

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
			if(pID && !UI()->GetActiveItem())
				UI()->SetActiveItem(pID);
		}
	}
	else if(pID && UI()->HotItem() == pID)
	{
		UI()->SetHotItem(0);
	}

}

bool CEditor::UiButton(const CUIRect& Rect, const char* pText, CUIButtonState* pButState)
{
	UiDoButtonBehavior(pButState, Rect, pButState);

	vec4 ShowButColor = StyleColorButton;
	if(pButState->m_Hovered)
		ShowButColor = StyleColorButtonHover;
	if(pButState->m_Pressed)
		ShowButColor = StyleColorButtonPressed;

	DrawRectBorder(Rect, ShowButColor, 1, StyleColorButtonBorder);
	DrawText(Rect, pText, 10.0f);
	return pButState->m_Clicked;
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
	mem_zero(m_UiGroupHidden, sizeof(m_UiGroupHidden));
	mem_zero(m_UiGroupOpen, sizeof(m_UiGroupOpen));
	m_UiGroupOpen[m_Map.m_GameGroupID] = true;
	mem_zero(m_UiLayerHidden, sizeof(m_UiLayerHidden));
	mem_zero(m_UiLayerHovered, sizeof(m_UiLayerHovered));
	ResetCamera();
}

void CEditor::ConLoad(IConsole::IResult* pResult, void* pUserData)
{
	CEditor *pSelf = (CEditor *)pUserData;

	const int InputTextLen = str_length(pResult->GetString(0));

	char aMapPath[256];
	bool AddMapPath = str_comp_nocase_num(pResult->GetString(0), "maps/", 5) != 0;
	bool AddMapExtension = InputTextLen <= 4 || str_comp_nocase_num(pResult->GetString(0)+InputTextLen-4, ".map", 4) != 0;
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
