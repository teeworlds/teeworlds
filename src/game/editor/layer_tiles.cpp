/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/input.h>
#include <engine/keys.h>

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

	m_Tele = 0;
	m_Speedup = 0;

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
	if(m_Tele)
		m_pEditor->RenderTools()->RenderTelemap(((CLayerTele*)this)->m_pTeleTile, m_Width, m_Height, 32.0f);
	if(m_Speedup)
		m_pEditor->RenderTools()->RenderSpeedupmap(((CLayerSpeedup*)this)->m_pSpeedupTile, m_Width, m_Height, 32.0f);
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
	if(m_pEditor->GetSelectedLayer(0) == m_pEditor->m_Map.m_pTeleLayer)
	{
		CLayerTele *pGrabbed = new CLayerTele(r.w, r.h);
		pGrabbed->m_pEditor = m_pEditor;
		pGrabbed->m_TexID = m_TexID;
		pGrabbed->m_Image = m_Image;
		
		pBrush->AddLayer(pGrabbed);
		
		// copy the tiles
		for(int y = 0; y < r.h; y++)
			for(int x = 0; x < r.w; x++)
				pGrabbed->m_pTiles[y*pGrabbed->m_Width+x] = m_pTiles[(r.y+y)*m_Width+(r.x+x)];
				
		// copy the tele data
		if(!m_pEditor->Input()->KeyPressed(KEY_SPACE))
			for(int y = 0; y < r.h; y++)
				for(int x = 0; x < r.w; x++)
					pGrabbed->m_pTeleTile[y*pGrabbed->m_Width+x] = ((CLayerTele*)this)->m_pTeleTile[(r.y+y)*m_Width+(r.x+x)];
	}
	else if(m_pEditor->GetSelectedLayer(0) == m_pEditor->m_Map.m_pSpeedupLayer)
	{
		CLayerSpeedup *pGrabbed = new CLayerSpeedup(r.w, r.h);
		pGrabbed->m_pEditor = m_pEditor;
		pGrabbed->m_TexID = m_TexID;
		pGrabbed->m_Image = m_Image;
		
		pBrush->AddLayer(pGrabbed);
		
		// copy the tiles
		for(int y = 0; y < r.h; y++)
			for(int x = 0; x < r.w; x++)
				pGrabbed->m_pTiles[y*pGrabbed->m_Width+x] = m_pTiles[(r.y+y)*m_Width+(r.x+x)];
				
		// copy the speedup data
		if(!m_pEditor->Input()->KeyPressed(KEY_SPACE))
			for(int y = 0; y < r.h; y++)
				for(int x = 0; x < r.w; x++)
					pGrabbed->m_pSpeedupTile[y*pGrabbed->m_Width+x] = ((CLayerSpeedup*)this)->m_pSpeedupTile[(r.y+y)*m_Width+(r.x+x)];
	}
	else
	{
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
	}

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

			// dont allow tele in and out tiles... same with speedup tile
			if(m_pEditor->GetSelectedLayer(0) == m_pEditor->m_Map.m_pGameLayer && (l->m_pTiles[y*l->m_Width+x].m_Index == TILE_TELEIN || l->m_pTiles[y*l->m_Width+x].m_Index == TILE_TELEOUT || l->m_pTiles[y*l->m_Width+x].m_Index == TILE_BOOST))
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

	if(!m_Game && !m_Tele && !m_Speedup)
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

	if(!m_Game && !m_Tele && !m_Speedup)
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
				if(!m_Game && !m_Tele && !m_Speedup)
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
	
	// resize tele layer if available
	if(m_Game && m_pEditor->m_Map.m_pTeleLayer && (m_pEditor->m_Map.m_pTeleLayer->m_Width != NewW || m_pEditor->m_Map.m_pTeleLayer->m_Height != NewH))
		m_pEditor->m_Map.m_pTeleLayer->Resize(NewW, NewH);
		
	// resize sppedup layer if available
	if(m_Game && m_pEditor->m_Map.m_pSpeedupLayer && (m_pEditor->m_Map.m_pSpeedupLayer->m_Width != NewW || m_pEditor->m_Map.m_pSpeedupLayer->m_Height != NewH))
		m_pEditor->m_Map.m_pSpeedupLayer->Resize(NewW, NewH);
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
	if(m_pEditor->m_Map.m_pGameLayer != this && m_pEditor->m_Map.m_pTeleLayer != this && m_pEditor->m_Map.m_pSpeedupLayer != this)
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

	if(m_pEditor->m_Map.m_pGameLayer == this || m_pEditor->m_Map.m_pTeleLayer == this || m_pEditor->m_Map.m_pSpeedupLayer == this) // remove the image and color properties if this is the game-, tele- or speedup layer
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

