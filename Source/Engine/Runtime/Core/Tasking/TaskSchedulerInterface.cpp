/*
=============================================================================
	File:	SlowTasks.cpp
	Desc:	Scheduler for slow tasks which usually last more than one frame
	(e.g. streaming, background resource loading, AI)
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/JobSystem_Jq.h>

/*static */const NwThreadContext& NwThreadContext::current()
{
	return Jq2CurrentThreadContext();
}

AllocatorI& threadLocalHeap()
{
	AllocatorI* heap = Jq2CurrentThreadContext().heap;
	mxASSERT_PTR(heap);
	return *heap;
}

/*
-----------------------------------------------------------------------------
	NwJobScheduler_Parallel
-----------------------------------------------------------------------------
*/
NwJobScheduler_Serial	NwJobScheduler_Serial::s_instance;

NwJobScheduler_Serial::NwJobScheduler_Serial()
{
	//
}

NwJobScheduler_Serial::~NwJobScheduler_Serial()
{
	//
}

JobID NwJobScheduler_Serial::AddJob(
	F_JobFunction callback,
	NwJobData & data,
	EJobPriority priority,
	int num_subtasks /*= 1*/, int problem_size /*= -1*/
)
{
	const NwThreadContext& threadCtx = Jq2CurrentThreadContext();

	if(problem_size > 0 && num_subtasks > 0)
	{
		const int num_items_per_subtask = problem_size / num_subtasks;

		for(int subtask_idx = 0;
			subtask_idx < num_subtasks;
			subtask_idx++)
		{
			const int range_start = subtask_idx * num_items_per_subtask;
			const int range_end = smallest( range_start + num_items_per_subtask, problem_size );
			const bool is_last_job = (range_end == problem_size);

			(*callback)( threadCtx, data, range_start, range_end, is_last_job );
		}
	}
	else
	{
		const int range_start = 0;
		const int range_end = problem_size;
		const bool is_last_job = true;

		(*callback)( threadCtx, data, range_start, range_end, is_last_job );
	}

	return NIL_JOB_ID;
}

void NwJobScheduler_Serial::waitFor(
	const JobID taskId
	, U32 nWaitFlag
	, U32 usWaitTime
	)
{
	(void)taskId;
	(void)nWaitFlag;
	(void)usWaitTime;
}

void NwJobScheduler_Serial::waitForAll()
{
	//
}

JobID NwJobScheduler_Serial::beginGroup()
{
	JobID dummy_result( 0 );
	return dummy_result;
}

void NwJobScheduler_Serial::endGroup()
{
	//
}

bool NwJobScheduler_Serial::isDone( const JobID taskId )
{
	(void)taskId;
	return true;
}

/*
-----------------------------------------------------------------------------
	NwJobScheduler_Parallel
-----------------------------------------------------------------------------
*/
NwJobScheduler_Parallel	NwJobScheduler_Parallel::s_instance;

NwJobScheduler_Parallel::NwJobScheduler_Parallel()
{
	//
}

NwJobScheduler_Parallel::~NwJobScheduler_Parallel()
{
	//
}

JobID NwJobScheduler_Parallel::AddJob(
	F_JobFunction callback,
	NwJobData & data,
	EJobPriority priority,
	int num_tasks /*= 1*/, int range /*= -1*/
)
{
	const JobID task_id( Jq2Add( callback, data, priority, num_tasks, range ) );
	return task_id;
}

void NwJobScheduler_Parallel::waitFor(
	const JobID taskId
	, U32 nWaitFlag
	, U32 usWaitTime
	)
{
	Jq2Wait( taskId.id, nWaitFlag, usWaitTime );
}

void NwJobScheduler_Parallel::waitForAll()
{
	Jq2WaitAll();
}

JobID NwJobScheduler_Parallel::beginGroup()
{
	const JobID task_id( Jq2GroupBegin() );
	return task_id;
}

void NwJobScheduler_Parallel::endGroup()
{
	Jq2GroupEnd();
}

bool NwJobScheduler_Parallel::isDone( const JobID taskId )
{
	return Jq2IsDone( taskId.id );
}
