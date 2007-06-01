#include <baselib/system.h>

#include <fstream>
#include <iostream>

#include <cstring>
#include <cstdio>

#include "config.h"

using namespace std;

configuration config;

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
	char var_str[128];
	char *val_str = strchr(line, '=');
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

	ifstream file(filename);
	string line;

	if (file)
	{
		while (getline(file, line))
		{
			config_set(line.c_str());
		}

		file.close();
	}
}

void config_save(const char *filename)
{
	dbg_msg("config/save", "saving config to %s", filename);

	ofstream file(filename);

    #define MACRO_CONFIG_INT(name,def,min,max) { file << # name << '=' << config.name << endl; }
    #define MACRO_CONFIG_STR(name,len,def) { file << # name << '=' << config.name << endl; }
 
    #include "config_variables.h" 
 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 

	file.close();
}

#define MACRO_CONFIG_INT(name,def,min,max) void config_set_ ## name (configuration *c, int val) { if (val < min) val = min; if (max != 0 && val > max) val = max; c->name = val; }
#define MACRO_CONFIG_STR(name,len,def) void config_set_ ## name (configuration *c, const char *str) { strncpy(c->name, str, len-1); c->name[sizeof(c->name)-1] = 0; }
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR
