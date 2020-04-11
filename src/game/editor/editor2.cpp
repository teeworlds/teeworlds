// LordSk
#include "editor2.h"

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

//#include <intrin.h>

// TODO:
// - Easily know if we're clicking on UI or elsewhere
// ---- what event gets handled where should be VERY clear

// - Binds
// - Smooth zoom
// - Fix selecting while moving the camera

// - Localize everything

// - Stability is very important (crashes should be easy to catch)
// - Input should be handled well (careful of input-locks https://github.com/teeworlds/teeworlds/issues/828)
// ! Do not use DoButton_SpriteClean (dunno why)

static char s_aEdMsg[256];
#define ed_log(...)\
	str_format(s_aEdMsg, sizeof(s_aEdMsg), __VA_ARGS__);\
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", s_aEdMsg)

#ifdef CONF_DEBUG
	#define ed_dbg(...)\
		str_format(s_aEdMsg, sizeof(s_aEdMsg), __VA_ARGS__);\
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", s_aEdMsg)
#else
	#define ed_dbg(...)
#endif

IEditor *CreateEditor2() { return new CEditor2; }

CEditor2::CEditor2()
{

}

CEditor2::~CEditor2()
{

}

void CEditor2::Init()
{
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pConsole = CreateConsole(CFGFLAG_EDITOR);
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	m_pConfig = m_pConfigManager->Values();
	m_RenderTools.Init(m_pConfig, m_pGraphics, &m_UI);
	m_UI.Init(m_pConfig, m_pGraphics, m_pTextRender);

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
	m_pConsole->Register("save", "r", CFGFLAG_EDITOR, ConSave, this, "Save map");
	m_pConsole->Register("+show_palette", "", CFGFLAG_EDITOR, ConShowPalette, this, "Show palette");
	m_pConsole->Register("game_view", "i", CFGFLAG_EDITOR, ConGameView, this, "Toggle game view");
	m_pConsole->Register("show_grid", "i", CFGFLAG_EDITOR, ConShowGrid, this, "Toggle grid");
	m_pConsole->Register("undo", "", CFGFLAG_EDITOR, ConUndo, this, "Undo");
	m_pConsole->Register("redo", "", CFGFLAG_EDITOR, ConRedo, this, "Redo");
	m_pConsole->Register("delete_image", "i", CFGFLAG_EDITOR, ConDeleteImage, this, "Delete image");
	m_InputConsole.Init(m_pConsole, m_pGraphics, &m_UI, m_pTextRender, m_pInput);

	m_ConfigShowGrid = true;
	m_ConfigShowGridMajor = false;
	m_ConfigShowGameEntities = false;
	m_ConfigShowExtendedTilemaps = false;
	m_Zoom = 1.0f;
	m_UiSelectedLayerID = -1;
	m_UiSelectedGroupID = -1;
	m_UiSelectedImageID = -1;
	m_BrushAutomapRuleID = -1;
	m_pHistoryEntryCurrent = 0x0;
	m_Page = PAGE_MAP_EDITOR;
	m_Tool = TOOL_TILE_BRUSH;
	m_UiDetailPanelIsOpen = false;
	m_MapViewZoom = 0;
	m_MapSaved = false;

	m_UiPopupStackCount = 0;
	m_UiCurrentPopupID = -1;

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
		m_RenderShotgunPickupSize = vec2(VisualSize * (SpriteW/ScaleFactor),
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
	m_Brush.m_aTiles.clear();

	HistoryClear();
	m_pHistoryEntryCurrent = 0x0;

	ResetCamera();

	/*
	m_Map.LoadDefault();
	OnMapLoaded();
	*/
	if(!LoadMap("maps/ctf7.map")) {
		dbg_break();
	}
}

void CEditor2::UpdateAndRender()
{
	Update();
	Render();
}

bool CEditor2::HasUnsavedData() const
{
	return !m_MapSaved;
}

void CEditor2::OnInput(IInput::CEvent Event)
{
	if(m_InputConsole.IsOpen())
	{
		m_InputConsole.OnInput(Event);
		return;
	}
}

void CEditor2::Update()
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

	m_MapViewZoom = 0;
	m_MapViewMove = vec2(0,0);

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
		// move view
		if(MouseButtons&MOUSE_RIGHT)
		{
			m_MapViewMove = m_UiMouseDelta;
		}

		// zoom with mouse wheel
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
			m_MapViewZoom = 1;
		else if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
			m_MapViewZoom = -1;

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

		if(IsCtrlPressed && Input()->KeyPress(KEY_S))
		{
			UserMapSave();
		}

		if(IsToolSelect() && Input()->KeyPress(KEY_ESCAPE)) {
			m_TileSelection.Deselect();
		}

		if(IsToolBrush() && Input()->KeyIsPressed(KEY_SPACE) && !m_UiBrushPaletteState.m_PopupEnabled)
		{
			PushPopup(&CEditor2::RenderPopupBrushPalette, &m_UiBrushPaletteState);
			m_UiBrushPaletteState.m_PopupEnabled = true;
		}

		if(IsToolBrush() && Input()->KeyPress(KEY_ESCAPE))
			BrushClear();
	}
}

void CEditor2::Render()
{
	const CUIRect UiScreenRect = *UI()->Screen();
	m_UiScreenRect = UiScreenRect;
	m_GfxScreenWidth = Graphics()->ScreenWidth();
	m_GfxScreenHeight = Graphics()->ScreenHeight();

	Graphics()->Clear(0.3f, 0.3f, 0.3f);

	if(m_Page == PAGE_MAP_EDITOR)
		RenderMap();
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

		const vec3 aToolColors[] = {
			vec3(1, 0.2f, 1),
			vec3(1, 0.5f, 0.2f),
			vec3(0.2f, 0.7f, 1),
		};
		const vec3& ToolColor = aToolColors[m_Tool];
		Graphics()->SetColor(ToolColor.r, ToolColor.g, ToolColor.b, 1);
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

void CEditor2::RenderLayerGameEntities(const CEditorMap2::CLayer& GameLayer)
{
	const CTile* pTiles = GameLayer.m_aTiles.base_ptr();
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

		CEntitySprite() {}
		CEntitySprite(int SpriteID_, vec2 Pos_, vec2 Size_)
		{
			m_SpriteID = SpriteID_;
			m_Pos = Pos_;
			m_Size = Size_;
		}
	};

	CEntitySprite aEntitySprites[2048];
	int EntitySpriteCount = 0;

	const float HealthArmorSize = 64*0.7071067811865475244f;
	const vec2 NinjaSize(128*(256/263.87876003953027518857f),
						 128*(64/263.87876003953027518857f));

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
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_HEALTH,
					PickupPos - vec2((HealthArmorSize-TileSize)*0.5f,(HealthArmorSize-TileSize)*0.5f),
					vec2(HealthArmorSize, HealthArmorSize)
				);
			}
			else if(Index == ENTITY_ARMOR_1)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_ARMOR,
					PickupPos - vec2((HealthArmorSize-TileSize)*0.5f,(HealthArmorSize-TileSize)*0.5f),
					vec2(HealthArmorSize, HealthArmorSize)
				);
			}
			else if(Index == ENTITY_WEAPON_GRENADE)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_GRENADE,
					PickupPos - vec2((m_RenderGrenadePickupSize.x-TileSize)*0.5f,
						(m_RenderGrenadePickupSize.y-TileSize)*0.5f),
					m_RenderGrenadePickupSize
				);
			}
			else if(Index == ENTITY_WEAPON_SHOTGUN)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_SHOTGUN,
					PickupPos - vec2((m_RenderShotgunPickupSize.x-TileSize)*0.5f,
						(m_RenderShotgunPickupSize.y-TileSize)*0.5f),
					m_RenderShotgunPickupSize
				);
			}
			else if(Index == ENTITY_WEAPON_LASER)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_LASER,
					PickupPos - vec2((m_RenderLaserPickupSize.x-TileSize)*0.5f,
						(m_RenderLaserPickupSize.y-TileSize)*0.5f),
					m_RenderLaserPickupSize
				);
			}
			else if(Index == ENTITY_POWERUP_NINJA)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_PICKUP_NINJA,
					PickupPos - vec2((NinjaSize.x-TileSize)*0.5f, (NinjaSize.y-TileSize)*0.5f),
					NinjaSize
				);
			}
			else if(Index == ENTITY_FLAGSTAND_RED)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_FLAG_RED,
					BasePos - vec2(0, 54),
					vec2(42, 84)
				);
			}
			else if(Index == ENTITY_FLAGSTAND_BLUE)
			{
				aEntitySprites[EntitySpriteCount++] = CEntitySprite(
					SPRITE_FLAG_BLUE,
					BasePos - vec2(0, 54),
					vec2(42, 84)
				);
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

inline vec2 CEditor2::CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY, float ParallaxX, float ParallaxY)
{
	// we add UiScreenRect.w*0.5 and UiScreenRect.h*0.5 because in the game the view
	// is based on the center of the screen
	const CUIRect UiScreenRect = m_UiScreenRect;
	const float MapOffX = (((m_MapUiPosOffset.x+UiScreenRect.w*0.5) * ParallaxX) - UiScreenRect.w*0.5)/UiScreenRect.w * WorldWidth + PosX;
	const float MapOffY = (((m_MapUiPosOffset.y+UiScreenRect.h*0.5) * ParallaxY) - UiScreenRect.h*0.5)/UiScreenRect.h * WorldHeight + PosY;
	return vec2(MapOffX, MapOffY);
}

vec2 CEditor2::CalcGroupWorldPosFromUiPos(int GroupID, float WorldWidth, float WorldHeight, vec2 UiPos)
{
	const CEditorMap2::CGroup& G = m_Map.m_aGroups.Get(GroupID);
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

CUIRect CEditor2::CalcUiRectFromGroupWorldRect(int GroupID, float WorldWidth, float WorldHeight,
	CUIRect WorldRect)
{
	const CEditorMap2::CGroup& G = m_Map.m_aGroups.Get(GroupID);
	const float OffX = G.m_OffsetX;
	const float OffY = G.m_OffsetY;
	const float ParaX = G.m_ParallaxX/100.f;
	const float ParaY = G.m_ParallaxY/100.f;

	const CUIRect UiScreenRect = m_UiScreenRect;
	const vec2 MapOff = CalcGroupScreenOffset(WorldWidth, WorldHeight, OffX, OffY, ParaX, ParaY);

	const float UiX = ((-MapOff.x + WorldRect.x) / WorldWidth) * UiScreenRect.w;
	const float UiY = ((-MapOff.y + WorldRect.y) / WorldHeight) * UiScreenRect.h;
	const float UiW = (WorldRect.w / WorldWidth) * UiScreenRect.w;
	const float UiH = (WorldRect.h / WorldHeight) * UiScreenRect.h;
	const CUIRect r = {UiX, UiY, UiW, UiH};
	return r;
}

void CEditor2::StaticEnvelopeEval(float TimeOffset, int EnvID, float* pChannels, void* pUser)
{
	CEditor2 *pThis = (CEditor2 *)pUser;
	if(EnvID >= 0)
		pThis->EnvelopeEval(TimeOffset, EnvID, pChannels);
}

void CEditor2::EnvelopeEval(float TimeOffset, int EnvID, float* pChannels)
{
	pChannels[0] = 0;
	pChannels[1] = 0;
	pChannels[2] = 0;
	pChannels[3] = 0;

	dbg_assert(EnvID < m_Map.m_aEnvelopes.size(), "EnvID out of bounds");
	if(EnvID >= m_Map.m_aEnvelopes.size())
		return;

	const CMapItemEnvelope& Env = m_Map.m_aEnvelopes[EnvID];
	const CEnvPoint* pPoints = &m_Map.m_aEnvPoints[0];

	float Time = Client()->LocalTime();
	RenderTools()->RenderEvalEnvelope(pPoints + Env.m_StartPoint, Env.m_NumPoints, 4,
									  Time+TimeOffset, pChannels);
}

void CEditor2::RenderMap()
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
	const int SelectedLayerID = m_UiSelectedLayerID != -1 ? m_UiSelectedLayerID : m_Map.m_GameLayerID;
	const int SelectedGroupID = m_UiSelectedGroupID != -1 ? m_UiSelectedGroupID : m_Map.m_GameGroupID;
	dbg_assert(m_Map.m_aLayers.IsValid(SelectedLayerID), "No layer selected");
	dbg_assert(m_Map.m_aGroups.IsValid(SelectedGroupID), "Parent group of selected layer not found");
	const CEditorMap2::CLayer& SelectedLayer = m_Map.m_aLayers.Get(SelectedLayerID);
	const CEditorMap2::CGroup& SelectedGroup = m_Map.m_aGroups.Get(SelectedGroupID);
	SelectedParallaxX = SelectedGroup.m_ParallaxX / 100.f;
	SelectedParallaxY = SelectedGroup.m_ParallaxY / 100.f;
	SelectedPositionX = SelectedGroup.m_OffsetX;
	SelectedPositionY = SelectedGroup.m_OffsetY;

	const vec2 SelectedScreenOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight, SelectedPositionX, SelectedPositionY, SelectedParallaxX, SelectedParallaxY);

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
	const int GroupCount = m_Map.m_GroupIDListCount;
	const u32* GroupIDList = m_Map.m_aGroupIDList;
	const int BaseTilemapFlags = m_ConfigShowExtendedTilemaps ? TILERENDERFLAG_EXTEND:0;

	for(int gi = 0; gi < GroupCount; gi++)
	{
		const u32 GroupID = GroupIDList[gi];
		const CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

		if(m_UiGroupState[GroupID].m_IsHidden)
			continue;

		// group clip
		const vec2 ClipOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight, 0, 0, 1, 1);
		const CUIRect ClipScreenRect = { ClipOff.x, ClipOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };
		Graphics()->MapScreen(ClipScreenRect.x, ClipScreenRect.y, ClipScreenRect.x+ClipScreenRect.w, ClipScreenRect.y+ClipScreenRect.h);

		const bool Clipped = Group.m_UseClipping && Group.m_ClipWidth > 0 && Group.m_ClipHeight > 0;
		if(Clipped)
		{
			float x0 = (Group.m_ClipX - ClipScreenRect.x) / ClipScreenRect.w;
			float y0 = (Group.m_ClipY - ClipScreenRect.y) / ClipScreenRect.h;
			float x1 = ((Group.m_ClipX+Group.m_ClipWidth) - ClipScreenRect.x) / ClipScreenRect.w;
			float y1 = ((Group.m_ClipY+Group.m_ClipHeight) - ClipScreenRect.y) / ClipScreenRect.h;

			if(x1 < 0.0f || x0 > 1.0f || y1 < 0.0f || y0 > 1.0f)
				continue;

			const float GfxScreenW = m_GfxScreenWidth;
			const float GfxScreenH = m_GfxScreenHeight;
			Graphics()->ClipEnable((int)(x0*GfxScreenW), (int)(y0*GfxScreenH),
				(int)((x1-x0)*GfxScreenW), (int)((y1-y0)*GfxScreenH));
		}

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
			if(m_UiLayerState[LyID].m_IsHidden)
				continue;

			const CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LyID);

			if(Layer.m_Type == LAYERTYPE_TILES)
			{
				const float LyWidth = Layer.m_Width;
				const float LyHeight = Layer.m_Height;
				vec4 LyColor = Layer.m_Color;
				const CTile *pTiles = Layer.m_aTiles.base_ptr();

				if(LyID == m_Map.m_GameLayerID)
				{
					if(m_ConfigShowGameEntities)
						RenderLayerGameEntities(Layer);
					continue;
				}

				if(m_UiGroupState[GroupID].m_IsHovered || m_UiLayerState[LyID].m_IsHovered)
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
											 StaticEnvelopeEval, this, Layer.m_ColorEnvelopeID,
											 Layer.m_ColorEnvOffset);

				Graphics()->BlendNormal();
				RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
											 BaseTilemapFlags|LAYERRENDERFLAG_TRANSPARENT,
											 StaticEnvelopeEval, this, Layer.m_ColorEnvelopeID,
											 Layer.m_ColorEnvOffset);
			}
			else if(Layer.m_Type == LAYERTYPE_QUADS)
			{
				if(Layer.m_ImageID == -1)
					Graphics()->TextureClear();
				else
					Graphics()->TextureSet(m_Map.m_Assets.m_aTextureHandle[Layer.m_ImageID]);

				Graphics()->BlendNormal();
				if(m_UiGroupState[GroupID].m_IsHovered || m_UiLayerState[LyID].m_IsHovered)
					Graphics()->BlendAdditive();

				RenderTools()->RenderQuads(Layer.m_aQuads.base_ptr(), Layer.m_aQuads.size(),
						LAYERRENDERFLAG_TRANSPARENT, StaticEnvelopeEval, this);
			}
		}

		if(Clipped)
			Graphics()->ClipDisable();
	}

	// game layer
	const int LyID = m_Map.m_GameLayerID;
	if(!m_ConfigShowGameEntities && !m_UiLayerState[LyID].m_IsHidden && !m_UiGroupState[m_Map.m_GameGroupID].m_IsHidden)
	{
		const vec2 MapOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight, 0, 0, 1, 1);
		CUIRect ScreenRect = { MapOff.x, MapOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };

		Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
							  ScreenRect.y+ScreenRect.h);

		const CEditorMap2::CLayer& LayerTile = m_Map.m_aLayers.Get(LyID);
		const float LyWidth = LayerTile.m_Width;
		const float LyHeight = LayerTile.m_Height;
		vec4 LyColor = LayerTile.m_Color;
		const CTile *pTiles = LayerTile.m_aTiles.base_ptr();

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
	float LayerWidth = SelectedLayer.m_Width * TileSize;
	float LayerHeight = SelectedLayer.m_Height * TileSize;

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

	if(SelectedLayer.IsTileLayer())
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
					Graphics()->SetColor(0.5f * GridAlpha, 0.5f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
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
					Graphics()->SetColor(1.0f * GridAlpha*2.0f, 1.0f * GridAlpha*2.0f, 1.0f * GridAlpha*2.0f, GridAlpha*2.0f);
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
	RenderMapOverlay();

	// user interface
	RenderMapEditorUI();
}

