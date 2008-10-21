/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include "e_jobs.h"

const char *engine_savepath(const char *filename, char *buffer, int max);
void engine_init(const char *appname);
void engine_parse_arguments(int argc, char **argv);

int engine_config_write_start();
void engine_config_write_line(const char *line);
void engine_config_write_stop();


enum
{
	LISTDIRTYPE_SAVE=1,
	LISTDIRTYPE_CURRENT=2,
	LISTDIRTYPE_DATA=4,
	LISTDIRTYPE_ALL = ~0
};

void engine_listdir(int types, const char *path, FS_LISTDIR_CALLBACK cb, void *user);
IOHANDLE engine_openfile(const char *filename, int flags);
void engine_getpath(char *buffer, int buffer_size, const char *filename, int flags);

int engine_stress(float probability);

typedef struct HOSTLOOKUP
{
	JOB job;
	char hostname[128];
	NETADDR addr;
} HOSTLOOKUP;

void engine_hostlookup(HOSTLOOKUP *lookup, const char *hostname);

enum
{
	MAX_MASTERSERVERS=16
};

void mastersrv_default();
int mastersrv_load();
int mastersrv_save();

int mastersrv_refresh_addresses();
void mastersrv_update();
int mastersrv_refreshing();
void mastersrv_dump_servers();
NETADDR mastersrv_get(int index);
const char *mastersrv_name(int index);
