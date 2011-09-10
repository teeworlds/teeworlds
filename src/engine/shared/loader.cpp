#include <base/system.h>
#include <base/tl/array.h>

#include <engine/loader.h>
#include <engine/storage.h>

CJobHandler g_JobHandler;

void CJobHandler::Init(int ThreadCount)
{
	// start threads
	for(int i = 0; i < ThreadCount; i++)
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "job %d", i);
		thread_create(WorkerThread, this, aBuf);
	}
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
		m_aQueues[QueueId].m_Orders.push(Order);
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
				if(pJobHandler->m_aQueues[i].m_WorkerCount < pJobHandler->m_aQueues[i].m_MaxWorkers && pJobHandler->m_aQueues[i].m_Orders.size())
				{
					Order = pJobHandler->m_aQueues[i].m_Orders.pop();
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
			/*unsigned Count =*/ atomic_dec(&pJobHandler->m_aQueues[QueueId].m_WorkerCount);
			atomic_inc(&pJobHandler->m_WorkDone);

			// we must signal again because there can be stuff in the queue but was blocked by
			// the max worker count
			/*if(Count == pJobHandler->m_aQueues[QueueId].m_MaxWorkers)
				pJobHandler->m_Semaphore.signal();
				*/
		}

		atomic_inc(&pJobHandler->m_WorkTurns);
	}
}


CSource_Disk::CSource_Disk(const char *pBase)
: CSource("disk")
{
	SetBaseDirectory(pBase);
}

void CSource_Disk::SetBaseDirectory(const char *pBase)
{
	if(pBase)
		str_copy(m_aBaseDirectory, pBase, sizeof(m_aBaseDirectory));
	else
		m_aBaseDirectory[0] = 0;
}

int CSource_Disk::LoadWholeFile(const char *pFilename, void **ppData, unsigned *pDataSize)
{
	// do search for reasource
	IOHANDLE hFile = io_open(pFilename, IOFLAG_READ);
	if(hFile == 0)
	{
		*ppData = 0;
		*pDataSize = 0;
		return -1;
	}

	*pDataSize = io_length(hFile);
	*ppData = mem_alloc(*pDataSize, sizeof(void*)); // TOOD: stream buffer perhaps
	long int Result = io_read(hFile, *ppData, *pDataSize);
	io_close(hFile);

	if(Result != *pDataSize)
	{
		mem_free(*ppData);
		*ppData = 0;
		*pDataSize = 0;
		return -2;
	}
	
	return 0;
}

bool CSource_Disk::Load(CLoadOrder *pOrder)
{
	char aFilename[512];
	str_format(aFilename, sizeof(aFilename), "%s/%s", m_aBaseDirectory, pOrder->m_pResource->Name());
	return LoadWholeFile(aFilename, &pOrder->m_pData, &pOrder->m_DataSize) == 0;
}


IResources::CSource::CSource(const char *pName)
{
	m_pName = pName;
	m_pNextSource = 0x0;
	m_pPrevSource = 0x0;
}

void IResources::CSource::Update()
{
	while(m_lFeedback.size())
	{
		CLoadOrder Order = m_lFeedback.pop();
		Feedback(&Order);
		FeedbackOrder(&Order);
	}

	while(m_lInput.size())
	{
		CLoadOrder Order = m_lInput.pop();

		if(Load(&Order))
		{
			dbg_msg("resources", "[%s] loaded %s", Name(), Order.m_pResource->Name());
			FeedbackOrder(&Order);
		}
		else
		{
			dbg_msg("resources", "[%s] sending %s", Name(), Order.m_pResource->Name());
			ForwardOrder(&Order);
		}
	}
}


class CResources : public IResources
{
	enum
	{
		// a bit ugly
		JOBQUEUE_IO = CJobHandler::NUM_QUEUES - 1,
		JOBQUEUE_PROCESS = JOBQUEUE_IO - 1,
	};

	class CHandlerEntry
	{
	public:
		const char *m_pType;
		IHandler *m_pHandler;
	};
	array<CHandlerEntry> m_lHandlers;

	array<IResource*> m_lpResources;

	IResource *FindResource(CResourceId Id)
	{
		// TODO: bad performance, linear search, string compares
		// TODO: check hash as well if wanted
		for(int i = 0; i < m_lpResources.size(); i++)
		{
			if(str_comp(m_lpResources[i]->Name(), Id.m_pName) == 0)
				return m_lpResources[i];
		}

		return 0x0;
	}

	IHandler *FindHandler(const char *pType)
	{
		// TODO: bad performance, linear search, string compares
		for(int i = 0; i < m_lHandlers.size(); i++)
		{
			if(str_comp(m_lHandlers[i].m_pType, pType) == 0)
				return m_lHandlers[i].m_pHandler;
		}

		return 0x0;
	}

	const char *GetTypeFromName(const char *pName)
	{
		const char *pType = 0x0;
		for(; *pName; pName++)
		{
			if(*pName == '.')
				pType = pName + 1;
			else if(*pName == '/')
				pType = 0x0;
		}
		return pType;
	}

	IResource *CreateResource(CResourceId Id)
	{
		const char *pType = GetTypeFromName(Id.m_pName);
		if(!pType)
		{
			dbg_msg("resources", "couldn't find resource type for '%s'", Id.m_pName);
			return 0x0;
		}

		IHandler *pHandler = FindHandler(pType);
		if(!pHandler)
			return 0x0;

		IResource *pResource = pHandler->Create(Id);
		pResource->m_Id = Id;
		pResource->m_pResources = this;
		pResource->m_pHandler = pHandler;

		// copy the name
		int NameSize = str_length(pResource->m_Id.m_pName) + 1;
		void *pName = mem_alloc(NameSize, sizeof(void*));
		mem_copy(pName, Id.m_pName, NameSize);
		pResource->m_Id.m_pName = (const char *)pName;

		LoadResource(pResource);
		return pResource;
	}