void CEditor2::RenderMapOverlay()
{
	// NOTE: we're in selected group world space here
	if(m_UiPopupStackCount > 0)
		return;

	const float TileSize = 32;

	const vec2 MouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID, m_ZoomWorldViewWidth,
		m_ZoomWorldViewHeight, m_UiMousePos);

	const int MouseTx = floor(MouseWorldPos.x/TileSize);
	const int MouseTy = floor(MouseWorldPos.y/TileSize);
	const vec2 GridMousePos(MouseTx*TileSize, MouseTy*TileSize);

	static CUIMouseDrag s_MapViewDrag;
	bool FinishedDragging = UiDoMouseDragging(m_UiMainViewRect, &s_MapViewDrag);

	const bool CanClick = s_MapViewDrag.m_Button.m_Hovered;

	if(CanClick)
	{
		m_MapUiPosOffset -= m_MapViewMove;
		if(m_MapViewZoom == 1)  ChangeZoom(m_Zoom / 1.1f);
		if(m_MapViewZoom == -1) ChangeZoom(m_Zoom * 1.1f);
	}

	// TODO: kinda weird?
	if(!CanClick)
		s_MapViewDrag = CUIMouseDrag();

	const int SelectedLayerID = m_UiSelectedLayerID != -1 ? m_UiSelectedLayerID : m_Map.m_GameLayerID;
	const CEditorMap2::CGroup& SelectedGroup = m_Map.m_aGroups.Get(m_UiSelectedGroupID);
	const CEditorMap2::CLayer& SelectedTileLayer = m_Map.m_aLayers.Get(SelectedLayerID);

	if(SelectedTileLayer.IsTileLayer())
	{
		// cell overlay, select rectangle
		if(IsToolBrush() || IsToolSelect())
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
			else if(m_Brush.IsEmpty())
			{
				const CUIRect HoverRect = {GridMousePos.x, GridMousePos.y, TileSize, TileSize};
				vec4 HoverColor = StyleColorTileHover;
				HoverColor.a += sinf(m_LocalTime * 2.0) * 0.1;
				DrawRect(HoverRect, HoverColor);
			}
		}

		if(IsToolSelect())
		{
			if(s_MapViewDrag.m_IsDragging)
				m_TileSelection.Deselect();

			if(FinishedDragging)
			{
				const vec2 StartMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
					m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_StartDragPos);
				const vec2 EndMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
					m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_EndDragPos);

				// const int StartTX = floor(StartMouseWorldPos.x/TileSize);
				// const int StartTY = floor(StartMouseWorldPos.y/TileSize);
				// const int EndTX = floor(EndMouseWorldPos.x/TileSize);
				// const int EndTY = floor(EndMouseWorldPos.y/TileSize);

				// const int SelStartX = clamp(min(StartTX, EndTX), 0, SelectedTileLayer.m_Width-1);
				// const int SelStartY = clamp(min(StartTY, EndTY), 0, SelectedTileLayer.m_Height-1);
				// const int SelEndX = clamp(max(StartTX, EndTX), 0, SelectedTileLayer.m_Width-1) + 1;
				// const int SelEndY = clamp(max(StartTY, EndTY), 0, SelectedTileLayer.m_Height-1) + 1;

				m_TileSelection.Select(
					floor(StartMouseWorldPos.x/TileSize),
					floor(StartMouseWorldPos.y/TileSize),
					floor(EndMouseWorldPos.x/TileSize),
					floor(EndMouseWorldPos.y/TileSize)
				);

				m_TileSelection.FitLayer(SelectedTileLayer);
			}

			if(m_TileSelection.IsSelected())
			{
				// fit selection to possibly newly selected layer
				m_TileSelection.FitLayer(SelectedTileLayer);

				CUIRect SelectRect = {
					m_TileSelection.m_StartTX * TileSize,
					m_TileSelection.m_StartTY * TileSize,
					(m_TileSelection.m_EndTX+1-m_TileSelection.m_StartTX)*TileSize,
					(m_TileSelection.m_EndTY+1-m_TileSelection.m_StartTY)*TileSize
				};
				vec4 HoverColor = StyleColorTileSelection;
				HoverColor.a += sinf(m_LocalTime * 2.0) * 0.05;
				DrawRectBorderMiddle(SelectRect, HoverColor, 2, vec4(1,1,1,1));
			}
		}

		if(IsToolBrush())
		{
			if(m_Brush.IsEmpty())
			{
				// get tiles from map when we're done selecting
				if(FinishedDragging)
				{
					const vec2 StartMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
						m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_StartDragPos);
					const vec2 EndMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
						m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_EndDragPos);

					const int StartTX = floor(StartMouseWorldPos.x/TileSize);
					const int StartTY = floor(StartMouseWorldPos.y/TileSize);
					const int EndTX = floor(EndMouseWorldPos.x/TileSize);
					const int EndTY = floor(EndMouseWorldPos.y/TileSize);

					TileLayerRegionToBrush(SelectedLayerID, StartTX, StartTY, EndTX, EndTY);
				}
			}
			else
			{
				// draw brush
				RenderBrush(GridMousePos);

				if(FinishedDragging)
				{
					const vec2 StartMouseWorldPos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID,
						m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, s_MapViewDrag.m_StartDragPos);
					const int StartTX = floor(StartMouseWorldPos.x/TileSize);
					const int StartTY = floor(StartMouseWorldPos.y/TileSize);

					const int RectStartX = min(MouseTx, StartTX);
					const int RectStartY = min(MouseTy, StartTY);
					const int RectEndX = max(MouseTx, StartTX);
					const int RectEndY = max(MouseTy, StartTY);

					// automap
					if(m_BrushAutomapRuleID >= 0)
					{
						// click without dragging, paint whole brush in place
						if(StartTX == MouseTx && StartTY == MouseTy)
							EditBrushPaintLayerAutomap(MouseTx, MouseTy, SelectedLayerID, m_BrushAutomapRuleID);
						else // drag, fill the rectangle by repeating the brush
						{
							EditBrushPaintLayerFillRectAutomap(RectStartX, RectStartY, RectEndX-RectStartX+1, RectEndY-RectStartY+1, SelectedLayerID, m_BrushAutomapRuleID);
						}
					}
					// no automap
					else
					{
						// click without dragging, paint whole brush in place
						if(StartTX == MouseTx && StartTY == MouseTy)
							EditBrushPaintLayer(MouseTx, MouseTy, SelectedLayerID);
						else // drag, fill the rectangle by repeating the brush
							EditBrushPaintLayerFillRectRepeat(RectStartX, RectStartY, RectEndX-RectStartX+1, RectEndY-RectStartY+1, SelectedLayerID);
					}
				}
			}
		}
	}

	if(IsToolDimension())
	{
		// draw clip rect
		const vec2 ClipOff = CalcGroupScreenOffset(m_ZoomWorldViewWidth, m_ZoomWorldViewHeight,
			0, 0, 1, 1);
		const CUIRect ClipScreenRect = { ClipOff.x, ClipOff.y, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight };
		Graphics()->MapScreen(ClipScreenRect.x, ClipScreenRect.y, ClipScreenRect.x+ClipScreenRect.w,
			ClipScreenRect.y+ClipScreenRect.h);

		const CUIRect ClipRect = {
			(float)SelectedGroup.m_ClipX,
			(float)SelectedGroup.m_ClipY,
			(float)SelectedGroup.m_ClipWidth,
			(float)SelectedGroup.m_ClipHeight
		};

		DrawRectBorder(ClipRect, vec4(0,0,0,0), 1, vec4(1,0,0,1));
	}
}

void CEditor2::RenderMenuBar(CUIRect TopPanel)
{
	DrawRect(TopPanel, StyleColorBg);

	CUIRect ButtonRect;
	TopPanel.VSplitLeft(50.0f, &ButtonRect, &TopPanel);

	static CUIButton s_File;
	if(UiButton(ButtonRect, "File", &s_File))
	{
		static CUIRect FileMenuRect = {ButtonRect.x, ButtonRect.y+ButtonRect.h, 120, 20*7};
		PushPopup(&CEditor2::RenderPopupMenuFile, &FileMenuRect);
	}

	TopPanel.VSplitLeft(50.0f, &ButtonRect, &TopPanel);
	static CUIButton s_Help;
	if(UiButton(ButtonRect, "Help", &s_Help))
	{

	}

	// tools
	if(m_Page == PAGE_MAP_EDITOR)
	{
		TopPanel.VSplitLeft(50.0f, 0, &TopPanel);
		static CUIButton s_ButTools[TOOL_COUNT_];
		const char* aButName[] = {
			"Se",
			"Di",
			"TB"
		};

		for(int t = 0; t < TOOL_COUNT_; t++)
		{
			TopPanel.VSplitLeft(25.0f, &ButtonRect, &TopPanel);
			if(UiButtonEx(ButtonRect, aButName[t], &s_ButTools[t], m_Tool == t ? StyleColorInputSelected : StyleColorButton, m_Tool == t ? StyleColorInputSelected : StyleColorButtonHover, StyleColorButtonPressed, StyleColorButtonBorder, 10.0f, CUI::ALIGN_LEFT))
				ChangeTool(t);
		}
	}

	TopPanel.VSplitRight(20.0f, &TopPanel, 0);
	TopPanel.VSplitRight(120.0f, &TopPanel, &ButtonRect);
	static CUIButton s_PageSwitchButton;
	if(m_Page == PAGE_MAP_EDITOR)
	{
		if(UiButton(ButtonRect, "Go to Asset Manager", &s_PageSwitchButton))
			ChangePage(PAGE_ASSET_MANAGER);
	}
	else if(m_Page == PAGE_ASSET_MANAGER)
	{
		if(UiButton(ButtonRect, "Go to Map Editor", &s_PageSwitchButton))
			ChangePage(PAGE_MAP_EDITOR);
	}
}

