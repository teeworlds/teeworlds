/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/console.h>
#include <engine/graphics.h>

#include "editor.h"
#include <game/generated/client_data.h>
#include <game/client/localization.h>
#include <game/client/render.h>


CLayerQuads::CLayerQuads()
{
	m_Type = LAYERTYPE_QUADS;
	str_copy(m_aName, "Quads", sizeof(m_aName));
	m_Image = -1;
}

CLayerQuads::~CLayerQuads()
{
}

void CLayerQuads::Render()
{
	Graphics()->TextureClear();
	if(m_Image >= 0 && m_Image < m_pEditor->m_Map.m_lImages.size())
		Graphics()->TextureSet(m_pEditor->m_Map.m_lImages[m_Image]->m_Texture);

	Graphics()->BlendNone();
	m_pEditor->RenderTools()->RenderQuads(m_lQuads.base_ptr(), m_lQuads.size(), LAYERRENDERFLAG_OPAQUE, m_pEditor->EnvelopeEval, m_pEditor);
	Graphics()->BlendNormal();
	m_pEditor->RenderTools()->RenderQuads(m_lQuads.base_ptr(), m_lQuads.size(), LAYERRENDERFLAG_TRANSPARENT, m_pEditor->EnvelopeEval, m_pEditor);
}

CQuad *CLayerQuads::NewQuad()
{
	m_pEditor->m_Map.m_Modified = true;

	CQuad *q = &m_lQuads[m_lQuads.add(CQuad())];

	q->m_PosEnv = -1;
	q->m_ColorEnv = -1;
	q->m_PosEnvOffset = 0;
	q->m_ColorEnvOffset = 0;
	int x = 0, y = 0;
	q->m_aPoints[0].x = x;
	q->m_aPoints[0].y = y;
	q->m_aPoints[1].x = x+64;
	q->m_aPoints[1].y = y;
	q->m_aPoints[2].x = x;
	q->m_aPoints[2].y = y+64;
	q->m_aPoints[3].x = x+64;
	q->m_aPoints[3].y = y+64;

	q->m_aPoints[4].x = x+32; // pivot
	q->m_aPoints[4].y = y+32;

	for(int i = 0; i < 5; i++)
	{
		q->m_aPoints[i].x <<= 10;
		q->m_aPoints[i].y <<= 10;
	}


	q->m_aTexcoords[0].x = 0;
	q->m_aTexcoords[0].y = 0;

	q->m_aTexcoords[1].x = 1<<10;
	q->m_aTexcoords[1].y = 0;

	q->m_aTexcoords[2].x = 0;
	q->m_aTexcoords[2].y = 1<<10;

	q->m_aTexcoords[3].x = 1<<10;
	q->m_aTexcoords[3].y = 1<<10;

	q->m_aColors[0].r = 255; q->m_aColors[0].g = 255; q->m_aColors[0].b = 255; q->m_aColors[0].a = 255;
	q->m_aColors[1].r = 255; q->m_aColors[1].g = 255; q->m_aColors[1].b = 255; q->m_aColors[1].a = 255;
	q->m_aColors[2].r = 255; q->m_aColors[2].g = 255; q->m_aColors[2].b = 255; q->m_aColors[2].a = 255;
	q->m_aColors[3].r = 255; q->m_aColors[3].g = 255; q->m_aColors[3].b = 255; q->m_aColors[3].a = 255;

	return q;
}

void CLayerQuads::BrushSelecting(CUIRect Rect)
{
	// draw selection rectangle
	IGraphics::CLineItem Array[4] = {
		IGraphics::CLineItem(Rect.x, Rect.y, Rect.x+Rect.w, Rect.y),
		IGraphics::CLineItem(Rect.x+Rect.w, Rect.y, Rect.x+Rect.w, Rect.y+Rect.h),
		IGraphics::CLineItem(Rect.x+Rect.w, Rect.y+Rect.h, Rect.x, Rect.y+Rect.h),
		IGraphics::CLineItem(Rect.x, Rect.y+Rect.h, Rect.x, Rect.y)};
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->LinesDraw(Array, 4);
	Graphics()->LinesEnd();
}

