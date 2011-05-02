/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdio.h>	// sscanf

#include <base/system.h>

#include <engine/engine.h>
#include <engine/masterserver.h>
#include <engine/storage.h>

#include "linereader.h"

class CMasterServer : public IEngineMasterServer
{
public:
	// master server functions
	struct CMasterInfo
	{
		char m_aHostname[128];
		NETADDR m_Addr;
		bool m_Valid;

		CHostLookup m_Lookup;
	} ;

	CMasterInfo m_aMasterServers[MAX_MASTERSERVERS];
	int m_NeedsUpdate;
	IEngine *m_pEngine;
	IStorage *m_pStorage;

	CMasterServer()
	{
		SetDefault();
		m_NeedsUpdate = -1;
		m_pEngine = 0;
	}

	virtual int RefreshAddresses(int Nettype)
	{
		int i;

		if(m_NeedsUpdate != -1)
			return 0;

		dbg_msg("engine/mastersrv", "refreshing master server addresses");

		// add lookup jobs
		for(i = 0; i < MAX_MASTERSERVERS; i++)
		{
			m_pEngine->HostLookup(&m_aMasterServers[i].m_Lookup, m_aMasterServers[i].m_aHostname, Nettype);
			m_aMasterServers[i].m_Valid = false;
		}

		m_NeedsUpdate = 1;
		return 0;
	}

	virtual void Update()
	{
		// check if we need to update
		if(m_NeedsUpdate != 1)
			return;
		m_NeedsUpdate = 0;

		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			if(m_aMasterServers[i].m_Lookup.m_Job.Status() != CJob::STATE_DONE)
				m_NeedsUpdate = 1;
			else
			{
				if(m_aMasterServers[i].m_Lookup.m_Job.Result() == 0)
				{
					m_aMasterServers[i].m_Addr = m_aMasterServers[i].m_Lookup.m_Addr;
					m_aMasterServers[i].m_Addr.port = 8300;
					m_aMasterServers[i].m_Valid = true;
				}
				else
					m_aMasterServers[i].m_Valid = false;
			}
		}

		if(!m_NeedsUpdate)
		{
			dbg_msg("engine/mastersrv", "saving addresses");
			Save();
		}
	}

	virtual int IsRefreshing()
	{
		return m_NeedsUpdate;
	}

	virtual NETADDR GetAddr(int Index)
	{
		return m_aMasterServers[Index].m_Addr;
	}

	virtual const char *GetName(int Index)
	{
		return m_aMasterServers[Index].m_aHostname;
	}

	virtual bool IsValid(int Index)
	{
		return m_aMasterServers[Index].m_Valid;
	}

	virtual void DumpServers()
	{
		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aMasterServers[i].m_Addr, aAddrStr, sizeof(aAddrStr));
			dbg_msg("mastersrv", "#%d = %s", i, aAddrStr);
		}
	}

	virtual void Init()
	{
		m_pEngine = Kernel()->RequestInterface<IEngine>();
		m_pStorage = Kernel()->RequestInterface<IStorage>();
	}

	virtual void SetDefault()
	{
		mem_zero(m_aMasterServers, sizeof(m_aMasterServers));
		for(int i = 0; i < MAX_MASTERSERVERS; i++)
			str_format(m_aMasterServers[i].m_aHostname, sizeof(m_aMasterServers[i].m_aHostname), "master%d.teeworlds.com", i+1);
	}

	virtual int Load()
	{
		CLineReader LineReader;
		IOHANDLE File;
		int Count = 0;
		if(!m_pStorage)
			return -1;

		// try to open file
		File = m_pStorage->OpenFile("masters.cfg", IOFLAG_READ, IStorage::TYPE_SAVE);
		if(!File)
			return -1;

		LineReader.Init(File);
		while(1)
		{
			CMasterInfo Info = {{0}};
			const char *pLine = LineReader.Get();
			if(!pLine)
				break;

			// parse line
			char aAddrStr[NETADDR_MAXSTRSIZE];
			if(sscanf(pLine, "%s %s", Info.m_aHostname, aAddrStr) == 2 && net_addr_from_str(&Info.m_Addr, aAddrStr) == 0)
			{
				Info.m_Addr.port = 8300;
				if(Count != MAX_MASTERSERVERS)
				{
					m_aMasterServers[Count] = Info;
					Count++;
				}
				//else
				//	dbg_msg("engine/mastersrv", "warning: skipped master server '%s' due to limit of %d", pLine, MAX_MASTERSERVERS);
			}
			//else
			//	dbg_msg("engine/mastersrv", "warning: couldn't parse master server '%s'", pLine);
		}

		io_close(File);
		return 0;
	}

	virtual int Save()
	{
		IOHANDLE File;

		if(!m_pStorage)
			return -1;

		// try to open file
		File = m_pStorage->OpenFile("masters.cfg", IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(!File)
			return -1;

		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aMasterServers[i].m_Addr, aAddrStr, sizeof(aAddrStr));
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "%s %s\n", m_aMasterServers[i].m_aHostname, aAddrStr);

			io_write(File, aBuf, str_length(aBuf));
		}

		io_close(File);
		return 0;
	}
};

IEngineMasterServer *CreateEngineMasterServer() { return new CMasterServer; }