void CEditor2::RenderMapEditorUI()
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect TopPanel, RightPanel, DetailPanel, ToolColumnRect;
	UiScreenRect.VSplitRight(150.0f, &m_UiMainViewRect, &RightPanel);
	m_UiMainViewRect.HSplitTop(MenuBarHeight, &TopPanel, &m_UiMainViewRect);

	RenderMenuBar(TopPanel);

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

	static CUIButton s_ButGroups;
	if(UiButton(ButtonRect, "Groups", &s_ButGroups))
	{
		s_CurrentTab = TAB_GROUPS;
	}

	static CUIButton s_ButHistory;
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
		RenderHistory(NavRect);
	}

	// detail panel
	if(m_UiDetailPanelIsOpen)
	{
		m_UiMainViewRect.VSplitRight(150, &m_UiMainViewRect, &DetailPanel);
		DrawRect(DetailPanel, StyleColorBg);
		RenderMapEditorUiDetailPanel(DetailPanel);
	}
	else
	{
		const float ButtonSize = 20.0f;
		const float Margin = 5.0f;
		m_UiMainViewRect.VSplitRight(ButtonSize + Margin*2, 0, &DetailPanel);
		DetailPanel.Margin(Margin, &ButtonRect);
		ButtonRect.h = ButtonSize;
	}

	UI()->ClipEnable(&m_UiMainViewRect); // clip main view rect

	// tools
	const float ButtonSize = 20.0f;
	const float Margin = 5.0f;
	m_UiMainViewRect.VSplitLeft(ButtonSize + Margin * 2, &ToolColumnRect, 0);
	ToolColumnRect.VMargin(Margin, &ToolColumnRect);

	static CUIButton s_ButTools[TOOL_COUNT_];
	const char* aButName[] = {
		"Se",
		"Di",
		"TB"
	};

	for(int t = 0; t < TOOL_COUNT_; t++)
	{
		ToolColumnRect.HSplitTop(Margin, 0, &ToolColumnRect);
		ToolColumnRect.HSplitTop(ButtonSize, &ButtonRect, &ToolColumnRect);

		if(UiButtonSelect(ButtonRect, aButName[t], &s_ButTools[t], m_Tool == t))
			ChangeTool(t);
	}

	// selection context buttons
	if(m_UiSelectedLayerID != -1 && IsToolSelect() && m_TileSelection.IsSelected())
	{
		const CEditorMap2::CLayer& SelectedTileLayer = m_Map.m_aLayers.Get(m_UiSelectedLayerID);

		if(SelectedTileLayer.IsTileLayer())
		{
			const float TileSize = 32;

			CUIRect SelectRect = {
				m_TileSelection.m_StartTX * TileSize,
				m_TileSelection.m_StartTY * TileSize,
				(m_TileSelection.m_EndTX+1-m_TileSelection.m_StartTX)*TileSize,
				(m_TileSelection.m_EndTY+1-m_TileSelection.m_StartTY)*TileSize
			};

			CUIRect UiRect = CalcUiRectFromGroupWorldRect(m_UiSelectedGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, SelectRect);

			const float Margin = 4.0f;
			const float ButtonHeight = 25;
			UiRect.y -= ButtonHeight + Margin;

			CUIRect LineRect, ButtonRect;
			UiRect.HSplitTop(ButtonHeight, &LineRect, 0);
			LineRect.VSplitLeft(30, &ButtonRect, &LineRect);
			LineRect.VSplitLeft(Margin, 0, &LineRect);

			static CUIButton s_ButFlipX, s_ButFlipY;

			if(UiButton(ButtonRect, "X/X", &s_ButFlipX))
			{
				EditTileSelectionFlipX(m_UiSelectedLayerID);
			}

			LineRect.VSplitLeft(30, &ButtonRect, &LineRect);
			LineRect.VSplitLeft(Margin, 0, &LineRect);

			if(UiButton(ButtonRect, "Y/Y", &s_ButFlipY))
			{
				EditTileSelectionFlipY(m_UiSelectedLayerID);
			}

			// Auto map
			CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(SelectedTileLayer.m_ImageID);

			if(pMapper)
			{
				const int RulesetCount = pMapper->RuleSetNum();
				static CUIButton s_ButtonAutoMap[16];
				 // TODO: find a better solution to this
				dbg_assert(RulesetCount <= 16, "RulesetCount is too big");

				for(int r = 0; r < RulesetCount; r++)
				{
					LineRect.VSplitLeft(50, &ButtonRect, &LineRect);
					LineRect.VSplitLeft(Margin, 0, &LineRect);

					if(UiButton(ButtonRect, pMapper->GetRuleSetName(r), &s_ButtonAutoMap[r]))
					{
						EditTileLayerAutoMapSection(m_UiSelectedLayerID, r, m_TileSelection.m_StartTX, m_TileSelection.m_StartTY, m_TileSelection.m_EndTX+1-m_TileSelection.m_StartTX, m_TileSelection.m_EndTY+1-m_TileSelection.m_StartTY);
					}
				}
			}
		}
	}

	// Clip and tilelayer resize handles
	if(IsToolDimension())
	{
		CEditorMap2::CGroup& SelectedGroup = m_Map.m_aGroups.Get(m_UiSelectedGroupID);
		if(SelectedGroup.m_UseClipping)
		{
			CUIRect ClipRect = {
				(float)SelectedGroup.m_ClipX,
				(float)SelectedGroup.m_ClipY,
				(float)SelectedGroup.m_ClipWidth,
				(float)SelectedGroup.m_ClipHeight
			};

			CUIRect ClipUiRect = CalcUiRectFromGroupWorldRect(m_Map.m_GameGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, ClipRect);

			const float HandleSize = 10.0f;
			const vec4 ColNormal(0.85f, 0.0f, 0.0f, 1);
			const vec4 ColActive(1.0f, 0.0f, 0.0f, 1);

			// handles
			CUIRect HandleTop = {
				ClipUiRect.x - HandleSize * 0.5f + ClipUiRect.w * 0.5f,
				ClipUiRect.y - HandleSize * 0.5f,
				HandleSize, HandleSize
			};

			CUIRect HandleBottom = {
				ClipUiRect.x - HandleSize * 0.5f + ClipUiRect.w * 0.5f,
				ClipUiRect.y - HandleSize * 0.5f + ClipUiRect.h,
				HandleSize, HandleSize
			};

			CUIRect HandleLeft = {
				ClipUiRect.x - HandleSize * 0.5f,
				ClipUiRect.y - HandleSize * 0.5f + ClipUiRect.h * 0.5f,
				HandleSize, HandleSize
			};

			CUIRect HandleRight = {
				ClipUiRect.x - HandleSize * 0.5f + ClipUiRect.w,
				ClipUiRect.y - HandleSize * 0.5f + ClipUiRect.h * 0.5f,
				HandleSize, HandleSize
			};

			const vec2 WorldMousePos = CalcGroupWorldPosFromUiPos(m_Map.m_GameGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, m_UiMousePos);

			static CUIGrabHandle s_GrabHandleTop, s_GrabHandleLeft, s_GrabHandleBot, s_GrabHandleRight;
			bool WasGrabbingTop = s_GrabHandleTop.m_IsGrabbed;
			bool WasGrabbingLeft = s_GrabHandleLeft.m_IsGrabbed;
			bool WasGrabbingBot = s_GrabHandleBot.m_IsGrabbed;
			bool WasGrabbingRight = s_GrabHandleRight.m_IsGrabbed;
			static int BeforeGrabbingClipX,BeforeGrabbingClipY, BeforeGrabbingClipWidth, BeforeGrabbingClipHeight;
			if(!WasGrabbingTop && !WasGrabbingBot)
			{
				BeforeGrabbingClipY = SelectedGroup.m_ClipY;
				BeforeGrabbingClipHeight = SelectedGroup.m_ClipHeight;
			}
			if(!WasGrabbingLeft && !WasGrabbingRight)
			{
				BeforeGrabbingClipX = SelectedGroup.m_ClipX;
				BeforeGrabbingClipWidth = SelectedGroup.m_ClipWidth;
			}

			vec2 ToolTipPos(0,0);

			if(UiGrabHandle(HandleTop, &s_GrabHandleTop, ColNormal, ColActive))
			{
				EditHistCondGroupChangeClipY(m_UiSelectedGroupID, WorldMousePos.y, false);
				ToolTipPos = vec2(HandleTop.x, HandleTop.y);
			}

			if(UiGrabHandle(HandleLeft, &s_GrabHandleLeft, ColNormal, ColActive))
			{
				EditHistCondGroupChangeClipX(m_UiSelectedGroupID, WorldMousePos.x, false);
				ToolTipPos = vec2(HandleLeft.x, HandleLeft.y);
			}

			if(UiGrabHandle(HandleBottom, &s_GrabHandleBot, ColNormal, ColActive))
			{
				EditHistCondGroupChangeClipBottom(m_UiSelectedGroupID, WorldMousePos.y, false);
				ToolTipPos = vec2(HandleBottom.x, HandleBottom.y);
			}

			if(UiGrabHandle(HandleRight, &s_GrabHandleRight, ColNormal, ColActive))
			{
				EditHistCondGroupChangeClipRight(m_UiSelectedGroupID, WorldMousePos.x, false);
				ToolTipPos = vec2(HandleRight.x, HandleRight.y);
			}

			// finished grabbing
			if(!s_GrabHandleLeft.m_IsGrabbed && WasGrabbingLeft)
			{
				SelectedGroup.m_ClipX = BeforeGrabbingClipX;
				SelectedGroup.m_ClipWidth = BeforeGrabbingClipWidth;
				EditHistCondGroupChangeClipX(m_UiSelectedGroupID, WorldMousePos.x, true);
			}

			if(!s_GrabHandleTop.m_IsGrabbed && WasGrabbingTop)
			{
				SelectedGroup.m_ClipY = BeforeGrabbingClipY;
				SelectedGroup.m_ClipHeight = BeforeGrabbingClipHeight;
				EditHistCondGroupChangeClipY(m_UiSelectedGroupID, WorldMousePos.y, true);
			}

			if(!s_GrabHandleRight.m_IsGrabbed && WasGrabbingRight)
			{
				SelectedGroup.m_ClipWidth = BeforeGrabbingClipWidth;
				EditHistCondGroupChangeClipRight(m_UiSelectedGroupID, WorldMousePos.x, true);
			}

			if(!s_GrabHandleBot.m_IsGrabbed && WasGrabbingBot)
			{
				SelectedGroup.m_ClipHeight = BeforeGrabbingClipHeight;
				EditHistCondGroupChangeClipBottom(m_UiSelectedGroupID, WorldMousePos.y, true);
			}

			// Size tooltip info
			if(s_GrabHandleLeft.m_IsGrabbed || s_GrabHandleTop.m_IsGrabbed ||
			   s_GrabHandleRight.m_IsGrabbed || s_GrabHandleBot.m_IsGrabbed)
			{
				CUIRect ToolTipRect = {
					ToolTipPos.x + 20.0f,
					ToolTipPos.y,
					30, 48
				};
				DrawRectBorder(ToolTipRect, vec4(0.6f, 0.0f, 0.0f, 1.0f), 1, vec4(1.0f, 1.0f, 1.0f, 1));

				ToolTipRect.x += 2;
				CUIRect TopPart;
				char aWidthBuff[16];
				char aHeightBuff[16];
				char aPosXBuff[16];
				char aPosYBuff[16];
				str_format(aPosXBuff, sizeof(aPosXBuff), "%d", SelectedGroup.m_ClipX);
				str_format(aPosYBuff, sizeof(aPosYBuff), "%d", SelectedGroup.m_ClipY);
				str_format(aWidthBuff, sizeof(aWidthBuff), "%d", SelectedGroup.m_ClipWidth);
				str_format(aHeightBuff, sizeof(aHeightBuff), "%d", SelectedGroup.m_ClipHeight);

				ToolTipRect.HSplitTop(12, &TopPart, &ToolTipRect);
				DrawText(TopPart, aPosXBuff, 8, vec4(1, 1, 1, 1.0));

				ToolTipRect.HSplitTop(12, &TopPart, &ToolTipRect);
				DrawText(TopPart, aPosYBuff, 8, vec4(1, 1, 1, 1.0));

				ToolTipRect.HSplitTop(12, &TopPart, &ToolTipRect);
				DrawText(TopPart, aWidthBuff, 8, vec4(1, 1, 1, 1.0));

				ToolTipRect.HSplitTop(12, &TopPart, &ToolTipRect);
				DrawText(TopPart, aHeightBuff, 8, vec4(1, 1, 1, 1.0));
			}
		}

		// tile layer resize
		CEditorMap2::CLayer& SelectedTileLayer = m_Map.m_aLayers.Get(m_UiSelectedLayerID);

		if(SelectedTileLayer.IsTileLayer())
		{
			const float TileSize = 32;
			CUIRect LayerRect = {
				0,
				0,
				SelectedTileLayer.m_Width * TileSize,
				SelectedTileLayer.m_Height * TileSize
			};

			const float HandleSize = 10.0f;
			const vec4 ColNormal(0.85f, 0.85f, 0.85f, 1);
			const vec4 ColActive(1.0f, 1.0f, 1.0f, 1);

			static CUIGrabHandle s_GrabHandleBot, s_GrabHandleRight, s_GrabHandleBotRight;
			bool WasGrabbingBot = s_GrabHandleBot.m_IsGrabbed;
			bool WasGrabbingRight = s_GrabHandleRight.m_IsGrabbed;
			bool WasGrabbingBotRight = s_GrabHandleBotRight.m_IsGrabbed;

			static CUIRect PreviewRect;

			bool IsAnyHandleGrabbed = true;
			if(!s_GrabHandleBot.m_IsGrabbed && !s_GrabHandleRight.m_IsGrabbed && !s_GrabHandleBotRight.m_IsGrabbed)
			{
				PreviewRect = LayerRect;
				IsAnyHandleGrabbed = false;
			}

			CUIRect LayerUiRect = CalcUiRectFromGroupWorldRect(m_UiSelectedGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, PreviewRect);

			// handles
			CUIRect HandleBottom = {
				LayerUiRect.x - HandleSize * 0.5f + LayerUiRect.w * 0.5f,
				LayerUiRect.y - HandleSize * 0.5f + LayerUiRect.h,
				HandleSize, HandleSize
			};

			CUIRect HandleRight = {
				LayerUiRect.x - HandleSize * 0.5f + LayerUiRect.w,
				LayerUiRect.y - HandleSize * 0.5f + LayerUiRect.h * 0.5f,
				HandleSize, HandleSize
			};

			CUIRect HandleBottomRight = {
				LayerUiRect.x - HandleSize * 0.5f + LayerUiRect.w,
				LayerUiRect.y - HandleSize * 0.5f + LayerUiRect.h,
				HandleSize, HandleSize
			};

			if(IsAnyHandleGrabbed)
				DrawRectBorder(LayerUiRect, vec4(1,1,1,0.2f), 1, vec4(1, 1, 1, 1));

			const vec2 WorldMousePos = CalcGroupWorldPosFromUiPos(m_UiSelectedGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, m_UiMousePos);

			vec2 ToolTipPos(0, 0);
			if(UiGrabHandle(HandleBottom, &s_GrabHandleBot, ColNormal, ColActive))
			{
				PreviewRect.h = (int)round(WorldMousePos.y / TileSize) * TileSize;
				ToolTipPos = vec2(HandleBottom.x, HandleBottom.y + HandleSize);
			}

			if(UiGrabHandle(HandleRight, &s_GrabHandleRight, ColNormal, ColActive))
			{
				PreviewRect.w = (int)round(WorldMousePos.x / TileSize) * TileSize;
				ToolTipPos = vec2(HandleRight.x, HandleRight.y);
			}

			if(UiGrabHandle(HandleBottomRight, &s_GrabHandleBotRight, ColNormal, ColActive))
			{
				PreviewRect.w = (int)round(WorldMousePos.x / TileSize) * TileSize;
				PreviewRect.h = (int)round(WorldMousePos.y / TileSize) * TileSize;
				ToolTipPos = vec2(HandleBottomRight.x, HandleBottomRight.y);
			}

			// clamp
			if(PreviewRect.w < TileSize)
				PreviewRect.w = TileSize;
			if(PreviewRect.h < TileSize)
				PreviewRect.h = TileSize;

			// Width/height tooltip info
			if(IsAnyHandleGrabbed)
			{
				CUIRect ToolTipRect = {
					ToolTipPos.x + 20.0f,
					ToolTipPos.y,
					30, 25
				};
				DrawRectBorder(ToolTipRect, vec4(0.6f, 0.6f, 0.6f, 1.0), 1, vec4(1.0f, 0.5f, 0, 1));

				ToolTipRect.x += 2;
				CUIRect TopPart, BotPart;
				ToolTipRect.HSplitMid(&TopPart, &BotPart);

				char aWidthBuff[16];
				char aHeightBuff[16];
				str_format(aWidthBuff, sizeof(aWidthBuff), "%d", (int)(PreviewRect.w / TileSize));
				str_format(aHeightBuff, sizeof(aHeightBuff), "%d", (int)(PreviewRect.h / TileSize));

				DrawText(TopPart, aWidthBuff, 8, vec4(1, 1, 1, 1.0));
				DrawText(BotPart, aHeightBuff, 8, vec4(1, 1, 1, 1.0));
			}

			// IF we let go of any handle, resize tile layer
			if((WasGrabbingBot && !s_GrabHandleBot.m_IsGrabbed) ||
			   (WasGrabbingRight && !s_GrabHandleRight.m_IsGrabbed) ||
			   (WasGrabbingBotRight && !s_GrabHandleBotRight.m_IsGrabbed))
			{
				EditTileLayerResize(m_UiSelectedLayerID, (int)(PreviewRect.w / TileSize), (int)(PreviewRect.h / TileSize));
			}
		}
	}

	UI()->ClipDisable(); // main view rect clip

	// popups
	RenderPopups();
}

