/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "memory.h"
#include "threading.h"

#if defined(CONF_FAMILY_UNIX)
	#include <sys/time.h>
	#include <pthread.h>
	#include <unistd.h>
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(CONF_ARCH_IA32) || defined(CONF_ARCH_AMD64)
	#include <immintrin.h> //_mm_pause
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Threads */

struct THREAD_RUN
{
	void (*threadfunc)(void *);
	void *u;
};

#if defined(CONF_FAMILY_UNIX)
static void *thread_run(void *user)
#elif defined(CONF_FAMILY_WINDOWS)
static unsigned long __stdcall thread_run(void *user)
#else
#error not implemented
#endif
{
	struct THREAD_RUN *data = user;
	void (*threadfunc)(void *) = data->threadfunc;
	void *u = data->u;
	free(data);
	threadfunc(u);
	return 0;
}

void *thread_init(void (*threadfunc)(void *), void *u)
{
	struct THREAD_RUN *data = malloc(sizeof(*data));
	data->threadfunc = threadfunc;
	data->u = u;
#if defined(CONF_FAMILY_UNIX)
	{
		pthread_t id;
		if(pthread_create(&id, NULL, thread_run, data) != 0)
		{
			return 0;
		}
		return (void*)id;
	}
#elif defined(CONF_FAMILY_WINDOWS)
	return CreateThread(NULL, 0, thread_run, data, 0, NULL);
#else
	#error not implemented
#endif
}

void thread_wait(void *thread)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_join((pthread_t)thread, NULL);
#elif defined(CONF_FAMILY_WINDOWS)
	WaitForSingleObject((HANDLE)thread, INFINITE);
#else
	#error not implemented
#endif
}

void thread_destroy(void *thread)
{
#if defined(CONF_FAMILY_UNIX)
	void *r = 0;
	pthread_join((pthread_t)thread, &r);
#else
	/*#error not implemented*/
#endif
}

void thread_yield()
{
#if defined(CONF_FAMILY_UNIX)
	sched_yield();
#elif defined(CONF_FAMILY_WINDOWS)
	Sleep(0);
#else
	#error not implemented
#endif
}

void thread_sleep(int milliseconds)
{
#if defined(CONF_FAMILY_UNIX)
	usleep(milliseconds*1000);
#elif defined(CONF_FAMILY_WINDOWS)
	Sleep(milliseconds);
#else
	#error not implemented
#endif
}

void thread_detach(void *thread)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)(thread));
#elif defined(CONF_FAMILY_WINDOWS)
	CloseHandle(thread);
#else
	#error not implemented
#endif
}


/* Locks */

#if defined(CONF_FAMILY_UNIX)
typedef pthread_mutex_t LOCKINTERNAL;
#elif defined(CONF_FAMILY_WINDOWS)
typedef CRITICAL_SECTION LOCKINTERNAL;
#else
	#error not implemented on this platform
#endif

LOCK lock_create()
{
	LOCKINTERNAL *lock = (LOCKINTERNAL*)mem_alloc(sizeof(LOCKINTERNAL), 4);

#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_init(lock, 0x0);
#elif defined(CONF_FAMILY_WINDOWS)
	InitializeCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
	return (LOCK)lock;
}

void lock_destroy(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_destroy((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	DeleteCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
	mem_free(lock);
}

int lock_trylock(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	return pthread_mutex_trylock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	return !TryEnterCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

void lock_wait(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_lock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	EnterCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

void lock_unlock(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_unlock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	LeaveCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}


/* Semaphores */

#if !defined(CONF_PLATFORM_MACOSX)
	#if defined(CONF_FAMILY_UNIX)
	void semaphore_init(SEMAPHORE *sem) { sem_init(sem, 0, 0); }
	void semaphore_wait(SEMAPHORE *sem) { sem_wait(sem); }
	void semaphore_signal(SEMAPHORE *sem) { sem_post(sem); }
	void semaphore_destroy(SEMAPHORE *sem) { sem_destroy(sem); }
	#elif defined(CONF_FAMILY_WINDOWS)
	void semaphore_init(SEMAPHORE *sem) { *sem = CreateSemaphore(0, 0, 10000, 0); }
	void semaphore_wait(SEMAPHORE *sem) { WaitForSingleObject((HANDLE)*sem, INFINITE); }
	void semaphore_signal(SEMAPHORE *sem) { ReleaseSemaphore((HANDLE)*sem, 1, NULL); }
	void semaphore_destroy(SEMAPHORE *sem) { CloseHandle((HANDLE)*sem); }
	#else
		#error not implemented on this platform
	#endif
#endif


/* Misc */

void cpu_relax()
{
#if defined(CONF_ARCH_IA32) || defined(CONF_ARCH_AMD64)
	_mm_pause();
#endif
}


#if defined(__cplusplus)
}
#endif
