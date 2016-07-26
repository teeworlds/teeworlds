/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/color.h>
#include <base/math.h>

#include <engine/console.h>
#include <engine/graphics.h>

#include "editor.h"
#include <generated/client_data.h>
#include <game/client/localization.h>
#include <game/client/render.h>


CLayerEntities::CLayerEntities()
{
	m_Type = MODAPI_MAPLAYERTYPE_ENTITIES;
	str_copy(m_aName, "Entities", sizeof(m_aName));
	
	m_Resource = 0;
}

CLayerEntities::~CLayerEntities()
{
}

void CLayerEntities::Render()
{
	if(m_Resource < 0 || m_Resource >= m_pEditor->m_lEditorResources.size())
		return;
	
	Graphics()->BlendNormal();
	Graphics()->TextureSet(m_pEditor->m_lEditorResources[m_Resource].m_Texture);
	
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f,1.0f,1.0f,1.0f);
	
	for(int i=0; i<m_lEntityPoints.size(); i++)
	{
		CModAPI_MapEntity_Point *pt = &m_lEntityPoints[i];
	
		int SpriteX = pt->m_Type%16;
		int SpriteY = pt->m_Type/16;
	
		Graphics()->QuadsSetSubset(
			static_cast<float>(SpriteX)/16.0f,
			static_cast<float>(SpriteY)/16.0f,
			static_cast<float>(SpriteX+1)/16.0f,
			static_cast<float>(SpriteY+1)/16.0f);
	
		float x = pt->x;
		float y = pt->y;
				
		IGraphics::CQuadItem QuadItem(x, y, m_pEditor->m_WorldZoom * 32.0f, m_pEditor->m_WorldZoom * 32.0f);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();
}

CModAPI_MapEntity_Point *CLayerEntities::NewPoint()
{
	m_pEditor->m_Map.m_Modified = true;

	CModAPI_MapEntity_Point *pt = &m_lEntityPoints[m_lEntityPoints.add(CModAPI_MapEntity_Point())];

	pt->x = 0;
	pt->y = 0;
	pt->m_Type = -1;

	return pt;
}

void CLayerEntities::BrushSelecting(CUIRect Rect)
{
	// draw selection rectangle
	vec4 RectColor = HexToRgba(g_Config.m_EdColorSelectionQuad);
	IGraphics::CLineItem Array[4] = {
		IGraphics::CLineItem(Rect.x, Rect.y, Rect.x+Rect.w, Rect.y),
		IGraphics::CLineItem(Rect.x+Rect.w, Rect.y, Rect.x+Rect.w, Rect.y+Rect.h),
		IGraphics::CLineItem(Rect.x+Rect.w, Rect.y+Rect.h, Rect.x, Rect.y+Rect.h),
		IGraphics::CLineItem(Rect.x, Rect.y+Rect.h, Rect.x, Rect.y)};
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(RectColor.r*RectColor.a, RectColor.g*RectColor.a, RectColor.b*RectColor.a, RectColor.a);
	Graphics()->LinesDraw(Array, 4);
	Graphics()->LinesEnd();
}

int CLayerEntities::BrushGrab(CLayerGroup *pBrush, CUIRect Rect)
{
	return 0;
}

void CLayerEntities::BrushPlace(CLayer *pBrush, float wx, float wy)
{
	
}

void CLayerEntities::BrushFlipX()
{
	
}

void CLayerEntities::BrushFlipY()
{
	
}

void CLayerEntities::BrushRotate(float Amount)
{
	
}

void CLayerEntities::GetSize(float *w, float *h) const
{
	*w = 0; *h = 0;
}

int CLayerEntities::RenderProperties(CUIRect *pToolBox)
{
	// layer props
	//~ enum
	//~ {
		//~ PROP_TYPE=0,
		//~ NUM_PROPS,
	//~ };

	//~ CProperty aProps[] = {
		//~ {"Type", -1, PROPTYPE_ENTITY, -1, 0},
		//~ {0},
	//~ };

	//~ static int s_aIds[NUM_PROPS] = {0};
	//~ int NewVal = 0;
	//~ int Prop = m_pEditor->DoProperties(pToolBox, aProps, s_aIds, &NewVal);
	//~ if(Prop != -1)
		//~ m_pEditor->m_Map.m_Modified = true;

	//~ if(Prop == PROP_TYPE)
	//~ {
		//~ if(NewVal >= 0)
			//~ m_Image = NewVal%m_pEditor->m_Map.m_lImages.size();
		//~ else
			//~ m_Image = -1;
	//~ }

	return 0;
}


void CLayerEntities::ModifyImageIndex(INDEX_MODIFY_FUNC Func)
{
	
}

void CLayerEntities::ModifyEnvelopeIndex(INDEX_MODIFY_FUNC Func)
{
	
}
