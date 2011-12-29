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
	};

	enum
	{
		STATE_INIT,
		STATE_UPDATE,
		STATE_READY,
	};

	CMasterInfo m_aMasterServers[MAX_MASTERSERVERS];
	int m_State;
	IEngine *m_pEngine;
	IStorage *m_pStorage;

	CMasterServer()
	{
		SetDefault();
		m_State = STATE_INIT;
		m_pEngine = 0;
		m_pStorage = 0;
	}

	virtual int RefreshAddresses(int Nettype)
	{
		if(m_State != STATE_INIT)
			return -1;

		dbg_msg("engine/mastersrv", "refreshing master server addresses");

		// add lookup jobs
		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			m_pEngine->HostLookup(&m_aMasterServers[i].m_Lookup, m_aMasterServers[i].m_aHostname, Nettype);
			m_aMasterServers[i].m_Valid = false;
		}

		m_State = STATE_UPDATE;
		return 0;
	}

	virtual void Update()
	{
		// check if we need to update
		if(m_State != STATE_UPDATE)
			return;
		m_State = STATE_READY;

		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			if(m_aMasterServers[i].m_Lookup.m_Job.Status() != CJob::STATE_DONE)
				m_State = STATE_UPDATE;
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

		if(m_State == STATE_READY)
		{
			dbg_msg("engine/mastersrv", "saving addresses");
			Save();
		}
	}

	virtual int IsRefreshing()
	{
		return m_State != STATE_READY;
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
		if(!m_pStorage)
			return -1;

		// try to open file
		IOHANDLE File = m_pStorage->OpenFile("masters.cfg", IOFLAG_READ, IStorage::TYPE_SAVE);
		if(!File)
			return -1;

		CLineReader LineReader;
		LineReader.Init(File);
		while(1)
		{
			CMasterInfo Info = {{0}};
			const char *pLine = LineReader.Get();
			if(!pLine)
				break;

			// parse line
			char aAddrStr[NETADDR_MAXSTRSIZE];
			if(sscanf(pLine, "%127s %47s", Info.m_aHostname, aAddrStr) == 2 && net_addr_from_str(&Info.m_Addr, aAddrStr) == 0)
			{
				Info.m_Addr.port = 8300;
				bool Added = false;
				for(int i = 0; i < MAX_MASTERSERVERS; ++i)
					if(str_comp(m_aMasterServers[i].m_aHostname, Info.m_aHostname) == 0)
					{
						m_aMasterServers[i] = Info;
						Added = true;
						break;
					}

				if(!Added)
				{
					for(int i = 0; i < MAX_MASTERSERVERS; ++i)
						if(m_aMasterServers[i].m_Addr.type == NETTYPE_INVALID)
						{
							m_aMasterServers[i] = Info;
							Added = true;
							break;
						}
				}

				if(!Added)
					break;
			}
		}

		io_close(File);
		return 0;
	}

	virtual int Save()
	{
		if(!m_pStorage)
			return -1;

		// try to open file
		IOHANDLE File = m_pStorage->OpenFile("masters.cfg", IOFLAG_WRITE, IStorage::TYPE_SAVE);
		if(!File)
			return -1;

		for(int i = 0; i < MAX_MASTERSERVERS; i++)
		{
			char aAddrStr[NETADDR_MAXSTRSIZE];
			if(m_aMasterServers[i].m_Addr.type != NETTYPE_INVALID)
				net_addr_str(&m_aMasterServers[i].m_Addr, aAddrStr, sizeof(aAddrStr), true);
			else
				aAddrStr[0] = 0;
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "%s %s", m_aMasterServers[i].m_aHostname, aAddrStr);
			io_write(File, aBuf, str_length(aBuf));
			io_write_newline(File);
		}

		io_close(File);
		return 0;
	}
};

IEngineMasterServer *CreateEngineMasterServer() { return new CMasterServer; }
