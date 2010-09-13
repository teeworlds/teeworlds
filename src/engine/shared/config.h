// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_SHARED_E_CONFIG_H
#define ENGINE_SHARED_E_CONFIG_H

struct CConfiguration
{ 
    #define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) int m_##Name;
    #define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) char m_##Name[Len]; // Flawfinder: ignore
    #include "config_variables.h" 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
};

extern CConfiguration g_Config;

enum
{
	CFGFLAG_SAVE=1,
	CFGFLAG_CLIENT=2,
	CFGFLAG_SERVER=4,
	CFGFLAG_STORE=8
};

#endif
