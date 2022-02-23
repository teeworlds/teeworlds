/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/console.h>
#include "editor.h"


CLayerWater::CLayerWater(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, LAYERNAME_WATER, sizeof(m_aName));
	m_Flags = LAYERFLAG_OPERATIONAL|LAYERFLAG_CUSTOM_GAMELAYER;
	m_Color.r = 0;
	m_Color.g = 162;
	m_Color.b = 255;
	m_Color.a = 64;
}

CLayerWater::~CLayerWater()
{
}

int CLayerWater::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