void CEditor2::RenderMapEditorUiLayerGroups(CUIRect NavRect)
{
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;
	const float ShowButtonWidth = 15.0f;

	CUIRect ActionLineRect, ButtonRect;
	NavRect.HSplitBottom((ButtonHeight + Spacing) * 3.0f, &NavRect, &ActionLineRect);

	// TODO: keeping track of Layer/Group UI state is not great for now, make something better

	const int TotalGroupCount = m_Map.m_aGroups.Count();
	const int TotalLayerCount = m_Map.m_aLayers.Count();

	// TODO: move to group/layer ui state?
	static array2<CUIButton> s_UiGroupButState;
	static array2<CUIButton> s_UiGroupShowButState;
	static array2<CUIButton> s_UiLayerButState;
	static array2<CUIButton> s_UiLayerShowButState;

	ArraySetSizeAndZero(&s_UiGroupButState, TotalGroupCount);
	ArraySetSizeAndZero(&s_UiGroupShowButState, TotalGroupCount);
	ArraySetSizeAndZero(&s_UiLayerButState, TotalLayerCount); // FIXME: can LayerID (m_aLayers.Get(LayerId)) be >= TotalLayerCount? This is probably a bad way of tracking button/other state.
	ArraySetSizeAndZero(&s_UiLayerShowButState, TotalLayerCount);

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOff(0, 0);
	UiBeginScrollRegion(&s_ScrollRegion, &NavRect, &ScrollOff);
	NavRect.y += ScrollOff.y;

	// drag to reorder items
	static CUIMouseDrag s_DragMove;
	static int DragMoveGroupListIndex = -1;
	static int DragMoveLayerListIndex = -1;
	static int DragMoveParentGroupListIndex = -1;
	if(!s_DragMove.m_IsDragging)
	{
		DragMoveGroupListIndex = -1;
		DragMoveLayerListIndex = -1;
		DragMoveParentGroupListIndex = -1;
	}

	bool OldIsMouseDragging = s_DragMove.m_IsDragging;
	bool FinishedMouseDragging = UiDoMouseDraggingNoID(NavRect, &s_DragMove);
	bool StartedMouseDragging = s_DragMove.m_IsDragging && OldIsMouseDragging == false;

	bool DisplayDragMoveOverlay = s_DragMove.m_IsDragging && (DragMoveGroupListIndex >= 0 || DragMoveLayerListIndex >= 0);
	CUIRect DragMoveOverlayRect;
	int DragMoveOffset = 0;
	int DragMoveGroupIdOffset = 0; // for group dragging
	// -----------------------

	const int GroupCount = m_Map.m_GroupIDListCount;
	const u32* aGroupIDList = m_Map.m_aGroupIDList;
	for(int gi = 0; gi < GroupCount; gi++)
	{
		const int GroupID = aGroupIDList[gi];
		const CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

		if(gi != 0)
			NavRect.HSplitTop(Spacing, 0, &NavRect);
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
		UiScrollRegionAddRect(&s_ScrollRegion, ButtonRect);

		// check whole line for hover
		CUIButton WholeLineState;
		UiDoButtonBehaviorNoID(ButtonRect, &WholeLineState);
		m_UiGroupState[GroupID].m_IsHovered = WholeLineState.m_Hovered;

		// drag started on this item
		if(StartedMouseDragging && WholeLineState.m_Hovered)
		{
			DragMoveGroupListIndex = gi;
		}
		if(DragMoveGroupListIndex == gi)
		{
			DragMoveOverlayRect = ButtonRect;
			DragMoveOffset = (int)(m_UiMousePos.y - ButtonRect.y - ButtonHeight/2.0f)/(Spacing+ButtonHeight);
		}

		CUIRect ExpandBut, ShowButton;

		// show button
		ButtonRect.VSplitRight(ShowButtonWidth, &ButtonRect, &ShowButton);
		CUIButton& ShowButState = s_UiGroupShowButState[GroupID];
		UiDoButtonBehavior(&ShowButState, ShowButton, &ShowButState);

		if(ShowButState.m_Clicked)
			m_UiGroupState[GroupID].m_IsHidden ^= 1;

		const bool IsShown = !m_UiGroupState[GroupID].m_IsHidden;

		vec4 ShowButColor = StyleColorButton;
		if(ShowButState.m_Hovered)
			ShowButColor = StyleColorButtonHover;
		if(ShowButState.m_Pressed)
			ShowButColor = StyleColorButtonPressed;

		DrawRectBorder(ShowButton, ShowButColor, 1, StyleColorButtonBorder);
		DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

		// group button
		CUIButton& ButState = s_UiGroupButState[GroupID];
		UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

		if(ButState.m_Clicked)
		{
			if(m_UiSelectedGroupID == GroupID)
				m_UiGroupState[GroupID].m_IsOpen ^= 1;

			m_UiSelectedGroupID = GroupID;
			if(Group.m_LayerCount > 0)
				m_UiSelectedLayerID = Group.m_apLayerIDs[0];
			else
				m_UiSelectedLayerID = -1;
		}

		const bool IsSelected = m_UiSelectedGroupID == GroupID;
		const bool IsOpen = m_UiGroupState[GroupID].m_IsOpen;

		vec4 ButColor = StyleColorButton;
		if(ButState.m_Hovered)
			ButColor = StyleColorButtonHover;
		if(ButState.m_Pressed)
			ButColor = StyleColorButtonPressed;

		if(IsSelected)
			DrawRectBorder(ButtonRect, ButColor, 1, StyleColorButtonBorder);
		else
			DrawRect(ButtonRect, ButColor);


		ButtonRect.VSplitLeft(ButtonRect.h, &ExpandBut, &ButtonRect);
		DrawText(ExpandBut, IsOpen ? "-" : "+", FontSize);

		char aGroupName[64];
		if(m_Map.m_GameGroupID == GroupID)
			str_format(aGroupName, sizeof(aGroupName), "Game group");
		else
			str_format(aGroupName, sizeof(aGroupName), "Group #%d", gi);
		DrawText(ButtonRect, aGroupName, FontSize);

		if(m_UiGroupState[GroupID].m_IsOpen)
		{
			const int LayerCount = Group.m_LayerCount;

			for(int li = 0; li < LayerCount; li++)
			{
				const int LyID = Group.m_apLayerIDs[li];
				dbg_assert(m_Map.m_aLayers.IsValid(LyID), "LayerID out of bounds");

				const CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LyID);
				NavRect.HSplitTop(Spacing, 0, &NavRect);
				NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
				UiScrollRegionAddRect(&s_ScrollRegion, ButtonRect);
				ButtonRect.VSplitLeft(10.0f, 0, &ButtonRect);

				// check whole line for hover
				CUIButton WholeLineState;
				UiDoButtonBehaviorNoID(ButtonRect, &WholeLineState);
				m_UiLayerState[LyID].m_IsHovered = WholeLineState.m_Hovered;

				// drag started on this item
				if(StartedMouseDragging && WholeLineState.m_Hovered)
				{
					DragMoveLayerListIndex = li;
					DragMoveParentGroupListIndex = gi;
				}
				if(DragMoveLayerListIndex == li && DragMoveParentGroupListIndex == gi)
				{
					DragMoveOverlayRect = ButtonRect;
					DragMoveOffset = (int)(m_UiMousePos.y - ButtonRect.y - ButtonHeight/2.0f)/(Spacing+ButtonHeight);
				}

				// show button
				ButtonRect.VSplitRight(ShowButtonWidth, &ButtonRect, &ShowButton);
				CUIButton& ShowButState = s_UiLayerShowButState[LyID];
				UiDoButtonBehavior(&ShowButState, ShowButton, &ShowButState);

				if(ShowButState.m_Clicked)
					m_UiLayerState[LyID].m_IsHidden ^= 1;

				const bool IsShown = !m_UiLayerState[LyID].m_IsHovered;

				vec4 ShowButColor = StyleColorButton;
				if(ShowButState.m_Hovered)
					ShowButColor = StyleColorButtonHover;
				if(ShowButState.m_Pressed)
					ShowButColor = StyleColorButtonPressed;

				DrawRectBorder(ShowButton, ShowButColor, 1, StyleColorButtonBorder);
				DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

				// layer button
				CUIButton& ButState = s_UiLayerButState[LyID];
				UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

				vec4 ButColor = StyleColorButton;
				if(ButState.m_Hovered)
					ButColor = StyleColorButtonHover;
				if(ButState.m_Pressed)
					ButColor = StyleColorButtonPressed;

				if(ButState.m_Clicked)
				{
					if(m_UiSelectedGroupID == GroupID && m_UiSelectedLayerID == LyID)
					{
						m_UiDetailPanelIsOpen ^= 1;
					}
					else
					{
						m_UiSelectedLayerID = LyID;
						m_UiSelectedGroupID = GroupID;
					}
				}

				const bool IsSelected = m_UiSelectedLayerID == LyID;

				if(IsSelected)
				{
					// DrawRectBorder(ButtonRect, ButColor, 1, vec4(1, 0, 0, 1));
					DrawRect(ButtonRect, StyleColorButtonPressed); // Fisico

					if(m_UiDetailPanelIsOpen)
					{
						// Draw triangle on the left
						const vec4 Color = StyleColorButtonPressed;
						const float x = ButtonRect.x;
						const float y = ButtonRect.y;
						const float h = ButtonRect.h;

						Graphics()->TextureClear();
						Graphics()->QuadsBegin();
						IGraphics::CFreeformItem Triangle(
							x, y,
							x, y,
							x - h*0.5f, y + h*0.5f,
							x, y + h
						);

						Graphics()->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);

						Graphics()->QuadsDrawFreeform(&Triangle, 1);
						Graphics()->QuadsEnd();
					}
				}
				else
					DrawRect(ButtonRect, ButColor);

				char aLayerName[64];
				const int ImageID = Layer.m_ImageID;
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

	// add some extra padding
	NavRect.HSplitTop(10, &ButtonRect, &NavRect);
	UiScrollRegionAddRect(&s_ScrollRegion, ButtonRect);

	UiEndScrollRegion(&s_ScrollRegion);

	// Add buttons
	CUIRect ButtonRect2;
	ActionLineRect.HSplitTop(ButtonHeight, &ButtonRect, &ActionLineRect);
	ActionLineRect.HSplitTop(Spacing, 0, &ActionLineRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	static CUIButton s_ButAddTileLayer, s_ButAddQuadLayer, s_ButAddGroup;
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

	// delete buttons
	const bool IsGameLayer = m_UiSelectedLayerID == m_Map.m_GameLayerID;
	const bool IsGameGroup = m_UiSelectedGroupID == m_Map.m_GameGroupID;

	ActionLineRect.HSplitTop(ButtonHeight, &ButtonRect, &ActionLineRect);
	ActionLineRect.HSplitTop(Spacing, 0, &ActionLineRect);

	static CUIButton s_LayerDeleteButton;
	if(UiButtonEx(ButtonRect, Localize("Delete layer"), &s_LayerDeleteButton,
		vec4(0.4f, 0.04f, 0.04f, 1), vec4(0.96f, 0.16f, 0.16f, 1), vec4(0.31f, 0, 0, 1),
		vec4(0.63f, 0.035f, 0.035f, 1), 10, CUI::ALIGN_LEFT) && !IsGameLayer)
	{
		int SelectedLayerID = m_UiSelectedLayerID;
		int SelectedGroupID = m_UiSelectedGroupID;
		SelectLayerBelowCurrentOne();

		EditDeleteLayer(SelectedLayerID, SelectedGroupID);
	}

	ActionLineRect.HSplitTop(ButtonHeight, &ButtonRect, &ActionLineRect);

	// delete button
	static CUIButton s_GroupDeleteButton;
	if(UiButtonEx(ButtonRect, Localize("Delete group"), &s_GroupDeleteButton, vec4(0.4f, 0.04f, 0.04f, 1),
		vec4(0.96f, 0.16f, 0.16f, 1), vec4(0.31f, 0, 0, 1), vec4(0.63f, 0.035f, 0.035f, 1), 10, CUI::ALIGN_LEFT) && !IsGameGroup)
	{
		int ToDeleteID = m_UiSelectedGroupID;
		SelectGroupBelowCurrentOne();
		EditDeleteGroup(ToDeleteID);
	}

	// clamp DragMoveOffset
	if(DragMoveOffset != 0)
	{
		if(DragMoveLayerListIndex != -1)
			DragMoveOffset = LayerCalcDragMoveOffset(DragMoveParentGroupListIndex, DragMoveLayerListIndex, DragMoveOffset);
		else if(DragMoveGroupListIndex != -1)
			DragMoveGroupIdOffset = GroupCalcDragMoveOffset(DragMoveGroupListIndex, &DragMoveOffset);
	}

	// drag overlay indicator
	if(DisplayDragMoveOverlay && !UI()->MouseInside(&DragMoveOverlayRect) && DragMoveOffset != 0)
	{
		const float x = DragMoveOverlayRect.x;
		const float y = DragMoveOverlayRect.y
			+ (sign(DragMoveOffset) == 1 ? DragMoveOverlayRect.h : 0)
			+ (ButtonHeight+Spacing) * DragMoveOffset;
		const float w = DragMoveOverlayRect.w;

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		IGraphics::CFreeformItem RowIndicator(
			x, y + Spacing * sign(DragMoveOffset),
			x + w, y + Spacing * sign(DragMoveOffset),
			x, y,
			x + w, y
		);

		Graphics()->SetColor(1, 0, 0, 1);

		Graphics()->QuadsDrawFreeform(&RowIndicator, 1);
		Graphics()->QuadsEnd();
	}

	// finished dragging, move
	// TODO: merge the 2 ifs
	if(FinishedMouseDragging)
	{
		if(!IsInsideRect(s_DragMove.m_EndDragPos, DragMoveOverlayRect))
		{
			if(DragMoveGroupListIndex != -1)
			{
				ed_dbg("DragMoveGroupIdOffset = %d", DragMoveGroupIdOffset);
				int NewGroupListIndex = EditGroupOrderMove(DragMoveGroupListIndex, DragMoveGroupIdOffset);
				m_UiGroupState[m_Map.m_aGroupIDList[NewGroupListIndex]].m_IsOpen = true;

				if((int)m_Map.m_aGroupIDList[DragMoveGroupListIndex] == m_UiSelectedGroupID && NewGroupListIndex != DragMoveGroupListIndex)
				{
					m_UiSelectedGroupID = m_Map.m_aGroupIDList[NewGroupListIndex];
					m_UiSelectedLayerID = -1;
				}
			}
			else if(DragMoveLayerListIndex != -1)
			{
				int NewGroupListIndex = EditLayerOrderMove(DragMoveParentGroupListIndex, DragMoveLayerListIndex, DragMoveOffset);
				m_UiGroupState[m_Map.m_aGroupIDList[NewGroupListIndex]].m_IsOpen = true;
			}
		}
	}
}

void CEditor2::RenderHistory(CUIRect NavRect)
{
	CUIRect ButtonRect;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOff(0, 0);
	UiBeginScrollRegion(&s_ScrollRegion, &NavRect, &ScrollOff);
	NavRect.y += ScrollOff.y;

	CHistoryEntry* pFirstEntry = m_pHistoryEntryCurrent;
	while(pFirstEntry->m_pPrev)
		pFirstEntry = pFirstEntry->m_pPrev;

	static CUIButton s_ButEntry[50];
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;

	CHistoryEntry* pCurrentEntry = pFirstEntry;
	int i = 0;
	while(pCurrentEntry)
	{
		NavRect.HSplitTop(ButtonHeight*2, &ButtonRect, &NavRect);
		NavRect.HSplitTop(Spacing, 0, &NavRect);
		//UiScrollRegionAddRect(&s_ScrollRegion, ButtonRect);

		// somewhat hacky
		CUIButton& ButState = s_ButEntry[i % (sizeof(s_ButEntry)/sizeof(s_ButEntry[0]))];
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

	// some padding at the end
	NavRect.HSplitTop(10, &ButtonRect, &NavRect);
	UiScrollRegionAddRect(&s_ScrollRegion, ButtonRect);

	UiEndScrollRegion(&s_ScrollRegion);
}

void CEditor2::RenderMapEditorUiDetailPanel(CUIRect DetailRect)
{
	DetailRect.Margin(3.0f, &DetailRect);

	// GROUP/LAYER DETAILS

	// group
	dbg_assert(m_UiSelectedGroupID >= 0, "No selected group");
	CEditorMap2::CGroup& SelectedGroup = m_Map.m_aGroups.Get(m_UiSelectedGroupID);
	const bool IsGameGroup = m_UiSelectedGroupID == m_Map.m_GameGroupID;
	char aBuff[128];

	CUIRect ButtonRect, ButtonRect2;
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;

	// close button
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	static CUIButton s_CloseDetailPanelButton;
	if(UiButton(ButtonRect, ">", &s_CloseDetailPanelButton, 8))
		m_UiDetailPanelIsOpen = false;

	// label
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	DrawRect(ButtonRect, StyleColorButtonPressed);
	DrawText(ButtonRect, Localize("Group"), FontSize);

	if(!IsGameGroup)
	{
		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUITextInput s_TIGroupName;

		static char aBeforeSelectionName[sizeof(SelectedGroup.m_aName)];
		char aNewName[sizeof(SelectedGroup.m_aName)];
		str_copy(aNewName, SelectedGroup.m_aName, sizeof(aNewName));

		// save name before text input selection
		if(!s_TIGroupName.m_Selected)
			str_copy(aBeforeSelectionName, SelectedGroup.m_aName, sizeof(aBeforeSelectionName));

		// "preview" new name instantly
		if(UiTextInput(ButtonRect, aNewName, sizeof(aNewName), &s_TIGroupName))
			EditHistCondGroupChangeName(m_UiSelectedGroupID, aNewName, false);

		// if not selected, restore old name and change to new name with history entry
		if(!s_TIGroupName.m_Selected)
		{
			str_copy(SelectedGroup.m_aName, aBeforeSelectionName, sizeof(SelectedGroup.m_aName));
			EditHistCondGroupChangeName(m_UiSelectedGroupID, aNewName, true);
		}
	}

	// parallax
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawText(ButtonRect, Localize("Parallax"), FontSize);

	ButtonRect2.VSplitMid(&ButtonRect, &ButtonRect2);
	static CUIIntegerInput s_IntInpParallaxX, s_IntInpParallaxY;

	// before selection
	static int BsGroupParallaX, BsGroupParallaY;
	if(!s_IntInpParallaxX.m_TextInput.m_Selected && !s_IntInpParallaxY.m_TextInput.m_Selected)
	{
		BsGroupParallaX = SelectedGroup.m_ParallaxX;
		BsGroupParallaY = SelectedGroup.m_ParallaxY;
	}

	int NewGroupParallaxX = SelectedGroup.m_ParallaxX;
	int NewGroupParallaxY = SelectedGroup.m_ParallaxY;
	bool ParallaxChanged = false;
	ParallaxChanged |= UiIntegerInput(ButtonRect, &NewGroupParallaxX, &s_IntInpParallaxX);
	ParallaxChanged |= UiIntegerInput(ButtonRect2, &NewGroupParallaxY, &s_IntInpParallaxY);
	if(ParallaxChanged)
		EditHistCondGroupChangeParallax(m_UiSelectedGroupID, NewGroupParallaxX, NewGroupParallaxY, false);

	// restore "before preview" parallax, the nchange to new one
	if(!s_IntInpParallaxX.m_TextInput.m_Selected && !s_IntInpParallaxY.m_TextInput.m_Selected)
	{
		SelectedGroup.m_ParallaxX = BsGroupParallaX;
		SelectedGroup.m_ParallaxY = BsGroupParallaY;
		EditHistCondGroupChangeParallax(m_UiSelectedGroupID, NewGroupParallaxX, NewGroupParallaxY, true);
	}

	// offset
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);

	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawText(ButtonRect, Localize("Offset"), FontSize);

	ButtonRect2.VSplitMid(&ButtonRect, &ButtonRect2);
	static CUIIntegerInput s_IntInpOffsetX, s_IntInpOffsetY;

	// before selection
	static int BsGroupOffsetX, BsGroupOffsetY;
	if(!s_IntInpOffsetX.m_TextInput.m_Selected && !s_IntInpOffsetY.m_TextInput.m_Selected)
	{
		BsGroupOffsetX = SelectedGroup.m_OffsetX;
		BsGroupOffsetY = SelectedGroup.m_OffsetY;
	}

	int NewGroupOffsetX = SelectedGroup.m_OffsetX;
	int NewGroupOffsetY = SelectedGroup.m_OffsetY;
	bool OffsetChanged = false;
	OffsetChanged |= UiIntegerInput(ButtonRect, &NewGroupOffsetX, &s_IntInpOffsetX);
	OffsetChanged |= UiIntegerInput(ButtonRect2, &NewGroupOffsetY, &s_IntInpOffsetY);
	if(OffsetChanged)
		EditHistCondGroupChangeOffset(m_UiSelectedGroupID, NewGroupOffsetX, NewGroupOffsetY, false);

	// restore "before preview" offset, the nchange to new one
	if(!s_IntInpOffsetX.m_TextInput.m_Selected && !s_IntInpOffsetY.m_TextInput.m_Selected)
	{
		SelectedGroup.m_OffsetX = BsGroupOffsetX;
		SelectedGroup.m_OffsetY = BsGroupOffsetY;
		EditHistCondGroupChangeOffset(m_UiSelectedGroupID, NewGroupOffsetX, NewGroupOffsetY, true);
	}

	// clip
	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawRect(ButtonRect2, StyleColorBg);
	DrawText(ButtonRect, Localize("Clip start"), FontSize);
	ButtonRect2.VSplitMid(&ButtonRect, &ButtonRect2);

	str_format(aBuff, sizeof(aBuff), "%d", SelectedGroup.m_ClipX);
	DrawText(ButtonRect, aBuff, FontSize);
	str_format(aBuff, sizeof(aBuff), "%d", SelectedGroup.m_ClipY);
	DrawText(ButtonRect2, aBuff, FontSize);

	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawRect(ButtonRect2, StyleColorBg);
	DrawText(ButtonRect, Localize("Clip size"), FontSize);
	ButtonRect2.VSplitMid(&ButtonRect, &ButtonRect2);

	str_format(aBuff, sizeof(aBuff), "%d", SelectedGroup.m_ClipWidth);
	DrawText(ButtonRect, aBuff, FontSize);
	str_format(aBuff, sizeof(aBuff), "%d", SelectedGroup.m_ClipHeight);
	DrawText(ButtonRect2, aBuff, FontSize);

	DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
	DetailRect.HSplitTop(Spacing, 0, &DetailRect);
	ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
	DrawRect(ButtonRect, vec4(0,0,0,1));
	DrawText(ButtonRect, Localize("Use clipping"), FontSize);
	static CUICheckboxYesNo s_CbClipping;
	bool NewUseClipping = SelectedGroup.m_UseClipping;
	if(UiCheckboxYesNo(ButtonRect2, &NewUseClipping, &s_CbClipping))
		EditGroupUseClipping(m_UiSelectedGroupID, NewUseClipping);

	// layer
	if(m_UiSelectedLayerID >= 0)
	{
		CEditorMap2::CLayer& SelectedLayer = m_Map.m_aLayers.Get(m_UiSelectedLayerID);
		const bool IsGameLayer = m_UiSelectedLayerID == m_Map.m_GameLayerID;

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		// label
		DrawRect(ButtonRect, StyleColorButtonPressed);
		DrawText(ButtonRect, Localize("Layer"), FontSize);

		if(!IsGameLayer)
		{
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);

			static CUITextInput s_TILayerName;

			static char aBeforeSelectionName[sizeof(SelectedLayer.m_aName)];
			char aNewName[sizeof(SelectedLayer.m_aName)];
			str_copy(aNewName, SelectedLayer.m_aName, sizeof(aNewName));

			// save name before text input selection
			if(!s_TILayerName.m_Selected)
				str_copy(aBeforeSelectionName, SelectedLayer.m_aName, sizeof(aBeforeSelectionName));

			// "preview" new name instantly
			if(UiTextInput(ButtonRect, aNewName, sizeof(aNewName), &s_TILayerName))
				EditHistCondLayerChangeName(m_UiSelectedLayerID, aNewName, false);

			// if not selected, restore old name and change to new name with history entry
			if(!s_TILayerName.m_Selected)
			{
				str_copy(SelectedLayer.m_aName, aBeforeSelectionName, sizeof(SelectedLayer.m_aName));
				EditHistCondLayerChangeName(m_UiSelectedLayerID, aNewName, true);
			}
		}

		// tile layer
		if(SelectedLayer.IsTileLayer())
		{
			// size
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
			DrawRect(ButtonRect, vec4(0,0,0,1));
			DrawRect(ButtonRect2, StyleColorBg);

			str_format(aBuff, sizeof(aBuff), "%d", SelectedLayer.m_Width);
			DrawText(ButtonRect, Localize("Width"), FontSize);
			DrawText(ButtonRect2, aBuff, FontSize);

			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
			DrawRect(ButtonRect, vec4(0,0,0,1));
			DrawRect(ButtonRect2, StyleColorBg);

			str_format(aBuff, sizeof(aBuff), "%d", SelectedLayer.m_Height);
			DrawText(ButtonRect, Localize("Height"), FontSize);
			DrawText(ButtonRect2, aBuff, FontSize);

			// game layer
			if(IsGameLayer)
			{

			}
			else
			{
				// image
				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				DetailRect.HSplitTop(Spacing * 2, 0, &DetailRect);
				static CUIButton s_ImageButton;
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

				static CUIButton s_SliderColorR, s_SliderColorG, s_SliderColorB, s_SliderColorA;

				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				ButtonRect.VMargin(2, &ButtonRect);
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);

				vec4 NewColor = SelectedLayer.m_Color;
				bool SliderModified = false;
				static bool AnySliderSelected = false;
				static vec4 ColorBeforePreview = SelectedLayer.m_Color;

				if(!AnySliderSelected)
					ColorBeforePreview = SelectedLayer.m_Color;
				AnySliderSelected = false;

				vec4 SliderColor(0.7f, 0.1f, 0.1f, 1);
				SliderModified |= UiSliderFloat(ButtonRect, &NewColor.r, 0.0f, 1.0f, &s_SliderColorR, &SliderColor);
				AnySliderSelected |= UI()->CheckActiveItem(&s_SliderColorR);

				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				ButtonRect.VMargin(2, &ButtonRect);
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);

				SliderColor = vec4(0.1f, 0.7f, 0.1f, 1);
				SliderModified |= UiSliderFloat(ButtonRect, &NewColor.g, 0.0f, 1.0f, &s_SliderColorG, &SliderColor);
				AnySliderSelected |= UI()->CheckActiveItem(&s_SliderColorG);

				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				ButtonRect.VMargin(2, &ButtonRect);
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);

				SliderColor = vec4(0.1f, 0.1f, 0.7f, 1);
				SliderModified |= UiSliderFloat(ButtonRect, &NewColor.b, 0.0f, 1.0f, &s_SliderColorB, &SliderColor);
				AnySliderSelected |= UI()->CheckActiveItem(&s_SliderColorB);

				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				ButtonRect.VMargin(2, &ButtonRect);
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);

				SliderColor = vec4(0.5f, 0.5f, 0.5f, 1);
				SliderModified |= UiSliderFloat(ButtonRect, &NewColor.a, 0.0f, 1.0f, &s_SliderColorA, &SliderColor);
				AnySliderSelected |= UI()->CheckActiveItem(&s_SliderColorA);

				// "preview" change instantly
				if(SliderModified)
					EditHistCondLayerChangeColor(m_UiSelectedLayerID, NewColor, false);

				// restore old color before preview, then change it
				if(!AnySliderSelected)
				{
					SelectedLayer.m_Color = ColorBeforePreview;
					EditHistCondLayerChangeColor(m_UiSelectedLayerID, NewColor, true);
				}

				// Auto map
				CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(SelectedLayer.m_ImageID);

				if(pMapper)
				{
					DetailRect.HSplitTop(Spacing, 0, &DetailRect);
					DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
					DetailRect.HSplitTop(Spacing, 0, &DetailRect);

					// label
					DrawRect(ButtonRect, StyleColorButtonPressed);
					DrawText(ButtonRect, Localize("Auto-map"), FontSize);

					const int RulesetCount = pMapper->RuleSetNum();
					static CUIButton s_ButtonAutoMap[16];
					 // TODO: find a better solution to this
					dbg_assert(RulesetCount <= 16, "RulesetCount is too big");

					for(int r = 0; r < RulesetCount; r++)
					{
						DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
						DetailRect.HSplitTop(Spacing, 0, &DetailRect);

						if(UiButton(ButtonRect, pMapper->GetRuleSetName(r), &s_ButtonAutoMap[r], FontSize))
						{
							EditTileLayerAutoMapWhole(m_UiSelectedLayerID, r);
						}
					}
				}

				// high detail
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);
				DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
				DetailRect.HSplitTop(Spacing, 0, &DetailRect);
				ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
				DrawRect(ButtonRect, vec4(0,0,0,1));
				DrawText(ButtonRect, Localize("High Detail"), FontSize);
				static CUICheckboxYesNo s_CbHighDetail;
				bool NewHighDetail = SelectedLayer.m_HighDetail;
				if(UiCheckboxYesNo(ButtonRect2, &NewHighDetail, &s_CbHighDetail))
					EditLayerHighDetail(m_UiSelectedLayerID, NewHighDetail);
			}
		}
		else if(SelectedLayer.IsQuadLayer())
		{
			// image
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			static CUIButton s_ImageButton;
			const char* pText = SelectedLayer.m_ImageID >= 0 ?
				m_Map.m_Assets.m_aImageNames[SelectedLayer.m_ImageID].m_Buff : Localize("none");
			if(UiButton(ButtonRect, pText, &s_ImageButton, FontSize))
			{
				int ImageID = SelectedLayer.m_ImageID + 1;
				if(ImageID >= m_Map.m_Assets.m_ImageCount)
					ImageID = -1;
				EditLayerChangeImage(m_UiSelectedLayerID, ImageID);
			}

			// high detail
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			ButtonRect.VSplitMid(&ButtonRect, &ButtonRect2);
			DrawRect(ButtonRect, vec4(0,0,0,1));
			DrawText(ButtonRect, Localize("High Detail"), FontSize);
			static CUICheckboxYesNo s_CbHighDetail;
			bool NewHighDetail = SelectedLayer.m_HighDetail;
			if(UiCheckboxYesNo(ButtonRect2, &NewHighDetail, &s_CbHighDetail))
				EditLayerHighDetail(m_UiSelectedLayerID, NewHighDetail);


			// quad count
			DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
			DetailRect.HSplitTop(Spacing, 0, &DetailRect);
			DrawRect(ButtonRect, vec4(0,0,0,1));
			str_format(aBuff, sizeof(aBuff), "%d Quad%s:", SelectedLayer.m_aQuads.size(), SelectedLayer.m_aQuads.size() > 1 ? "s" : "");
			DrawText(ButtonRect, aBuff, FontSize);

			const int QuadCount = SelectedLayer.m_aQuads.size();
			if(QuadCount > 0)
			{
				static CScrollRegion s_QuadListSR;
				vec2 QuadListScrollOff(0, 0);
				CUIRect QuadListRegionRect = DetailRect;
				UiBeginScrollRegion(&s_QuadListSR, &DetailRect, &QuadListScrollOff);
				QuadListRegionRect.y += QuadListScrollOff.y;

				enum { MAX_QUAD_BUTTONS = 32 };
				static CUIButton s_Buttons[MAX_QUAD_BUTTONS];

				// quad list
				for(int QuadId = 0; QuadId < QuadCount; QuadId++)
				{
					const CQuad& Quad = SelectedLayer.m_aQuads[QuadId];
					CUIRect PreviewRect;
					QuadListRegionRect.HSplitTop(ButtonHeight, &ButtonRect, &QuadListRegionRect);
					QuadListRegionRect.HSplitTop(Spacing, 0, &QuadListRegionRect);
					ButtonRect.VSplitRight(8.0f, &ButtonRect, 0);
					ButtonRect.VSplitRight(ButtonHeight, &ButtonRect, &PreviewRect);

					CUIButton& ButBeh = s_Buttons[QuadId%MAX_QUAD_BUTTONS]; // hacky
					UiDoButtonBehavior(&ButBeh, ButtonRect, &ButBeh);

					vec4 ButtonColor = StyleColorButton;
					if(ButBeh.m_Pressed) {
						ButtonColor = StyleColorButtonPressed;
					}
					else if(ButBeh.m_Hovered) {
						ButtonColor = StyleColorButtonHover;
					}

					if(ButBeh.m_Clicked) {
						CenterViewOnQuad(Quad);
					}

					DrawRect(ButtonRect, ButtonColor);
					str_format(aBuff, sizeof(aBuff), "Quad #%d", QuadId+1);
					DrawText(ButtonRect, aBuff, FontSize);

					// preview
					CUIRect PreviewRectBorder = {PreviewRect.x-1, PreviewRect.y-1, PreviewRect.w+2, PreviewRect.h+2};
					DrawRect(PreviewRectBorder, StyleColorButtonBorder);
					DrawRect(PreviewRect, vec4(0,0,0,1));
					PreviewRect.h /= 2;
					PreviewRect.w /= 2;
					DrawRect(PreviewRect, vec4(Quad.m_aColors[0].r/255.0f, Quad.m_aColors[0].g/255.0f, Quad.m_aColors[0].b/255.0f, Quad.m_aColors[0].a/255.0f));
					PreviewRect.x += PreviewRect.w;
					DrawRect(PreviewRect, vec4(Quad.m_aColors[1].r/255.0f, Quad.m_aColors[1].g/255.0f, Quad.m_aColors[1].b/255.0f, Quad.m_aColors[1].a/255.0f));
					PreviewRect.x -= PreviewRect.w;
					PreviewRect.y += PreviewRect.h;
					DrawRect(PreviewRect, vec4(Quad.m_aColors[2].r/255.0f, Quad.m_aColors[2].g/255.0f, Quad.m_aColors[2].b/255.0f, Quad.m_aColors[2].a/255.0f));
					PreviewRect.x += PreviewRect.w;
					DrawRect(PreviewRect, vec4(Quad.m_aColors[3].r/255.0f, Quad.m_aColors[3].g/255.0f, Quad.m_aColors[3].b/255.0f, Quad.m_aColors[3].a/255.0f));

					UiScrollRegionAddRect(&s_QuadListSR, ButtonRect);
				}

				UiEndScrollRegion(&s_QuadListSR);
			}
		}
	}
}