int CLayerQuads::BrushGrab(CLayerGroup *pBrush, CUIRect Rect)
{
	// create new layers
	CLayerQuads *pGrabbed = new CLayerQuads();
	pGrabbed->m_pEditor = m_pEditor;
	pGrabbed->m_Image = m_Image;
	pBrush->AddLayer(pGrabbed);

	//dbg_msg("", "%f %f %f %f", rect.x, rect.y, rect.w, rect.h);
	for(int i = 0; i < m_lQuads.size(); i++)
	{
		CQuad *q = &m_lQuads[i];
		float px = fx2f(q->m_aPoints[4].x);
		float py = fx2f(q->m_aPoints[4].y);

		if(px > Rect.x && px < Rect.x+Rect.w && py > Rect.y && py < Rect.y+Rect.h)
		{
			CQuad n;
			n = *q;

			for(int p = 0; p < 5; p++)
			{
				n.m_aPoints[p].x -= f2fx(Rect.x);
				n.m_aPoints[p].y -= f2fx(Rect.y);
			}

			pGrabbed->m_lQuads.add(n);
		}
	}

	return pGrabbed->m_lQuads.size()?1:0;
}

void CLayerQuads::BrushPlace(CLayer *pBrush, float wx, float wy)
{
	CLayerQuads *l = (CLayerQuads *)pBrush;
	for(int i = 0; i < l->m_lQuads.size(); i++)
	{
		CQuad n = l->m_lQuads[i];

		for(int p = 0; p < 5; p++)
		{
			n.m_aPoints[p].x += f2fx(wx);
			n.m_aPoints[p].y += f2fx(wy);
		}

		m_lQuads.add(n);
	}
	m_pEditor->m_Map.m_Modified = true;
}

void CLayerQuads::BrushFlipX()
{
}

void CLayerQuads::BrushFlipY()
{
}

void Rotate(vec2 *pCenter, vec2 *pPoint, float Rotation)
{
	float x = pPoint->x - pCenter->x;
	float y = pPoint->y - pCenter->y;
	pPoint->x = x * cosf(Rotation) - y * sinf(Rotation) + pCenter->x;
	pPoint->y = x * sinf(Rotation) + y * cosf(Rotation) + pCenter->y;
}

void CLayerQuads::BrushRotate(float Amount)
{
	vec2 Center;
	GetSize(&Center.x, &Center.y);
	Center.x /= 2;
	Center.y /= 2;

	for(int i = 0; i < m_lQuads.size(); i++)
	{
		CQuad *q = &m_lQuads[i];

		for(int p = 0; p < 5; p++)
		{
			vec2 Pos(fx2f(q->m_aPoints[p].x), fx2f(q->m_aPoints[p].y));
			Rotate(&Center, &Pos, Amount);
			q->m_aPoints[p].x = f2fx(Pos.x);
			q->m_aPoints[p].y = f2fx(Pos.y);
		}
	}
}

void CLayerQuads::GetSize(float *w, float *h)
{
	*w = 0; *h = 0;

	for(int i = 0; i < m_lQuads.size(); i++)
	{
		for(int p = 0; p < 5; p++)
		{
			*w = max(*w, fx2f(m_lQuads[i].m_aPoints[p].x));
			*h = max(*h, fx2f(m_lQuads[i].m_aPoints[p].y));
		}
	}
}

extern int gs_SelectedPoints;

int CLayerQuads::RenderProperties(CUIRect *pToolBox)
{
	// layer props
	enum
	{
		PROP_IMAGE=0,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Image", m_Image, PROPTYPE_IMAGE, -1, 0},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = m_pEditor->DoProperties(pToolBox, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		m_pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_IMAGE)
	{
		if(NewVal >= 0)
			m_Image = NewVal%m_pEditor->m_Map.m_lImages.size();
		else
			m_Image = -1;
	}

	return 0;
}


void CLayerQuads::ModifyImageIndex(INDEX_MODIFY_FUNC Func)
{
	Func(&m_Image);
}

void CLayerQuads::ModifyEnvelopeIndex(INDEX_MODIFY_FUNC Func)
{
	for(int i = 0; i < m_lQuads.size(); i++)
	{
		Func(&m_lQuads[i].m_PosEnv);
		Func(&m_lQuads[i].m_ColorEnv);
	}
}
