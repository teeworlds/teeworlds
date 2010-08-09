// copyright (c) 2007 magnus auvinen, see licence.txt for more info

#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/engine.h>
#include <engine/shared/network.h>
#include <engine/console.h>
#include "linereader.h"

// compiled-in data-dir path
#define DATA_DIR "data"

//static int engine_find_datadir(char *argv0);
/*
static void con_dbg_dumpmem(IConsole::IResult *result, void *user_data)
{
	mem_debug_dump();
}

static void con_dbg_lognetwork(IConsole::IResult *result, void *user_data)
{
	CNetBase::OpenLog("network_sent.dat", "network_recv.dat");
}*/

/*
static char application_save_path[512] = {0};
static char datadir[512] = {0};

const char *engine_savepath(const char *filename, char *buffer, int max)
{
	str_format(buffer, max, "%s/%s", application_save_path, filename);
	return buffer;
}*/

void CEngine::Init(const char *pAppname)
{
	dbg_logger_stdout();
	dbg_logger_debugger();
	
	//
	dbg_msg("engine", "running on %s-%s-%s", CONF_FAMILY_STRING, CONF_PLATFORM_STRING, CONF_ARCH_STRING);
#ifdef CONF_ARCH_ENDIAN_LITTLE
	dbg_msg("engine", "arch is little endian");
#elif defined(CONF_ARCH_ENDIAN_BIG)
	dbg_msg("engine", "arch is big endian");
#else
	dbg_msg("engine", "unknown endian");
#endif

	// init the network
	net_init();
	CNetBase::Init();
	
	m_HostLookupPool.Init(1);

	//MACRO_REGISTER_COMMAND("dbg_dumpmem", "", CFGFLAG_SERVER|CFGFLAG_CLIENT, con_dbg_dumpmem, 0x0, "Dump the memory");
	//MACRO_REGISTER_COMMAND("dbg_lognetwork", "", CFGFLAG_SERVER|CFGFLAG_CLIENT, con_dbg_lognetwork, 0x0, "Log the network");
	
	// reset the config
	//config_reset();
}

void CEngine::InitLogfile()
{
	// open logfile if needed
	if(g_Config.m_Logfile[0])
		dbg_logger_file(g_Config.m_Logfile);
}

static int HostLookupThread(void *pUser)
{
	CHostLookup *pLookup = (CHostLookup *)pUser;
	net_host_lookup(pLookup->m_aHostname, &pLookup->m_Addr, NETTYPE_IPV4);
	return 0;
}

void CEngine::HostLookup(CHostLookup *pLookup, const char *pHostname)
{
	str_copy(pLookup->m_aHostname, pHostname, sizeof(pLookup->m_aHostname));
	m_HostLookupPool.Add(&pLookup->m_Job, HostLookupThread, pLookup);
}