CLayerTele::CLayerTele(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, "Tele", sizeof(m_aName));
	m_Tele = 1;
	
	m_pTeleTile = new CTeleTile[w*h];
	mem_zero(m_pTeleTile, w*h*sizeof(CTeleTile));
}

CLayerTele::~CLayerTele()
{
	delete[] m_pTeleTile;
}

void CLayerTele::Resize(int NewW, int NewH)
{
	// resize tele data
	CTeleTile *pNewTeleData = new CTeleTile[NewW*NewH];
	mem_zero(pNewTeleData, NewW*NewH*sizeof(CTeleTile));

	// copy old data	
	for(int y = 0; y < min(NewH, m_Height); y++)
		mem_copy(&pNewTeleData[y*NewW], &m_pTeleTile[y*m_Width], min(m_Width, NewW)*sizeof(CTeleTile));
	
	// replace old
	delete [] m_pTeleTile;
	m_pTeleTile = pNewTeleData;
	
	// resize tile data
	CLayerTiles::Resize(NewW, NewH);
	
	// resize gamelayer too
	if(m_pEditor->m_Map.m_pGameLayer->m_Width != NewW || m_pEditor->m_Map.m_pGameLayer->m_Height != NewH)
		m_pEditor->m_Map.m_pGameLayer->Resize(NewW, NewH);
}

void CLayerTele::Shift(int Direction)
{
	CLayerTiles::Shift(Direction);
	
	switch(Direction)
	{
	case 1:
		{
			// left
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pTeleTile[y*m_Width], &m_pTeleTile[y*m_Width+1], (m_Width-1)*sizeof(CTeleTile));
		}
		break;
	case 2:
		{
			// right
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pTeleTile[y*m_Width+1], &m_pTeleTile[y*m_Width], (m_Width-1)*sizeof(CTeleTile));
		}
		break;
	case 4:
		{
			// up
			for(int y = 0; y < m_Height-1; ++y)
				mem_copy(&m_pTeleTile[y*m_Width], &m_pTeleTile[(y+1)*m_Width], m_Width*sizeof(CTeleTile));
		}
		break;
	case 8:
		{
			// down
			for(int y = m_Height-1; y > 0; --y)
				mem_copy(&m_pTeleTile[y*m_Width], &m_pTeleTile[(y-1)*m_Width], m_Width*sizeof(CTeleTile));
		}
	}
}

void CLayerTele::BrushDraw(CLayer *pBrush, float wx, float wy)
{
	CLayerTele *l = (CLayerTele *)pBrush;
	int sx = ConvertX(wx);
	int sy = ConvertY(wy);
	
	for(int y = 0; y < l->m_Height; y++)
		for(int x = 0; x < l->m_Width; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			if(fx<0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;
			
			if(l->m_pTiles[y*l->m_Width+x].m_Index == TILE_TELEIN || l->m_pTiles[y*l->m_Width+x].m_Index == TILE_TELEOUT)
			{
				if(l->m_pTeleTile[y*l->m_Width+x].m_Number)
					m_pTeleTile[fy*m_Width+fx].m_Number = l->m_pTeleTile[y*l->m_Width+x].m_Number;
				else
				{
					if(!m_pEditor->m_TeleNum)
					{
						m_pTeleTile[fy*m_Width+fx].m_Number = 0;
						m_pTeleTile[fy*m_Width+fx].m_Type = 0;
						m_pTiles[fy*m_Width+fx].m_Index = 0;
						continue;
					}
					else
						m_pTeleTile[fy*m_Width+fx].m_Number = m_pEditor->m_TeleNum;
				}
				
				m_pTeleTile[fy*m_Width+fx].m_Type = l->m_pTiles[y*l->m_Width+x].m_Index;
				m_pTiles[fy*m_Width+fx].m_Index = l->m_pTiles[y*l->m_Width+x].m_Index;
			}
			else
			{
				m_pTeleTile[fy*m_Width+fx].m_Number = 0;
				m_pTeleTile[fy*m_Width+fx].m_Type = 0;
				m_pTiles[fy*m_Width+fx].m_Index = 0;
			}
		}
}

void CLayerTele::BrushFlipX()
{
	CLayerTiles::BrushFlipX();
	
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width/2; x++)
		{
			CTeleTile Tmp = m_pTeleTile[y*m_Width+x];
			m_pTeleTile[y*m_Width+x] = m_pTeleTile[y*m_Width+m_Width-1-x];
			m_pTeleTile[y*m_Width+m_Width-1-x] = Tmp;
		}
}

