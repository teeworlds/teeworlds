/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>
#include "e_jobs.h"

void worker_thread(void *user)
{
	JOBPOOL *pool = (JOBPOOL *)user;
	
	while(1)
	{
		JOB *job = 0;
		
		/* fetch job from queue */
		lock_wait(pool->lock);
		if(pool->first_job)
		{
			job = pool->first_job;
			pool->first_job = pool->first_job->next;
			if(pool->first_job)
				pool->first_job->prev = 0;
			else
				pool->last_job = 0;
		}
		lock_release(pool->lock);
		
		/* do the job if we have one */
		if(job)
		{
			job->status = JOBSTATUS_RUNNING;
			job->result = job->func(job->func_data);
			job->status = JOBSTATUS_DONE;
		}
		else
			thread_sleep(10);
	}
	
}

int jobs_initpool(JOBPOOL *pool, int num_threads)
{
	int i;
	
	/* empty the pool */
	mem_zero(pool, sizeof(JOBPOOL));
	pool->lock = lock_create();
	
	/* start threads */
	for(i = 0; i < num_threads; i++)
		thread_create(worker_thread, pool);
	return 0;
}

int jobs_add(JOBPOOL *pool, JOB *job, JOBFUNC func, void *data)
{
	mem_zero(job, sizeof(JOB));
	job->func = func;
	job->func_data = data;
	
	lock_wait(pool->lock);
	
	/* add job to queue */
	job->prev = pool->last_job;
	if(pool->last_job)
		pool->last_job->next = job;
	pool->last_job = job;
	if(!pool->first_job)
		pool->first_job = job;
	
	lock_release(pool->lock);
	return 0;
}

int jobs_status(JOB *job)
{
	return job->status;
}
