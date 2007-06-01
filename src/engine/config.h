#ifndef _CONFIG_H
#define _CONFIG_H

struct configuration
{ 
    #define MACRO_CONFIG_INT(name,def,min,max) int name;
    #define MACRO_CONFIG_STR(name,len,def) char name[len];
    #include "config_variables.h" 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
}; 

extern configuration config;

void config_reset();
void config_load(const char *filename);
void config_save(const char *filename);

#define MACRO_CONFIG_INT(name,def,min,max) int config_get_ ## name (configuration *c);
#define MACRO_CONFIG_STR(name,len,def) const char *config_get_ ## name (configuration *c);
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#define MACRO_CONFIG_INT(name,def,min,max) void config_set_ ## name (configuration *c, int val);
#define MACRO_CONFIG_STR(name,len,def) void config_set_ ## name (configuration *c, const char *str);
#include "config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#endif