void CLayerTele::BrushFlipY()
{
	CLayerTiles::BrushFlipY();
	
	for(int y = 0; y < m_Height/2; y++)
		for(int x = 0; x < m_Width; x++)
		{
			CTeleTile Tmp = m_pTeleTile[y*m_Width+x];
			m_pTeleTile[y*m_Width+x] = m_pTeleTile[(m_Height-1-y)*m_Width+x];
			m_pTeleTile[(m_Height-1-y)*m_Width+x] = Tmp;
		}
}

void CLayerTele::BrushRotate(float Amount)
{
	int Rotation = (round_to_int(360.0f*Amount/(pi*2))/90)%4;	// 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation +=4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation			
		CTeleTile *pTempData1 = new CTeleTile[m_Width*m_Height];
		CTile *pTempData2 = new CTile[m_Width*m_Height];
		mem_copy(pTempData1, m_pTeleTile, m_Width*m_Height*sizeof(CTeleTile));
		mem_copy(pTempData2, m_pTiles, m_Width*m_Height*sizeof(CTile));
		CTeleTile *pDst1 = m_pTeleTile;
		CTile *pDst2 = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height-1; y >= 0; --y, ++pDst1, ++pDst2)
			{
				*pDst1 = pTempData1[y*m_Width+x];
				*pDst2 = pTempData2[y*m_Width+x];
			}

		int Temp = m_Width;
		m_Width = m_Height;
		m_Height = Temp;
		delete[] pTempData1;
		delete[] pTempData2;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerTele::FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect)
{
	if(m_Readonly)
		return;
		
	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);
	
	CLayerTele *pLt = static_cast<CLayerTele*>(pBrush);
	
	for(int y = 0; y <= h; y++)
	{
		for(int x = 0; x <= w; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			
			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;
			
			if(Empty)
			{
                m_pTiles[fy*m_Width+fx].m_Index = 0;
				m_pTeleTile[fy*m_Width+fx].m_Number = 0;
				m_pTeleTile[fy*m_Width+fx].m_Type = 0;
			}
			else
			{
				m_pTiles[fy*m_Width+fx] = pLt->m_pTiles[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)];
				if(m_pTiles[fy*m_Width+fx].m_Index == TILE_TELEIN || m_pTiles[fy*m_Width+fx].m_Index == TILE_TELEOUT)
				{
					m_pTeleTile[fy*m_Width+fx].m_Type = m_pTiles[fy*m_Width+fx].m_Index;
					if(!pLt->m_pTeleTile[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)].m_Number && m_pEditor->m_TeleNum && m_pTiles[fy*m_Width+fx].m_Index > 0)
						m_pTeleTile[fy*m_Width+fx].m_Number = m_pEditor->m_TeleNum;
					else
					{
						m_pTeleTile[fy*m_Width+fx].m_Number = pLt->m_pTeleTile[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)].m_Number;
						if(!m_pTeleTile[fy*m_Width+fx].m_Number)
						{
							m_pTiles[fy*m_Width+fx].m_Index = 0;
							m_pTeleTile[fy*m_Width+fx].m_Type = 0;
						}
					}
				}
				else
				{
					m_pTiles[fy*m_Width+fx].m_Index = 0;
					m_pTeleTile[fy*m_Width+fx].m_Type = 0;
					m_pTeleTile[fy*m_Width+fx].m_Number = 0;
				}
			}
		}
	}
}

CLayerSpeedup::CLayerSpeedup(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, "Speedup", sizeof(m_aName));
	m_Speedup = 1;
	
	m_pSpeedupTile = new CSpeedupTile[w*h];
	mem_zero(m_pSpeedupTile, w*h*sizeof(CSpeedupTile));
}

