/* CWebapp Class by Sushi and Redix*/
#ifndef GAME_WEBAPP_H
#define GAME_WEBAPP_H

#include <base/tl/array.h>
#include <engine/shared/jobs.h>

#include "data.h"

class CWebapp
{
	class IStorage *m_pStorage;

	CJobPool m_JobPool;

	NETADDR m_Addr;
	NETSOCKET m_Socket;

	array<CJob*> m_Jobs;

	bool m_Online;

public:

	class CHeader
	{
	public:
		int m_Size;
		int m_StatusCode;
		long m_ContentLength;
	};

	LOCK m_OutputLock;

	IDataOut *m_pFirst;
	IDataOut *m_pLast;

	CWebapp(class IStorage *pStorage, const char* WebappIp);
	virtual ~CWebapp();

	class IStorage *Storage() { return m_pStorage; }

	NETADDR Addr() { return m_Addr; }
	NETSOCKET Socket() {return m_Socket; }

	bool IsOnline() { return m_Online; }
	void SetOnline(bool Online) { m_Online = Online; }

	void AddOutput(class IDataOut *pOut);

	bool Connect();
	void Disconnect();

	int UpdateJobs();

	int GetHeaderInfo(char *pStr, int MaxSize, CHeader *pHeader);
	int RecvHeader(char *pBuf, int MaxSize, CHeader *pHeader);

	int SendAndReceive(const char *pInString, char **ppOutString);

	CJob *AddJob(JOBFUNC pfnFunc, class IDataIn *pUserData, bool NeedOnline = 1);
};

#endif
