#include <base/system.h>
#include <base/tl/array.h>
#include <engine/loader.h>

void CJobHandler::Init(int ThreadCount)
{
	// start threads
	for(int i = 0; i < ThreadCount; i++)
		thread_create(WorkerThread, this);
}

void CJobHandler::ConfigureQueue(int QueueId, int MaxWorkers)
{
	m_aQueues[QueueId].m_MaxWorkers = MaxWorkers;
}

void CJobHandler::Kick(int QueueId, FJob pfnJob, void *pData)
{
	COrder Order;
	Order.m_pfnProcess = pfnJob;
	Order.m_pData = pData;
	
	{
		scope_lock locker(&m_Lock); // TODO: ugly lock
		m_aQueues[QueueId].m_Orders.Push(&Order);
		sync_barrier();
	}

	m_Semaphore.signal();
}

void CJobHandler::WorkerThread(void *pUser)
{
	CJobHandler *pJobHandler = reinterpret_cast<CJobHandler *>(pUser);
	
	while(1)
	{
		// wait for something to happen
		pJobHandler->m_Semaphore.wait();
		
		// Look for work
		COrder Order;
		int QueueId = -1;

		{
			scope_lock locker(&pJobHandler->m_Lock); // TODO: ugly lock
			for(int i = 0; i < NUM_QUEUES; i++)
			{
				if(pJobHandler->m_aQueues[i].m_WorkerCount < pJobHandler->m_aQueues[i].m_MaxWorkers && pJobHandler->m_aQueues[i].m_Orders.NumElements())
				{
					mem_copy(&Order, pJobHandler->m_aQueues[i].m_Orders.Next(), sizeof(COrder));
					pJobHandler->m_aQueues[i].m_Orders.Pop();
					pJobHandler->m_aQueues[i].m_WorkerCount++;
					QueueId = i;
					break;
				}
			}
			sync_barrier();
		}

		if(Order.m_pfnProcess)
		{
			// do the job!
			Order.m_pfnProcess(pJobHandler, Order.m_pData);

			// no need to take a lock for this one
			sync_barrier();
			atomic_dec(&pJobHandler->m_aQueues[QueueId].m_WorkerCount);
			atomic_inc(&pJobHandler->m_WorkDone);

			// we must signal again because there can be stuff in the queue but was blocked by
			// the max worker count
			pJobHandler->m_Semaphore.signal();
		}

		atomic_inc(&pJobHandler->m_WorkTurns);
	}	
}


#if 0
class lock
{
	friend class scope_lock;

	LOCK var;

	void take() { lock_wait(var); }
	void release() { lock_release(var); }

public:
	lock()
	{
		var = lock_create();
	}

	~lock()
	{
		lock_destroy(var);
	}
};

class scope_lock
{
	lock *var;
public:
	scope_lock(lock *l)
	{
		var = l;
		var->take();
	}

	~scope_lock()
	{
		var->release();
	}
		
};

#endif

/*
	Behaviours:
		* Handlers are called from the loader thread
*/
class CLoader : public ILoader
{
	TRingBufferMWSR<CRequest,1024> m_Queue;
	semaphore m_Semaphore;

	class CHandlerEntry
	{
	public:
		unsigned m_Type;
		IHandler *m_pHandler;
	};

	array<CHandlerEntry> m_lHandlers;

	array<IResource*> m_lpResources;
	lock m_Lock;

	IResource *FindResource(CResourceId Id)
	{
		// TODO: linear search, bad performance
		// TODO: check hash as well if wanted
		// TODO: string compares, bad performance
		for(int i = 0; i < m_lpResources.size(); i++)
		{
			if(m_lpResources[i]->m_Id.m_Type == Id.m_Type && str_comp(m_lpResources[i]->m_Id.m_pName, Id.m_pName) == 0)
			{
				return m_lpResources[i];
			}
		}

		return 0x0;
	}

	IHandler *FindHandler(unsigned Type)
	{
		// TODO: linear search, bad performance
		for(int i = 0; i < m_lHandlers.size(); i++)
		{
			if(m_lHandlers[i].m_Type == Type)
				return m_lHandlers[i].m_pHandler;
		}

		return 0x0;
	}

	IResource *CreateResource(CResourceId Id)
	{
		IHandler *pHandler = FindHandler(Id.m_Type);
		if(!pHandler)
			return 0x0;

		IResource *pResource = pHandler->Create(Id);
		pResource->m_Id = Id; // TODO: copy the string?
		QueueRequest(pResource);
		return pResource;
	}

	void QueueRequest(IResource *pResource)
	{
		CRequest Request;
		Request.m_Id = pResource->m_Id;
		m_Queue.Push(&Request);
		m_Semaphore.signal();
	}

public:
	CLoader()
	{
		thread_create(LoaderThreadBnc, this);
	}

	virtual void AssignHandler(unsigned Type, IHandler *pHandler)
	{
		scope_lock locker(&m_Lock);
		CHandlerEntry Entry;
		Entry.m_Type = Type;
		Entry.m_pHandler = pHandler;
		m_lHandlers.add(Entry);
	}

	virtual IResource *GetResource(CResourceId Id)
	{
		scope_lock locker(&m_Lock);
		IResource *pResource = FindResource(Id);
		if(pResource)
			return pResource;
		return CreateResource(Id);
	}

private:
	int LoadResource(CRequest *pRequest)
	{
		do
		{
			// do search for reasource
			IOHANDLE hFile = io_open(pRequest->m_Id.m_pName, IOFLAG_READ);
			if(hFile == 0)
				break;

			pRequest->m_DataSize = io_length(hFile);
			pRequest->m_pData = mem_alloc(pRequest->m_DataSize, sizeof(void*)); // TOOD: stream buffer perhaps
			long int Result = io_read(hFile,pRequest->m_pData, pRequest->m_DataSize);
			io_close(hFile);

			if(Result != pRequest->m_DataSize)
				break;

			return 0;
		} while(0);

		// do error handling
		if(pRequest->m_pData)
		{
			mem_free(pRequest->m_pData),
			pRequest->m_pData = 0;
		}
		pRequest->m_DataSize = 0;

		// return error
		return -1;
	}

	void LoaderThread()
	{
		while(1)
		{
			// wait on signal
			m_Semaphore.wait();

			// handle a request
			CRequest *pRequest = 0;
			while((pRequest = m_Queue.Next()) != 0)
			{
				if(LoadResource(pRequest))
				{
					scope_lock locker(&m_Lock);

					// dig out the resource, it might be deleted while we loaded it
					IResource *pResource = FindResource(pRequest->m_Id);
					if(pResource)
					{
						// do callback?
						// find handler
						for(int i = 0; i < m_lHandlers.size(); i++)
							if(m_lHandlers[i].m_Type == pRequest->m_Id.m_Type)
							{
								m_lHandlers[i].m_pHandler->Handle(pResource, pRequest);
								break;
							}
					}
					
					// free request
					mem_free(pRequest->m_pData);
				}
				else
				{
					// report error?
				}
			}
		}
	}
	
	static void LoaderThreadBnc(void *pThis) { ((CLoader *)pThis)->LoaderThread();}
};

ILoader *CreateLoader() { return new CLoader(); }