CLayerSpeedup::~CLayerSpeedup()
{
	delete[] m_pSpeedupTile;
}

void CLayerSpeedup::Resize(int NewW, int NewH)
{
	// resize speedup data
	CSpeedupTile *pNewSpeedupData = new CSpeedupTile[NewW*NewH];
	mem_zero(pNewSpeedupData, NewW*NewH*sizeof(CSpeedupTile));

	// copy old data	
	for(int y = 0; y < min(NewH, m_Height); y++)
		mem_copy(&pNewSpeedupData[y*NewW], &m_pSpeedupTile[y*m_Width], min(m_Width, NewW)*sizeof(CSpeedupTile));
	
	// replace old
	delete [] m_pSpeedupTile;
	m_pSpeedupTile = pNewSpeedupData;
	
	// resize tile data
	CLayerTiles::Resize(NewW, NewH);
	
	// resize gamelayer too
	if(m_pEditor->m_Map.m_pGameLayer->m_Width != NewW || m_pEditor->m_Map.m_pGameLayer->m_Height != NewH)
		m_pEditor->m_Map.m_pGameLayer->Resize(NewW, NewH);
}

void CLayerSpeedup::Shift(int Direction)
{
	CLayerTiles::Shift(Direction);
	
	switch(Direction)
	{
	case 1:
		{
			// left
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pSpeedupTile[y*m_Width], &m_pSpeedupTile[y*m_Width+1], (m_Width-1)*sizeof(CSpeedupTile));
		}
		break;
	case 2:
		{
			// right
			for(int y = 0; y < m_Height; ++y)
				mem_move(&m_pSpeedupTile[y*m_Width+1], &m_pSpeedupTile[y*m_Width], (m_Width-1)*sizeof(CSpeedupTile));
		}
		break;
	case 4:
		{
			// up
			for(int y = 0; y < m_Height-1; ++y)
				mem_copy(&m_pSpeedupTile[y*m_Width], &m_pSpeedupTile[(y+1)*m_Width], m_Width*sizeof(CSpeedupTile));
		}
		break;
	case 8:
		{
			// down
			for(int y = m_Height-1; y > 0; --y)
				mem_copy(&m_pSpeedupTile[y*m_Width], &m_pSpeedupTile[(y-1)*m_Width], m_Width*sizeof(CSpeedupTile));
		}
	}
}

void CLayerSpeedup::BrushDraw(CLayer *pBrush, float wx, float wy)
{
	CLayerSpeedup *l = (CLayerSpeedup *)pBrush;
	int sx = ConvertX(wx);
	int sy = ConvertY(wy);
	
	for(int y = 0; y < l->m_Height; y++)
		for(int x = 0; x < l->m_Width; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			if(fx<0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;
			
			if(l->m_pTiles[y*l->m_Width+x].m_Index == TILE_BOOST)
			{
				if(l->m_pSpeedupTile[y*l->m_Width+x].m_Force || l->m_pSpeedupTile[y*l->m_Width+x].m_Angle)
				{
					m_pSpeedupTile[fy*m_Width+fx].m_Force = l->m_pSpeedupTile[y*l->m_Width+x].m_Force;
					m_pSpeedupTile[fy*m_Width+fx].m_Angle = l->m_pSpeedupTile[y*l->m_Width+x].m_Angle;
					m_pTiles[fy*m_Width+fx].m_Index = l->m_pTiles[y*l->m_Width+x].m_Index;
				}
				else if(m_pEditor->m_SpeedupForce)
				{
					m_pSpeedupTile[fy*m_Width+fx].m_Force = m_pEditor->m_SpeedupForce;
					m_pSpeedupTile[fy*m_Width+fx].m_Angle = m_pEditor->m_SpeedupAngle;
					m_pTiles[fy*m_Width+fx].m_Index = l->m_pTiles[y*l->m_Width+x].m_Index;
				}
				else
				{
					m_pSpeedupTile[fy*m_Width+fx].m_Force = 0;
					m_pSpeedupTile[fy*m_Width+fx].m_Angle = 0;
					m_pTiles[fy*m_Width+fx].m_Index = 0;
				}
			}
			else
			{
				m_pSpeedupTile[fy*m_Width+fx].m_Force = 0;
				m_pSpeedupTile[fy*m_Width+fx].m_Angle = 0;
				m_pTiles[fy*m_Width+fx].m_Index = 0;
			}
		}
}

void CLayerSpeedup::BrushFlipX()
{
	CLayerTiles::BrushFlipX();
	
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width/2; x++)
		{
			CSpeedupTile Tmp = m_pSpeedupTile[y*m_Width+x];
			m_pSpeedupTile[y*m_Width+x] = m_pSpeedupTile[y*m_Width+m_Width-1-x];
			m_pSpeedupTile[y*m_Width+m_Width-1-x] = Tmp;
		}
}

