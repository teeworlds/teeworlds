
#pragma once

#include <base/system.h>

inline unsigned atomic_inc(volatile unsigned *pValue)
{
	return __sync_fetch_and_add(pValue, 1);
}

inline unsigned atomic_dec(volatile unsigned *pValue)
{
	return __sync_fetch_and_add(pValue, -1);
}

inline unsigned atomic_compswap(volatile unsigned *pValue, unsigned comperand, unsigned value)
{
	return __sync_val_compare_and_swap(pValue, comperand, value);
}

inline void sync_barrier()
{
	__sync_synchronize();
}

#include <semaphore.h>
typedef sem_t SEMAPHORE;
inline void semaphore_init(SEMAPHORE *sem) { sem_init(sem, 0, 0); }
inline void semaphore_wait(SEMAPHORE *sem) { sem_wait(sem); }
inline void semaphore_signal(SEMAPHORE *sem) { sem_post(sem); }
inline void semaphore_destroy(SEMAPHORE *sem) { sem_destroy(sem); }

class semaphore
{
	SEMAPHORE sem;
public:
	semaphore() { semaphore_init(&sem); }
	~semaphore() { semaphore_destroy(&sem); }
	void wait() { semaphore_wait(&sem); }
	void signal() { semaphore_signal(&sem); }
};

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

/*
	RingBuffer - Multiple Writers, Single Reader

	Behaviours:
		* Any thread can write to it (Push)
		* One thread can read from it (Pop)
		* Push blocks if the buffer is full
*/
template<class TTYPE, int TSIZE>
class TRingBufferMWSR
{
	TTYPE m_aData[TSIZE];
	volatile unsigned m_Put; // multiple writers
	volatile unsigned m_Write; // multiple writers
	volatile unsigned m_Get; // single writer

	enum
	{
		MASK = TSIZE-1,
	};

public:
	explicit TRingBufferMWSR()
	{
		m_Put = m_Write = m_Get = 0;
	}

	TTYPE *Next()
	{
		if(!NumElements())
			return 0;
		return &m_aData[m_Get&MASK];
	}

	void Pop()
	{
		//assert(NumElements());
		m_Get++;
	}

	void Push(TTYPE *pData)
	{
		unsigned idx = atomic_inc(&m_Write);

		// wait for it to become available
		while(idx-m_Get >= TSIZE) { thread_yield(); }

		// copy the data
		mem_copy(&m_aData[idx&MASK], pData, sizeof(TTYPE));
		sync_barrier();

		// wait for our turn to increase the put count
		while(m_Put != idx) { thread_yield(); }
		atomic_inc(&m_Put);
	}

	unsigned NumElements() const { return m_Put - m_Get; }
};

/*
	RingBuffer - Single Writer, Single Reader

	Behaviours:
		* Any thread can write to it (Push)
		* One thread can read from it (Pop)
		* Push returns false if buffer is full
*/
template<class TTYPE, int TSIZE>
class TRingBufferSWSR
{
	TTYPE m_aData[TSIZE];
	unsigned m_Put; // single writer
	unsigned m_Get; // single writer

	enum
	{
		MASK = TSIZE-1,
	};

public:
	explicit TRingBufferSWSR()
	{
		m_Put = m_Get = 0;
	}

	TTYPE *Next()
	{
		if(!NumElements())
			return 0;
		return &m_aData[m_Get&MASK];
	}

	void Pop()
	{
		assert(NumElements());
		m_Get++;
	}

	bool Push(TTYPE *pData)
	{
		if(NumElements() == TSIZE)
			return false;

		// copy the data
		mem_copy(&m_aData[m_Put&MASK], pData, sizeof(TTYPE));
		m_Put++;
		return true;
	}

	unsigned NumElements() const { return m_Put - m_Get; }
};

