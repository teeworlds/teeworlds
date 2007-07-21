static const char *MASTERSERVER_ADDRESS = "localhost";
static const int MASTERSERVER_PORT = 8383;

static const int MAX_SERVERS = 200;

/*enum {
	SERVERBROWSE_NULL=0,
	SERVERBROWSE_HEARTBEAT,
	SERVERBROWSE_GETLIST,
	SERVERBROWSE_LIST,
	SERVERBROWSE_GETINFO,
	SERVERBROWSE_INFO,
};*/

static const unsigned char SERVERBROWSE_HEARTBEAT[] = {255, 255, 255, 255, 'b', 'e', 'a', 't'};
static const unsigned char SERVERBROWSE_GETLIST[] = {255, 255, 255, 255, 'r', 'e', 'q', 't'};
static const unsigned char SERVERBROWSE_LIST[] = {255, 255, 255, 255, 'l', 'i', 's', 't'};
static const unsigned char SERVERBROWSE_GETINFO[] = {255, 255, 255, 255, 'g', 'i', 'e', 'f'};
static const unsigned char SERVERBROWSE_INFO[] = {255, 255, 255, 255, 'i', 'n', 'f', 'o'};
