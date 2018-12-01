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
		Group.m_LayerCount = 0;

		for(int li = 0; li < GroupLayerCount; li++)
		{
			CMapItemLayer* pLayer = (CMapItemLayer*)m_File.GetItem(LayersStart+GroupLayerStart+li, 0, 0);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				const CMapItemLayerTilemap& Tilemap = *(CMapItemLayerTilemap*)pLayer;
				dbg_msg("editor", "Group#%d Layer=%d (w=%d, h=%d)", gi, li,
						Tilemap.m_Width, Tilemap.m_Height);

				if(!(Tilemap.m_Flags&TILESLAYERFLAG_GAME))
				{
					m_MapMaxWidth = max(m_MapMaxWidth, Tilemap.m_Width);
					m_MapMaxHeight = max(m_MapMaxHeight, Tilemap.m_Height);

					CLayer LayerTile;
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
						m_aTiles.add(pTiles[ti]);
						TileCount -= pTiles[ti].m_Skip;
					}

					if(Group.m_LayerCount < MAX_GROUP_LAYERS)
						Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;
				}
			}
		}

		m_aGroups.add(Group);
	}

	// load textures
	// TODO: move this out?
	int ImagesStart, ImagesCount;
	m_File.GetType(MAPITEMTYPE_IMAGE, &ImagesStart, &ImagesCount);

	for(int i = 0; i < ImagesCount && i < MAX_TEXTURES; i++)
	{
		int TextureFlags = 0;
		/*bool FoundQuadLayer = false;
		bool FoundTileLayer = false;
		for(int k = 0; k < pLayers->NumLayers(); k++)
		{
			const CMapItemLayer * const pLayer = pLayers->GetLayer(k);
			if(!FoundQuadLayer && pLayer->m_Type == LAYERTYPE_QUADS && ((const CMapItemLayerQuads * const)pLayer)->m_Image == i)
				FoundQuadLayer = true;
			if(!FoundTileLayer && pLayer->m_Type == LAYERTYPE_TILES && ((const CMapItemLayerTilemap * const)pLayer)->m_Image == i)
				FoundTileLayer = true;
		}
		if(FoundTileLayer)
			TextureFlags = FoundQuadLayer ? IGraphics::TEXLOAD_MULTI_DIMENSION : IGraphics::TEXLOAD_ARRAY_256;*/

		TextureFlags = IGraphics::TEXLOAD_ARRAY_256;

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
}

void CEditor::UpdateAndRender()
{
	Update();
	Render();
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

	Input()->Clear();
}

void CEditor::Render()
{
	const CUIRect UiScreenRect = *UI()->Screen();
	m_UiScreenRect = UiScreenRect;

	// basic start
	Graphics()->Clear(0.3f, 0.3f, 0.3f);

	const float ScreenWidth = (float)Graphics()->ScreenWidth();
	const float ScreenHeight = (float)Graphics()->ScreenHeight();
	const float MapScreenWidth = ScreenWidth * m_Zoom;
	const float MapScreenHeight = ScreenHeight * m_Zoom;
	const float MapOffX = m_MapUiPosOffset.x/UiScreenRect.w * MapScreenWidth;
	const float MapOffY = m_MapUiPosOffset.y/UiScreenRect.h * MapScreenHeight;
	CUIRect ScreenRect = { MapOffX, MapOffY, MapScreenWidth, MapScreenHeight };

	const float FakeToScreenX = ScreenWidth/MapScreenWidth;
	const float FakeToScreenY = ScreenHeight/MapScreenHeight;
	const float TileSize = 32;

	m_GfxScreenWidth = ScreenWidth;
	m_GfxScreenHeight = ScreenHeight;

	// background
	{
		Graphics()->MapScreen(0, 0, MapScreenWidth, MapScreenHeight);
		Graphics()->TextureSet(m_CheckerTexture);
		Graphics()->BlendNormal();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);

		// align background with grid
		float StartX = fract(MapOffX/(TileSize*2));
		float StartY = fract(MapOffY/(TileSize*2));
		Graphics()->QuadsSetSubset(StartX, StartY,
								   MapScreenWidth/(TileSize*2)+StartX,
								   MapScreenHeight/(TileSize*2)+StartY);

		IGraphics::CQuadItem QuadItem(0, 0, MapScreenWidth, MapScreenHeight);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	// render map
	Graphics()->MapScreen(ScreenRect.x, ScreenRect.y, ScreenRect.x+ScreenRect.w, ScreenRect.y+ScreenRect.h);

	CTile* pTileBuffer = &m_Map.m_aTiles[0];
	const int GroupCount = m_Map.m_aGroups.size();

	for(int gi = 0; gi < GroupCount; gi++)
	{
		CEditorMap::CGroup& Group = m_Map.m_aGroups[gi];
		const int LayerCount = Group.m_LayerCount;

		for(int li = 0; li < LayerCount; li++)
		{
			const int LyID = Group.m_apLayerIDs[li];
			const CEditorMap::CLayer& LayerTile = m_Map.m_aLayers[LyID];
			const float LyWidth = LayerTile.m_Width;
			const float LyHeight = LayerTile.m_Height;
			vec4 LyColor = LayerTile.m_Color;
			CTile *pTiles = pTileBuffer + LayerTile.m_TileStartID;

			if(m_UiLayerHovered[LyID])
				LyColor = vec4(1, 0, 1, 1);

			if(LayerTile.m_ImageID == -1)
				Graphics()->TextureClear();
			else
				Graphics()->TextureSet(m_Map.m_aTextures[LayerTile.m_ImageID]);

			Graphics()->BlendNone();
			RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
										 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_OPAQUE,
										 0, 0, -1, 0);

			Graphics()->BlendNormal();
			RenderTools()->RenderTilemap(pTiles, LyWidth, LyHeight, TileSize, LyColor,
										 /*TILERENDERFLAG_EXTEND|*/LAYERRENDERFLAG_TRANSPARENT,
										 0, 0, -1, 0);
		}
	}
	Graphics()->BlendNormal();

	// origin and border
	IGraphics::CQuadItem RectX(0, 0, TileSize, 2/FakeToScreenY);
	IGraphics::CQuadItem RectY(0, 0, 2/FakeToScreenX, TileSize);
	const float MapMaxWidth = m_Map.m_MapMaxWidth * TileSize;
	const float MapMaxHeight = m_Map.m_MapMaxHeight * TileSize;
	const float bw = 1.0f / FakeToScreenX;
	const float bh = 1.0f / FakeToScreenY;
	IGraphics::CQuadItem aBorders[4] = {
		IGraphics::CQuadItem(0, 0, MapMaxWidth, bh),
		IGraphics::CQuadItem(0, MapMaxHeight, MapMaxWidth, bh),
		IGraphics::CQuadItem(0, 0, bw, MapMaxHeight),
		IGraphics::CQuadItem(MapMaxWidth, 0, bw, MapMaxHeight)
	};

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// grid
	if(m_ConfigShowGrid)
	{
		const float GridAlpha = 0.25f;
		Graphics()->SetColor(1.0f * GridAlpha, 1.0f * GridAlpha, 1.0f * GridAlpha, GridAlpha);
		float StartX = MapOffX - fract(MapOffX/TileSize) * TileSize;
		float StartY = MapOffY - fract(MapOffY/TileSize) * TileSize;
		float EndX = MapOffX+MapScreenWidth;
		float EndY = MapOffY+MapScreenHeight;
		for(float x = StartX; x < EndX; x+= TileSize)
		{
			IGraphics::CQuadItem Line(x, MapOffY, bw, MapScreenHeight);
			Graphics()->QuadsDrawTL(&Line, 1);
		}
		for(float y = StartY; y < EndY; y+= TileSize)
		{
			IGraphics::CQuadItem Line(MapOffX, y, MapScreenWidth, bh);
			Graphics()->QuadsDrawTL(&Line, 1);
		}
	}


	Graphics()->SetColor(1, 1, 1, 1);
	Graphics()->QuadsDrawTL(aBorders, 4);
	Graphics()->SetColor(0, 0, 1, 1);
	Graphics()->QuadsDrawTL(&RectX, 1);
	Graphics()->SetColor(0, 1, 0, 1);
	Graphics()->QuadsDrawTL(&RectY, 1);

	Graphics()->QuadsEnd();

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

