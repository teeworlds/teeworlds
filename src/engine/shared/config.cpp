/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/console.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <game/version.h>


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

static void Con_SaveConfig(IConsole::IResult *pResult, void *pUserData)
{
	char aFilename[128];
	if(pResult->NumArguments())
		str_format(aFilename, sizeof(aFilename), "configs/%s.cfg", pResult->GetString(0));
	else
	{
		char aDate[20];
		str_timestamp(aDate, sizeof(aDate));
		str_format(aFilename, sizeof(aFilename), "configs/config_%s.cfg", aDate);
	}
	((CConfigManager *)pUserData)->Save(aFilename);
}

CConfigManager::CConfigManager()
{
	m_pStorage = 0;
	m_pConsole = 0;
	m_ConfigFile = 0;
	m_FlagMask = 0;
	m_NumCallbacks = 0;
}

void CConfigManager::Init(int FlagMask)
{
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_FlagMask = FlagMask;
	Reset();

	if(m_pConsole)
		m_pConsole->Register("save_config", "?s[file]", CFGFLAG_SERVER|CFGFLAG_CLIENT|CFGFLAG_STORE, Con_SaveConfig, this, "Save config to file");
}

void CConfigManager::Reset()
{
	#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) m_Values.m_##Name = def;
	#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) str_copy(m_Values.m_##Name, def, len);

	#include "config_variables.h"

	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR
}

void CConfigManager::RestoreStrings()
{
	#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc)	// nop
	#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) if(!m_Values.m_##Name[0] && def[0]) str_copy(m_Values.m_##Name, def, len);

	#include "config_variables.h"

	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR
}

void CConfigManager::Save(const char *pFilename)
{
	if(!m_pStorage)
		return;

	if(!pFilename)
		pFilename = SETTINGS_FILENAME ".cfg";
	m_ConfigFile = m_pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);

	if(!m_ConfigFile)
		return;

	WriteLine("# Teeworlds " GAME_VERSION);

	char aLineBuf[1024*2];
	char aEscapeBuf[1024*2];

	#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) if(((flags)&(CFGFLAG_SAVE))&&((flags)&(m_FlagMask))&&(m_Values.m_##Name!=int(def))){ str_format(aLineBuf, sizeof(aLineBuf), "%s %i", #ScriptName, m_Values.m_##Name); WriteLine(aLineBuf); }
	#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) if(((flags)&(CFGFLAG_SAVE))&&((flags)&(m_FlagMask)&&(str_comp(m_Values.m_##Name,def)))){ EscapeParam(aEscapeBuf, m_Values.m_##Name, sizeof(aEscapeBuf)); str_format(aLineBuf, sizeof(aLineBuf), "%s \"%s\"", #ScriptName, aEscapeBuf); WriteLine(aLineBuf); }

	#include "config_variables.h"

	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR

	for(int i = 0; i < m_NumCallbacks; i++)
		m_aCallbacks[i].m_pfnFunc(this, m_aCallbacks[i].m_pUserData);

	io_close(m_ConfigFile);

	if(m_pConsole)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "saved config to '%s'", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "config", aBuf);
	}
}

void CConfigManager::RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData)
{
	dbg_assert(m_NumCallbacks < MAX_CALLBACKS, "too many config callbacks");
	m_aCallbacks[m_NumCallbacks].m_pfnFunc = pfnFunc;
	m_aCallbacks[m_NumCallbacks].m_pUserData = pUserData;
	m_NumCallbacks++;
}

void CConfigManager::WriteLine(const char *pLine)
{
	if(!m_ConfigFile)
		return;

	io_write(m_ConfigFile, pLine, str_length(pLine));
	io_write_newline(m_ConfigFile);
}

IConfigManager *CreateConfigManager() { return new CConfigManager; }
