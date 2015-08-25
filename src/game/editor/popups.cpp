/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/tl/array.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>

#include "editor.h"


// popup menu handling
static struct
{
	CUIRect m_Rect;
	void *m_pId;
	int (*m_pfnFunc)(CEditor *pEditor, CUIRect Rect);
	int m_IsMenu;
	void *m_pExtra;
} s_UiPopups[8];

static int g_UiNumPopups = 0;

void CEditor::UiInvokePopupMenu(void *pID, int Flags, float x, float y, float Width, float Height, int (*pfnFunc)(CEditor *pEditor, CUIRect Rect), void *pExtra)
{
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", "invoked");
	if(x + Width > UI()->Screen()->w)
		x -= Width;
	if(y + Height > UI()->Screen()->h)
		y -= Height;
	s_UiPopups[g_UiNumPopups].m_pId = pID;
	s_UiPopups[g_UiNumPopups].m_IsMenu = Flags;
	s_UiPopups[g_UiNumPopups].m_Rect.x = x;
	s_UiPopups[g_UiNumPopups].m_Rect.y = y;
	s_UiPopups[g_UiNumPopups].m_Rect.w = Width;
	s_UiPopups[g_UiNumPopups].m_Rect.h = Height;
	s_UiPopups[g_UiNumPopups].m_pfnFunc = pfnFunc;
	s_UiPopups[g_UiNumPopups].m_pExtra = pExtra;
	g_UiNumPopups++;
}

void CEditor::UiDoPopupMenu()
{
	for(int i = 0; i < g_UiNumPopups; i++)
	{
		bool Inside = UI()->MouseInside(&s_UiPopups[i].m_Rect);
		UI()->SetHotItem(&s_UiPopups[i].m_pId);

		if(UI()->ActiveItem() == &s_UiPopups[i].m_pId)
		{
			if(!UI()->MouseButton(0))
			{
				if(!Inside)
					g_UiNumPopups--;
				UI()->SetActiveItem(0);
			}
		}
		else if(UI()->HotItem() == &s_UiPopups[i].m_pId)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(&s_UiPopups[i].m_pId);
		}

		int Corners = CUI::CORNER_ALL;
		if(s_UiPopups[i].m_IsMenu)
			Corners = CUI::CORNER_R|CUI::CORNER_B;

		CUIRect r = s_UiPopups[i].m_Rect;
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), Corners, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), Corners, 3.0f);
		r.Margin(4.0f, &r);

		if(s_UiPopups[i].m_pfnFunc(this, r))
		{
			g_UiNumPopups--;
			UI()->SetActiveItem(0);
		}

		if(Input()->KeyDown(KEY_ESCAPE))
		{
			g_UiNumPopups--;
			UI()->SetActiveItem(0);
		}
	}
}


