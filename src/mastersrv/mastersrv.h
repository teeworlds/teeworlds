/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
static const int MASTERSERVER_PORT = 8300;

enum {
	MAX_SERVERS = 200
};

typedef struct MASTERSRV_ADDR
{
	unsigned char ip[4];
	unsigned char port[2];
} MASTERSRV_ADDR;

static const unsigned char SERVERBROWSE_HEARTBEAT[] = {255, 255, 255, 255, 'b', 'e', 'a', 't'};

static const unsigned char SERVERBROWSE_GETLIST[] = {255, 255, 255, 255, 'r', 'e', 'q', 't'};
static const unsigned char SERVERBROWSE_LIST[] = {255, 255, 255, 255, 'l', 'i', 's', 't'};

static const unsigned char SERVERBROWSE_GETCOUNT[] = {255, 255, 255, 255, 'c', 'o', 'u', 'n'};
static const unsigned char SERVERBROWSE_COUNT[] = {255, 255, 255, 255, 's', 'i', 'z', 'e'};

static const unsigned char SERVERBROWSE_GETINFO[] = {255, 255, 255, 255, 'g', 'i', 'e', 'f'};
static const unsigned char SERVERBROWSE_INFO[] = {255, 255, 255, 255, 'i', 'n', 'f', 'o'};

static const unsigned char SERVERBROWSE_GETINFO_LAN[] = {255, 255, 255, 255, 'g', 'i', 'e', 'L'};
static const unsigned char SERVERBROWSE_INFO_LAN[] = {255, 255, 255, 255, 'i', 'n', 'f', 'L'};

static const unsigned char SERVERBROWSE_FWCHECK[] = {255, 255, 255, 255, 'f', 'w', '?', '?'};
static const unsigned char SERVERBROWSE_FWRESPONSE[] = {255, 255, 255, 255, 'f', 'w', '!', '!'};
static const unsigned char SERVERBROWSE_FWOK[] = {255, 255, 255, 255, 'f', 'w', 'o', 'k'};
static const unsigned char SERVERBROWSE_FWERROR[] = {255, 255, 255, 255, 'f', 'w', 'e', 'r'};
