/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
enum
{
	MAPRES_REGISTERED=0x8000,
	MAPRES_IMAGE=0x8001,
	MAPRES_TILEMAP=0x8002,
	MAPRES_COLLISIONMAP=0x8003,
	MAPRES_TEMP_THEME=0x8fff,
};

struct mapres_theme
{
	int id;
};
