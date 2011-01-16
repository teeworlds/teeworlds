/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_E_ENGINE_H
#define ENGINE_SHARED_E_ENGINE_H

#include "jobs.h"

class CHostLookup
{
public:
	CJob m_Job;
	char m_aHostname[128];
	NETADDR m_Addr;
};

class CEngine
{
	class CJobPool m_HostLookupPool;

public:
	void Init(const char *pAppname);
	void InitLogfile();
	void HostLookup(CHostLookup *pLookup, const char *pHostname);
};

#endif
