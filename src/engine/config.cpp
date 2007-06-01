#include <baselib/system.h>

#include <cstring>
#include <cstdio>

#include "config.h"

configuration config;

void config_reset()
{
    #define MACRO_CONFIG_INT(name,def,min,max) config.name = def;
    #define MACRO_CONFIG_STR(name,len,def) strncpy(config.name, def, len);
 
    #include "config_variables.h" 
 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
}

void config_load(const char *filename)
{
	dbg_msg("config/load", "loading %s", filename);
}

void config_save(const char *filename)
{
	dbg_msg("config/save", "saving config to %s", filename);
}

#define MACRO_CONFIG_INT(name,def,min,max) void config_set_ ## name (configuration *c, int val) { if (val < min) val = min; if (max != 0 && val > max) val = max; c->name = val; }
#define MACRO_CONFIG_STR(name,len,def) void config_set_ ## name (configuration *c, char *str) { strncpy(c->name, def, len-1); c->name[sizeof(c->name)-1] = 0; }
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR
