/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

CConfiguration g_Config;

class CConfig : public IConfig
{
	IStorage *m_pStorage;
	IOHANDLE m_ConfigFile;

	struct CCallback
	{
		SAVECALLBACKFUNC m_pfnFunc;
		void *m_pUserData;
	};

	enum
	{
		MAX_CALLBACKS = 16
	};

	CCallback m_aCallbacks[MAX_CALLBACKS];
	int m_NumCallbacks;

	void EscapeParam(char *pDst, const char *pSrc, int size)
	{
		for(int i = 0; *pSrc && i < size - 1; ++i)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst = 0;
	}

public:

	CConfig()
	{
		m_ConfigFile = 0;
		m_NumCallbacks = 0;
	}

	virtual void Init()
	{
		m_pStorage = Kernel()->RequestInterface<IStorage>();
		Reset();
	}

	virtual void Reset()
	{
		#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) g_Config.m_##Name = def;
		#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) str_copy(g_Config.m_##Name, def, len);

		#include "config_variables.h"

		#undef MACRO_CONFIG_INT
		#undef MACRO_CONFIG_STR
	}

	virtual void RestoreStrings()
	{
		#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc)	// nop
		#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) if(!g_Config.m_##Name[0] && def[0]) str_copy(g_Config.m_##Name, def, len);

		#include "config_variables.h"

		#undef MACRO_CONFIG_INT
		#undef MACRO_CONFIG_STR
	}

	virtual void Save()
	{
		if(!m_pStorage)
			return;
		m_ConfigFile = m_pStorage->OpenFile("settings.cfg", IOFLAG_WRITE, IStorage::TYPE_SAVE);

		if(!m_ConfigFile)
			return;

		char aLineBuf[1024*2];
		char aEscapeBuf[1024*2];

		#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) if((flags)&CFGFLAG_SAVE){ str_format(aLineBuf, sizeof(aLineBuf), "%s %i", #ScriptName, g_Config.m_##Name); WriteLine(aLineBuf); }
		#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) if((flags)&CFGFLAG_SAVE){ EscapeParam(aEscapeBuf, g_Config.m_##Name, sizeof(aEscapeBuf)); str_format(aLineBuf, sizeof(aLineBuf), "%s \"%s\"", #ScriptName, aEscapeBuf); WriteLine(aLineBuf); }

		#include "config_variables.h"

		#undef MACRO_CONFIG_INT
		#undef MACRO_CONFIG_STR

		for(int i = 0; i < m_NumCallbacks; i++)
			m_aCallbacks[i].m_pfnFunc(this, m_aCallbacks[i].m_pUserData);

		io_close(m_ConfigFile);
		m_ConfigFile = 0;
	}

	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData)
	{
		dbg_assert(m_NumCallbacks < MAX_CALLBACKS, "too many config callbacks");
		m_aCallbacks[m_NumCallbacks].m_pfnFunc = pfnFunc;
		m_aCallbacks[m_NumCallbacks].m_pUserData = pUserData;
		m_NumCallbacks++;
	}

	virtual void WriteLine(const char *pLine)
	{
		if(!m_ConfigFile)
			return;
		io_write(m_ConfigFile, pLine, str_length(pLine));
		io_write_newline(m_ConfigFile);
	}
};

IConfig *CreateConfig() { return new CConfig; }
