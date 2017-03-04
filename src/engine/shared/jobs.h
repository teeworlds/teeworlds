/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_JOBS_H
#define ENGINE_SHARED_JOBS_H
typedef int (*JOBFUNC)(void *pData);

class CJobPool;

class CJob
{
	friend class CJobPool;

	CJob *m_pPrev;
	CJob *m_pNext;

	volatile int m_Status;
	volatile int m_Result;

	JOBFUNC m_pfnFunc;
	void *m_pFuncData;
public:
	CJob()
	{
		m_Status = STATE_DONE;
		m_pFuncData = 0;
	}

	enum
	{
		STATE_PENDING=0,
		STATE_RUNNING,
		STATE_DONE
	};

	int Status() const { return m_Status; }
	int Result() const {return m_Result; }
};

class CJobPool
{
	LOCK m_Lock;
	CJob *m_pFirstJob;
	CJob *m_pLastJob;

	static void WorkerThread(void *pUser);

public:
	CJobPool();

	int Init(int NumThreads);
	int Add(CJob *pJob, JOBFUNC pfnFunc, void *pData);
};
#endif
