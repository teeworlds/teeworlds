
typedef int (*JOBFUNC)(void *data);

typedef struct JOB
{
	struct JOBPOOL *pool;
	struct JOB *prev;
	struct JOB *next;
	volatile int status;
	volatile int result;
	JOBFUNC func;
	void *func_data;
} JOB;

typedef struct JOBPOOL
{
	LOCK lock;
	JOB *first_job;
	JOB *last_job;
} JOBPOOL;

enum
{
	JOBSTATUS_PENDING=0,
	JOBSTATUS_RUNNING,
	JOBSTATUS_DONE
	/*JOBSTATUS_ABORTING,*/
	/*JOBSTATUS_ABORTED,*/
};

int jobs_initpool(JOBPOOL *pool, int num_threads);
int jobs_add(JOBPOOL *pool, JOB *job, JOBFUNC func, void *data);
int jobs_status(JOB *job);
