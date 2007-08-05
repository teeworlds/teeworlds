#include <baselib/system.h>
#include <baselib/stream/file.h>
#include <baselib/stream/line.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "config.h"

configuration config;

using namespace baselib;

void config_reset()
{
    #define MACRO_CONFIG_INT(name,def,min,max) config.name = def;
    #define MACRO_CONFIG_STR(name,len,def) strncpy(config.name, def, len);
 
    #include "config_variables.h" 
 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
}

void strip_spaces(char **p)
{
	char *&s = *p;
	
	while (*s == ' ')
		++s;

	char *end = s + strlen(s);
	while (end > s && *(end - 1) == ' ')
		*--end = 0;
}

void config_set(const char *line)
{
	const char *c = strchr(line, '=');
	if (c)
	{
		char var[256];
		char val[256];

		strcpy(val, c+1);

		mem_copy(var, line, c - line);
		var[c - line] = 0;

		char *var_str = var;
		char *val_str = val;

		strip_spaces(&var_str);
		strip_spaces(&val_str);

		#define MACRO_CONFIG_INT(name,def,min,max) { if (strcmp(#name, var_str) == 0) config_set_ ## name (&config, atoi(val_str)); }
    	#define MACRO_CONFIG_STR(name,len,def) { if (strcmp(#name, var_str) == 0) { config_set_ ## name (&config, val_str); } }
 
    	#include "config_variables.h" 
 
    	#undef MACRO_CONFIG_INT 
    	#undef MACRO_CONFIG_STR 
	}
}

void config_load(const char *filename)
{
	if (filename[0] == '~')
	{
		char *home = getenv("HOME");
		if (home)
		{
			char full_path[1024];
			sprintf(full_path, "%s%s", home, filename+1);
			filename = full_path;
		}
	}


	dbg_msg("config/load", "loading %s", filename);

	file_stream file;

	if (file.open_r(filename))
	{
		char *line;
		line_stream lstream(&file);

		while ((line = lstream.get_line()))
			config_set(line);

		file.close();
	}
}

void config_save(const char *filename)
{
	if (filename[0] == '~')
	{
		char *home = getenv("HOME");
		if (home)
		{
			char full_path[1024];
			sprintf(full_path, "%s%s", home, filename+1);
			filename = full_path;
		}
	}


	dbg_msg("config/save", "saving config to %s", filename);

	file_stream file;

	if (file.open_w(filename))
	{
		
    	#define MACRO_CONFIG_INT(name,def,min,max) { char str[256]; sprintf(str, "%s=%i", #name, config.name); file.write(str, strlen(str)); file.write("\n", 1); }
    	#define MACRO_CONFIG_STR(name,len,def) { file.write(#name, strlen(#name)); file.write("=", 1); file.write(config.name, strlen(config.name)); file.write("\n", 1); }
 
    	#include "config_variables.h" 
 
    	#undef MACRO_CONFIG_INT 
    	#undef MACRO_CONFIG_STR 

		file.close();
	}
	else
		dbg_msg("config/save", "couldn't open %s for writing. :(", filename);
}

#define MACRO_CONFIG_INT(name,def,min,max) int config_get_ ## name (configuration *c) { return c->name; }
#define MACRO_CONFIG_STR(name,len,def) const char *config_get_ ## name (configuration *c) { return c->name; }
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#define MACRO_CONFIG_INT(name,def,min,max) void config_set_ ## name (configuration *c, int val) { if (val < min) val = min; if (max != 0 && val > max) val = max; c->name = val; }
#define MACRO_CONFIG_STR(name,len,def) void config_set_ ## name (configuration *c, const char *str) { strncpy(c->name, str, len-1); c->name[sizeof(c->name)-1] = 0; }
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR
