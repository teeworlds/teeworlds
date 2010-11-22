/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H
#include<engine/shared/protocol.h>

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
	ENTITY_LASER_MEDIUM,
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
	ENTITY_PLASMAU,
	//DDRace - Shotgun
	ENTITY_CRAZY_SHOTGUN_EX,
	ENTITY_CRAZY_SHOTGUN,
	//DDRace - Draggers
	ENTITY_DRAGGER_WEAK=42,
	ENTITY_DRAGGER_NORMAL,
	ENTITY_DRAGGER_STRONG,
	//Draggers Behind Walls
	ENTITY_DRAGGER_WEAK_NW,
	ENTITY_DRAGGER_NORMAL_NW,
	ENTITY_DRAGGER_STRONG_NW,
	//Doors
	ENTITY_DOOR=49,
	//End Of Lower Tiles
	NUM_ENTITIES,
	//Start From Top Left
	//Tile Controllers
	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_NOLASER,
	TILE_THROUGH = 6,
	TILE_FREEZE = 9,
	TILE_TELEINEVIL,
	TILE_UNFREEZE,
	//Switches
	TILE_SWITCHTIMEDOPEN = 22,
	TILE_SWITCHTIMEDCLOSE,
	TILE_SWITCHOPEN,
	TILE_SWITCHCLOSE,
	TILE_TELEIN,
	TILE_TELEOUT,
	TILE_BOOST,
	TILE_BEGIN = 33,
	TILE_END,
	TILE_STOP = 60,
	TILE_STOPS,
	TILE_STOPA,
	TILE_CP = 64,
	TILE_CP_F,
	TILE_NPC = 72,
	TILE_EHOOK,
	TILE_NOHIT,
	TILE_NPH,//Remember to change this in collision.cpp if you add anymore tiles
	//End of higher tiles
	//Layers
	LAYER_GAME=0,
	LAYER_FRONT,
	LAYER_TELE,
	LAYER_SPEEDUP,
	LAYER_SWITCH,
	NUM_LAYERS,
	//Flags
	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,
	//Rotation
	ROTATION_0 = 0,
	ROTATION_90 = TILEFLAG_ROTATE,
	ROTATION_180 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP),
	ROTATION_270 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP|TILEFLAG_ROTATE),
	
	LAYERFLAG_DETAIL=1,
	
	ENTITY_OFFSET=255-16*4,
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
	unsigned char m_MaxSpeed;
	unsigned char m_Type;
	short m_Angle;
};

class CSwitchTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
	unsigned char m_Flags;
	unsigned char m_Delay;
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

class CDoorTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	int m_Number;
};

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
