/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct
{ 
    #define MACRO_CONFIG_INT(name,def,min,max,save,desc) int name;
    #define MACRO_CONFIG_STR(name,len,def,save,desc) char name[len]; /* Flawfinder: ignore */
    #include "e_config_variables.h" 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
} CONFIGURATION;

extern CONFIGURATION config;

void config_init();
void config_reset();
void config_save();

enum
{
	CFGFLAG_SAVE=1,
	CFGFLAG_CLIENT=2,
	CFGFLAG_SERVER=4
};

typedef int (*CONFIG_INT_GETTER)(CONFIGURATION *c);
typedef const char *(*CONFIG_STR_GETTER)(CONFIGURATION *c);
typedef void (*CONFIG_INT_SETTER)(CONFIGURATION *c, int val);
typedef void (*CONFIG_STR_SETTER)(CONFIGURATION *c, const char *str);

#define MACRO_CONFIG_INT(name,def,min,max,flags,desc) int config_get_ ## name (CONFIGURATION *c);
#define MACRO_CONFIG_STR(name,len,def,flags,desc) const char *config_get_ ## name (CONFIGURATION *c);
#include "e_config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#define MACRO_CONFIG_INT(name,def,min,max,flags,desc) void config_set_ ## name (CONFIGURATION *c, int val);
#define MACRO_CONFIG_STR(name,len,def,flags,desc) void config_set_ ## name (CONFIGURATION *c, const char *str);
#include "e_config_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

#ifdef __cplusplus
}
#endif

#endif
