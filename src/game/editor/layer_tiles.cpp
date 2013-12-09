/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/generated/client_data.h>
#include <game/client/render.h>
#include "editor.h"

#include <game/localization.h>

CLayerTiles::CLayerTiles(int w, int h)
{
	m_Type = LAYERTYPE_TILES;
	str_copy(m_aName, "Tiles", sizeof(m_aName));
	m_Width = w;
	m_Height = h;
	m_Image = -1;
	m_TexID = -1;
	m_Game = 0;
	m_Color.r = 255;
	m_Color.g = 255;
	m_Color.b = 255;
	m_Color.a = 255;
	m_ColorEnv = -1;
	m_ColorEnvOffset = 0;

	m_pTiles = new CTile[m_Width*m_Height];
	mem_zero(m_pTiles, m_Width*m_Height*sizeof(CTile));
}

CLayerTiles::~CLayerTiles()
{
	delete [] m_pTiles;
}

void CLayerTiles::PrepareForSave()
{
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
			m_pTiles[y*m_Width+x].m_Flags &= TILEFLAG_VFLIP|TILEFLAG_HFLIP|TILEFLAG_ROTATE;

	if(m_Image != -1 && m_Color.a == 255)
	{
		for(int y = 0; y < m_Height; y++)
			for(int x = 0; x < m_Width; x++)
				m_pTiles[y*m_Width+x].m_Flags |= m_pEditor->m_Map.m_lImages[m_Image]->m_aTileFlags[m_pTiles[y*m_Width+x].m_Index];
	}
}

void CLayerTiles::MakePalette()
{
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
			m_pTiles[y*m_Width+x].m_Index = y*16+x;
}

void CLayerTiles::Render()
{
	if(m_Image >= 0 && m_Image < m_pEditor->m_Map.m_lImages.size())
		m_TexID = m_pEditor->m_Map.m_lImages[m_Image]->m_TexID;
	Graphics()->TextureSet(m_TexID);
	vec4 Color = vec4(m_Color.r/255.0f, m_Color.g/255.0f, m_Color.b/255.0f, m_Color.a/255.0f);
	Graphics()->BlendNone();
	m_pEditor->RenderTools()->RenderTilemap(m_pTiles, m_Width, m_Height, 32.0f, Color, LAYERRENDERFLAG_OPAQUE,
												m_pEditor->EnvelopeEval, m_pEditor, m_ColorEnv, m_ColorEnvOffset);
	Graphics()->BlendNormal();
	m_pEditor->RenderTools()->RenderTilemap(m_pTiles, m_Width, m_Height, 32.0f, Color, LAYERRENDERFLAG_TRANSPARENT,
												m_pEditor->EnvelopeEval, m_pEditor, m_ColorEnv, m_ColorEnvOffset);
}

int CLayerTiles::ConvertX(float x) const { return (int)(x/32.0f); }
int CLayerTiles::ConvertY(float y) const { return (int)(y/32.0f); }

void CLayerTiles::Convert(CUIRect Rect, RECTi *pOut)
{
	pOut->x = ConvertX(Rect.x);
	pOut->y = ConvertY(Rect.y);
	pOut->w = ConvertX(Rect.x+Rect.w+31) - pOut->x;
	pOut->h = ConvertY(Rect.y+Rect.h+31) - pOut->y;
}

void CLayerTiles::Snap(CUIRect *pRect)
{
	RECTi Out;
	Convert(*pRect, &Out);
	pRect->x = Out.x*32.0f;
	pRect->y = Out.y*32.0f;
	pRect->w = Out.w*32.0f;
	pRect->h = Out.h*32.0f;
}

void CLayerTiles::Clamp(RECTi *pRect)
{
	if(pRect->x < 0)
	{
		pRect->w += pRect->x;
		pRect->x = 0;
	}

	if(pRect->y < 0)
	{
		pRect->h += pRect->y;
		pRect->y = 0;
	}

	if(pRect->x+pRect->w > m_Width)
		pRect->w = m_Width - pRect->x;

	if(pRect->y+pRect->h > m_Height)
		pRect->h = m_Height - pRect->y;

	if(pRect->h < 0)
		pRect->h = 0;
	if(pRect->w < 0)
		pRect->w = 0;
}

