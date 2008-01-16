/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct
{ 
    #define MACRO_CONFIG_INT(name,def,min,max) int name;
    #define MACRO_CONFIG_STR(name,len,def) char name[len];
    #include "e_config_variables.h" 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
} CONFIGURATION;

extern CONFIGURATION config;

void config_init();
void config_set(const char *line);
void config_reset();
void config_load(const char *filename);
void config_save(const char *filename);

typedef int (*config_int_getter)(CONFIGURATION *c);
typedef const char *(*config_str_getter)(CONFIGURATION *c);
typedef void (*config_int_setter)(CONFIGURATION *c, int val);
typedef void (*config_str_setter)(CONFIGURATION *c, const char *str);

#define MACRO_CONFIG_INT(name,def,min,max) int config_get_ ## name (CONFIGURATION *c);
#define MACRO_CONFIG_STR(name,len,def) const char *config_get_ ## name (CONFIGURATION *c);
#include "e_config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#define MACRO_CONFIG_INT(name,def,min,max) void config_set_ ## name (CONFIGURATION *c, int val);
#define MACRO_CONFIG_STR(name,len,def) void config_set_ ## name (CONFIGURATION *c, const char *str);
#include "e_config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#ifdef __cplusplus
}
#endif

#endif