int CEditor::PopupGroup(CEditor *pEditor, CUIRect View)
{
	// remove group button
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;

	// don't allow deletion of game group
	if(pEditor->m_Map.m_pGameGroup != pEditor->GetSelectedGroup())
	{
		if(pEditor->DoButton_Editor(&s_DeleteButton, "Delete group", 0, &Button, 0, "Delete group"))
		{
			pEditor->m_Map.DeleteGroup(pEditor->m_SelectedGroup);
			pEditor->m_SelectedGroup = max(0, pEditor->m_SelectedGroup-1);
			return 1;
		}
	}
	else
	{
		if(pEditor->DoButton_Editor(&s_DeleteButton, "Clean-up game tiles", 0, &Button, 0, "Removes game tiles that aren't based on a layer"))
		{
			// gather all tile layers
			array<CLayerTiles*> Layers;
			for(int i = 0; i < pEditor->m_Map.m_pGameGroup->m_lLayers.size(); ++i)
			{
				if(pEditor->m_Map.m_pGameGroup->m_lLayers[i] != pEditor->m_Map.m_pGameLayer && pEditor->m_Map.m_pGameGroup->m_lLayers[i]->m_Type == LAYERTYPE_TILES)
					Layers.add(static_cast<CLayerTiles *>(pEditor->m_Map.m_pGameGroup->m_lLayers[i]));
			}

			// search for unneeded game tiles
			CLayerTiles *gl = pEditor->m_Map.m_pGameLayer;
			for(int y = 0; y < gl->m_Height; ++y)
				for(int x = 0; x < gl->m_Width; ++x)
				{
					if(gl->m_pTiles[y*gl->m_Width+x].m_Index > static_cast<unsigned char>(TILE_NOHOOK))
						continue;

					bool Found = false;
					for(int i = 0; i < Layers.size(); ++i)
					{
						if(x < Layers[i]->m_Width && y < Layers[i]->m_Height && Layers[i]->m_pTiles[y*Layers[i]->m_Width+x].m_Index)
						{
							Found = true;
							break;
						}
					}

					if(!Found)
					{
						gl->m_pTiles[y*gl->m_Width+x].m_Index = TILE_AIR;
						pEditor->m_Map.m_Modified = true;
					}
				}

			return 1;
		}
	}

	// new tile layer
	View.HSplitBottom(10.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewQuadLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewQuadLayerButton, "Add quads layer", 0, &Button, 0, "Creates a new quad layer"))
	{
		CLayer *l = new CLayerQuads;
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_Collapse = false;
		return 1;
	}

	// new quad layer
	View.HSplitBottom(5.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewTileLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewTileLayerButton, "Add tile layer", 0, &Button, 0, "Creates a new tile layer"))
	{
		CLayer *l = new CLayerTiles(50, 50);
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_Collapse = false;
		return 1;
	}

	// group name
	if(!pEditor->GetSelectedGroup()->m_GameGroup)
	{
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static float s_Name = 0;
		pEditor->UI()->DoLabel(&Button, "Name:", 10.0f, -1, -1);
		Button.VSplitLeft(40.0f, 0, &Button);
		if(pEditor->DoEditBox(&s_Name, &Button, pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_aName, sizeof(pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_aName), 10.0f, &s_Name))
			pEditor->m_Map.m_Modified = true;
	}

	enum
	{
		PROP_ORDER=0,
		PROP_POS_X,
		PROP_POS_Y,
		PROP_PARA_X,
		PROP_PARA_Y,
		PROP_USE_CLIPPING,
		PROP_CLIP_X,
		PROP_CLIP_Y,
		PROP_CLIP_W,
		PROP_CLIP_H,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Order", pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{"Pos X", -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para X", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para Y", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxY, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{"Use Clipping", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_UseClipping, PROPTYPE_BOOL, 0, 1},
		{"Clip X", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip Y", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip W", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipW, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip H", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipH, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;

	// cut the properties that isn't needed
	if(pEditor->GetSelectedGroup()->m_GameGroup)
		aProps[PROP_POS_X].m_pName = 0;

	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_ORDER)
		pEditor->m_SelectedGroup = pEditor->m_Map.SwapGroups(pEditor->m_SelectedGroup, NewVal);

	// these can not be changed on the game group
	if(!pEditor->GetSelectedGroup()->m_GameGroup)
	{
		if(Prop == PROP_PARA_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxX = NewVal;
		else if(Prop == PROP_PARA_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxY = NewVal;
		else if(Prop == PROP_POS_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetX = -NewVal;
		else if(Prop == PROP_POS_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetY = -NewVal;
		else if(Prop == PROP_USE_CLIPPING) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_UseClipping = NewVal;
		else if(Prop == PROP_CLIP_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipX = NewVal;
		else if(Prop == PROP_CLIP_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipY = NewVal;
		else if(Prop == PROP_CLIP_W) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipW = NewVal;
		else if(Prop == PROP_CLIP_H) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipH = NewVal;
	}

	return 0;
}

int CEditor::PopupLayer(CEditor *pEditor, CUIRect View)
{
	// remove layer button
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;

	// don't allow deletion of game layer
	if(pEditor->m_Map.m_pGameLayer != pEditor->GetSelectedLayer(0) &&
		pEditor->DoButton_Editor(&s_DeleteButton, "Delete layer", 0, &Button, 0, "Deletes the layer"))
	{
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->DeleteLayer(pEditor->m_SelectedLayer);
		return 1;
	}

	// layer name
	if(pEditor->m_Map.m_pGameLayer != pEditor->GetSelectedLayer(0))
	{
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static float s_Name = 0;
		pEditor->UI()->DoLabel(&Button, "Name:", 10.0f, -1, -1);
		Button.VSplitLeft(40.0f, 0, &Button);
		if(pEditor->DoEditBox(&s_Name, &Button, pEditor->GetSelectedLayer(0)->m_aName, sizeof(pEditor->GetSelectedLayer(0)->m_aName), 10.0f, &s_Name))
			pEditor->m_Map.m_Modified = true;
	}

	View.HSplitBottom(10.0f, &View, 0);

	CLayerGroup *pCurrentGroup = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup];
	CLayer *pCurrentLayer = pEditor->GetSelectedLayer(0);

	enum
	{
		PROP_GROUP=0,
		PROP_ORDER,
		PROP_HQ,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Group", pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{"Order", pEditor->m_SelectedLayer, PROPTYPE_INT_STEP, 0, pCurrentGroup->m_lLayers.size()},
		{"Detail", pCurrentLayer->m_Flags&LAYERFLAG_DETAIL, PROPTYPE_BOOL, 0, 1},
		{0},
	};

	if(pEditor->m_Map.m_pGameLayer == pEditor->GetSelectedLayer(0)) // dont use Group and Detail from the selection if this is the game layer
	{
		aProps[0].m_Type = PROPTYPE_NULL;
		aProps[2].m_Type = PROPTYPE_NULL;
	}

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_ORDER)
		pEditor->m_SelectedLayer = pCurrentGroup->SwapLayers(pEditor->m_SelectedLayer, NewVal);
	else if(Prop == PROP_GROUP && pCurrentLayer->m_Type != LAYERTYPE_GAME)
	{
		if(NewVal >= 0 && NewVal < pEditor->m_Map.m_lGroups.size())
		{
			pCurrentGroup->m_lLayers.remove(pCurrentLayer);
			pEditor->m_Map.m_lGroups[NewVal]->m_lLayers.add(pCurrentLayer);
			pEditor->m_SelectedGroup = NewVal;
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[NewVal]->m_lLayers.size()-1;
		}
	}
	else if(Prop == PROP_HQ)
	{
		pCurrentLayer->m_Flags &= ~LAYERFLAG_DETAIL;
		if(NewVal)
			pCurrentLayer->m_Flags |= LAYERFLAG_DETAIL;
	}

	return pCurrentLayer->RenderProperties(&View);
}

int CEditor::PopupQuad(CEditor *pEditor, CUIRect View)
{
	CQuad *pQuad = pEditor->GetSelectedQuad();

	CUIRect Button;

	// delete button
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;
	if(pEditor->DoButton_Editor(&s_DeleteButton, "Delete", 0, &Button, 0, "Deletes the current quad"))
	{
		CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
		if(pLayer)
		{
			pEditor->m_Map.m_Modified = true;
			pLayer->m_lQuads.remove_index(pEditor->m_SelectedQuad);
			pEditor->m_SelectedQuad--;
		}
		return 1;
	}

	// aspect ratio button
	View.HSplitBottom(10.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
	if(pLayer && pLayer->m_Image >= 0 && pLayer->m_Image < pEditor->m_Map.m_lImages.size())
	{
		static int s_AspectRatioButton = 0;
		if(pEditor->DoButton_Editor(&s_AspectRatioButton, "Aspect ratio", 0, &Button, 0, "Resizes the current Quad based on the aspect ratio of the image"))
		{
			int Top = pQuad->m_aPoints[0].y;
			int Left = pQuad->m_aPoints[0].x;
			int Right = pQuad->m_aPoints[0].x;

			for(int k = 1; k < 4; k++)
			{
				if(pQuad->m_aPoints[k].y < Top) Top = pQuad->m_aPoints[k].y;
				if(pQuad->m_aPoints[k].x < Left) Left = pQuad->m_aPoints[k].x;
				if(pQuad->m_aPoints[k].x > Right) Right = pQuad->m_aPoints[k].x;
			}

			int Height = (Right-Left)*pEditor->m_Map.m_lImages[pLayer->m_Image]->m_Height/pEditor->m_Map.m_lImages[pLayer->m_Image]->m_Width;

			pQuad->m_aPoints[0].x = Left; pQuad->m_aPoints[0].y = Top;
			pQuad->m_aPoints[1].x = Right; pQuad->m_aPoints[1].y = Top;
			pQuad->m_aPoints[2].x = Left; pQuad->m_aPoints[2].y = Top+Height;
			pQuad->m_aPoints[3].x = Right; pQuad->m_aPoints[3].y = Top+Height;
			pEditor->m_Map.m_Modified = true;
			return 1;
		}
	}

	// align button
	View.HSplitBottom(6.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_AlignButton = 0;
	if(pEditor->DoButton_Editor(&s_AlignButton, "Align", 0, &Button, 0, "Aligns coordinates of the quad points"))
	{
		for(int k = 1; k < 4; k++)
		{
			pQuad->m_aPoints[k].x = 1000.0f * (int(pQuad->m_aPoints[k].x) / 1000);
			pQuad->m_aPoints[k].y = 1000.0f * (int(pQuad->m_aPoints[k].y) / 1000);
		}
		pEditor->m_Map.m_Modified = true;
		return 1;
	}

	// square button
	View.HSplitBottom(6.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_Button = 0;
	if(pEditor->DoButton_Editor(&s_Button, "Square", 0, &Button, 0, "Squares the current quad"))
	{
		int Top = pQuad->m_aPoints[0].y;
		int Left = pQuad->m_aPoints[0].x;
		int Bottom = pQuad->m_aPoints[0].y;
		int Right = pQuad->m_aPoints[0].x;

		for(int k = 1; k < 4; k++)
		{
			if(pQuad->m_aPoints[k].y < Top) Top = pQuad->m_aPoints[k].y;
			if(pQuad->m_aPoints[k].x < Left) Left = pQuad->m_aPoints[k].x;
			if(pQuad->m_aPoints[k].y > Bottom) Bottom = pQuad->m_aPoints[k].y;
			if(pQuad->m_aPoints[k].x > Right) Right = pQuad->m_aPoints[k].x;
		}

		pQuad->m_aPoints[0].x = Left; pQuad->m_aPoints[0].y = Top;
		pQuad->m_aPoints[1].x = Right; pQuad->m_aPoints[1].y = Top;
		pQuad->m_aPoints[2].x = Left; pQuad->m_aPoints[2].y = Bottom;
		pQuad->m_aPoints[3].x = Right; pQuad->m_aPoints[3].y = Bottom;
		pEditor->m_Map.m_Modified = true;
		return 1;
	}


	enum
	{
		PROP_POS_X=0,
		PROP_POS_Y,
		PROP_POS_ENV,
		PROP_POS_ENV_OFFSET,
		PROP_COLOR_ENV,
		PROP_COLOR_ENV_OFFSET,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Pos X", pQuad->m_aPoints[4].x/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", pQuad->m_aPoints[4].y/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos. Env", pQuad->m_PosEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Pos. TO", pQuad->m_PosEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Color Env", pQuad->m_ColorEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Color TO", pQuad->m_ColorEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_POS_X)
	{
		float Offset = NewVal*1000-pQuad->m_aPoints[4].x;
		for(int k = 0; k < 5; ++k)
			pQuad->m_aPoints[k].x += Offset;
	}
	if(Prop == PROP_POS_Y)
	{
		float Offset = NewVal*1000-pQuad->m_aPoints[4].y;
		for(int k = 0; k < 5; ++k)
			pQuad->m_aPoints[k].y += Offset;
	}
	if(Prop == PROP_POS_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pQuad->m_PosEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 3)
				{
					pQuad->m_PosEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_POS_ENV_OFFSET) pQuad->m_PosEnvOffset = NewVal;
	if(Prop == PROP_COLOR_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pQuad->m_ColorEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 4)
				{
					pQuad->m_ColorEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_COLOR_ENV_OFFSET) pQuad->m_ColorEnvOffset = NewVal;

	return 0;
}

int CEditor::PopupPoint(CEditor *pEditor, CUIRect View)
{
	CQuad *pQuad = pEditor->GetSelectedQuad();

	enum
	{
		PROP_POS_X=0,
		PROP_POS_Y,
		PROP_COLOR,
		NUM_PROPS,
	};

	int Color = 0;
	int x = 0, y = 0;

	for(int v = 0; v < 4; v++)
	{
		if(pEditor->m_SelectedPoints&(1<<v))
		{
			Color = 0;
			Color |= pQuad->m_aColors[v].r<<24;
			Color |= pQuad->m_aColors[v].g<<16;
			Color |= pQuad->m_aColors[v].b<<8;
			Color |= pQuad->m_aColors[v].a;

			x = pQuad->m_aPoints[v].x/1000;
			y = pQuad->m_aPoints[v].y/1000;
		}
	}


	CProperty aProps[] = {
		{"Pos X", x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Color", Color, PROPTYPE_COLOR, -1, pEditor->m_Map.m_lEnvelopes.size()},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_POS_X)
	{
		for(int v = 0; v < 4; v++)
			if(pEditor->m_SelectedPoints&(1<<v))
				pQuad->m_aPoints[v].x = NewVal*1000;
	}
	if(Prop == PROP_POS_Y)
	{
		for(int v = 0; v < 4; v++)
			if(pEditor->m_SelectedPoints&(1<<v))
				pQuad->m_aPoints[v].y = NewVal*1000;
	}
	if(Prop == PROP_COLOR)
	{
		for(int v = 0; v < 4; v++)
		{
			if(pEditor->m_SelectedPoints&(1<<v))
			{
				pQuad->m_aColors[v].r = (NewVal>>24)&0xff;
				pQuad->m_aColors[v].g = (NewVal>>16)&0xff;
				pQuad->m_aColors[v].b = (NewVal>>8)&0xff;
				pQuad->m_aColors[v].a = NewVal&0xff;
			}
		}
	}

	return 0;
}

int CEditor::PopupNewFolder(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Create new folder", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	if(pEditor->m_FileDialogErrString[0] == 0)
	{
		// interaction box
		View.HSplitBottom(40.0f, &View, 0);
		View.VMargin(40.0f, &View);
		View.HSplitBottom(20.0f, &View, &Label);
		static float s_FolderBox = 0;
		pEditor->DoEditBox(&s_FolderBox, &Label, pEditor->m_FileDialogNewFolderName, sizeof(pEditor->m_FileDialogNewFolderName), 15.0f, &s_FolderBox);
		View.HSplitBottom(20.0f, &View, &Label);
		pEditor->UI()->DoLabel(&Label, "Name:", 10.0f, -1);

		// button bar
		ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
		ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
		static int s_CreateButton = 0;
		if(pEditor->DoButton_Editor(&s_CreateButton, "Create", 0, &Label, 0, 0))
		{
			// create the folder
			if(*pEditor->m_FileDialogNewFolderName)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s/%s", pEditor->m_pFileDialogPath, pEditor->m_FileDialogNewFolderName);
				if(pEditor->Storage()->CreateFolder(aBuf, IStorage::TYPE_SAVE))
				{
					pEditor->FilelistPopulate(IStorage::TYPE_SAVE);
					return 1;
				}
				else
					str_copy(pEditor->m_FileDialogErrString, "Unable to create the folder", sizeof(pEditor->m_FileDialogErrString));
			}
		}
		ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
		ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
		static int s_AbortButton = 0;
		if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
			return 1;
	}
	else
	{
		// error text
		View.HSplitTop(30.0f, 0, &View);
		View.VMargin(40.0f, &View);
		View.HSplitTop(20.0f, &Label, &View);
		pEditor->UI()->DoLabel(&Label, "Error:", 10.0f, -1);
		View.HSplitTop(20.0f, &Label, &View);
		pEditor->UI()->DoLabel(&Label, "Unable to create the folder", 10.0f, -1, View.w);

		// button
		ButtonBar.VMargin(ButtonBar.w/2.0f-55.0f, &ButtonBar);
		static int s_CreateButton = 0;
		if(pEditor->DoButton_Editor(&s_CreateButton, "Ok", 0, &ButtonBar, 0, 0))
			return 1;
	}

	return 0;
}

int CEditor::PopupMapInfo(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar, Button;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Map details", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	View.VMargin(40.0f, &View);

	// author box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Author:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_AuthorBox = 0;
	pEditor->DoEditBox(&s_AuthorBox, &Button, pEditor->m_Map.m_MapInfo.m_aAuthorTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aAuthorTmp), 10.0f, &s_AuthorBox);

	// version box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Version:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_VersionBox = 0;
	pEditor->DoEditBox(&s_VersionBox, &Button, pEditor->m_Map.m_MapInfo.m_aVersionTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aVersionTmp), 10.0f, &s_VersionBox);

	// credits box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Credits:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_CreditsBox = 0;
	pEditor->DoEditBox(&s_CreditsBox, &Button, pEditor->m_Map.m_MapInfo.m_aCreditsTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aCreditsTmp), 10.0f, &s_CreditsBox);

	// license box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "License:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_LicenseBox = 0;
	pEditor->DoEditBox(&s_LicenseBox, &Button, pEditor->m_Map.m_MapInfo.m_aLicenseTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aLicenseTmp), 10.0f, &s_LicenseBox);

	// button bar
	ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
	static int s_CreateButton = 0;
	if(pEditor->DoButton_Editor(&s_CreateButton, "Save", 0, &Label, 0, 0))
	{
		str_copy(pEditor->m_Map.m_MapInfo.m_aAuthor, pEditor->m_Map.m_MapInfo.m_aAuthorTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aAuthor));
		str_copy(pEditor->m_Map.m_MapInfo.m_aVersion, pEditor->m_Map.m_MapInfo.m_aVersionTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aVersion));
		str_copy(pEditor->m_Map.m_MapInfo.m_aCredits, pEditor->m_Map.m_MapInfo.m_aCreditsTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aCredits));
		str_copy(pEditor->m_Map.m_MapInfo.m_aLicense, pEditor->m_Map.m_MapInfo.m_aLicenseTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aLicense));
		return 1;
	}

	ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
	static int s_AbortButton = 0;
	if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
		return 1;

	return 0;
}