void CLayerTiles::BrushSelecting(CUIRect Rect)
{
	Graphics()->TextureSet(-1);
	m_pEditor->Graphics()->QuadsBegin();
	m_pEditor->Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.4f);
	Snap(&Rect);
	IGraphics::CQuadItem QuadItem(Rect.x, Rect.y, Rect.w, Rect.h);
	m_pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);
	m_pEditor->Graphics()->QuadsEnd();
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d,%d", ConvertX(Rect.w), ConvertY(Rect.h));
	TextRender()->Text(0, Rect.x+3.0f, Rect.y+3.0f, m_pEditor->m_ShowPicker?15.0f:15.0f*m_pEditor->m_WorldZoom, aBuf, -1);
}

int CLayerTiles::BrushGrab(CLayerGroup *pBrush, CUIRect Rect)
{
	RECTi r;
	Convert(Rect, &r);
	Clamp(&r);

	if(!r.w || !r.h)
		return 0;

	// create new layers
	CLayerTiles *pGrabbed = new CLayerTiles(r.w, r.h);
	pGrabbed->m_pEditor = m_pEditor;
	pGrabbed->m_TexID = m_TexID;
	pGrabbed->m_Image = m_Image;
	pGrabbed->m_Game = m_Game;
	pBrush->AddLayer(pGrabbed);

	// copy the tiles
	for(int y = 0; y < r.h; y++)
		for(int x = 0; x < r.w; x++)
			pGrabbed->m_pTiles[y*pGrabbed->m_Width+x] = m_pTiles[(r.y+y)*m_Width+(r.x+x)];

	return 1;
}

void CLayerTiles::FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect)
{
	if(m_Readonly)
		return;

	Snap(&Rect);

	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);

	CLayerTiles *pLt = static_cast<CLayerTiles*>(pBrush);

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			int fx = x+sx;
			int fy = y+sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(Empty)
				m_pTiles[fy*m_Width+fx].m_Index = 1;
			else
				m_pTiles[fy*m_Width+fx] = pLt->m_pTiles[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)];
		}
	}
	m_pEditor->m_Map.m_Modified = true;
}

void CLayerTiles::BrushDraw(CLayer *pBrush, float wx, float wy)
{
	if(m_Readonly)
		return;

	//
	CLayerTiles *l = (CLayerTiles *)pBrush;
	int sx = ConvertX(wx);
	int sy = ConvertY(wy);

	for(int y = 0; y < l->m_Height; y++)
		for(int x = 0; x < l->m_Width; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			if(fx<0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			m_pTiles[fy*m_Width+fx] = l->m_pTiles[y*l->m_Width+x];
		}
	m_pEditor->m_Map.m_Modified = true;
}

void CLayerTiles::BrushFlipX()
{
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width/2; x++)
		{
			CTile Tmp = m_pTiles[y*m_Width+x];
			m_pTiles[y*m_Width+x] = m_pTiles[y*m_Width+m_Width-1-x];
			m_pTiles[y*m_Width+m_Width-1-x] = Tmp;
		}

	if(!m_Game)
		for(int y = 0; y < m_Height; y++)
			for(int x = 0; x < m_Width; x++)
				m_pTiles[y*m_Width+x].m_Flags ^= m_pTiles[y*m_Width+x].m_Flags&TILEFLAG_ROTATE ? TILEFLAG_HFLIP : TILEFLAG_VFLIP;
}

void CLayerTiles::BrushFlipY()
{
	for(int y = 0; y < m_Height/2; y++)
		for(int x = 0; x < m_Width; x++)
		{
			CTile Tmp = m_pTiles[y*m_Width+x];
			m_pTiles[y*m_Width+x] = m_pTiles[(m_Height-1-y)*m_Width+x];
			m_pTiles[(m_Height-1-y)*m_Width+x] = Tmp;
		}

	if(!m_Game)
		for(int y = 0; y < m_Height; y++)
			for(int x = 0; x < m_Width; x++)
				m_pTiles[y*m_Width+x].m_Flags ^= m_pTiles[y*m_Width+x].m_Flags&TILEFLAG_ROTATE ? TILEFLAG_VFLIP : TILEFLAG_HFLIP;
}