void CEditor2::RenderPopupMenuFile(void* pPopupData)
{
	CUIRect Rect = *(CUIRect*)pPopupData;

	// render the actual menu
	static CUIButton s_NewMapButton;
	static CUIButton s_SaveButton;
	static CUIButton s_SaveAsButton;
	static CUIButton s_OpenButton;
	static CUIButton s_OpenCurrentButton;
	static CUIButton s_AppendButton;
	static CUIButton s_ExitButton;

	// whole screen "button" that prevents clicking on other stuff when exiting
	static CUIButton OverlayFakeButton;
	UiDoButtonBehavior(&OverlayFakeButton, m_UiScreenRect, &OverlayFakeButton);
	if(OverlayFakeButton.m_Clicked)
	{
		ExitPopup();
	}

	CUIRect Slot;
	// Rect.HSplitTop(2.0f, &Slot, &Rect);
	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "New", &s_NewMapButton))
	{
		ExitPopup();
		UserMapNew();
	}

	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "Load", &s_OpenButton))
	{
		ExitPopup();
		UserMapLoad();
	}

	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "Append", &s_AppendButton))
	{
	}

	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "Save", &s_SaveButton))
	{
		ExitPopup();
		UserMapSave();
	}

	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "Save As", &s_SaveAsButton))
	{
		ExitPopup();
		UserMapSaveAs();
	}

	Rect.HSplitTop(20.0f, &Slot, &Rect);
	if(UiButton(Slot, "Exit", &s_ExitButton))
	{
		Config()->m_ClEditor = 0;
	}
}

