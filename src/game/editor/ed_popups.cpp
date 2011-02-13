/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include "ed_editor.h"

#include <game/localization.h>

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
			g_UiNumPopups--;
			
		if(Input()->KeyDown(KEY_ESCAPE))
			g_UiNumPopups--;
	}
}


int CEditor::PopupGroup(CEditor *pEditor, CUIRect View)
{
	// remove group button
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;
	
	// don't allow deletion of game group
	if(pEditor->m_Map.m_pGameGroup != pEditor->GetSelectedGroup() &&
		pEditor->DoButton_Editor(&s_DeleteButton, Localize("Delete group"), 0, &Button, 0, Localize("Delete group")))
	{
		pEditor->m_Map.DeleteGroup(pEditor->m_SelectedGroup);
		pEditor->m_SelectedGroup = max(0, pEditor->m_SelectedGroup-1);
		return 1;
	}

	// new tile layer
	View.HSplitBottom(10.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewQuadLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewQuadLayerButton, Localize("Add quads layer"), 0, &Button, 0, Localize("Creates a new quad layer")))
	{
		CLayer *l = new CLayerQuads;
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		return 1;
	}

	// new quad layer
	View.HSplitBottom(5.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewTileLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewTileLayerButton, Localize("Add tile layer"), 0, &Button, 0, Localize("Creates a new tile layer")))
	{
		CLayer *l = new CLayerTiles(50, 50);
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		return 1;
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
		{Localize("Order"), pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{Localize("Pos X"), -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Pos Y"), -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Para X"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Para Y"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxY, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{Localize("Use Clipping"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_UseClipping, PROPTYPE_BOOL, 0, 1},
		{Localize("Clip X"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Clip Y"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Clip W"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipW, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Clip H"), pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipH, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{0},
	};
	
	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	
	// cut the properties that isn't needed
	if(pEditor->GetSelectedGroup()->m_GameGroup)
		aProps[PROP_POS_X].m_pName = 0;
		
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
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
		pEditor->DoButton_Editor(&s_DeleteButton, Localize("Delete layer"), 0, &Button, 0, Localize("Deletes the layer")))
	{
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->DeleteLayer(pEditor->m_SelectedLayer);
		return 1;
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
		{Localize("Group"), pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{Localize("Order"), pEditor->m_SelectedLayer, PROPTYPE_INT_STEP, 0, pCurrentGroup->m_lLayers.size()},
		{Localize("Detail"), pCurrentLayer->m_Flags&LAYERFLAG_DETAIL, PROPTYPE_BOOL, 0, 1},
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
	if(pEditor->DoButton_Editor(&s_DeleteButton, Localize("Delete"), 0, &Button, 0, Localize("Deletes the current quad")))
	{
		CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
		if(pLayer)
		{
			pLayer->m_lQuads.remove_index(pEditor->m_SelectedQuad);
			pEditor->m_SelectedQuad--;
		}
		return 1;
	}

	View.HSplitBottom(10.0f, &View, &Button);

	// aspect ratio button
	CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
	if(pLayer && pLayer->m_Image >= 0 && pLayer->m_Image < pEditor->m_Map.m_lImages.size())
	{
		View.HSplitBottom(12.0f, &View, &Button);
		static int s_AspectRatioButton = 0;
		if(pEditor->DoButton_Editor(&s_AspectRatioButton, Localize("Aspect ratio"), 0, &Button, 0, Localize("Resizes the current Quad based on the aspect ratio of the image")))
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
			return 1;
		}
		View.HSplitBottom(6.0f, &View, &Button);
		
	}

	// square button
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_Button = 0;
	if(pEditor->DoButton_Editor(&s_Button, Localize("Square"), 0, &Button, 0, Localize("Squares the current quad")))
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
		return 1;
	}


	enum
	{
		PROP_POS_ENV=0,
		PROP_POS_ENV_OFFSET,
		PROP_COLOR_ENV,
		PROP_COLOR_ENV_OFFSET,
		NUM_PROPS,
	};
	
	CProperty aProps[] = {
		{Localize("Pos. Env"), pQuad->m_PosEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{Localize("Pos. TO"), pQuad->m_PosEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{Localize("Color Env"), pQuad->m_ColorEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{Localize("Color TO"), pQuad->m_ColorEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		
		{0},
	};
	
	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);		
	
	if(Prop == PROP_POS_ENV) pQuad->m_PosEnv = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
	if(Prop == PROP_POS_ENV_OFFSET) pQuad->m_PosEnvOffset = NewVal;
	if(Prop == PROP_COLOR_ENV) pQuad->m_ColorEnv = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
	if(Prop == PROP_COLOR_ENV_OFFSET) pQuad->m_ColorEnvOffset = NewVal;
	
	return 0;
}

int CEditor::PopupPoint(CEditor *pEditor, CUIRect View)
{
	CQuad *pQuad = pEditor->GetSelectedQuad();
	
	enum
	{
		PROP_COLOR=0,
		NUM_PROPS,
	};
	
	int Color = 0;

	for(int v = 0; v < 4; v++)
	{
		if(pEditor->m_SelectedPoints&(1<<v))
		{
			Color = 0;
			Color |= pQuad->m_aColors[v].r<<24;
			Color |= pQuad->m_aColors[v].g<<16;
			Color |= pQuad->m_aColors[v].b<<8;
			Color |= pQuad->m_aColors[v].a;
		}
	}
	
	
	CProperty aProps[] = {
		{Localize("Color"), Color, PROPTYPE_COLOR, -1, pEditor->m_Map.m_lEnvelopes.size()},
		{0},
	};
	
	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);		
	if(Prop == PROP_COLOR)
	{
		for(int v = 0; v < 4; v++)
		{
			if(pEditor->m_SelectedPoints&(1<<v))
			{
				Color = 0;
				pQuad->m_aColors[v].r = (NewVal>>24)&0xff;
				pQuad->m_aColors[v].g = (NewVal>>16)&0xff;
				pQuad->m_aColors[v].b = (NewVal>>8)&0xff;
				pQuad->m_aColors[v].a = NewVal&0xff;
			}
		}
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
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], Localize("None"), i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = -1;
		}
		else
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], pEditor->m_Map.m_lImages[i]->m_aName, i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = i;
		}
	}
	
	if(ShowImage >= 0 && ShowImage < pEditor->m_Map.m_lImages.size())
		pEditor->Graphics()->TextureSet(pEditor->m_Map.m_lImages[ShowImage]->m_TexID);
	else
		pEditor->Graphics()->TextureSet(-1);
	pEditor->Graphics()->QuadsBegin();
	IGraphics::CQuadItem QuadItem(ImageView.x, ImageView.y, ImageView.w, ImageView.h);
	pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);
	pEditor->Graphics()->QuadsEnd();

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
	/*	This is for scripts/update_localization.py to work, don't remove!
		Localize("Clear"); Localize("Collision"); Localize("Death"); Localize("Unhookable"); */
	static const char *s_pButtonNames[] = { "Clear", "Collision", "Death", "Unhookable" };
	static unsigned s_NumButtons = sizeof(s_pButtonNames) / sizeof(char*);
	CUIRect Button;

	for(unsigned i = 0; i < s_NumButtons; ++i)
	{
		View.HSplitTop(2.0f, 0, &View);
		View.HSplitTop(12.0f, &Button, &View);
		if(pEditor->DoButton_Editor(&s_pButtonNames[i], Localize(s_pButtonNames[i]), 0, &Button, 0, 0))
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