int CEditor::PopupEvent(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	if(pEditor->m_PopupEventType == POPEVENT_EXIT)
		pEditor->UI()->DoLabel(&Label, "Exit the editor", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_LOAD)
		pEditor->UI()->DoLabel(&Label, "Load map", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		pEditor->UI()->DoLabel(&Label, "New map", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
		pEditor->UI()->DoLabel(&Label, "Save map", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	// notification text
	View.HSplitTop(30.0f, 0, &View);
	View.VMargin(40.0f, &View);
	View.HSplitTop(20.0f, &Label, &View);
	if(pEditor->m_PopupEventType == POPEVENT_EXIT)
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you exit the editor.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_LOAD)
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you load a new map.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you create a new map.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
		pEditor->UI()->DoLabel(&Label, "The file already exists.\nDo you want to overwrite the map?", 10.0f, -1);

	// button bar
	ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
	static int s_OkButton = 0;
	if(pEditor->DoButton_Editor(&s_OkButton, "Ok", 0, &Label, 0, 0))
	{
		if(pEditor->m_PopupEventType == POPEVENT_EXIT)
			g_Config.m_ClEditor = 0;
		else if(pEditor->m_PopupEventType == POPEVENT_LOAD)
			pEditor->InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_MAP, "Load map", "Load", "maps", "", pEditor->CallbackOpenMap, pEditor);
		else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		{
			pEditor->Reset();
			pEditor->m_aFileName[0] = 0;
		}
		else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
			pEditor->CallbackSaveMap(pEditor->m_aFileSaveName, IStorage::TYPE_SAVE, pEditor);
		pEditor->m_PopupEventWasActivated = false;
		return 1;
	}
	ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
	static int s_AbortButton = 0;
	if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
	{
		pEditor->m_PopupEventWasActivated = false;
		return 1;
	}

	return 0;
}


static int g_SelectImageSelected = -100;
static int g_SelectImageCurrent = -100;

int CEditor::PopupSelectImage(CEditor *pEditor, CUIRect View)
{
	CUIRect ButtonBar, ImageView;
	View.VSplitLeft(80.0f, &ButtonBar, &View);
	View.Margin(10.0f, &ImageView);

	int ShowImage = g_SelectImageCurrent;

	for(int i = -1; i < pEditor->m_Map.m_lImages.size(); i++)
	{
		CUIRect Button;
		ButtonBar.HSplitTop(12.0f, &Button, &ButtonBar);
		ButtonBar.HSplitTop(2.0f, 0, &ButtonBar);

		if(pEditor->UI()->MouseInside(&Button))
			ShowImage = i;

		if(i == -1)
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], "None", i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = -1;
		}
		else
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], pEditor->m_Map.m_lImages[i]->m_aName, i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = i;
		}
	}

	if(ShowImage >= 0 && ShowImage < pEditor->m_Map.m_lImages.size())
	{
		if(ImageView.h < ImageView.w)
			ImageView.w = ImageView.h;
		else
			ImageView.h = ImageView.w;
		float Max = (float)(max(pEditor->m_Map.m_lImages[ShowImage]->m_Width, pEditor->m_Map.m_lImages[ShowImage]->m_Height));
		ImageView.w *= pEditor->m_Map.m_lImages[ShowImage]->m_Width/Max;
		ImageView.h *= pEditor->m_Map.m_lImages[ShowImage]->m_Height/Max;
		pEditor->Graphics()->TextureSet(pEditor->m_Map.m_lImages[ShowImage]->m_TexID);
		pEditor->Graphics()->BlendNormal();
		pEditor->Graphics()->WrapClamp();
		pEditor->Graphics()->QuadsBegin();
		IGraphics::CQuadItem QuadItem(ImageView.x, ImageView.y, ImageView.w, ImageView.h);
		pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);
		pEditor->Graphics()->QuadsEnd();
		pEditor->Graphics()->WrapNormal();
	}

	return 0;
}