void CEditor::RenderUI()
{
	const CUIRect UiScreenRect = m_UiScreenRect;
	Graphics()->MapScreen(UiScreenRect.x, UiScreenRect.y, UiScreenRect.w, UiScreenRect.h);

	CUIRect RightPanel;
	UiScreenRect.VSplitRight(150, 0, &RightPanel);

	DrawRect(RightPanel, vec4(0.03, 0, 0.085, 1));

	CUIRect NavRect, ButtonRect;
	RightPanel.Margin(3.0f, &NavRect);

	static CUIButtonState s_UiGroupButState[MAX_GROUPS];
	static CUIButtonState s_UiLayerButState[MAX_LAYERS];
	const float FontSize = 8.0f;
	const float ButtonHeight = 20.0f;
	const float Spacing = 2.0f;

	const int GroupCount = m_Map.m_aGroups.size();
	for(int gi = 0; gi < GroupCount; gi++)
	{
		if(gi != 0)
			NavRect.HSplitTop(Spacing, 0, &NavRect);
		NavRect.HSplitTop(ButtonHeight, &ButtonRect, &NavRect);

		CUIButtonState& ButState = s_UiGroupButState[gi];
		UiDoButtonBehavior((&m_Map.m_aGroups)+gi, ButtonRect, &ButState);

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

		CUIRect ExpandBut;
		ButtonRect.VSplitLeft(ButtonRect.h, &ExpandBut, &ButtonRect);

		DrawText(ExpandBut, IsOpen ? "-" : "+", FontSize);

		char aGroupName[64];
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
				CUIButtonState& ButState = s_UiLayerButState[LyID];
				UiDoButtonBehavior((&m_Map.m_aGroups)+gi*1000+li, ButtonRect, &ButState);

				vec4 ButColor(0.062, 0, 0.19, 1);
				if(ButState.m_Hovered)
					ButColor = vec4(0.28, 0.10, 0.64, 1);
				if(ButState.m_Pressed)
					ButColor = vec4(0.13, 0, 0.40, 1);

				m_UiLayerHovered[LyID] = ButState.m_Hovered;
				if(ButState.m_Clicked)
					m_UiSelectedLayer = LyID;

				const bool IsSelected = m_UiSelectedLayer == LyID;

				if(IsSelected)
					DrawRectBorder(ButtonRect, ButColor, 1, vec4(1, 0, 0, 1));
				else
					DrawRect(ButtonRect, ButColor);

				char aLayerName[64];
				str_format(aLayerName, sizeof(aLayerName), "Layer #%d", li);
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
	if(!UI()->MouseButton(0) && pButState->m_Pressed)
		pButState->m_Clicked = true;
	else
		pButState->m_Clicked = false;

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
			if(pID)
				UI()->SetActiveItem(pID);
		}
	}
}

void CEditor::Reset()
{

}
