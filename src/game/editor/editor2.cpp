// LordSk
#include "editor2.h"

#include <engine/client.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/textrender.h>

inline float fract(float f)
{
	return f - (int)f;
}

bool CEditorMap::Save(IStorage* pStorage, const char* pFileName)
{
	return false;
}

bool CEditorMap::Load(IStorage* pStorage, IGraphics* pGraphics, const char* pFileName)
{
	if(!m_File.Load(pFileName, pStorage))
		return false;

	int GroupsStart, GroupsNum;
	int LayersStart, LayersNum;
	m_File.GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
	m_File.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);

	dbg_msg("editor", "GroupsStart=%d GroupsNum=%d LayersStart=%d LayersNum=%d",
			GroupsStart, GroupsNum, LayersStart, LayersNum);

	for(int gi = 0; gi < GroupsNum; gi++)
	{
		CMapItemGroup* pGroup = (CMapItemGroup*)m_File.GetItem(GroupsStart+gi, 0, 0);
		dbg_msg("editor", "Group#%d NumLayers=%d", gi, pGroup->m_NumLayers);
		const int GroupLayerCount = pGroup->m_NumLayers;
		const int GroupLayerStart = pGroup->m_StartLayer;
		CEditorMap::CGroup Group;
		Group.m_Parallax = vec2(pGroup->m_ParallaxX, pGroup->m_ParallaxY);
		Group.m_Position = vec2(pGroup->m_OffsetX, pGroup->m_OffsetY);
		Group.m_LayerCount = 0;

		for(int li = 0; li < GroupLayerCount; li++)
		{
			CMapItemLayer* pLayer = (CMapItemLayer*)m_File.GetItem(LayersStart+GroupLayerStart+li, 0, 0);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				const CMapItemLayerTilemap& Tilemap = *(CMapItemLayerTilemap*)pLayer;
				dbg_msg("editor", "Group#%d Layer=%d (w=%d, h=%d)", gi, li,
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
				LayerTile.m_TileStartID = m_aTiles.size();
				const int LayerID = m_aLayers.add(LayerTile);

				// TODO: this is extremely slow
				CTile *pTiles = (CTile *)m_File.GetData(Tilemap.m_Data);
				int TileCount = Tilemap.m_Width*Tilemap.m_Height;
				for(int ti = 0; ti < TileCount; ti++)
				{
					CTile t = pTiles[ti];
					const int Skips = t.m_Skip;
					t.m_Skip = 0;

					for(int s = 0; s < Skips; s++)
						m_aTiles.add(t);
					m_aTiles.add(t);
				}

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;

				if(Tilemap.m_Flags&TILESLAYERFLAG_GAME)
				{
					m_GameLayerID = LayerID;
					m_GameGroupID = m_aGroups.size();
				}
			}
			else if(pLayer->m_Type == LAYERTYPE_QUADS)
			{
				const CMapItemLayerQuads& ItemQuadLayer = *(CMapItemLayerQuads*)pLayer;

				CLayer LayerQuad;
				LayerQuad.m_Type = LAYERTYPE_QUADS;
				LayerQuad.m_ImageID = ItemQuadLayer.m_Image;
				LayerQuad.m_QuadStartID = m_aQuads.size();
				LayerQuad.m_QuadCount = ItemQuadLayer.m_NumQuads;
				const int LayerID = m_aLayers.add(LayerQuad);

				// TODO: this is extremely slow
				CQuad *pQuads = (CQuad *)m_File.GetData(ItemQuadLayer.m_Data);
				const int QuadCount = LayerQuad.m_QuadCount;
				for(int qi = 0; qi < QuadCount; qi++)
				{
					m_aQuads.add(pQuads[qi]);
				}

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;
			}
		}

		m_aGroups.add(Group);
	}

	dbg_assert(m_GameLayerID >= 0, "Game layer not found");
	dbg_assert(m_GameGroupID >= 0, "Game group not found");

	// load textures
	// TODO: move this out?
	int ImagesStart, ImagesCount;
	m_File.GetType(MAPITEMTYPE_IMAGE, &ImagesStart, &ImagesCount);

	for(int i = 0; i < ImagesCount && i < MAX_TEXTURES; i++)
	{
		int TextureFlags = 0;
		bool FoundQuadLayer = false;
		bool FoundTileLayer = false;
		const int LayerCount = m_aLayers.size();
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

		CMapItemImage *pImg = (CMapItemImage *)m_File.GetItem(ImagesStart+i, 0, 0);
		if(pImg->m_External || (pImg->m_Version > 1 && pImg->m_Format != CImageInfo::FORMAT_RGB &&
								pImg->m_Format != CImageInfo::FORMAT_RGBA))
		{
			char Buf[256];
			char *pName = (char *)m_File.GetData(pImg->m_ImageName);
			str_format(Buf, sizeof(Buf), "mapres/%s.png", pName);
			m_aTextures[i] = pGraphics->LoadTexture(Buf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO,
													TextureFlags);
			dbg_msg("editor", "mapres/%s.png loaded", pName);
		}
		else
		{
			void *pData = m_File.GetData(pImg->m_ImageData);
			m_aTextures[i] = pGraphics->LoadTextureRaw(pImg->m_Width, pImg->m_Height,
				pImg->m_Version == 1 ? CImageInfo::FORMAT_RGBA : pImg->m_Format, pData,
				CImageInfo::FORMAT_RGBA, TextureFlags);
		}
		dbg_assert(m_aTextures[i].IsValid(), "Invalid texture");
	}

	return true;
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
	m_pConsole = Kernel()->RequestInterface<IConsole>();
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

	if(!m_Map.Load(m_pStorage, m_pGraphics, "maps/ctf7.map")) {
		dbg_break();
	}
	m_UiSelectedLayerID = m_Map.m_GameLayerID;
	m_UiSelectedGroupID = m_Map.m_GameGroupID;
}