void CEditor::PopupSelectImageInvoke(int Current, float x, float y)
{
	static int s_SelectImagePopupId = 0;
	g_SelectImageSelected = -100;
	g_SelectImageCurrent = Current;
	UiInvokePopupMenu(&s_SelectImagePopupId, 0, x, y, 400, 300, PopupSelectImage);
}

int CEditor::PopupSelectImageResult()
{
	if(g_SelectImageSelected == -100)
		return -100;

	g_SelectImageCurrent = g_SelectImageSelected;
	g_SelectImageSelected = -100;
	return g_SelectImageCurrent;
}

static int s_GametileOpSelected = -1;

int CEditor::PopupSelectGametileOp(CEditor *pEditor, CUIRect View)
{
	static const char *s_pButtonNames[] = { "Clear", "Collision", "Death", "Unhookable" };
	static unsigned s_NumButtons = sizeof(s_pButtonNames) / sizeof(char*);
	CUIRect Button;

	for(unsigned i = 0; i < s_NumButtons; ++i)
	{
		View.HSplitTop(2.0f, 0, &View);
		View.HSplitTop(12.0f, &Button, &View);
		if(pEditor->DoButton_Editor(&s_pButtonNames[i], s_pButtonNames[i], 0, &Button, 0, 0))
			s_GametileOpSelected = i;
	}

	return 0;
}

