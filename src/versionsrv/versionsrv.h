// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#ifndef VERSIONSRV_VERSIONSRV_H
#define VERSIONSRV_VERSIONSRV_H
static const int VERSIONSRV_PORT = 8302;

static const unsigned char VERSION_DATA[] = {0x00, 0, 5, 1};

static const unsigned char VERSIONSRV_GETVERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 'g'};
static const unsigned char VERSIONSRV_VERSION[] = {255, 255, 255, 255, 'v', 'e', 'r', 's'};
#endif
