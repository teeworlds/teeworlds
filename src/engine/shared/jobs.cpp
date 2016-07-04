/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "jobs.h"

CJobPool::CJobPool()
{
	// empty the pool
	m_Lock = lock_create();
	m_pFirstJob = 0;
	m_pLastJob = 0;
}

void CJobPool::WorkerThread(void *pUser)
{
	CJobPool *pPool = (CJobPool *)pUser;

	while(1)
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
		lock_unlock(pPool->m_Lock);

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
	for(int i = 0; i < NumThreads; i++)
		thread_init(WorkerThread, this);
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

	lock_unlock(m_Lock);
	return 0;
}

