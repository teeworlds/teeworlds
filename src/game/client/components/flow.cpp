/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/layers.h>
#include "camera.h"
#include "effects.h"
#include "flow.h"

CFlow::CFlow()
{
	m_pCells = 0;
	m_Height = 0;
	m_Width = 0;
	m_Spacing = 16;
}

CFlow::~CFlow()
{
	if(m_pCells)
		mem_free(m_pCells);
}

void CFlow::DbgRender()
{
	if(!m_pCells)
		return;

	IGraphics::CLineItem Array[1024];
	int NumItems = 0;
	Graphics()->TextureSet(-1);
	Graphics()->LinesBegin();
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
		{
			vec2 Pos(x*m_Spacing, y*m_Spacing);
			vec2 Vel = m_pCells[y*m_Width+x].m_Vel * 0.01f;
			Array[NumItems++] = IGraphics::CLineItem(Pos.x, Pos.y, Pos.x+Vel.x, Pos.y+Vel.y);
			if(NumItems == 1024)
			{
				Graphics()->LinesDraw(Array, 1024);
				NumItems = 0;
			}
		}

	if(NumItems)
		Graphics()->LinesDraw(Array, NumItems);
	Graphics()->LinesEnd();
}

void CFlow::OnMapLoad()
{
	if(m_pCells)
	{
		mem_free(m_pCells);
		m_pCells = 0;
	}

	CMapItemLayerTilemap *pTilemap = Layers()->GameLayer();

	m_Width = pTilemap->m_Width*32/m_Spacing;
	m_Height = pTilemap->m_Height*32/m_Spacing;

	// allocate and clear
	m_pCells = (CCell *)mem_alloc(sizeof(CCell)*m_Width*m_Height, 1);
	for(int y = 0; y < m_Height; y++)
		for(int x = 0; x < m_Width; x++)
			m_pCells[y*m_Width+x].m_Vel = vec2(0,0);
}

void CFlow::Update()
{
	if(!m_pCells || !g_Config.m_ClRenderFLow)
		return;

	int StartY, StartX, EndY, EndX;
	GetWindow(&StartY, &StartX, &EndY, &EndX);

	for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
			if(y < 0 || y >= m_Height || x < 0 || x >= m_Width)
				continue;

			m_pCells[y*m_Width+x].m_Vel *= 0.85f;
		}
}

vec2 CFlow::Get(vec2 Pos)
{
	if(!m_pCells)
		return vec2(0,0);

	int StartY, StartX, EndY, EndX;
	GetWindow(&StartY, &StartX, &EndY, &EndX);

	int x = (int)(Pos.x / m_Spacing);
	int y = (int)(Pos.y / m_Spacing);
	if(x < StartX || y < StartY || x >= EndX || y >= EndY)
		return vec2(0,0);

	return m_pCells[y*m_Width+x].m_Vel;
}

void CFlow::Add(vec2 Pos, vec2 Vel, float Size)
{
	if(!m_pCells || !g_Config.m_ClRenderFLow || !m_pClient->m_pEffects->FrameSync())
		return;

	int StartY, StartX, EndY, EndX;
	GetWindow(&StartY, &StartX, &EndY, &EndX);

	int x = (int)(Pos.x / m_Spacing);
	int y = (int)(Pos.y / m_Spacing);
	if(x < StartX || y < StartY || x >= EndX || y >= EndY)
		return;

	m_pCells[y*m_Width+x].m_Vel += Vel;
}

void CFlow::GetWindow(int *StartY, int *StartX, int *EndY, int *EndX)
{
	float Points[4];
	vec2 Center = m_pClient->m_pCamera->m_Center;
	RenderTools()->MapscreenToWorld(Center.x, Center.y, 1.0f, 1.0f,
		0.0f, 0.0f, Graphics()->ScreenAspect(), m_pClient->m_pCamera->m_Zoom, Points);

	*StartY = clamp((int)(Points[1]/m_Spacing)-1, 0, m_Height);
	*StartX = clamp((int)(Points[0]/m_Spacing)-1, 0, m_Width);
	*EndY = clamp((int)(Points[3]/m_Spacing)+1, 0, m_Height);
	*EndX = clamp((int)(Points[2]/m_Spacing)+1, 0, m_Width);
}