void CEditor::PopupSelectGametileOpInvoke(float x, float y)
{
	static int s_SelectGametileOpPopupId = 0;
	s_GametileOpSelected = -1;
	UiInvokePopupMenu(&s_SelectGametileOpPopupId, 0, x, y, 120.0f, 70.0f, PopupSelectGametileOp);
}

int CEditor::PopupSelectGameTileOpResult()
{
	if(s_GametileOpSelected < 0)
		return -1;

	int Result = s_GametileOpSelected;
	s_GametileOpSelected = -1;
	return Result;
}

static int s_AutoMapConfigSelected = -1;

int CEditor::PopupSelectConfigAutoMap(CEditor *pEditor, CUIRect View)
{
	CLayerTiles *pLayer = static_cast<CLayerTiles*>(pEditor->GetSelectedLayer(0));
	CUIRect Button;
	static int s_AutoMapperConfigButtons[256];
	CAutoMapper *pAutoMapper = &pEditor->m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper;

	for(int i = 0; i < pAutoMapper->ConfigNamesNum(); ++i)
	{
		View.HSplitTop(2.0f, 0, &View);
		View.HSplitTop(12.0f, &Button, &View);
		if(pEditor->DoButton_Editor(&s_AutoMapperConfigButtons[i], pAutoMapper->GetConfigName(i), 0, &Button, 0, 0))
			s_AutoMapConfigSelected = i;
	}

	return 0;
}

void CEditor::PopupSelectConfigAutoMapInvoke(float x, float y)
{
	static int s_AutoMapConfigSelectID = 0;
	s_AutoMapConfigSelected = -1;
	CLayerTiles *pLayer = static_cast<CLayerTiles*>(GetSelectedLayer(0));
	if(pLayer && pLayer->m_Image >= 0 && pLayer->m_Image < m_Map.m_lImages.size() &&
		m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper.ConfigNamesNum())
		UiInvokePopupMenu(&s_AutoMapConfigSelectID, 0, x, y, 120.0f, 12.0f+14.0f*m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper.ConfigNamesNum(), PopupSelectConfigAutoMap);
}

int CEditor::PopupSelectConfigAutoMapResult()
{
	if(s_AutoMapConfigSelected < 0)
		return -1;

	int Result = s_AutoMapConfigSelected;
	s_AutoMapConfigSelected = -1;
	return Result;
}
