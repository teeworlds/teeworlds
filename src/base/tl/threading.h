
#pragma once

#include "../system.h"

/*
	atomic_inc - should return the value after increment
	atomic_dec - should return the value after decrement
	atomic_compswap - should return the value before the eventual swap
	
*/

#if defined(__GNUC__)

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

#elif defined(_MSC_VER)
	#include <intrin.h>

	inline unsigned atomic_inc(volatile unsigned *pValue)
	{
		return _InterlockedIncrement((volatile long *)pValue);
	}
	
	inline unsigned atomic_dec(volatile unsigned *pValue)
	{
		return _InterlockedDecrement((volatile long *)pValue);
	}

	inline unsigned atomic_compswap(volatile unsigned *pValue, unsigned comperand, unsigned value)
	{
		return _InterlockedCompareExchange((volatile long *)pValue, (long)value, (long)comperand);
	}

	inline void sync_barrier()
	{
		_ReadWriteBarrier();
	}
#else
	#error missing atomic implementation for this compiler
#endif

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
