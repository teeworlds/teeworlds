
#pragma once

#include "../system.h"

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