void CLayerSpeedup::BrushFlipY()
{
	CLayerTiles::BrushFlipY();
	
	for(int y = 0; y < m_Height/2; y++)
		for(int x = 0; x < m_Width; x++)
		{
			CSpeedupTile Tmp = m_pSpeedupTile[y*m_Width+x];
			m_pSpeedupTile[y*m_Width+x] = m_pSpeedupTile[(m_Height-1-y)*m_Width+x];
			m_pSpeedupTile[(m_Height-1-y)*m_Width+x] = Tmp;
		}
}

void CLayerSpeedup::BrushRotate(float Amount)
{
	int Rotation = (round_to_int(360.0f*Amount/(pi*2))/90)%4;	// 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation +=4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation			
		CSpeedupTile *pTempData1 = new CSpeedupTile[m_Width*m_Height];
		CTile *pTempData2 = new CTile[m_Width*m_Height];
		mem_copy(pTempData1, m_pSpeedupTile, m_Width*m_Height*sizeof(CSpeedupTile));
		mem_copy(pTempData2, m_pTiles, m_Width*m_Height*sizeof(CTile));
		CSpeedupTile *pDst1 = m_pSpeedupTile;
		CTile *pDst2 = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height-1; y >= 0; --y, ++pDst1, ++pDst2)
			{
				*pDst1 = pTempData1[y*m_Width+x];
				*pDst2 = pTempData2[y*m_Width+x];
			}

		int Temp = m_Width;
		m_Width = m_Height;
		m_Height = Temp;
		delete[] pTempData1;
		delete[] pTempData2;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerSpeedup::FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect)
{
	if(m_Readonly)
		return;
		
	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);
	
	CLayerSpeedup *pLt = static_cast<CLayerSpeedup*>(pBrush);
	
	for(int y = 0; y <= h; y++)
	{
		for(int x = 0; x <= w; x++)
		{
			int fx = x+sx;
			int fy = y+sy;
			
			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;
			
			if(Empty)
			{
                m_pTiles[fy*m_Width+fx].m_Index = 0;
				m_pSpeedupTile[fy*m_Width+fx].m_Force = 0;
				m_pSpeedupTile[fy*m_Width+fx].m_Angle = 0;
			}
			else
			{
				m_pTiles[fy*m_Width+fx] = pLt->m_pTiles[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)];
				if(m_pTiles[fy*m_Width+fx].m_Index == TILE_BOOST)
				{
					if(!pLt->m_pSpeedupTile[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)].m_Force && m_pEditor->m_SpeedupForce && m_pTiles[fy*m_Width+fx].m_Index > 0)
					{
						m_pSpeedupTile[fy*m_Width+fx].m_Force = m_pEditor->m_SpeedupForce;
						m_pSpeedupTile[fy*m_Width+fx].m_Angle = m_pEditor->m_SpeedupAngle;
					}
					else
					{
						m_pSpeedupTile[fy*m_Width+fx].m_Force = pLt->m_pSpeedupTile[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)].m_Force;
						m_pSpeedupTile[fy*m_Width+fx].m_Angle = pLt->m_pSpeedupTile[(y*pLt->m_Width + x%pLt->m_Width) % (pLt->m_Width*pLt->m_Height)].m_Angle;
						if(!m_pSpeedupTile[fy*m_Width+fx].m_Force)
						{
							m_pTiles[fy*m_Width+fx].m_Index = 0;
							m_pSpeedupTile[fy*m_Width+fx].m_Angle = 0;
						}
					}
				}
				else
				{
					m_pTiles[fy*m_Width+fx].m_Index = 0;
					m_pSpeedupTile[fy*m_Width+fx].m_Force = 0;
					m_pSpeedupTile[fy*m_Width+fx].m_Angle = 0;
				}
			}
		}
	}
}
