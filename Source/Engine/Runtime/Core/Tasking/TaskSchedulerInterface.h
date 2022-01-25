/*
=============================================================================
	File:	JobScheduler.h
	Desc:	A system for performing parallelized tasks
			using multiple threads/processors.
=============================================================================
*/

#pragma once

mxDECLARE_64BIT_HANDLE(JobID);

static const JobID NIL_JOB_ID( ~0ull );


///
mxPREALIGN(16) struct NwJobData
{
	char	blob[192];

public:
	NwJobData()
	{
		mxZERO_OUT(blob);
	}
	/// NOTE: don't forget to manually call the constructor if needed
	template< class T >
	inline void CastTo( T *&_o )
	{
		mxSTATIC_ASSERT2( sizeof(T) <= sizeof(NwJobData), Job_Data_size_is_too_big );
		_o = reinterpret_cast< T* >( this );
	}
} mxPOSTALIGN(16);

/// used only for checking
template< class JOB >
struct TJobBase
{
	mxSTATIC_ASSERT(sizeof(JOB) <= sizeof(NwJobData));

	const NwJobData& asJobData() const {
		return *(NwJobData*) this;
	}
};

struct NwThreadContext
{
	AllocatorI *	heap;	//!< thread-local memory allocator for temporary, short-lived allocations
	U32		threadIndex;	//!< 0 if main thread, >0 for workers
	char	padding[116];	//!< storage for ScratchAllocator and padding
//	U32		localSpaceSize;
//	void *	localSpace;	//!< local memory private to this thread
//	char	buffer[1024];	//!< for string formatting + padding to avoid false sharing
public:
	static const NwThreadContext& current();
};

extern AllocatorI& threadLocalHeap();


/// Execute the task
/// Perform a unit of work which is independent of the rest,
/// i.e. one which can be performed in parallel.
typedef ERet (*F_JobFunction)(
							  const NwThreadContext& context
							  , NwJobData & job_data

							  //
							  , int range_start, int range_end

							  // for invoking job destructor
							  , bool is_last_job
							  );

