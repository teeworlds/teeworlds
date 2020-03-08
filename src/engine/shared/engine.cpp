/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h> // srand

#include <base/system.h>

#include <engine/console.h>
#include <engine/engine.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>


static int HostLookupThread(void *pUser)
{
	CHostLookup *pLookup = (CHostLookup *)pUser;
	return net_host_lookup(pLookup->m_aHostname, &pLookup->m_Addr, pLookup->m_Nettype);
}

class CEngine : public IEngine
{
public:
	CConfig *m_pConfig;
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	bool m_Logging;
	IOHANDLE m_DataLogSent;
	IOHANDLE m_DataLogRecv;
	const char *m_pAppname;

	static void Con_DbgLognetwork(IConsole::IResult *pResult, void *pUserData)
	{
		CEngine *pEngine = static_cast<CEngine *>(pUserData);

		if(pEngine->m_Logging)
		{
			pEngine->StopLogging();
		}
		else
		{
			char aBuf[32];
			str_timestamp(aBuf, sizeof(aBuf));
			char aFilenameSent[128], aFilenameRecv[128];
			str_format(aFilenameSent, sizeof(aFilenameSent), "dumps/%s_network_sent_%s.txt", pEngine->m_pAppname, aBuf);
			str_format(aFilenameRecv, sizeof(aFilenameRecv), "dumps/%s_network_recv_%s.txt", pEngine->m_pAppname, aBuf);
			pEngine->StartLogging(pEngine->m_pStorage->OpenFile(aFilenameSent, IOFLAG_WRITE, IStorage::TYPE_SAVE),
									pEngine->m_pStorage->OpenFile(aFilenameRecv, IOFLAG_WRITE, IStorage::TYPE_SAVE));
		}
	}

	CEngine(const char *pAppname)
	{
		srand(time_get());
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

		
		m_JobPool.Init(1);

		m_DataLogSent = 0;
		m_DataLogRecv = 0;
		m_Logging = false;
		m_pAppname = pAppname;
	}

	~CEngine()
	{
		StopLogging();
	}

	void Init()
	{
		m_pConfig = Kernel()->RequestInterface<IConfigManager>()->Values();
		m_pConsole = Kernel()->RequestInterface<IConsole>();
		m_pStorage = Kernel()->RequestInterface<IStorage>();

		if(!m_pConsole || !m_pStorage)
			return;

		m_pConsole->Register("dbg_lognetwork", "", CFGFLAG_SERVER|CFGFLAG_CLIENT, Con_DbgLognetwork, this, "Log the network");
	}

	void InitLogfile()
	{
		// open logfile if needed
		if(m_pConfig->m_Logfile[0])
		{
			char aBuf[32];
			if(m_pConfig->m_LogfileTimestamp)
				str_timestamp(aBuf, sizeof(aBuf));
			else
				aBuf[0] = 0;
			char aLogFilename[128];			
			str_format(aLogFilename, sizeof(aLogFilename), "dumps/%s%s.txt", m_pConfig->m_Logfile, aBuf);
			IOHANDLE Handle = m_pStorage->OpenFile(aLogFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
			if(Handle)
				dbg_logger_filehandle(Handle);
			else
				dbg_msg("engine/logfile", "failed to open '%s' for logging", aLogFilename);
		}
	}

	void QueryNetLogHandles(IOHANDLE *pHDLSend, IOHANDLE *pHDLRecv)
	{
		*pHDLSend = m_DataLogSent;
		*pHDLRecv = m_DataLogRecv;
	}

	void StartLogging(IOHANDLE DataLogSent, IOHANDLE DataLogRecv)
	{
		if(DataLogSent)
		{
			m_DataLogSent = DataLogSent;
			dbg_msg("engine", "logging network sent packages");
		}
		else
			dbg_msg("engine", "failed to start logging network sent packages");

		if(DataLogRecv)
		{
			m_DataLogRecv = DataLogRecv;
			dbg_msg("engine", "logging network recv packages");
		}
		else
			dbg_msg("engine", "failed to start logging network recv packages");

		m_Logging = true;
	}

	void StopLogging()
	{
		if(m_DataLogSent)
		{
			dbg_msg("engine", "stopped logging network sent packages");
			io_close(m_DataLogSent);
			m_DataLogSent = 0;
		}
		if(m_DataLogRecv)
		{
			dbg_msg("engine", "stopped logging network recv packages");
			io_close(m_DataLogRecv);
			m_DataLogRecv = 0;
		}
		m_Logging = false;
	}

	void HostLookup(CHostLookup *pLookup, const char *pHostname, int Nettype)
	{
		str_copy(pLookup->m_aHostname, pHostname, sizeof(pLookup->m_aHostname));
		pLookup->m_Nettype = Nettype;
		AddJob(&pLookup->m_Job, HostLookupThread, pLookup);
	}

	void AddJob(CJob *pJob, JOBFUNC pfnFunc, void *pData)
	{
		if(m_pConfig->m_Debug)
			dbg_msg("engine", "job added");
		m_JobPool.Add(pJob, pfnFunc, pData);
	}
};

IEngine *CreateEngine(const char *pAppname) { return new CEngine(pAppname); }