void CEditor2::RenderPopupBrushPalette(void* pPopupData)
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	DrawRect(UiScreenRect, vec4(0, 0, 0, 0.5)); // darken the background a bit

	// whole screen "button" that prevents clicking on other stuff
	static CUIButton OverlayFakeButton;
	UiDoButtonBehavior(&OverlayFakeButton, UiScreenRect, &OverlayFakeButton);

	CUIRect MainRect = {0, 0, m_UiMainViewRect.h, m_UiMainViewRect.h};
	MainRect.x += (m_UiMainViewRect.w - MainRect.w) * 0.5;
	MainRect.Margin(50.0f, &MainRect);
	m_UiPopupBrushPaletteRect = MainRect;
	//DrawRect(MainRect, StyleColorBg);

	CUIRect TopRow;
	MainRect.HSplitTop(40, &TopRow, &MainRect);

	const CEditorMap2::CLayer& SelectedTileLayer = m_Map.m_aLayers.Get(m_UiSelectedLayerID);
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

	CUIBrushPalette& Bps = m_UiBrushPaletteState;
	u8* aTileSelected = Bps.m_aTileSelected;

	// right click clears brush
	if(UI()->MouseButtonClicked(1))
	{
		BrushClear();
	}

	// do mouse dragging
	static CUIMouseDrag s_DragState;
	bool FinishedDragging = UiDoMouseDragging(m_UiPopupBrushPaletteImageRect, &s_DragState);
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

		array2<CTile> aBrushTiles;
		const int Width = EndX - StartX;
		const int Height = EndY - StartY;
		aBrushTiles.add_empty(Width * Height);

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

		SetNewBrush(aBrushTiles.base_ptr(), Width, Height);
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
	if(s_DragState.m_IsDragging)
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

	static CUIButton s_ButClear;
	if(UiButton(ButtonRect, Localize("Clear"), &s_ButClear))
	{
		BrushClear();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(100, &ButtonRect, &TopRow);

	static CUIButton s_ButEraser;
	if(UiButton(ButtonRect, Localize("Eraser"), &s_ButEraser))
	{
		BrushClear();
		CTile TileZero;
		mem_zero(&TileZero, sizeof(TileZero));
		SetNewBrush(&TileZero, 1, 1);
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(40, &ButtonRect, &TopRow);
	static CUIButton s_ButFlipX;
	if(UiButton(ButtonRect, Localize("X/X"), &s_ButFlipX))
	{
		BrushFlipX();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(40, &ButtonRect, &TopRow);
	static CUIButton s_ButFlipY;
	if(UiButton(ButtonRect, Localize("Y/Y"), &s_ButFlipY))
	{
		BrushFlipY();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(50, &ButtonRect, &TopRow);
	static CUIButton s_ButRotClockwise;
	if(UiButton(ButtonRect, "90° ⟳", &s_ButRotClockwise))
	{
		BrushRotate90Clockwise();
	}

	TopRow.VSplitLeft(2, 0, &TopRow);
	TopRow.VSplitLeft(50, &ButtonRect, &TopRow);
	static CUIButton s_ButRotCounterClockwise;
	if(UiButton(ButtonRect, "90° ⟲", &s_ButRotCounterClockwise))
	{
		BrushRotate90CounterClockwise();
	}

	// Auto map
	CUIRect RightCol = ImageRect;
	RightCol.x = ImageRect.x + ImageRect.w + 2;
	RightCol.w = 80;

	// reset selected rule when changing image
	if(SelectedTileLayer.m_ImageID != m_UiBrushPaletteState.m_ImageID)
		m_BrushAutomapRuleID = -1;
	m_UiBrushPaletteState.m_ImageID = SelectedTileLayer.m_ImageID;

	CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(SelectedTileLayer.m_ImageID);
	const float ButtonHeight = 20;
	const float Spacing = 2;

	if(pMapper)
	{
		const int RulesetCount = pMapper->RuleSetNum();
		static CUIButton s_ButtonAutoMap[16];
		 // TODO: find a better solution to this
		dbg_assert(RulesetCount <= 16, "RulesetCount is too big");

		for(int r = 0; r < RulesetCount; r++)
		{
			RightCol.HSplitTop(ButtonHeight, &ButtonRect, &RightCol);
			RightCol.HSplitTop(Spacing, 0, &RightCol);

			bool Selected = m_BrushAutomapRuleID == r;
			if(UiButtonSelect(ButtonRect, pMapper->GetRuleSetName(r), &s_ButtonAutoMap[r], Selected, 10))
			{
				m_BrushAutomapRuleID = r;
			}
		}
	}

	RenderBrush(m_UiMousePos);

	if((!IsToolBrush() || !Input()->KeyIsPressed(KEY_SPACE)) && m_UiBrushPaletteState.m_PopupEnabled)
	{
		m_UiBrushPaletteState.m_PopupEnabled = false;
		ExitPopup();
	}
}

int CEditor2::CUIFileSelect::EditorListdirCallback(const CFsFileInfo* info, int IsDir, int StorageType, void *pUser)
{
	CUIFileSelect *pState = (CUIFileSelect *)pUser;

	if(!str_comp(info->m_pName, ".") || !str_comp(info->m_pName, ".."))
		return 0;

	CEditor2::CFileListItem Item;
	str_copy(Item.m_aFilename, info->m_pName, sizeof(Item.m_aFilename));
	if(IsDir)
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", info->m_pName);
	else
		str_copy(Item.m_aName, info->m_pName, sizeof(Item.m_aName));

	Item.m_IsDir = IsDir != 0;
	Item.m_IsLink = false;
	Item.m_StorageType = StorageType;
	Item.m_Created = info->m_TimeCreated;
	Item.m_Modified = info->m_TimeModified;
	pState->m_aFileList.add(Item);

	return 0;
}

void CEditor2::CUIFileSelect::PopulateFileList(IStorage *pStorage, int StorageType)
{
	m_aFileList.clear();
	pStorage->ListDirectoryFileInfo(StorageType, m_aPath, EditorListdirCallback, this);
	GenerateListBoxEntries();

	pStorage->GetCompletePath(IStorage::TYPE_SAVE, m_aPath, m_aCompletePath, sizeof(m_aCompletePath));
}

void CEditor2::CUIFileSelect::GenerateListBoxEntries()
{
	// An array<const char *> might help get allocations down
	if(m_pListBoxEntries)
	{
		free(m_pListBoxEntries);
	}

	m_pListBoxEntries = (CUIListBox::Entry *)mem_alloc(m_aFileList.size() * sizeof(*m_pListBoxEntries), 0);
	for(int i = 0; i < m_aFileList.size(); i++)
	{
		m_pListBoxEntries[i].m_Id = i;
		m_pListBoxEntries[i].m_paData[0] = m_aFileList[i].m_aName;
		m_pListBoxEntries[i].m_paData[1] = &m_aFileList[i].m_Modified;
		m_pListBoxEntries[i].m_paData[2] = &m_aFileList[i].m_Created;
	}
}

bool CEditor2::DoFileSelect(CUIRect MainRect, CUIFileSelect *pState)
{
	const float Padding = 20.0f;
	const float FontSize = 7.0f;
	const vec4 White(1, 1, 1, 1);

	MainRect.Margin(Padding * 3/4, &MainRect);

	CUIRect Top, Search;
	MainRect.HSplitTop(20.0f, &Top, &MainRect);
	Top.VSplitLeft(Top.w * 4/5 - (Padding/2), &Top, &Search);
	Search.VSplitLeft(Padding/2, 0, &Search);

	// Debounce this a little for better performance?
	static CUITextInput s_SearchBox;
	UiTextInput(Search, pState->m_aFilter, sizeof(pState->m_aFilter), &s_SearchBox);

	CUIRect BNewFolder, BRefresh, BUp, BPrev, BNext, Path;
	Top.VSplitLeft(Top.h, &BNewFolder, &Top);
	Top.VSplitLeft(Padding / 8, 0, &Top);
	Top.VSplitLeft(Top.h, &BRefresh, &Top);
	Top.VSplitLeft(Padding / 8, 0, &Top);
	Top.VSplitLeft(Top.h, &BUp, &Top);
	Top.VSplitLeft(Padding / 8, 0, &Top);
	Top.VSplitLeft(Top.h, &BPrev, &Top);
	Top.VSplitLeft(Padding / 8, 0, &Top);
	Top.VSplitLeft(Top.h, &BNext, &Top);
	Top.VSplitLeft(Padding / 8, 0, &Path);

	static CUIButton s_BNewFolder;
	if(UiButton(BNewFolder, "+", &s_BNewFolder, 15.0f))
	{
		pState->PopulateFileList(Storage(), IStorage::TYPE_SAVE);
	}

	static CUIButton s_BRefresh;
	if(UiButton(BRefresh, "\xE2\x86\xBB", &s_BRefresh, 15.0f))
	{
		pState->PopulateFileList(Storage(), IStorage::TYPE_SAVE);
	}

	static CUIButton s_BPrev;
	if(UiButton(BPrev, "\xE2\x86\x90", &s_BPrev, 15.0f))
	{
		// Need to implement some history
		ed_dbg("button down");
	}

	static CUIButton s_BNext;
	if(UiButton(BNext, "\xE2\x86\x92", &s_BNext, 15.0f))
	{
		// Need to implement some history
		ed_dbg("button down");
	}

	DrawRectBorder(Path, StyleColorButton, 1, StyleColorButtonBorder);

	MainRect.HSplitTop(Padding * 1/2, 0, &MainRect);

	CUIRect Bookmarks, Browser, Preview;
	MainRect.VSplitLeft(MainRect.w * 1/5 - (Padding/2), &Bookmarks, &MainRect);
	MainRect.VSplitLeft(Padding/2, 0, &MainRect);
	MainRect.VSplitLeft(MainRect.w * 3/4 - (Padding/2), &Browser, &Preview);
	Preview.VSplitLeft(Padding/2, 0, &Preview);

	Preview.h = Preview.w;

	{
		CUIRect Label;
		Bookmarks.HSplitTop(Padding / 2, &Label, &Bookmarks);
		Label.Margin(1.0f, &Label);
		DrawText(Label, "Favorites:", Label.h);
	}

	Browser.HSplitTop(Padding / 2, 0, &Browser);

	{
		CUIRect Label;
		Preview.HSplitTop(Padding / 2, &Label, &Preview);
		Label.Margin(1.0f, &Label);
		DrawText(Label, "Preview:", Label.h);
	}

	Bookmarks.HSplitBottom(20.0f + Padding, &Bookmarks, 0);
	DrawRect(Bookmarks, vec4(0.4f, 0.4f, 0.5f, 1.0f));

	DrawRect(Preview, vec4(0.4f, 0.5f, 0.5f, 1.0f));

	CUIRect Bottom;
	Browser.HSplitBottom(20.0f, &Browser, &Bottom);
	Browser.HSplitBottom(Padding, &Browser, 0);

	static const CUIListBox::ColData s_aColumns[] = {{CUIListBox::COLTYPE_TEXT, 4, "Name"},
		{CUIListBox::COLTYPE_DATE, 2, "Modified"},
		{CUIListBox::COLTYPE_DATE, 2, "Created"}};

	bool Open = false;

	static CUIListBox s_Browser;
	if(UiListBox(Browser, s_aColumns, 3, pState->m_pListBoxEntries,
			pState->m_aFileList.size(), pState->m_aFilter, 0, &s_Browser) ||
		Input()->KeyPress(KEY_RETURN))
	{
		Open = true;
		pState->m_Selected = s_Browser.m_Selected;
	}

	static CUIButton s_BUp;
	if(UiButton(BUp, "\xE2\x86\x91", &s_BUp, 15.0f))
	{
		if(fs_parent_dir(pState->m_aPath))
			pState->m_aPath[0] = '\0';

		s_Browser.m_Selected = -1;
		pState->PopulateFileList(Storage(), IStorage::TYPE_SAVE);
	}

	DrawText(Path, pState->m_aCompletePath, FontSize);

	const float ButtonPadding = (Bottom.h - FontSize - 3.0f) * 0.5f;

	const char *CancelText = Localize("Cancel");
	const float CancelTextW = TextRender()->TextWidth(0, FontSize, CancelText, -1, -1);
	const float ButtonW = min(Bottom.w / 7, CancelTextW + ButtonPadding) + ButtonPadding;
	const vec4 ButtonTextColor(1.0f, 1.0f, 1.0f, 1.0f);

	{
		CUIRect BCancel;
		Bottom.VSplitRight(ButtonW, &Bottom, &BCancel);
		Bottom.VSplitRight(Padding / 2, &Bottom, 0);

		static CUIButton s_BCancel;
		if(UiButton(BCancel, CancelText, &s_BCancel, FontSize))
		{
			pState->m_aOutputPath[0] = '\0'; // TODO: return that we hit cancel?
			return true;
		}
	}

	static char s_aNewFileName[128];
	{
		CUIRect SelectedFile, BOpen;
		Bottom.VSplitRight(ButtonW, &SelectedFile, &BOpen);

		static CUIButton s_BOpen;
		const char *pButtonText = pState->m_pButtonText;
		if(pState->m_NewFile && pState->m_Selected >= 0 && pState->m_aFileList[pState->m_Selected].m_IsDir)
		{
			pButtonText = "Open";
		}
		if(UiButton(BOpen, Localize(pButtonText), &s_BOpen, FontSize))
		{
			Open = true;
			pState->m_Selected = s_Browser.m_Selected;
		}

		if(SelectedFile.w - ButtonW / 2 > 3 * ButtonW)
			SelectedFile.VSplitLeft(ButtonW / 2, 0, &SelectedFile);

		if(!pState->m_NewFile)
		{
			DrawRectBorder(SelectedFile, StyleColorButton, 1, StyleColorButtonBorder);
			if(s_Browser.m_Selected >= 0)
				DrawText(SelectedFile, pState->m_aFileList[s_Browser.m_Selected].m_aFilename, FontSize);
		}
		else
		{
			static CUITextInput s_NewFileName;
			if(s_Browser.m_Selected >= 0)
				str_copy(s_aNewFileName,  pState->m_aFileList[s_Browser.m_Selected].m_aFilename, sizeof(s_aNewFileName));

			if(UiTextInput(SelectedFile, s_aNewFileName, sizeof(s_aNewFileName), &s_NewFileName))
			{
				s_Browser.m_Selected = -1;
				for(int i = 0; i < pState->m_aFileList.size(); i++)
				{
					if(!str_comp(pState->m_aFileList[i].m_aFilename, s_aNewFileName))
					{
						s_Browser.m_Selected = i;
						break;
					}
				}
			}
		}
	}

	if(pState->m_Selected >= 0)
	{
		int Selected = pState->m_Selected;
		if(pState->m_aFileList[Selected].m_IsDir)
		{
			if(!str_comp(pState->m_aFileList[Selected].m_aFilename, ".."))
			{
				if(fs_parent_dir(pState->m_aPath))
					pState->m_aPath[0] = '\0';
			}
			else
			{
				if(pState->m_aPath[0])
					str_append(pState->m_aPath, "/", sizeof(pState->m_aPath));

				str_append(pState->m_aPath, pState->m_aFileList[Selected].m_aFilename,
					sizeof(pState->m_aPath));
			}

			pState->PopulateFileList(Storage(), IStorage::TYPE_SAVE);
			s_Browser.m_Selected = pState->m_Selected = -1;
		}
		else
		{
			str_format(pState->m_aOutputPath, sizeof(pState->m_aOutputPath), "%s/%s", pState->m_aPath, pState->m_aFileList[Selected].m_aFilename);
			return true;
		}
	}
	else if(Open && pState->m_NewFile && s_aNewFileName[0])
	{
		str_format(pState->m_aOutputPath, sizeof(pState->m_aOutputPath), "%s/%s", pState->m_aPath, s_aNewFileName);
		return true;
	}

	return false;
}

void CEditor2::RenderPopupMapLoad(void* pPopupData)
{
	CUIPopupMapLoad& Popup = *(CUIPopupMapLoad*)pPopupData;

	if(Popup.m_DoSaveMapBefore)
	{
		if(DoPopupYesNo(&Popup.m_PopupWarningSaveMap))
		{
			Popup.m_DoSaveMapBefore = false;
			if(Popup.m_PopupWarningSaveMap.m_Choice)
				UserMapSave();
		}

		return; // TODO: do not return here, add another CUIFileSelect state (one for loading one for saving)
	}

	if(Input()->KeyPress(KEY_ESCAPE))
	{
		ExitPopup();
		return;
	}

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	// whole screen "button" that prevents clicking on other stuff
	DrawRect(UiScreenRect, vec4(0.0, 0, 0, 0.5));
	static CUIButton OverlayFakeButton;
	UiDoButtonBehavior(&OverlayFakeButton, UiScreenRect, &OverlayFakeButton);

	const float Scale = 5.0f/6.0f;
	CUIRect MainRect = {UiScreenRect.w * (1 - Scale) * 0.5f, UiScreenRect.h * (1 - Scale) * 0.5f, UiScreenRect.w * Scale, UiScreenRect.h * Scale};

	DrawRectBorderOutside(MainRect, StyleColorBg, 2, vec4(0.145f, 0.0f, 0.4f, 1.0f));

	CUIRect TitleRect;
	MainRect.HSplitTop(20.0f, &TitleRect, &MainRect);
	DrawText(TitleRect, Localize("Load"), 10);

	if(DoFileSelect(MainRect, &m_UiFileSelectState))
	{
		if(m_UiFileSelectState.m_aOutputPath[0])
			LoadMap(m_UiFileSelectState.m_aOutputPath);

		ExitPopup();
	}
}

void CEditor2::RenderPopupMapSaveAs(void* pPopupData)
{
	if(Input()->KeyPress(KEY_ESCAPE))
	{
		ExitPopup();
		return;
	}

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	// whole screen "button" that prevents clicking on other stuff
	DrawRect(UiScreenRect, vec4(0.0, 0, 0, 0.5));
	static CUIButton OverlayFakeButton;
	UiDoButtonBehavior(&OverlayFakeButton, UiScreenRect, &OverlayFakeButton);

	const float Scale = 5.0f/6.0f;
	CUIRect MainRect = {UiScreenRect.w * (1 - Scale) * 0.5f, UiScreenRect.h * (1 - Scale) * 0.5f, UiScreenRect.w * Scale, UiScreenRect.h * Scale};

	DrawRectBorderOutside(MainRect, StyleColorBg, 2, vec4(0.145f, 0.0f, 0.4f, 1.0f));

	CUIRect TitleRect;
	MainRect.HSplitTop(20.0f, &TitleRect, &MainRect);
	DrawText(TitleRect, Localize("Save as"), 10);

	if(DoFileSelect(MainRect, &m_UiFileSelectState))
	{
		const char* pOutput = m_UiFileSelectState.m_aOutputPath;
		char aRevisedOutput[512];
		if(!str_endswith(pOutput, ".map"))
		{
			str_format(aRevisedOutput, sizeof(aRevisedOutput), "%s.map", pOutput);
			pOutput = aRevisedOutput;
		}

		// TODO: "Map saved" message or indicator of any kind
		SaveMap(pOutput);
		ExitPopup();
	}
}

void CEditor2::RenderPopupYesNo(void* pPopupData)
{
	const float Padding = 5.0f;
	const float Scale = 0.4f;
	const vec4 White(1, 1, 1, 1);

	CUIPopupYesNo& Popup = *(CUIPopupYesNo*)pPopupData;
	if(Input()->KeyPress(KEY_ESCAPE))
	{
		Popup.m_Choice = false;
		Popup.m_Done = true;
		ExitPopup();
	}

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	DrawRect(UiScreenRect, vec4(0.0, 0, 0, 0.5)); // darken the background (should be in a common function)
	// whole screen "button" that prevents clicking on other stuff
	static CUIButton OverlayFakeButton;
	UiDoButtonBehavior(&OverlayFakeButton, UiScreenRect, &OverlayFakeButton);

	float w = UiScreenRect.w * Scale, h = UiScreenRect.h * Scale * 0.5f;
	CUIRect MainRect = {(UiScreenRect.w - w) * 0.5f, (UiScreenRect.h - h) * 0.5f, w, h};

	DrawRectBorderOutside(MainRect, StyleColorBg, 2, vec4(0.145f, 0.0f, 0.4f, 1.0f));
	MainRect.Margin(Padding * 3/4, &MainRect);

	CUIRect Title, Text, Buttons;
	MainRect.HSplitTop(MainRect.h / 4, &Title, &Text);
	Text.HSplitTop(MainRect.h / 4, &Text, &Buttons);
	Buttons.Margin(Buttons.h * 0.5f - 10, &Buttons);

	DrawText(Title, Popup.m_pTitle, 15.0f, White, CUI::ALIGN_CENTER);
	DrawText(Text, Popup.m_pQuestionText, 10.0f, White, CUI::ALIGN_CENTER);

	CUIRect BYes, BNo;
	Buttons.VSplitMid(&BNo, &BYes, 10.0f);

	static CUIButton s_BNo, s_BYes;
	if(UiButton(BNo, Localize("No"), &s_BNo, 10.0f, CUI::ALIGN_CENTER))
	{
		Popup.m_Choice = false;
		Popup.m_Done = true;
		ExitPopup();
	}
	else if(UiButton(BYes, Localize("Yes"), &s_BYes, 10.0f, CUI::ALIGN_CENTER))
	{
		Popup.m_Choice = true;
		Popup.m_Done = true;
		ExitPopup();
	}
}

void CEditor2::RenderPopupMapNew(void* pPopupData)
{
	CUIPopupMapNew& Popup = *(CUIPopupMapNew*)pPopupData;

	if(Popup.m_DoSaveMapBefore)
	{
		if(DoPopupYesNo(&Popup.m_PopupWarningSaveMap))
		{
			Popup.m_DoSaveMapBefore = false;
			if(Popup.m_PopupWarningSaveMap.m_Choice)
				UserMapSave();
		}

		return;
	}

	if(Input()->KeyPress(KEY_ESCAPE))
	{
		ExitPopup();
		return;
	}

	// TODO: New map popup (template based)
	if(true)
	{
		Reset();
		ExitPopup();
	}
}

bool CEditor2::DoPopupYesNo(CUIPopupYesNo* state)
{
	if(!state->m_Active)
	{
		PushPopup(&CEditor2::RenderPopupYesNo, state);
		state->m_Active = true;
	}

	if(state->m_Done)
	{
		state->m_Done = false;
		return true;
	}
	return false;
}

void CEditor2::RenderBrush(vec2 Pos)
{
	if(m_Brush.IsEmpty())
		return;

	float ScreenX0, ScreenX1, ScreenY0, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->MapScreen(ScreenX0 - Pos.x, ScreenY0 - Pos.y,
		ScreenX1 - Pos.x, ScreenY1 - Pos.y);

	const CEditorMap2::CLayer& SelectedTileLayer = m_Map.m_aLayers.Get(m_UiSelectedLayerID);
	dbg_assert(SelectedTileLayer.IsTileLayer(), "Selected layer is not a tile layer");

	const float TileSize = 32;
	const CUIRect BrushRect = {0, 0, m_Brush.m_Width * TileSize, m_Brush.m_Height * TileSize};
	DrawRect(BrushRect, vec4(1, 1, 1, 0.1f));

	IGraphics::CTextureHandle LayerTexture;
	if(m_UiSelectedLayerID == m_Map.m_GameLayerID)
		LayerTexture = m_EntitiesTexture;
	else
		LayerTexture = m_Map.m_Assets.m_aTextureHandle[SelectedTileLayer.m_ImageID];

	const vec4 White(1, 1, 1, 1);

	Graphics()->TextureSet(LayerTexture);
	Graphics()->BlendNone();
	RenderTools()->RenderTilemap(m_Brush.m_aTiles.base_ptr(), m_Brush.m_Width, m_Brush.m_Height, TileSize, White, LAYERRENDERFLAG_OPAQUE, 0, 0, -1, 0);
	Graphics()->BlendNormal();
	RenderTools()->RenderTilemap(m_Brush.m_aTiles.base_ptr(), m_Brush.m_Width, m_Brush.m_Height, TileSize, White, LAYERRENDERFLAG_TRANSPARENT, 0, 0, -1, 0);

	DrawRectBorder(BrushRect, vec4(0, 0, 0, 0), 1, vec4(1, 1, 1, 1));

	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

struct CImageNameItem
{
	int m_Index;
	CEditorMap2::CImageName m_Name;
};

static int CompareImageNameItems(const void* pA, const void* pB)
{
	const CImageNameItem& a = *(CImageNameItem*)pA;
	const CImageNameItem& b = *(CImageNameItem*)pB;
	return str_comp_nocase(a.m_Name.m_Buff, b.m_Name.m_Buff);
}

void CEditor2::RenderAssetManager()
{
	CEditorMap2::CAssets& Assets = m_Map.m_Assets;

	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect TopPanel, RightPanel, MainViewRect;
	UiScreenRect.VSplitRight(150.0f, &MainViewRect, &RightPanel);
	MainViewRect.HSplitTop(MenuBarHeight, &TopPanel, &MainViewRect);

	DrawRect(RightPanel, StyleColorBg);

	CUIRect NavRect, ButtonRect, DetailRect;
	RightPanel.Margin(3.0f, &NavRect);

	NavRect.HSplitBottom(200, &NavRect, &DetailRect);
	UI()->ClipEnable(&NavRect);

	static CUIButton s_UiImageButState[CEditorMap2::MAX_IMAGES] = {};
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;

	static CImageNameItem s_aImageItems[CEditorMap2::MAX_IMAGES];
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
			StyleColorButton,StyleColorButtonHover, StyleColorButtonPressed, ColBorder, FontSize,
			CUI::ALIGN_LEFT))
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

		static CUIButton s_ButDelete;
		if(UiButton(ButtonRect, Localize("Delete"), &s_ButDelete))
		{
			EditDeleteImage(m_UiSelectedImageID);
		}

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUITextInput s_TextInputAdd;
		static char aAddPath[256] = "grass_main.png";
		UiTextInput(ButtonRect, aAddPath, sizeof(aAddPath), &s_TextInputAdd);

		DetailRect.HSplitTop(ButtonHeight, &ButtonRect, &DetailRect);
		DetailRect.HSplitTop(Spacing, 0, &DetailRect);

		static CUIButton s_ButAdd;
		if(UiButton(ButtonRect, Localize("Add image"), &s_ButAdd))
		{
			EditAddImage(aAddPath);
		}
	}

	if(m_UiSelectedImageID >= Assets.m_ImageCount)
		m_UiSelectedImageID = -1;

	RenderMenuBar(TopPanel);
}

void CEditor2::Reset()
{
	m_Map.LoadDefault();
	OnMapLoaded();
}

void CEditor2::ResetCamera()
{
	m_Zoom = 1;
	m_MapUiPosOffset = vec2(0,-MenuBarHeight);
}

void CEditor2::ChangeZoom(float Zoom)
{
	Zoom = clamp(Zoom, 0.2f, 5.f);

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

void CEditor2::ChangePage(int Page)
{
	UI()->SetHotItem(0);
	UI()->SetActiveItem(0);
	m_Page = Page;
}

void CEditor2::ChangeTool(int Tool)
{
	if(m_Tool == Tool) return;

	if(m_Tool == TOOL_TILE_BRUSH)
		BrushClear();

	m_Tool = Tool;
}

void CEditor2::SelectLayerBelowCurrentOne()
{
	dbg_assert(m_Map.m_aLayers.IsValid(m_UiSelectedLayerID), "m_UiSelectedLayerID is invalid");
	dbg_assert(m_Map.m_aGroups.IsValid(m_UiSelectedGroupID), "m_UiSelectedGroupID is invalid");

	const CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(m_UiSelectedGroupID);
	int LayerPos = -1;
	for(int i = 0; i < Group.m_LayerCount; i++)
	{
		if(Group.m_apLayerIDs[i] == m_UiSelectedLayerID)
		{
			LayerPos = i;
			break;
		}
	}

	dbg_assert(LayerPos != -1, "m_UiSelectedLayerID not found in selected group");

	if(Group.m_LayerCount > 1) {
		m_UiSelectedLayerID = Group.m_apLayerIDs[(LayerPos+1) % Group.m_LayerCount];
		dbg_assert(m_Map.m_aLayers.IsValid(m_UiSelectedLayerID), "m_UiSelectedLayerID is invalid");
	}
	else {
		m_UiSelectedLayerID = -1;
	}

	// slightly overkill
	dbg_assert(m_Map.m_aGroups.IsValid(m_UiSelectedGroupID), "m_UiSelectedGroupID is invalid");
}

void CEditor2::SelectGroupBelowCurrentOne()
{
	dbg_assert(m_Map.m_aGroups.IsValid(m_UiSelectedGroupID), "m_UiSelectedGroupID is invalid");

	int GroupPos = -1;
	const int Count = m_Map.m_GroupIDListCount;
	for(int i = 0; i < Count; i++)
	{
		if(m_Map.m_aGroupIDList[i] == (u32)m_UiSelectedGroupID)
		{
			GroupPos = i;
			break;
		}
	}

	dbg_assert(GroupPos != -1, "m_UiSelectedGroupID not found in m_Map.m_aGroupIDList");

	m_UiSelectedGroupID = m_Map.m_aGroupIDList[(GroupPos+1) % Count];
	const CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(m_UiSelectedGroupID);
	if(Group.m_LayerCount > 0) {
		m_UiSelectedLayerID = Group.m_apLayerIDs[0];
		dbg_assert(m_Map.m_aLayers.IsValid(m_UiSelectedLayerID), "m_UiSelectedLayerID is invalid");
	}
	else {
		m_UiSelectedLayerID = -1;
	}

	// slightly overkill
	dbg_assert(m_Map.m_aGroups.IsValid(m_UiSelectedGroupID), "m_UiSelectedGroupID is invalid");
}

// returns the GroupIdOffset, also clamps pRelativePos
int CEditor2::GroupCalcDragMoveOffset(int ParentGroupListIndex, int* pRelativePos)
{
	if(*pRelativePos == 0)
		return 0;

	const int RelativeDir = sign(*pRelativePos);
	int CurrentPos = 0, CurrentGroupIndex = ParentGroupListIndex, GroupIdOffset;

	// skip current group if we are moving down
	if(RelativeDir == +1 && m_UiGroupState[m_Map.m_aGroupIDList[ParentGroupListIndex]].m_IsOpen)
		CurrentPos += m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex]).m_LayerCount;

	for(GroupIdOffset = 0; ; GroupIdOffset += RelativeDir)
	{
		// check if we are at the group list boundaries
		if((RelativeDir == -1 && CurrentGroupIndex <= 0) || (RelativeDir == +1 && CurrentGroupIndex >= m_Map.m_GroupIDListCount-1))
			break;

		const int LastPos = CurrentPos;
		const int RelevantGroupIndex = (RelativeDir == -1) ? CurrentGroupIndex-1 : CurrentGroupIndex+1;
		const bool RelevantGroupIsOpen = m_UiGroupState[m_Map.m_aGroupIDList[RelevantGroupIndex]].m_IsOpen;
		const int RelevantGroupLayerCount = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[RelevantGroupIndex]).m_LayerCount;

		if(RelevantGroupIsOpen)
			CurrentPos += RelevantGroupLayerCount*RelativeDir;
		CurrentPos += RelativeDir; // add the group header too
		CurrentGroupIndex += RelativeDir;

		if((RelativeDir == -1 && CurrentPos < *pRelativePos) || (RelativeDir == +1 && CurrentPos > *pRelativePos))
		{
			// we went over the asked position, go back to the last valid position
			CurrentPos = LastPos;
			break;
		}
	}
	*pRelativePos = GroupIdOffset ? CurrentPos : 0; // make sure pRelativePos stays at 0 if we are not moving (necessary when moving down)
	return GroupIdOffset;
}