	ringbuffer_mwsr<IResource*, 1024> m_lInserts; // job threads writes, main thread reads
	ringbuffer_swsr<IResource*, 1024> m_lDestroys; // main thread only

	struct CLoadJobInfo
	{
		CResources *m_pThis;
		IResource *m_pResource;
		void *m_pData;
		unsigned m_DataSize;
	};

	void LoadResource(IResource *pResource)
	{
		CSource::CLoadOrder Order = {0,0,0};
		Order.m_pResource = pResource;
		m_SourceStart.AddOrder(&Order);
	}

	static int Job_ProcessData(CJobHandler *pJobHandler, void *pData)
	{
		CLoadJobInfo *pInfo = reinterpret_cast<CLoadJobInfo*>(pData);

		// find the handler and let it load the resource
		IHandler *pHandler = pInfo->m_pResource->m_pHandler;

		//dbg_msg("resources", "processing '%s'", pInfo->m_pResource->m_Id.m_pName);
		if(pHandler->Load(pInfo->m_pResource, pInfo->m_pData, pInfo->m_DataSize) != 0)
		{
			dbg_msg("resources", "failed to process '%s'", pInfo->m_pResource->m_Id.m_pName);
			pInfo->m_pResource->m_State = IResource::STATE_ERROR;
			return -2;
		}

		// queue the resource for insert 
		pInfo->m_pThis->m_lInserts.push(pInfo->m_pResource);

		// do some clean up the job data
		mem_free(pInfo->m_pData);		
		pJobHandler->FreeJobData(pInfo);
		return 0;
	}

	class CSource_StartPoint : public CSource
	{
	public:
		CSource_StartPoint()
		: CSource("startpoint")
		{}

		virtual void Feedback(CLoadOrder *pOrder)
		{
			// kick the data off to processing
			dbg_msg("resources", "[%s] adding processing job for '%s'", Name(), pOrder->m_pResource->Name());

			CLoadJobInfo *pInfo = g_JobHandler.AllocJobData<CLoadJobInfo>();
			pInfo->m_pThis = (CResources *)Resources();
			pInfo->m_pResource = pOrder->m_pResource;
			pInfo->m_pData = pOrder->m_pData;
			pInfo->m_DataSize = pOrder->m_DataSize;
			g_JobHandler.Kick(JOBQUEUE_IO, Job_ProcessData, pInfo);
		}

		void AddOrder(CLoadOrder *pOrder)
		{
			// skip this source
			NextSource()->m_lInput.push(*pOrder);
			NextSource()->m_Semaphore.signal();
		}
	};

	class CSource_EndPoint : public CSource
	{
	public:
		CSource_EndPoint()
		: CSource("endpoint")
		{}
				
		virtual bool Load(CLoadOrder *pOrder)
		{
			// if the order has come this far, it has gone wrong.
			dbg_msg("resources", "[%s] end station for '%s'", pOrder->m_pResource->Name());
			// TODO: set it to a failure to load
			return true;
		}
	};

	virtual void AddSource(CSource *pSource)
	{
		CSource *pAfter = m_SourceEnd.m_pPrevSource;
		pAfter->m_pNextSource = pSource;
		pSource->m_pPrevSource = pAfter;
		pSource->m_pNextSource = &m_SourceEnd;
		m_SourceEnd.m_pPrevSource = pSource;

		StartSource(pSource);
	}

	void StartSource(CSource *pSource)
	{
		pSource->m_pResources = this;
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "source %s", pSource->Name());
		thread_create(CSource::ThreadFunc, pSource, aBuf);
	}

	CSource_StartPoint m_SourceStart;
	CSource_EndPoint m_SourceEnd;

public:
	CResources()
	{
		m_SourceStart.m_pNextSource = &m_SourceEnd;
		m_SourceEnd.m_pPrevSource = &m_SourceStart;

		StartSource(&m_SourceStart);
		StartSource(&m_SourceEnd);
	}

	virtual void AssignHandler(const char *pType, IHandler *pHandler)
	{
		CHandlerEntry Entry;
		Entry.m_pType = pType;
		Entry.m_pHandler = pHandler;
		m_lHandlers.add(Entry);
	}

	virtual IResource *GetResource(CResourceId Id)
	{
		IResource *pResource = FindResource(Id);
		if(pResource)
			return pResource;
		return CreateResource(Id);
	}

	virtual void Update()
	{
		// handle all resource deletes
		// push a null marker, we are done with the deletes when we hit this marker
		m_lDestroys.push(0x0); 
		while(true)
		{
			IResource *pResource = m_lDestroys.pop();
			if(pResource == 0x0)
				break;
			
			// TODO: we need to make sure that this isn't in the loading queue when we delete it
			if(pResource->IsLoading())
			{
				m_lDestroys.push(pResource);
			}
			else
			{
				pResource->m_pHandler->Destroy(pResource);
				assert(pResource->Name() != 0); // make sure that the handler didn't call delete on the resource
				delete pResource;
			}
		}

		// handle all resource inserts
		while(m_lInserts.size())
		{
			IResource *pResource = m_lInserts.pop();
			pResource->m_pHandler->Insert(pResource);
			pResource->m_State = IResource::STATE_LOADED;
		}
	}

	virtual	void Destroy(IResource *pResource)
	{
		m_lpResources.remove_fast(pResource);
		m_lDestroys.push(pResource);
	}
};

IResources *IResources::CreateInstance() { return new CResources(); }
