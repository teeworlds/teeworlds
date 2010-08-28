#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

// layer types
enum
{
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME, // not used
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,
	
	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,
	

	CURVETYPE_STEP=0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,
	
	// game layer tiles
	ENTITY_NULL=0,
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
	ENTITY_WEAPON_RIFLE,
	//DDRace - Main Lasers
	ENTITY_LASER_FAST_CW,
	ENTITY_LASER_NORMAL_CW,
	ENTITY_LASER_SLOW_CW,
	ENTITY_LASER_STOP,
	ENTITY_LASER_SLOW_CCW,
	ENTITY_LASER_NORMAL_CCW,
	ENTITY_LASER_FAST_CCW,
	//DDRace - Laser Modifiers
	ENTITY_LASER_SHORT,
	ENTITY_LASER_MIDDLE,
	ENTITY_LASER_LONG,
	ENTITY_LASER_C_SLOW,
	ENTITY_LASER_C_NORMAL,
	ENTITY_LASER_C_FAST,
	ENTITY_LASER_O_SLOW,
	ENTITY_LASER_O_NORMAL,
	ENTITY_LASER_O_FAST,
	//DDRace - Plasma
	ENTITY_PLASMAE=29,
	ENTITY_PLASMAF,
	ENTITY_PLASMA,
	//DDRace - Shotgun
	ENTITY_CRAZY_SHOTGUN_U_EX=33,
	ENTITY_CRAZY_SHOTGUN_R_EX,
	ENTITY_CRAZY_SHOTGUN_D_EX,
	ENTITY_CRAZY_SHOTGUN_L_EX,
	ENTITY_CRAZY_SHOTGUN_U,
	ENTITY_CRAZY_SHOTGUN_R,
	ENTITY_CRAZY_SHOTGUN_D,
	ENTITY_CRAZY_SHOTGUN_L,
	//DDRace - Draggers
	ENTITY_DRAGGER_WEAK=42,
	ENTITY_DRAGGER_NORMAL,
	ENTITY_DRAGGER_STRONG,
	//Draggers Behind Walls
	ENTITY_DRAGGER_WEAK_NW,
	ENTITY_DRAGGER_NORMAL_NW,
	ENTITY_DRAGGER_STRONG_NW,
	//DDrace - Doors
	ENTITY_DOOR=49,
	ENTITY_TRIGGER,
	ENTITY_CONNECTOR_D=52,//TODO: in ddrace stable used this one no need others rename ENTITY_CONNECTOR
	ENTITY_CONNECTOR_DR,
	ENTITY_CONNECTOR_R,
	ENTITY_CONNECTOR_RU,
	ENTITY_CONNECTOR_U,
	ENTITY_CONNECTOR_UL,
	ENTITY_CONNECTOR_L,
	ENTITY_CONNECTOR_LD,
	//End Of Lower Tiles
	NUM_ENTITIES,
	//Start From Top Left
	//Tile Controllers
	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_NOLASER,
	TILE_THROUGH=6,
	TILE_FREEZE=9,
	TILE_UNFREEZE=11,
	TILE_BOOST_L=18,
	TILE_BOOST_R,
	TILE_BOOST_D,
	TILE_BOOST_U,
	TILE_BOOST_L2,
	TILE_BOOST_R2,
	TILE_BOOST_D2,
	TILE_BOOST_U2,
	TILE_TELEIN,
	TILE_TELEOUT,
	TILE_BOOST,
	TILE_STOPL,
	TILE_STOPR,
	TILE_STOPB,
	TILE_STOPT,
	TILE_BEGIN,
	TILE_END,
	TILE_CP_D=64,
	TILE_CP_U,
	TILE_CP_R,
	TILE_CP_L,
	TILE_CP_D_F,
	TILE_CP_U_F,
	TILE_CP_R_F,
	TILE_CP_L_F,
	TILE_NPC,
	TILE_EHOOK,
	TILE_NOHIT,
	TILE_NPH,
	//End of higher tiles
	//Untouchable Elements
	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	
	LAYERFLAG_DETAIL=1,
	
	ENTITY_OFFSET=255-16*4,
};

enum
{
	LAYER_GAME,
	LAYER_FRONT,
	LAYER_LASER,
	LAYER_TELE,
	LAYER_SPEEDUP,
	NUM_LAYERS
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

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

class CTeleTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
};

class CSpeedupTile
{
public:
	unsigned char m_Force;
	short m_Angle;
};

struct CMapItemImage
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
} ;


struct CMapItemGroup : public CMapItemGroup_v1
{
	enum { CURRENT_VERSION=2 };
	
	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	CMapItemLayer m_Layer;
	int m_Version;
	
	int m_Width;
	int m_Height;
	int m_Flags;
	
	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;
	
	int m_Image;
	int m_Data;
	
	int m_Tele;
	int m_Speedup;
	int m_Front;
	int m_Switch;
} ;

struct CMapItemLayerQuads
{
	CMapItemLayer m_Layer;
	int m_Version;
	
	int m_NumQuads;
	int m_Data;
	int m_Image;
} ;

struct CMapItemVersion
{
	int m_Version;
} ;

struct CEnvPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)
	
	bool operator<(const CEnvPoint &Other) { return m_Time < Other.m_Time; }
} ;

struct CMapItemEnvelope
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

#endif