void CLayerTiles::BrushRotate(float Amount)
{
	int Rotation = (round_to_int(360.0f*Amount/(pi*2))/90)%4;	// 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation +=4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation
		CTile *pTempData = new CTile[m_Width*m_Height];
		mem_copy(pTempData, m_pTiles, m_Width*m_Height*sizeof(CTile));
		CTile *pDst = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height-1; y >= 0; --y, ++pDst)
			{
				*pDst = pTempData[y*m_Width+x];
				if(!m_Game)
				{
					if(pDst->m_Flags&TILEFLAG_ROTATE)
						pDst->m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP);
					pDst->m_Flags ^= TILEFLAG_ROTATE;
				}
			}

		int Temp = m_Width;
		m_Width = m_Height;
		m_Height = Temp;
		delete[] pTempData;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerTiles::Resize(int NewW, int NewH)
{
	CTile *pNewData = new CTile[NewW*NewH];
	mem_zero(pNewData, NewW*NewH*sizeof(CTile));

	// copy old data
	for(int y = 0; y < min(NewH, m_Height); y++)
		mem_copy(&pNewData[y*NewW], &m_pTiles[y*m_Width], min(m_Width, NewW)*sizeof(CTile));

	// replace old
	delete [] m_pTiles;
	m_pTiles = pNewData;
	m_Width = NewW;
	m_Height = NewH;
}

void CLayerTiles::Shift(int Direction)
{
	switch(Direction)
	{
	case 1:
		{
			// left
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pTiles[y*m_Width], &m_pTiles[y*m_Width+1], (m_Width-1)*sizeof(CTile));
		}
		break;
	case 2:
		{
			// right
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pTiles[y*m_Width+1], &m_pTiles[y*m_Width], (m_Width-1)*sizeof(CTile));
		}
		break;
	case 4:
		{
			// up
			for(int y = 0; y < m_Height-1; ++y)
				mem_copy(&m_pTiles[y*m_Width], &m_pTiles[(y+1)*m_Width], m_Width*sizeof(CTile));
		}
		break;
	case 8:
		{
			// down
			for(int y = m_Height-1; y > 0; --y)
				mem_copy(&m_pTiles[y*m_Width], &m_pTiles[(y-1)*m_Width], m_Width*sizeof(CTile));
		}
	}
}

void CLayerTiles::ShowInfo()
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->TextureSet(m_pEditor->Client()->GetDebugFont());
	Graphics()->QuadsBegin();

	int StartY = max(0, (int)(ScreenY0/32.0f)-1);
	int StartX = max(0, (int)(ScreenX0/32.0f)-1);
	int EndY = min((int)(ScreenY1/32.0f)+1, m_Height);
	int EndX = min((int)(ScreenX1/32.0f)+1, m_Width);

	for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
			int c = x + y*m_Width;
			if(m_pTiles[c].m_Index)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "%i", m_pTiles[c].m_Index);
				m_pEditor->Graphics()->QuadsText(x*32, y*32, 16.0f, aBuf);

				char aFlags[4] = {	m_pTiles[c].m_Flags&TILEFLAG_VFLIP ? 'V' : ' ',
									m_pTiles[c].m_Flags&TILEFLAG_HFLIP ? 'H' : ' ',
									m_pTiles[c].m_Flags&TILEFLAG_ROTATE? 'R' : ' ',
									0};
				m_pEditor->Graphics()->QuadsText(x*32, y*32+16, 16.0f, aFlags);
			}
			x += m_pTiles[c].m_Skip;
		}

	Graphics()->QuadsEnd();
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