int CEditor2::LayerCalcDragMoveOffset(int ParentGroupListIndex, int LayerListIndex, int RelativePos)
{
	if(RelativePos == 0)
		return 0;
	else if(RelativePos < 0)
	{
		for(int Pos = -1; Pos >= RelativePos; Pos--)
		{
			const bool CurrentGroupIsOpen = m_UiGroupState[m_Map.m_aGroupIDList[ParentGroupListIndex]].m_IsOpen;
			LayerListIndex--;
			if(LayerListIndex < 0 || !CurrentGroupIsOpen)
			{
				ParentGroupListIndex--;
				if(ParentGroupListIndex < 0)
					return Pos+1;
				LayerListIndex = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex]).m_LayerCount;
			}
		}
	}
	else
	{
		for(int Pos = 1; Pos <= RelativePos; Pos++)
		{
			CEditorMap2::CGroup& ParentGroup = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex]);
			const bool CurrentGroupIsOpen = m_UiGroupState[m_Map.m_aGroupIDList[ParentGroupListIndex]].m_IsOpen;
			LayerListIndex++;
			if(LayerListIndex >= ParentGroup.m_LayerCount || !CurrentGroupIsOpen)
			{
				ParentGroupListIndex++;
				if(ParentGroupListIndex >= m_Map.m_GroupIDListCount)
					return Pos-1;
				LayerListIndex = -1;
			}
		}
	}
	return RelativePos;
}

void CEditor2::SetNewBrush(CTile* aTiles, int Width, int Height)
{
	dbg_assert(Width > 0 && Height > 0, "Brush: wrong dimensions");
	m_Brush.m_Width = Width;
	m_Brush.m_Height = Height;
	m_Brush.m_aTiles.clear();
	m_Brush.m_aTiles.add(aTiles, Width*Height);
}

void CEditor2::BrushClear()
{
	m_Brush.m_Width = 0;
	m_Brush.m_Height = 0;
	m_Brush.m_aTiles.clear();
	mem_zero(m_UiBrushPaletteState.m_aTileSelected, sizeof(m_UiBrushPaletteState.m_aTileSelected));
}

void CEditor2::BrushFlipX()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	array2<CTile>& aTiles = m_Brush.m_aTiles;
	array2<CTile> aTilesCopy;
	aTilesCopy.add(aTiles.base_ptr(), aTiles.size());

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

void CEditor2::BrushFlipY()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	array2<CTile>& aTiles = m_Brush.m_aTiles;
	array2<CTile> aTilesCopy;
	aTilesCopy.add(aTiles.base_ptr(), aTiles.size());

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

void CEditor2::BrushRotate90Clockwise()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	array2<CTile>& aTiles = m_Brush.m_aTiles;
	array2<CTile> aTilesCopy;
	aTilesCopy.add(aTiles.base_ptr(), aTiles.size());

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

