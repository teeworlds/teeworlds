#include "kernel.h"

#include "loader_help.h"

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
		TRingBufferSWSR<COrder, 1024> m_Orders;
	};

	enum
	{
		NUM_QUEUES = 16,
	};

	CJobHandler() { Init(4); }
	void Init(int ThreadCount);
	void ConfigureQueue(int QueueId, int MaxWorkers);
	//void Kick(int QueueId, FJob pfnJob, void *pJobData, FJobDone pfnJobDone);

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

/*
	Behaviours:
		* Handlers are called from the loader thread
*/
class ILoader : public IInterface
{
	MACRO_INTERFACE("loader", 0)
public:
	class IHandler;
	class CRequest;
	class IResource;

	class CResourceId
	{
	public:
		unsigned m_Type;
		unsigned m_Hash;
		const char *m_pName;
	};

	class CRequest
	{
	public:
		// input
		CResourceId m_Id;

		// output
		void *m_pData;
		long int m_DataSize;

		// callbacks?
		IHandler *m_pHandler;

		CRequest()
		{
			mem_zero(this, sizeof(*this));
		}

		~CRequest()
		{
			FreeData();
		}

		void FreeData()
		{
			if(m_pData)
				mem_free(m_pData);
			m_pData = 0;
			m_DataSize = 0;
		}
	};

	class IResource
	{
	public:
		virtual ~IResource() {}
		CResourceId m_Id;

		unsigned m_Flags;
	};

	class IHandler
	{
	public:
		virtual IResource *Create(CResourceId Id) = 0;
		virtual void Handle(IResource *pResource, CRequest *pRequest) = 0; // can be called from a separate thread
	};


	virtual ~ILoader() {}

	virtual void AssignHandler(unsigned Type, IHandler *pHandler) = 0;
	//virtual void QueueRequest(IResource *pResource) = 0;

	virtual IResource *GetResource(CResourceId Id) = 0;

	template<class T>
	T *GetResource(const char *pName)
	{
		CResourceId Id;
		Id.m_Type = T::RESOURCE_TYPE;
		Id.m_pName = pName;
		Id.m_Hash = 0;
		return static_cast<T*>(GetResource(Id));
	}

};

#define MACRO_RESOURCETYPE(a,b,c,d) ((a<<24) | (b<<16) | (c<<8) | d)
#define DECLARE_RESOURCE(a,b,c,d) enum { RESOURCE_TYPE = MACRO_RESOURCETYPE(a,b,c,d) };

extern ILoader *CreateLoader();
