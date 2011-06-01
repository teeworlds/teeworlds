/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_ENGINE_H
#define ENGINE_SHARED_ENGINE_H

#include <engine/engine.h>

class CEngine : public IEngine
{
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	CJobPool m_JobPool;
	bool m_Logging;

	static void Con_DbgDumpmem(IConsole::IResult *pResult, void *pUserData);
	static void Con_DbgLognetwork(IConsole::IResult *pResult, void *pUserData);

public:
	CEngine(const char *pAppname);

	virtual void Init();
	virtual void InitLogfile();
	virtual void HostLookup(CHostLookup *pLookup, const char *pHostname, int Nettype);
	virtual void AddJob(CJob *pJob, JOBFUNC pfnFunc, void *pData);
};

#endif