int CLayerTiles::RenderProperties(CUIRect *pToolBox)
{
	CUIRect Button;

	bool InGameGroup = !find_linear(m_pEditor->m_Map.m_pGameGroup->m_lLayers.all(), this).empty();
	if(m_pEditor->m_Map.m_pGameLayer != this)
	{
		if(m_Image >= 0 && m_Image < m_pEditor->m_Map.m_lImages.size() && m_pEditor->m_Map.m_lImages[m_Image]->m_AutoMapper.IsLoaded())
		{
			static int s_AutoMapperButton = 0;
			pToolBox->HSplitBottom(12.0f, pToolBox, &Button);
			if(m_pEditor->DoButton_Editor(&s_AutoMapperButton, "Auto map", 0, &Button, 0, ""))
				m_pEditor->PopupSelectConfigAutoMapInvoke(m_pEditor->UI()->MouseX(), m_pEditor->UI()->MouseY());

			int Result = m_pEditor->PopupSelectConfigAutoMapResult();
			if(Result > -1)
			{
				m_pEditor->m_Map.m_lImages[m_Image]->m_AutoMapper.Proceed(this, Result);
				return 1;
			}
		}
	}
	else
		InGameGroup = false;

	if(InGameGroup)
	{
		pToolBox->HSplitBottom(2.0f, pToolBox, 0);
		pToolBox->HSplitBottom(12.0f, pToolBox, &Button);
		static int s_ColclButton = 0;
		if(m_pEditor->DoButton_Editor(&s_ColclButton, "Game tiles", 0, &Button, 0, "Constructs game tiles from this layer"))
			m_pEditor->PopupSelectGametileOpInvoke(m_pEditor->UI()->MouseX(), m_pEditor->UI()->MouseY());

		int Result = m_pEditor->PopupSelectGameTileOpResult();
		if(Result > -1)
		{
			CLayerTiles *gl = m_pEditor->m_Map.m_pGameLayer;
			int w = min(gl->m_Width, m_Width);
			int h = min(gl->m_Height, m_Height);
			for(int y = 0; y < h; y++)
				for(int x = 0; x < w; x++)
					if(m_pTiles[y*m_Width+x].m_Index)
						gl->m_pTiles[y*gl->m_Width+x].m_Index = TILE_AIR+Result;

			return 1;
		}
	}

	enum
	{
		PROP_WIDTH=0,
		PROP_HEIGHT,
		PROP_SHIFT,
		PROP_IMAGE,
		PROP_COLOR,
		PROP_COLOR_ENV,
		PROP_COLOR_ENV_OFFSET,
		NUM_PROPS,
	};

	int Color = 0;
	Color |= m_Color.r<<24;
	Color |= m_Color.g<<16;
	Color |= m_Color.b<<8;
	Color |= m_Color.a;

	CProperty aProps[] = {
		{"Width", m_Width, PROPTYPE_INT_SCROLL, 1, 1000000000},
		{"Height", m_Height, PROPTYPE_INT_SCROLL, 1, 1000000000},
		{"Shift", 0, PROPTYPE_SHIFT, 0, 0},
		{"Image", m_Image, PROPTYPE_IMAGE, 0, 0},
		{"Color", Color, PROPTYPE_COLOR, 0, 0},
		{"Color Env", m_ColorEnv+1, PROPTYPE_INT_STEP, 0, m_pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Color TO", m_ColorEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{0},
	};

	if(m_pEditor->m_Map.m_pGameLayer == this) // remove the image and color properties if this is the game layer
	{
		aProps[3].m_pName = 0;
		aProps[4].m_pName = 0;
	}

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = m_pEditor->DoProperties(pToolBox, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		m_pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_WIDTH && NewVal > 1)
		Resize(NewVal, m_Height);
	else if(Prop == PROP_HEIGHT && NewVal > 1)
		Resize(m_Width, NewVal);
	else if(Prop == PROP_SHIFT)
		Shift(NewVal);
	else if(Prop == PROP_IMAGE)
	{
		if (NewVal == -1)
		{
			m_TexID = -1;
			m_Image = -1;
		}
		else
			m_Image = NewVal%m_pEditor->m_Map.m_lImages.size();
	}
	else if(Prop == PROP_COLOR)
	{
		m_Color.r = (NewVal>>24)&0xff;
		m_Color.g = (NewVal>>16)&0xff;
		m_Color.b = (NewVal>>8)&0xff;
		m_Color.a = NewVal&0xff;
	}
	if(Prop == PROP_COLOR_ENV)
	{
		int Index = clamp(NewVal-1, -1, m_pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-m_ColorEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < m_pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || m_pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 4)
				{
					m_ColorEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_COLOR_ENV_OFFSET)
		m_ColorEnvOffset = NewVal;

	return 0;
}


void CLayerTiles::ModifyImageIndex(INDEX_MODIFY_FUNC Func)
{
	Func(&m_Image);
}

void CLayerTiles::ModifyEnvelopeIndex(INDEX_MODIFY_FUNC Func)
{
}
