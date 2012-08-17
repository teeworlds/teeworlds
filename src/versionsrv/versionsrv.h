/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef VERSIONSRV_VERSIONSRV_H
#define VERSIONSRV_VERSIONSRV_H
static const int VERSIONSRV_PORT = 8302;

struct CMapVersion
{
	char m_aName[8];
	unsigned char m_aCrc[4];
	unsigned char m_aSize[4];
};

static const unsigned char VERSIONSRV_GETVERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 'g'};
static const unsigned char VERSIONSRV_VERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 's'};

static const unsigned char VERSIONSRV_GETMAPLIST[] = {255, 255, 255, 255, 'v', 'm', 'l', 'g'};
static const unsigned char VERSIONSRV_MAPLIST[] = {255, 255, 255, 255, 'v', 'm', 'l', 's'};
#endif
