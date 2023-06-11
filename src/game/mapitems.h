/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

// layer types
enum
{
	LAYERTYPE_INVALID = 0,
	LAYERTYPE_GAME, // unused
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,

	MAPITEMTYPE_VERSION = 0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,


	CURVETYPE_STEP = 0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	CURVETYPE_BEZIER,
	NUM_CURVETYPES,

	// game layer tiles
	ENTITY_NULL = 0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_RED,
	ENTITY_SPAWN_BLUE,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_LASER,
	NUM_ENTITIES,

	TILE_AIR = 0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,

	TILEFLAG_VFLIP = 1,
	TILEFLAG_HFLIP = 2,
	TILEFLAG_OPAQUE = 4,
	TILEFLAG_ROTATE = 8,

	LAYERFLAG_DETAIL = 1,
	TILESLAYERFLAG_GAME = 1,

	ENTITY_OFFSET = 255 - 16 * 4,
};

struct CPoint
{
	int x, y; // 22.10 fixed point
};

struct CColor
{
	int r, g, b, a;
};

struct CQuad
{
	CPoint m_aPoints[5];
	CColor m_aColors[4];
	CPoint m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};

struct CTile
{
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};


struct CMapItemInfo_v1
{
	enum
	{
		CURRENT_VERSION = 1,
	};

	int m_Version;
	int m_Author; // Data index
	int m_MapVersion; // Data index
	int m_Credits; // Data index
	int m_License; // Data index
};

typedef CMapItemInfo_v1 CMapItemInfo;


struct CMapItemImage_v1
{
	enum
	{
		CURRENT_VERSION = 1,
		MIN_DIMENSION = 0, // external images may use this
		MAX_DIMENSION = 8192,
	};

	int m_Version;
	int m_Width;
	int m_Height;
	int m_External; // 0 or 1
	int m_ImageName; // Data index
	int m_ImageData; // Data index
};

struct CMapItemImage_v2 : public CMapItemImage_v1
{
	enum
	{
		CURRENT_VERSION = 2,
	};

	int m_Format; // Default before this version is CImageInfo::FORMAT_RGBA
};

typedef CMapItemImage_v2 CMapItemImage;


struct CMapItemGroup_v1
{
	enum
	{
		CURRENT_VERSION = 1,
	};

	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
};

struct CMapItemGroup_v2 : public CMapItemGroup_v1
{
	enum
	{
		CURRENT_VERSION = 2,
	};

	int m_UseClipping; // 0 or 1
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;
};

struct CMapItemGroup_v3 : public CMapItemGroup_v2
{
	enum
	{
		CURRENT_VERSION = 3,
	};

	int m_aName[3];
};

typedef CMapItemGroup_v3 CMapItemGroup;

// This struct is a prefix of the other layers types and must not be changed.
struct CMapItemLayer
{
	int m_Version; // unused
	int m_Type;
	int m_Flags;
};

// There is no version 1
struct CMapItemLayerTilemap_v2
{
	enum
	{
		CURRENT_VERSION = 2,
		MIN_DIMENSION = 1,
		MAX_DIMENSION = 5000,
	};

	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data; // Data index
};

struct CMapItemLayerTilemap_v3 : public CMapItemLayerTilemap_v2
{
	enum
	{
		CURRENT_VERSION = 3,
	};

	int m_aName[3];
};

struct CMapItemLayerTilemap_v4 : public CMapItemLayerTilemap_v3
{
	enum
	{
		CURRENT_VERSION = 4,
	};

	// Tile data is using tile skip as of this version
};

typedef CMapItemLayerTilemap_v4 CMapItemLayerTilemap;


struct CMapItemLayerQuads_v1
{
	enum
	{
		CURRENT_VERSION = 1,
		MIN_QUADS = 0,
		MAX_QUADS = 100000,
	};

	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data; // Data index (swapped data)
	int m_Image;
};

struct CMapItemLayerQuads_v2 : public CMapItemLayerQuads_v1
{
	enum
	{
		CURRENT_VERSION = 2,
	};

	int m_aName[3];
};

typedef CMapItemLayerQuads_v2 CMapItemLayerQuads;


struct CMapItemVersion_v1
{
	enum
	{
		CURRENT_VERSION = 1,
	};

	int m_Version;
};

typedef CMapItemVersion_v1 CMapItemVersion;


// Used with CMapItemEnvelope v1 to v2
struct CEnvPoint_v1
{
	enum
	{
		MAX_CHANNELS = 4,
	};

	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[MAX_CHANNELS]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint_v1 &Other) const { return m_Time < Other.m_Time; }
};

// Used with CMapItemEnvelope v3
struct CEnvPoint_v2 : public CEnvPoint_v1
{
	// bezier curve only
	// dx in ms and dy as 22.10 fxp
	int m_aInTangentdx[MAX_CHANNELS];
	int m_aInTangentdy[MAX_CHANNELS];
	int m_aOutTangentdx[MAX_CHANNELS];
	int m_aOutTangentdy[MAX_CHANNELS];
};

typedef CEnvPoint_v2 CEnvPoint;


struct CMapItemEnvelope_v1
{
	enum
	{
		CURRENT_VERSION = 1,
	};

	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
};

struct CMapItemEnvelope_v2 : public CMapItemEnvelope_v1
{
	enum
	{
		CURRENT_VERSION = 2,
	};

	int m_Synchronized; // 0 or 1, default before this version is 1
};

struct CMapItemEnvelope_v3 : public CMapItemEnvelope_v2
{
	enum
	{
		CURRENT_VERSION = 3,
	};

	// Bezier curve support (at least CEnvPoint_v2 expected) as of this version
};

typedef CMapItemEnvelope_v3 CMapItemEnvelope;


class CMapItemChecker
{
public:
	static bool IsValid(const CMapItemInfo *pItem, unsigned Size);
	static bool IsValid(const CMapItemImage *pItem, unsigned Size);
	static bool IsValid(const CMapItemGroup *pItem, unsigned Size);
	static bool IsValid(const CMapItemLayer *pItem, unsigned Size);
	static bool IsValid(const CMapItemLayerTilemap *pItem, unsigned Size);
	static bool IsValid(const CMapItemLayerQuads *pItem, unsigned Size);
	static bool IsValid(const CMapItemVersion *pItem, unsigned Size);
	static bool IsValid(const CEnvPoint *pItem, unsigned Size, int EnvelopeItemVersion, int *pNumPoints = 0); // Multiple points are inside a single item
	static bool IsValid(const CMapItemEnvelope *pItem, unsigned Size);
};

#endif