void CEditor2::BrushRotate90CounterClockwise()
{
	if(m_Brush.m_Width <= 0)
		return;

	const int BrushWidth = m_Brush.m_Width;
	const int BrushHeight = m_Brush.m_Height;
	array2<CTile>& aTiles = m_Brush.m_aTiles;
	array2<CTile> aTilesCopy;
	aTilesCopy.add(aTiles.base_ptr(), aTiles.size());

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

void CEditor2::BrushPaintLayer(int PaintTX, int PaintTY, int LayerID)
{
	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	const int BrushW = m_Brush.m_Width;
	const int BrushH = m_Brush.m_Height;
	const int LayerW = Layer.m_Width;
	const int LayerH = Layer.m_Height;
	array2<CTile>& aLayerTiles = Layer.m_aTiles;
	array2<CTile>& aBrushTiles = m_Brush.m_aTiles;

	for(int ty = 0; ty < BrushH; ty++)
	{
		for(int tx = 0; tx < BrushW; tx++)
		{
			int BrushTid = ty * BrushW + tx;
			int LayerTx = tx + PaintTX;
			int LayerTy = ty + PaintTY;

			if(LayerTx < 0 || LayerTx > LayerW-1 || LayerTy < 0 || LayerTy > LayerH-1)
				continue;

			int LayerTid = LayerTy * LayerW + LayerTx;
			aLayerTiles[LayerTid] = aBrushTiles[BrushTid];
		}
	}
}

void CEditor2::BrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID)
{
	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	const int BrushW = m_Brush.m_Width;
	const int BrushH = m_Brush.m_Height;
	const int LayerW = Layer.m_Width;
	const int LayerH = Layer.m_Height;
	array2<CTile>& aLayerTiles = Layer.m_aTiles;
	array2<CTile>& aBrushTiles = m_Brush.m_aTiles;

	for(int ty = 0; ty < PaintH; ty++)
	{
		for(int tx = 0; tx < PaintW; tx++)
		{
			int BrushTid = (ty % BrushH) * BrushW + (tx % BrushW);
			int LayerTx = tx + PaintTX;
			int LayerTy = ty + PaintTY;

			if(LayerTx < 0 || LayerTx > LayerW-1 || LayerTy < 0 || LayerTy > LayerH-1)
				continue;

			int LayerTid = LayerTy * LayerW + LayerTx;
			aLayerTiles[LayerTid] = aBrushTiles[BrushTid];
		}
	}
}

void CEditor2::TileLayerRegionToBrush(int LayerID, int StartTX, int StartTY, int EndTX, int EndTY)
{
	const CEditorMap2::CLayer& TileLayer = m_Map.m_aLayers.Get(LayerID);
	dbg_assert(TileLayer.IsTileLayer(), "Layer is not a tile layer");

	const int SelStartX = clamp(min(StartTX, EndTX), 0, TileLayer.m_Width-1);
	const int SelStartY = clamp(min(StartTY, EndTY), 0, TileLayer.m_Height-1);
	const int SelEndX = clamp(max(StartTX, EndTX), 0, TileLayer.m_Width-1) + 1;
	const int SelEndY = clamp(max(StartTY, EndTY), 0, TileLayer.m_Height-1) + 1;
	const int Width = SelEndX - SelStartX;
	const int Height = SelEndY - SelStartY;

	array2<CTile> aExtractTiles;
	aExtractTiles.add_empty(Width * Height);

	const int LayerWidth = TileLayer.m_Width;
	const array2<CTile>& aLayerTiles = TileLayer.m_aTiles;
	const int StartTid = SelStartY * LayerWidth + SelStartX;
	const int LastTid = (SelEndY-1) * LayerWidth + SelEndX;

	for(int ti = StartTid; ti < LastTid; ti++)
	{
		const int tx = (ti % LayerWidth) - SelStartX;
		const int ty = (ti / LayerWidth) - SelStartY;
		if(tx >= 0 && tx < Width && ty >= 0 && ty < Height)
			aExtractTiles[ty * Width + tx] = aLayerTiles[ti];
	}

	SetNewBrush(aExtractTiles.base_ptr(), Width, Height);
}

void CEditor2::CenterViewOnQuad(const CQuad &Quad)
{
	// FIXME: does not work at all
	dbg_assert(m_UiSelectedGroupID >= 0, "No group selected");

	CUIRect Rect;
	Rect.x = fx2f(Quad.m_aPoints[0].x);
	Rect.y = fx2f(Quad.m_aPoints[0].y);
	Rect.w = 10;
	Rect.h = 10;

	CUIRect UiRect = CalcUiRectFromGroupWorldRect(m_UiSelectedGroupID, m_ZoomWorldViewWidth, m_ZoomWorldViewHeight, Rect);
	vec2 UiOffset = vec2(UiRect.x, UiRect.y);

	ed_log("ScreenOffset = { %g, %g }", UiOffset.x, UiOffset.y);
	m_MapUiPosOffset = UiOffset - vec2(m_UiScreenRect.w/2, m_UiScreenRect.h/2);
}

bool CEditor2::LoadMap(const char* pFileName)
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

bool CEditor2::SaveMap(const char* pFileName)
{
	if(m_Map.Save(pFileName))
	{
		m_MapSaved = true;
		// send rcon.. if we can
		// TODO: implement / uncomment
		if(Client()->RconAuthed())
		{/*
			CServerInfo CurrentServerInfo;
			Client()->GetServerInfo(&CurrentServerInfo);
			char aMapName[128];
			m_pEditor->ExtractName(pFileName, aMapName, sizeof(aMapName));
			if(!str_comp(aMapName, CurrentServerInfo.m_aMap))
				m_pEditor->Client()->Rcon("reload");
		*/}

		ed_log("map '%s' sucessfully saved.", pFileName);
		return true;
	}
	ed_log("failed to save map '%s'", pFileName);
	return false;
}

void CEditor2::OnMapLoaded()
{
	m_MapSaved = true;
	m_UiSelectedLayerID = m_Map.m_GameLayerID;
	m_UiSelectedGroupID = m_Map.m_GameGroupID;
	mem_zero(m_UiGroupState.data, sizeof(m_UiGroupState.data));
	mem_zero(m_UiLayerState.data, sizeof(m_UiLayerState.data));
	m_UiGroupState[m_Map.m_GameGroupID].m_IsOpen = true;
	mem_zero(&m_UiBrushPaletteState, sizeof(m_UiBrushPaletteState));
	ResetCamera();
	BrushClear();

	// clear history
	if(m_pHistoryEntryCurrent)
	{
		CHistoryEntry* pCurrent = m_pHistoryEntryCurrent->m_pNext;
		while(pCurrent)
		{
			HistoryDeleteEntry(pCurrent);
			pCurrent = pCurrent->m_pNext;
		}

		pCurrent = m_pHistoryEntryCurrent;
		while(pCurrent)
		{
			HistoryDeleteEntry(pCurrent);
			pCurrent = pCurrent->m_pPrev;
		}
		HistoryClear();
		m_pHistoryEntryCurrent = 0x0;
	}

	// initialize history
	m_pHistoryEntryCurrent = HistoryAllocEntry();
	m_pHistoryEntryCurrent->m_pSnap = m_Map.SaveSnapshot();
	m_pHistoryEntryCurrent->m_pUiSnap = SaveUiSnapshot();
	m_pHistoryEntryCurrent->SetAction("Map loaded");
	m_pHistoryEntryCurrent->SetDescription(m_Map.m_aPath);
}

void CEditor2::UserMapNew()
{
	m_UiPopupMapNew.Reset();
	m_UiPopupMapNew.m_DoSaveMapBefore = HasUnsavedData();
	PushPopup(&CEditor2::RenderPopupMapNew, &m_UiPopupMapNew);
}

void CEditor2::UserMapLoad()
{
	m_UiPopupMapLoad.Reset();
	m_UiPopupMapLoad.m_DoSaveMapBefore = HasUnsavedData();

	m_UiFileSelectState.m_pButtonText = Localize("Open");
	m_UiFileSelectState.m_NewFile = false;
	m_UiFileSelectState.m_Selected = -1;
	str_copy(m_UiFileSelectState.m_aPath, "maps", sizeof(m_UiFileSelectState.m_aPath));
	m_UiFileSelectState.m_pListBoxEntries = 0;

	m_UiFileSelectState.PopulateFileList(Storage(), IStorage::TYPE_SAVE);

	PushPopup(&CEditor2::RenderPopupMapLoad, &m_UiPopupMapLoad);
}

void CEditor2::UserMapSave()
{
	if(!m_Map.m_aPath[0]) // TODO: make a function for this
	{
		UserMapSaveAs();
	}
	else
	{
		// TODO: "Map saved" message or indicator of any kind
		SaveMap(m_Map.m_aPath);
	}
}

void CEditor2::UserMapSaveAs()
{
	m_UiFileSelectState.m_pButtonText = Localize("Save");
	m_UiFileSelectState.m_NewFile = true;
	m_UiFileSelectState.m_Selected = -1;
	str_copy(m_UiFileSelectState.m_aPath, "maps", sizeof(m_UiFileSelectState.m_aPath));
	m_UiFileSelectState.m_pListBoxEntries = 0;
	m_UiFileSelectState.PopulateFileList(Storage(), IStorage::TYPE_SAVE);

	PushPopup(&CEditor2::RenderPopupMapSaveAs, 0x0);
}

void CEditor2::HistoryClear()
{
	mem_zero(m_aHistoryEntryUsed, sizeof(m_aHistoryEntryUsed));
}

CEditor2::CHistoryEntry *CEditor2::HistoryAllocEntry()
{
	for(int i = 0; i < MAX_HISTORY; i++)
	{
		if(!m_aHistoryEntryUsed[i])
		{
			m_aHistoryEntryUsed[i] = 1;
			m_aHistoryEntries[i] = CHistoryEntry();
			return &m_aHistoryEntries[i];
		}
	}

	// if not found, get the last one
	CHistoryEntry* pCur = m_pHistoryEntryCurrent;
	while(pCur && pCur->m_pPrev)
	{
		pCur = pCur->m_pPrev;
	}

	pCur->m_pNext->m_pPrev = 0x0;
	*pCur = CHistoryEntry();
	return pCur;
}

void CEditor2::HistoryDeallocEntry(CEditor2::CHistoryEntry *pEntry)
{
	int Index = pEntry - m_aHistoryEntries;
	dbg_assert(Index >= 0 && Index < MAX_HISTORY, "History entry out of bounds");
	dbg_assert(pEntry->m_pSnap != 0x0, "Snapshot is null");
	mem_free(pEntry->m_pSnap);
	mem_free(pEntry->m_pUiSnap); // TODO: dealloc smarter, see above
	m_aHistoryEntryUsed[Index] = 0;
}

void CEditor2::HistoryNewEntry(const char* pActionStr, const char* pDescStr)
{
	m_MapSaved = false;

	CHistoryEntry* pEntry;
	pEntry = HistoryAllocEntry();
	pEntry->m_pSnap = m_Map.SaveSnapshot();
	pEntry->m_pUiSnap = SaveUiSnapshot();
	pEntry->SetAction(pActionStr);
	pEntry->SetDescription(pDescStr);

	// delete all the next entries from current
	CHistoryEntry* pCur = m_pHistoryEntryCurrent->m_pNext;
	while(pCur)
	{
		CHistoryEntry* pToDelete = pCur;
		pCur = pCur->m_pNext;
		HistoryDeallocEntry(pToDelete);
	}

	m_pHistoryEntryCurrent->m_pNext = pEntry;
	pEntry->m_pPrev = m_pHistoryEntryCurrent;
	m_pHistoryEntryCurrent = pEntry;
}

void CEditor2::HistoryRestoreToEntry(CHistoryEntry* pEntry)
{
	dbg_assert(pEntry && pEntry->m_pSnap, "History entry or snapshot invalid");
	m_Map.RestoreSnapshot(pEntry->m_pSnap);
	RestoreUiSnapshot(pEntry->m_pUiSnap);
	m_pHistoryEntryCurrent = pEntry;
}

void CEditor2::HistoryUndo()
{
	dbg_assert(m_pHistoryEntryCurrent != 0x0, "Current history entry is null");
	if(m_pHistoryEntryCurrent->m_pPrev)
		HistoryRestoreToEntry(m_pHistoryEntryCurrent->m_pPrev);
}

void CEditor2::HistoryRedo()
{
	dbg_assert(m_pHistoryEntryCurrent != 0x0, "Current history entry is null");
	if(m_pHistoryEntryCurrent->m_pNext)
		HistoryRestoreToEntry(m_pHistoryEntryCurrent->m_pNext);
}

void CEditor2::HistoryDeleteEntry(CEditor2::CHistoryEntry* pEntry)
{
	mem_free(pEntry->m_pSnap);
	mem_free(pEntry->m_pUiSnap);
}

CEditor2::CUISnapshot* CEditor2::SaveUiSnapshot()
{
	CUISnapshot* pUiSnap = (CUISnapshot*)mem_alloc(sizeof(CUISnapshot), 1); // TODO: alloc this smarter. Does this even need to be heap allocated?
	pUiSnap->m_SelectedLayerID = m_UiSelectedLayerID;
	pUiSnap->m_SelectedGroupID = m_UiSelectedGroupID;
	pUiSnap->m_ToolID = m_Tool;

	ed_dbg("NewUiSnapshot :: m_SelectedLayerID=%d m_SelectedGroupID=%d m_ToolID=%d", pUiSnap->m_SelectedLayerID, pUiSnap->m_SelectedGroupID, pUiSnap->m_ToolID);
	return pUiSnap;
}

void CEditor2::RestoreUiSnapshot(CUISnapshot* pUiSnap)
{
	m_UiSelectedLayerID = pUiSnap->m_SelectedLayerID;
	m_UiSelectedGroupID = pUiSnap->m_SelectedGroupID;
	dbg_assert(m_UiSelectedLayerID == -1 || m_Map.m_aLayers.IsValid(m_UiSelectedLayerID), "Selected layer is invalid");
	dbg_assert(m_Map.m_aGroups.IsValid(m_UiSelectedGroupID), "Selected group is invalid");
	m_Tool = pUiSnap->m_ToolID;
	BrushClear(); // TODO: save brush?
	m_TileSelection.Deselect(); // TODO: save selection?
}

void CEditor2::PushPopup(CUIPopup::Func_PopupRender pFuncRender, void* pPopupData)
{
	dbg_assert(m_UiPopupStackCount < m_UiPopupStack.Capacity(), "Popup stack is full");

	CUIPopup Popup;
	Popup.m_pData = pPopupData;
	Popup.m_pFuncRender = pFuncRender;
	m_UiPopupStack[m_UiPopupStackCount++] = Popup;
}

// Call this inside the popup render function
void CEditor2::ExitPopup()
{
	dbg_assert(m_UiPopupStackCount > 0, "Popup stack is empty");
	dbg_assert(m_UiCurrentPopupID >= 0 && m_UiCurrentPopupID < m_UiPopupStackCount, "Tried to ExitPopup outside the popup Render function");
	m_UiPopupStackToRemove[m_UiCurrentPopupID] = 1;
}

void CEditor2::RenderPopups()
{
	memset(m_UiPopupStackToRemove.data, 0, sizeof(m_UiPopupStackToRemove.data));

	const int PopupCount = m_UiPopupStackCount;
	for(int i = 0; i < PopupCount; i++)
	{
		m_UiCurrentPopupID = i;
		CUIPopup& Popup = m_UiPopupStack[i];
		(this->*Popup.m_pFuncRender)(Popup.m_pData);
	}

	m_UiCurrentPopupID = -1;

	bool DoRemove = false;
	for(int i = 0; i < PopupCount; i++)
	{
		if(m_UiPopupStackToRemove[i])
		{
			DoRemove = true;
			break;
		}
	}

	// remove popups
	if(DoRemove)
	{
		const int PopupCount = m_UiPopupStackCount; // get this once again since we might have pushed popups inside a popup render function
		CPlainArray<CUIPopup,32> Copy = m_UiPopupStack;
		m_UiPopupStackCount = 0;
		for(int i = 0; i < PopupCount; i++)
		{
			if(!m_UiPopupStackToRemove[i])
			{
				m_UiPopupStack[m_UiPopupStackCount++] = Copy[i];
			}
		}
	}
}

const char* CEditor2::GetLayerName(int LayerID)
{
	static char aBuff[64];
	const CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	if(Layer.m_aName[0])
	{
		str_format(aBuff, sizeof(aBuff), "%s %d", Layer.m_aName, LayerID);
		return aBuff;
	}

	if(Layer.IsTileLayer())
		str_format(aBuff, sizeof(aBuff), Localize("Tile %d"), LayerID);
	else if(Layer.IsQuadLayer())
		str_format(aBuff, sizeof(aBuff), Localize("Quad %d"), LayerID);
	else
		dbg_break();
	return aBuff;
}

void CEditor2::CTileSelection::FitLayer(const CEditorMap2::CLayer& TileLayer)
{
	dbg_assert(TileLayer.IsTileLayer(), "Layer is not a tile layer");
	// if either start or end point could be inside layer

	CUIRect SelectRect = {
		(float)m_StartTX,
		(float)m_StartTY,
		(float)m_EndTX + 1 - m_StartTX,
		(float)m_EndTY + 1 - m_StartTY
	};

	CUIRect LayerRect = {
		0, 0,
		(float)TileLayer.m_Width, (float)TileLayer.m_Height
	};

	if(DoRectIntersect(SelectRect, LayerRect))
	{
		m_StartTX = clamp(m_StartTX, 0, TileLayer.m_Width-1);
		m_StartTY = clamp(m_StartTY, 0, TileLayer.m_Height-1);
		m_EndTX = clamp(m_EndTX, 0, TileLayer.m_Width-1);
		m_EndTY = clamp(m_EndTY, 0, TileLayer.m_Height-1);
	}
	else
		Deselect();
}
