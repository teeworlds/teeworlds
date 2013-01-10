/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "jobs.h"

CJobPool::CJobPool()
{
	// empty the pool
	m_NumThreads = 0;
	m_Shutdown = false;
	m_Lock = lock_create();
	m_pFirstJob = 0;
	m_pLastJob = 0;
}

CJobPool::~CJobPool()
{
	m_Shutdown = true;
	for(int i = 0; i < m_NumThreads; i++)
	{
		thread_wait(m_apThreads[i]);
		thread_destroy(m_apThreads[i]);
	}
}

void CJobPool::WorkerThread(void *pUser)
{
	CJobPool *pPool = (CJobPool *)pUser;

	while(!pPool->m_Shutdown)
	{
		CJob *pJob = 0;

		// fetch job from queue
		lock_wait(pPool->m_Lock);
		if(pPool->m_pFirstJob)
		{
			pJob = pPool->m_pFirstJob;
			pPool->m_pFirstJob = pPool->m_pFirstJob->m_pNext;
			if(pPool->m_pFirstJob)
				pPool->m_pFirstJob->m_pPrev = 0;
			else
				pPool->m_pLastJob = 0;
		}
		lock_release(pPool->m_Lock);

		// do the job if we have one
		if(pJob)
		{
			pJob->m_Status = CJob::STATE_RUNNING;
			pJob->m_Result = pJob->m_pfnFunc(pJob->m_pFuncData);
			pJob->m_Status = CJob::STATE_DONE;
		}
		else
			thread_sleep(10);
	}

}

int CJobPool::Init(int NumThreads)
{
	// start threads
	m_NumThreads = NumThreads > MAX_THREADS ? MAX_THREADS : NumThreads;
	for(int i = 0; i < m_NumThreads; i++)
		m_apThreads[i] = thread_create(WorkerThread, this);
	return 0;
}

int CJobPool::Add(CJob *pJob, JOBFUNC pfnFunc, void *pData)
{
	mem_zero(pJob, sizeof(CJob));
	pJob->m_pfnFunc = pfnFunc;
	pJob->m_pFuncData = pData;

	lock_wait(m_Lock);

	// add job to queue
	pJob->m_pPrev = m_pLastJob;
	if(m_pLastJob)
		m_pLastJob->m_pNext = pJob;
	m_pLastJob = pJob;
	if(!m_pFirstJob)
		m_pFirstJob = pJob;

	lock_release(m_Lock);
	return 0;
}

