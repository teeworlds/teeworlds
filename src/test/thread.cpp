/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <gtest/gtest.h>

#include <base/system.h>

static void Nothing(void *pUser)
{
	(void)pUser;
}

TEST(Thread, Detach)
{
	void *pThread = thread_init(Nothing, 0);
	thread_detach(pThread);
}

static void SetToOne(void *pUser)
{
	*(int *)pUser = 1;
}

TEST(Thread, Wait)
{
	int Integer = 0;
	void *pThread = thread_init(SetToOne, &Integer);
	thread_wait(pThread);
	EXPECT_EQ(Integer, 1);
	thread_destroy(pThread);
}

TEST(Thread, Yield)
{
	thread_yield();
}

static void LockThread(void *pUser)
{
	LOCK *pLock = (LOCK *)pUser;
	lock_wait(*pLock);
	lock_unlock(*pLock);
}

TEST(Thread, Lock)
{
	LOCK Lock = lock_create();
	lock_wait(Lock);
	void *pThread = thread_init(LockThread, &Lock);
	lock_unlock(Lock);
	thread_wait(pThread);
	thread_destroy(pThread);
}

struct SemTestStruct
{
	SEMAPHORE sem;
	LOCK k_lock;
	int k;
};

static void SemThread(void *pUser)
{
	struct SemTestStruct *pContext = (struct SemTestStruct *)pUser;
	sphore_wait(&pContext->sem);
	lock_wait(pContext->k_lock);
	pContext->k--;
	lock_unlock(pContext->k_lock);
}

TEST(Thread, Semaphore)
{
	struct SemTestStruct Context;
	sphore_init(&Context.sem);
	Context.k_lock = lock_create();
	Context.k = 5;

	void *apThreads[3];
	for(int i = 0; i < 3; i++)
		apThreads[i] = thread_init(SemThread, &Context);

	for(int i = 0; i < 3; i++)
		sphore_signal(&Context.sem);

	for(int i = 0; i < 3; i++)
		thread_wait(apThreads[i]);

	EXPECT_EQ(Context.k, 2);

	lock_destroy(Context.k_lock);
	sphore_destroy(&Context.sem);
}
