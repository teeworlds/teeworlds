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

	virtual int RefreshAddresses()
	{
		int i;
		
		if(m_NeedsUpdate != -1)
			return 0;
		
		dbg_msg("engine/mastersrv", "refreshing master server addresses");

		// add lookup jobs
		for(i = 0; i < MAX_MASTERSERVERS; i++)	
			m_pEngine->HostLookup(&m_aMasterServers[i].m_Lookup, m_aMasterServers[i].m_aHostname);
		
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
				m_aMasterServers[i].m_Addr = m_aMasterServers[i].m_Lookup.m_Addr;
				m_aMasterServers[i].m_Addr.port = 8300;
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

	virtual void DumpServers()
	{
		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			dbg_msg("mastersrv", "#%d = %d.%d.%d.%d", i,
				m_aMasterServers[i].m_Addr.ip[0], m_aMasterServers[i].m_Addr.ip[1],
				m_aMasterServers[i].m_Addr.ip[2], m_aMasterServers[i].m_Addr.ip[3]);
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
			int aIp[4];
			const char *pLine = LineReader.Get();
			if(!pLine)
				break;

			// parse line	
			if(sscanf(pLine, "%s %d.%d.%d.%d", Info.m_aHostname, &aIp[0], &aIp[1], &aIp[2], &aIp[3]) == 5)
			{
				Info.m_Addr.ip[0] = (unsigned char)aIp[0];
				Info.m_Addr.ip[1] = (unsigned char)aIp[1];
				Info.m_Addr.ip[2] = (unsigned char)aIp[2];
				Info.m_Addr.ip[3] = (unsigned char)aIp[3];
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
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "%s %d.%d.%d.%d\n", m_aMasterServers[i].m_aHostname,
				m_aMasterServers[i].m_Addr.ip[0], m_aMasterServers[i].m_Addr.ip[1],
				m_aMasterServers[i].m_Addr.ip[2], m_aMasterServers[i].m_Addr.ip[3]);
				
			io_write(File, aBuf, str_length(aBuf));
		}
		
		io_close(File);
		return 0;
	}
};

IEngineMasterServer *CreateEngineMasterServer() { return new CMasterServer; }