template< class JOB >
F_JobFunction getJobFun()
{
	static_assert(sizeof(JOB) <= sizeof(NwJobData), "job size is too big");
	struct Wrapper
	{
		static ERet JobFunction(
			const NwThreadContext& context
			, NwJobData & job_data
			, int range_start, int range_end
			, bool is_last_job
			)
		{
			JOB *	typed_job;
			job_data.CastTo( typed_job );

			ERet result = typed_job->Run( context, range_start, range_end );

			if(is_last_job) {
				typed_job->~JOB();
			}

			return result;
		}
	};
	return &Wrapper::JobFunction;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// Interface

// must be synchronized with JQ2_PRIORITY_MAX
enum EJobPriority : U8
{
	JobPriority_Critical= 0,
	JobPriority_High	= 1,
	JobPriority_Normal	= 2,
	JobPriority_Low		= 3,
	JobPriority_COUNT	= 4,	//!< Marker, don't use!
};

//  what to execute while wailing
enum EJobWaitFlags
{
	//  what to execute while wailing
	WAITFLAG_EXECUTE_SUCCESSORS = 0x1,
	WAITFLAG_EXECUTE_ANY = 0x2,
	////  what to do when out of jobs
	WAITFLAG_BLOCK = 0x4L,
	WAITFLAG_SLEEP = 0x8L,
	WAITFLAG_SPIN = 0x10L,
	WAITFLAG_DEFAULTS = WAITFLAG_EXECUTE_SUCCESSORS,
};

/// in microseconds
static const U32 JOB_WAIT_DEFAULT_TIME_USEC = ~0;
static const U32 JOB_WAIT_TIME_INFINITE = ~0;


///
struct NwJobStats
{
	U32 num_finished_jobs;
	U32 num_locks;
	U32 num_cond_notifies;
	U32 num_cond_waits;
};

///
/// Asynchronous task/job system.
///
class NwJobSchedulerI: NonCopyable
{
public:
	/// Submits a new job to the scheduler and returns if the job queue is not full.
	/// If the job queue is full, the job is run.
	/// Should only be called from the main thread, or within a job.
	virtual JobID AddJob(
		F_JobFunction callback,
		NwJobData & data,
		EJobPriority priority = JobPriority_High,
		int num_tasks = 1, int range = -1
	) = 0;

	/// Execute all tasks until the job is completed.
	virtual void waitFor(
		const JobID taskId
		, U32 nWaitFlag = WAITFLAG_EXECUTE_SUCCESSORS | WAITFLAG_BLOCK
		, U32 usWaitTime = JOB_WAIT_DEFAULT_TIME_USEC
	) = 0;

	/// Waits for all jobs to complete - not guaranteed to work unless
	/// we know we are in a situation where jobs aren't being continuously added.
	virtual void waitForAll() = 0;

	/// AddJob a non-executing job to group all jobs added between begin/end;
	/// can (and often should) be called within a job in a worker thread.
	virtual JobID beginGroup() = 0;
	virtual void endGroup() = 0;

	///
	virtual bool isDone( const JobID taskId ) = 0;

public:
	template< class TASK >
	JobID AddJob(
		TASK* new_task
		, EJobPriority priority = JobPriority_High
		, int num_tasks = 1
		, int range = -1
		)
	{
		mxSTATIC_ASSERT( sizeof(NwJobData) >= sizeof(TASK) );
		mxASSERT(num_tasks != 0);
		mxASSERT(range != 0);
		return this->AddJob(
			getJobFun<TASK>()
			, *(NwJobData*)new_task
			, priority
			, num_tasks, range
			);
	}

protected:
	virtual ~NwJobSchedulerI() {}
};


///
/// Uses Jonas Meyer's Job Queue: https://bitbucket.org/jonasmeyer/jq
///
struct NwJobScheduler_Parallel: public NwJobSchedulerI, TSingleInstance< NwJobScheduler_Parallel >
{
	static NwJobScheduler_Parallel	s_instance;

	//
	NwJobScheduler_Parallel();
	virtual ~NwJobScheduler_Parallel();

	virtual JobID AddJob(
		F_JobFunction callback,
		NwJobData & data,
		EJobPriority priority = JobPriority_High,
		int num_tasks = 1, int range = -1
	) override;

	virtual void waitFor(
		const JobID taskId
		, U32 nWaitFlag = WAITFLAG_EXECUTE_SUCCESSORS | WAITFLAG_BLOCK
		, U32 usWaitTime = JOB_WAIT_DEFAULT_TIME_USEC
	) override;

	virtual void waitForAll() override;

	virtual JobID beginGroup() override;
	virtual void endGroup() override;

	virtual bool isDone( const JobID taskId ) override;
};

///
/// can be used for debugging or on single-core CPUs;
/// executes tasks immediately
///
struct NwJobScheduler_Serial: public NwJobSchedulerI
{
	static NwJobScheduler_Serial	s_instance;

	//
	NwJobScheduler_Serial();
	virtual ~NwJobScheduler_Serial();

	virtual JobID AddJob(
		F_JobFunction callback,
		NwJobData & data,
		EJobPriority priority = JobPriority_High,
		int num_tasks = 1, int range = -1
	) override;

	virtual void waitFor(
		const JobID taskId
		, U32 nWaitFlag = WAITFLAG_EXECUTE_SUCCESSORS | WAITFLAG_BLOCK
		, U32 usWaitTime = JOB_WAIT_DEFAULT_TIME_USEC
	) override;

	virtual void waitForAll() override;

	virtual JobID beginGroup() override;
	virtual void endGroup() override;

	virtual bool isDone( const JobID taskId ) override;
};


#if MX_DEVELOPER

	///
	#define WAIT_AND_MEASURE_TIME( task_code, task_scheduler, name )\
		do {\
			const U32 start_time_msec = mxGetTimeInMilliseconds();\
			task_scheduler->waitFor( task_code );\
			const U32 elapsed_time_msec = mxGetTimeInMilliseconds() - start_time_msec;\
			LogStream(LL_Trace), name, " took ", elapsed_time_msec, " msec";\
		} while(0)

#else	// !MX_DEVELOPER

	///
	#define WAIT_AND_MEASURE_TIME( task_code, task_scheduler, name )\
			do {\
				task_scheduler->waitFor( task_code );\
			} while(0)

#endif	// !MX_DEVELOPER


mxREMOVE_OLD_CODE
///
#define CREATE_TASK( task_scheduler, task_priority, task_class, ... )\
	{\
		NwJobData	task_data_##task_class;\
		mxSTATIC_ASSERT( sizeof(task_class) <= sizeof(task_data_##task_class) );\
		new(&task_data_##task_class) task_class( __VA_ARGS__ );\
		task_scheduler.AddJob(\
			getJobFun<task_class>()\
			, task_data_##task_class\
			, task_priority\
		);\
	}

///
#define nwCREATE_JOB( job_handle, task_scheduler, count, range, task_priority, task_class, ... )\
	{\
		NwJobData	task_data_##task_class;\
		mxSTATIC_ASSERT( sizeof(task_class) <= sizeof(task_data_##task_class) );\
		new(&task_data_##task_class) task_class( __VA_ARGS__ );\
		job_handle = task_scheduler.AddJob(\
			getJobFun<task_class>()\
			, task_data_##task_class\
			, task_priority\
			, count\
			, range\
		);\
	}


class AtomicCounter
{
	AtomicInt	_value;
public:
	AtomicCounter()
	{
		_value = 0;
	}
	int load() const
	{
		return AtomicLoad( _value );
	}
	void inc()
	{
		return AtomicIncrement( &_value, 1 );
	}
	void inc( unsigned count )
	{
		return AtomicIncrement( &_value, count );
	}
	void dec()
	{
		mxASSERT(this->load() > 0);
		return AtomicDecrement( &_value, 1 );
	}
	void dec( unsigned amount )
	{
		mxASSERT(this->load() > 0);
		return AtomicDecrement( &_value, amount );
	}
};

// Useful references:
// http://blog.s-schoener.com/2019-04-26-unity-job-zoo/
