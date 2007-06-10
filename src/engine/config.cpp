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

void config_set(const char *line)
{
	char var_str[256];
	char *val_str = (char *)strchr(line, '=');
	if (val_str)
	{
		memcpy(var_str, line, val_str - line);
		var_str[val_str - line] = 0;
		++val_str;

		#define MACRO_CONFIG_INT(name,def,min,max) { if (strcmp(#name, var_str) == 0) config_set_ ## name (&config, atoi(val_str)); }
    	#define MACRO_CONFIG_STR(name,len,def) { if (strcmp(#name, var_str) == 0) { config_set_ ## name (&config, val_str); } }
 
    	#include "config_variables.h" 
 
    	#undef MACRO_CONFIG_INT 
    	#undef MACRO_CONFIG_STR 
	}
}

void config_load(const char *filename)
{
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
