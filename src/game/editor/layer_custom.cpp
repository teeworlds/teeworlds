/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/console.h>
#include "editor.h"


CLayerCustom::CLayerCustom(int w, int h)
: CLayerTiles(w, h)
{
	m_Flags = LAYERFLAG_OPERATIONAL|LAYERFLAG_CUSTOM_GAMELAYER;
}

CLayerCustom::~CLayerCustom()
{
}

int CLayerCustom::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
