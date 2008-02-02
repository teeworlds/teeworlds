/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

const char *engine_savepath(const char *filename, char *buffer, int max);
void engine_init(const char *appname);
void engine_parse_arguments(int argc, char **argv);
void engine_writeconfig();
