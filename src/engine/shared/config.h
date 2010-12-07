/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_E_CONFIG_H
#define ENGINE_SHARED_E_CONFIG_H

struct CConfiguration
{ 
    #define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc,Level) int m_##Name;
    #define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc,Level) char m_##Name[Len]; // Flawfinder: ignore
    #include "config_variables.h" 
    #undef MACRO_CONFIG_INT 
    #undef MACRO_CONFIG_STR 
};

extern CConfiguration g_Config;



void dbg_msg1(const char * where, const char * format, ...);



enum
{
	CFGFLAG_SAVE=1,
	CFGFLAG_CLIENT=2,
	CFGFLAG_SERVER=4,
	CFGFLAG_STORE=8,
	CMDFLAG_CHEAT=16,
	CMDFLAG_TIMER=32,
	CMDFLAG_HELPERCMD=64,
};

#endif