void CEditor::UpdateAndRender()
{
	UI()->StartCheck();
	Update();
	Render();
	UI()->FinishCheck();
}

bool CEditor::HasUnsavedData() const
{
	return false;
}

void CEditor::Update()
{
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

	// move view
	if(MouseButtons&MOUSE_RIGHT)
	{
		m_MapUiPosOffset -= m_UiMouseDelta;
	}

	// zoom with mouse wheel
	if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
		m_Zoom *= 0.9;
	else if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
		m_Zoom *= 1.1;

	if(Input()->KeyPress(KEY_HOME))
	{
		m_Zoom = 1;
		m_MapUiPosOffset = vec2(0,0);
	}

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
		SelectedParallaxX = Group.m_Parallax.x / 100.f;
		SelectedParallaxY = Group.m_Parallax.y / 100.f;
		SelectedPositionX = Group.m_Position.x;
		SelectedPositionY = Group.m_Position.y;
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
	CTile* pTileBuffer = &m_Map.m_aTiles[0];
	CQuad* pQuadBuffer = &m_Map.m_aQuads[0];
	const int GroupCount = m_Map.m_aGroups.size();

	for(int gi = 0; gi < GroupCount; gi++)
	{
		if(m_UiGroupHidden[gi])
			continue;

		CEditorMap::CGroup& Group = m_Map.m_aGroups[gi];

		const float ParallaxX = Group.m_Parallax.x / 100.f;
		const float ParallaxY = Group.m_Parallax.y / 100.f;
		const float PositionX = Group.m_Position.x;
		const float PositionY = Group.m_Position.y;
		const vec2 MapOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight,
												  PositionX, PositionX, ParallaxX, ParallaxY);
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
				CTile *pTiles = pTileBuffer + Layer.m_TileStartID;

				if(LyID == m_Map.m_GameLayerID)
					continue;

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

				RenderTools()->RenderQuads(&pQuadBuffer[Layer.m_QuadStartID], Layer.m_QuadCount,
						LAYERRENDERFLAG_TRANSPARENT, 0, 0);
			}
		}
	}

	// game layer
	const int LyID = m_Map.m_GameLayerID;
	if(!m_UiLayerHidden[LyID] && !m_UiGroupHidden[m_Map.m_GameGroupID])
	{
		const vec2 MapOff = CalcGroupScreenOffset(ZoomWorldViewWidth, ZoomWorldViewHeight, 0, 0, 1, 1);
		CUIRect ScreenRect = { MapOff.x, MapOff.y, ZoomWorldViewWidth, ZoomWorldViewHeight };

		Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w,
							  ScreenRect.y+ScreenRect.h);

		const CEditorMap::CLayer& LayerTile = m_Map.m_aLayers[LyID];
		const float LyWidth = LayerTile.m_Width;
		const float LyHeight = LayerTile.m_Height;
		vec4 LyColor = LayerTile.m_Color;
		CTile *pTiles = pTileBuffer + LayerTile.m_TileStartID;

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

	// render mouse cursor
	{
		Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);
		Graphics()->TextureSet(m_CursorTexture);
		Graphics()->WrapClamp();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1,0,1,1);
		/*if(ms_pUiGotContext == UI()->HotItem())
			Graphics()->SetColor(1,0,0,1);*/
		IGraphics::CQuadItem QuadItem(m_UiMousePos.x, m_UiMousePos.y, 16.0f, 16.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		Graphics()->WrapNormal();
	}
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

