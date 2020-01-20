/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction - Threads, Locks, Semaphores
*/

#ifndef BASE_THREADING_H
#define BASE_THREADING_H

#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Group: Threads */

/*
	Function: thread_sleep
		Suspends the current thread for a given period.

	Parameters:
		milliseconds - Number of milliseconds to sleep.
*/
void thread_sleep(int milliseconds);

/*
	Function: thread_init
		Creates a new thread.

	Parameters:
		threadfunc - Entry point for the new thread.
		user - Pointer to pass to the thread.

*/
void *thread_init(void (*threadfunc)(void *), void *user);

/*
	Function: thread_wait
		Waits for a thread to be done or destroyed.

	Parameters:
		thread - Thread to wait for.
*/
void thread_wait(void *thread);

/*
	Function: thread_destroy
		Destroys a thread.

	Parameters:
		thread - Thread to destroy.
*/
void thread_destroy(void *thread);

/*
	Function: thread_yeild
		Yeild the current threads execution slice.
*/
void thread_yield();

/*
	Function: thread_detach
		Puts the thread in the detached thread, guaranteeing that
		resources of the thread will be freed immediately when the
		thread terminates.

	Parameters:
		thread - Thread to detach
*/
void thread_detach(void *thread);


/* Group: Locks */
typedef void* LOCK;

LOCK lock_create();
void lock_destroy(LOCK lock);
int lock_trylock(LOCK lock);
void lock_wait(LOCK lock);
void lock_unlock(LOCK lock);


/* Group: Semaphores */

#if !defined(CONF_PLATFORM_MACOSX)
	#if defined(CONF_FAMILY_UNIX)
		#include <semaphore.h>
		typedef sem_t SEMAPHORE;
	#elif defined(CONF_FAMILY_WINDOWS)
		typedef void* SEMAPHORE;
	#else
		#error missing sempahore implementation
	#endif

	void semaphore_init(SEMAPHORE *sem);
	void semaphore_wait(SEMAPHORE *sem);
	void semaphore_signal(SEMAPHORE *sem);
	void semaphore_destroy(SEMAPHORE *sem);
#endif


/* Group: Misc */

/*
	Function: cpu_relax
		Lets the cpu relax a bit.
*/
void cpu_relax();

#ifdef __cplusplus
}
#endif

#endif
