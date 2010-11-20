/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef VERSIONSRV_VERSIONSRV_H
#define VERSIONSRV_VERSIONSRV_H
static const int VERSIONSRV_PORT = 8302;

static const unsigned char VERSION_DATA[] = {0x00, 0, 5, 1};

static const unsigned char VERSIONSRV_GETVERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 'g'};
static const unsigned char VERSIONSRV_VERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 's'};
#endif
