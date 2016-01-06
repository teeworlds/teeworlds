#ifndef MODAPI_MAPITEM_H
#define MODAPI_MAPITEM_H

enum
{
	MODAPI_MAPLAYERTYPE_ENTITIES = LAYERTYPE_QUADS+1,
};

struct CModAPI_MapEntity_Point
{
	int x;
	int y;
	int m_Type;
};

struct CModAPI_MapItem_LayerEntities
{
	enum { CURRENT_VERSION=1 };

	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumPoints;
	int m_PointsData;

	int m_aName[3];
} ;

#endif