void CEditor::RenderUI()
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect RightPanel;
	UiScreenRect.VSplitRight(150, 0, &RightPanel);

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
		str_format(aBuff, sizeof(aBuff), "Parallax = (%g, %g)",
				   SelectedGroup.m_Parallax.x, SelectedGroup.m_Parallax.y);
		DrawText(ButtonRect, aBuff, FontSize);

		// position
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);
		NavRect.HSplitTop(Spacing, 0, &NavRect);
		DrawRect(ButtonRect, vec4(0,0,0,1));
		str_format(aBuff, sizeof(aBuff), "Position = (%g, %g)",
				   SelectedGroup.m_Position.x, SelectedGroup.m_Position.y);
		DrawText(ButtonRect, aBuff, FontSize);
	}

	const int GroupCount = m_Map.m_aGroups.size();
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

		vec4 ShowButColor(0.062, 0, 0.19, 1);
		if(ShowButState.m_Hovered)
			ShowButColor = vec4(0.28, 0.10, 0.64, 1);
		if(ShowButState.m_Pressed)
			ShowButColor = vec4(0.13, 0, 0.40, 1);

		DrawRectBorder(ShowButton, ShowButColor, 1, vec4(0.13, 0, 0.40, 1));
		DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

		// group button
		CUIButtonState& ButState = s_UiGroupButState[gi];
		UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

		if(ButState.m_Clicked)
			m_UiGroupOpen[gi] ^= 1;

		const bool IsOpen = m_UiGroupOpen[gi];

		vec4 ButColor(0.062, 0, 0.19, 1);
		if(ButState.m_Hovered)
			ButColor = vec4(0.28, 0.10, 0.64, 1);
		if(ButState.m_Pressed)
			ButColor = vec4(0.13, 0, 0.40, 1);

		if(IsOpen)
			DrawRectBorder(ButtonRect, ButColor, 1, vec4(0.13, 0, 0.40, 1));
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

				vec4 ShowButColor(0.062, 0, 0.19, 1);
				if(ShowButState.m_Hovered)
					ShowButColor = vec4(0.28, 0.10, 0.64, 1);
				if(ShowButState.m_Pressed)
					ShowButColor = vec4(0.13, 0, 0.40, 1);

				DrawRectBorder(ShowButton, ShowButColor, 1, vec4(0.13, 0, 0.40, 1));
				DrawText(ShowButton, IsShown ? "o" : "x", FontSize);

				// layer button
				CUIButtonState& ButState = s_UiLayerButState[LyID];
				UiDoButtonBehavior(&ButState, ButtonRect, &ButState);

				vec4 ButColor(0.062, 0, 0.19, 1);
				if(ButState.m_Hovered)
					ButColor = vec4(0.28, 0.10, 0.64, 1);
				if(ButState.m_Pressed)
					ButColor = vec4(0.13, 0, 0.40, 1);

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

}

void CEditor::Reset()
{

}
