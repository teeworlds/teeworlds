/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

const char *engine_savepath(const char *filename, char *buffer, int max);
void engine_init(const char *appname);
void engine_parse_arguments(int argc, char **argv);
void engine_writeconfig();
int engine_stress(float probability);


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
NETADDR4 mastersrv_get(int index);
const char *mastersrv_name(int index);
