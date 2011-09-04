#include "kernel.h"

#include <base/tl/ringbuffer.h>

class CJobHandler
{
public:
	class IJob;
	class COrder;
	class CQueue;

	typedef int (*FJob)(CJobHandler *pJobHandler, void *pData);

	class COrder
	{
	public:
		COrder()
		{
			m_pData = 0x0;
			m_pfnProcess = 0x0;
		}

		FJob m_pfnProcess;
		void *m_pData;
	};

	class CQueue
	{
	public:
		CQueue()
		{
			m_WorkerCount = 0;
			m_MaxWorkers = ~(0U); // unlimited by default
		}

		volatile unsigned m_WorkerCount;
		unsigned m_MaxWorkers;
		ringbuffer_swsr<COrder, 1024> m_Orders;
	};

	enum
	{
		NUM_QUEUES = 16,
	};

	CJobHandler() { Init(4); }
	void Init(int ThreadCount);
	void ConfigureQueue(int QueueId, int MaxWorkers);

	void *AllocJobData(unsigned DataSize) { return mem_alloc(DataSize, sizeof(void*)); }
	template<typename T> T *AllocJobData() { return (T *)AllocJobData(sizeof(T)); }
	void FreeJobData(void *pPtr) { mem_free(pPtr); }

	void Kick(int QueueId, FJob pfnJob, void *pData);

	unsigned volatile m_WorkDone;
	unsigned volatile m_WorkTurns;

private:
	CQueue m_aQueues[NUM_QUEUES];
	semaphore m_Semaphore;
	lock m_Lock; // TODO: bad performance, this lock can be removed and everything done with waitfree queues

	static void WorkerThread(void *pUser);
};

extern CJobHandler g_JobHandler;

extern int Helper_LoadFile(const char *pFilename, void **ppData, unsigned *pDataSize);

class IResource;

/*
	Behaviours:
		* Handlers are called from the loader thread
*/
class IResources : public IInterface
{
	MACRO_INTERFACE("resources", 0)
public:
	class IHandler;

	class CResourceId
	{
	public:
		unsigned m_ContentHash;
		unsigned m_NameHash;
		const char *m_pName;
	};

	// TODO: do we want this?
	class CResourceLocator // TODO: haha, rename
	{
		int m_iHandler;
		int m_Index;
	public:
	};

	class IHandler
	{
	public:
		virtual ~IHandler() {}

		// called from the main thread
		virtual IResource *Create(CResourceId Id) = 0;

		// called from job thread
		virtual bool Load(IResource *pResource, void *pData, unsigned DataSize) = 0;

		// called from the main thread
		virtual bool Insert(IResource *pResource) = 0;
	};


	virtual ~IResources() {}

	virtual void AssignHandler(const char *pType, IHandler *pHandler) = 0;

	virtual void Update() = 0;

	virtual IResource *GetResource(CResourceId Id) = 0;

	/*template<class T>*/
	IResource *GetResource(const char *pName)
	{
		CResourceId Id;
		Id.m_pName = pName;
		Id.m_NameHash = 0;
		Id.m_ContentHash = 0;
		return GetResource(Id);
	}

	//FetchResource(int Id)

	static IResources *CreateInstance();
};


class IResource
{
public:
	IResource()
	{
		mem_zero(&m_Id, sizeof(m_Id));
	}

	virtual ~IResource()
	{
		if(m_Id.m_pName)
			mem_free((void*)m_Id.m_pName);
		m_Id.m_pName = 0x0;
	}

	IResources::CResourceId m_Id;
	IResources::IHandler *m_pHandler;

	bool IsLoaded() const;


	/*enum
	{
		FLAG_LOADING,
		xFLAG_LOADED,
	};*/

	unsigned m_Flags;
};

//typedef IResources::IResource IResource;

#define MACRO_RESOURCETYPE(a,b,c,d) ((a<<24) | (b<<16) | (c<<8) | d)
#define DECLARE_RESOURCE(a,b,c,d) enum { RESOURCE_TYPE = MACRO_RESOURCETYPE(a,b,c,d) };
