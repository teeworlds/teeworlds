/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

// layer types
enum
{
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME,
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
	CURVETYPE_BEZIER,
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
	ENTITY_WEAPON_LASER,
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
	ENTITY_PLASMAU,
	//DDRace - Shotgun
	ENTITY_CRAZY_SHOTGUN_U_EX,
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
	TILE_THROUGH_CUT,
	TILE_THROUGH,

	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,

	LAYERFLAG_DETAIL=1,
	TILESLAYERFLAG_GAME=1,

	// DDRace

	TILE_FREEZE=9,
	TILE_TELEINEVIL,
	TILE_UNFREEZE,
	TILE_BOOSTS,

	TILE_TELEINWEAPON=14,
	TILE_TELEINHOOK,

	TILE_CP_D=64,
	TILE_CP_U,
	TILE_CP_R,
	TILE_CP_L,
	TILE_CP_D_F,
	TILE_CP_U_F,
	TILE_CP_R_F,
	TILE_CP_L_F,

	// Switches
	TILE_SWITCHTIMEDOPEN = 22,
	TILE_SWITCHTIMEDCLOSE,
	TILE_SWITCHOPEN,
	TILE_SWITCHCLOSE,
	TILE_TELEIN,
	TILE_TELEOUT,
	TILE_BOOST,
	TILE_TELECHECK,
	TILE_TELECHECKOUT,
	TILE_TELECHECKIN,
	TILE_REFILL_JUMPS = 32,
	TILE_BEGIN,
	TILE_END,
	TILE_CHECKPOINT_FIRST = 35,
	TILE_CHECKPOINT_LAST = 59,
	TILE_STOP = 60,
	TILE_STOPS,
	TILE_STOPA,
	TILE_TELECHECKINEVIL,
	TILE_CP,
	TILE_CP_F,
	TILE_THROUGH_ALL,
	TILE_THROUGH_DIR,
	TILE_TUNE1,
	TILE_OLDLASER = 71,
	TILE_NPC,
	TILE_EHOOK,
	TILE_NOHIT,
	TILE_NPH,
	TILE_UNLOCK_TEAM,
	TILE_PENALTY = 79,
	TILE_NPC_END = 88,
	TILE_SUPER_END,
	TILE_JETPACK_END,
	TILE_NPH_END,
	TILE_BONUS = 95,
	TILE_TELE_GUN_ENABLE = 96,
	TILE_TELE_GUN_DISABLE = 97,
	TILE_ALLOW_TELE_GUN = 98,
	TILE_ALLOW_BLUE_TELE_GUN = 99,
	TILE_NPC_START = 104,
	TILE_SUPER_START,
	TILE_JETPACK_START,
	TILE_NPH_START,
	TILE_TELE_GRENADE_ENABLE = 112,
	TILE_TELE_GRENADE_DISABLE = 113,
	TILE_TELE_LASER_ENABLE = 128,
	TILE_TELE_LASER_DISABLE = 129,
	TILE_CREDITS_1 = 140,
	TILE_CREDITS_2 = 141,
	TILE_CREDITS_3 = 142,
	TILE_CREDITS_4 = 143,
	TILE_CREDITS_5 = 156,
	TILE_CREDITS_6 = 157,
	TILE_CREDITS_7 = 158,
	TILE_CREDITS_8 = 159,
	TILE_ENTITIES_OFF_1 = 190,
	TILE_ENTITIES_OFF_2,
	//End of higher tiles

	//Rotation
	ROTATION_0 = 0,
	ROTATION_90 = TILEFLAG_ROTATE,
	ROTATION_180 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP),
	ROTATION_270 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP|TILEFLAG_ROTATE),

	TILESLAYERFLAG_TELE=2,
	TILESLAYERFLAG_SPEEDUP=4,
	TILESLAYERFLAG_FRONT=8,
	TILESLAYERFLAG_SWITCH=16,
	TILESLAYERFLAG_TUNE=32,

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

struct CMapItemInfo
{
	enum { CURRENT_VERSION=1 };
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
} ;

struct CMapItemImage_v1
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemImage : public CMapItemImage_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Format;
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
	enum { CURRENT_VERSION=3 };

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	enum { CURRENT_VERSION=4 };

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

	int m_aName[3];

	// DDRace

	int m_Tele;
	int m_Speedup;
	int m_Front;
	int m_Switch;
	int m_Tune;
} ;

struct CMapItemLayerQuads
{
	enum { CURRENT_VERSION=2 };

	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
} ;

struct CMapItemVersion
{
	enum { CURRENT_VERSION=1 };

	int m_Version;
} ;

struct CEnvPoint_v1
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint_v1 &Other) const { return m_Time < Other.m_Time; }
} ;

struct CEnvPoint : public CEnvPoint_v1
{
	// bezier curve only
	// dx in ms and dy as 22.10 fxp
	int m_aInTangentdx[4];
	int m_aInTangentdy[4];
	int m_aOutTangentdx[4];
	int m_aOutTangentdy[4];

	bool operator<(const CEnvPoint& other) const { return m_Time < other.m_Time; }
};

struct CMapItemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

struct CMapItemEnvelope_v2 : public CMapItemEnvelope_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Synchronized;
};

struct CMapItemEnvelope : public CMapItemEnvelope_v2
{
	// bezier curve support
	enum { CURRENT_VERSION=3 };
};

// DDRace

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

class CDoorTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	int m_Number;
};

class CTuneTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
};

#endif
