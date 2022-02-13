/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/console.h>
#include "editor.h"


CLayerMaterial::CLayerMaterial(int w, int h)
: CLayerTiles(w, h)
{
	str_copy(m_aName, "Material", sizeof(m_aName));
	m_Flags = LAYERFLAG_OPERATIONAL;
}

CLayerMaterial::~CLayerMaterial()
{
}

int CLayerMaterial::RenderProperties(CUIRect *pToolbox)
{
	int r = CLayerTiles::RenderProperties(pToolbox);
	m_Image = -1;
	return r;
}
