// Jonas Meyer's Job Queue: https://bitbucket.org/jonasmeyer/jq
//
// Simple Job Queue with priorities & child jobs
// - as soon as a job is added, it can be executed
// - if a job is added from inside a job it becomes a child job
// - When waiting for a job you also wait for all children
// - Implemented using c++11 thread/mutex/condition_variable
//	   on win32 CRITICAL_SECTION/Semaphore is used instead.
// - _not_ using jobstealing & per thread queues, so unsuitable for lots of very small jobs w. high contention
// - Different ways to store functions entrypoints
//     default: builtin fixed size (JQ2_FUNCTION_SIZE) byte callback object. 
//              only for trivial types(memcpyable), compilation fails otherwise
//              never allocates, fails compilation if lambda contains more than JQ2_FUNCTION_SIZE bytes.
//     define JQ2_NO_LAMBDAS: raw function pointer with void* argument. Adds extra argument to Jq2Add
//     define JQ2_USE_STD_FUNCTION: use std::function. This will do a heap alloc if you capture too much (24b on osx, 16 on win32).
//     
// - Built in support for splitting ranges between jobs
//
//   example usage
//	
//		Jq2Start(2); //startup Jq2 with 2 worker threads
//		U64 nJobHandle = Jq2Add( [](int start, int end) {} ), PRIORITY, NUM_JOBS, RANGE);
//		Jq2Wait(nJobHandle);
//		Jq2Stop(); //shutdown Jq2
//
//		PRIORITY: 				[0-JQ2_PRIORITY_MAX], lower priority gets executed earlier
//		NUM_JOBS(optional): 	number of times to spawn job
//		RANGE(optional): 		range to split between jobs

//	Todo:
//
//		Lockless:
//			Alloc/Free Job
//			Priority list
//			lockless finish list
//
//			
#pragma once

#include <Core/Tasking/TaskSchedulerInterface.h>

/// Execute a task at a later time.
U64 Jq2Add(
			F_JobFunction JobFunc,
			const NwJobData& _data,
			U8 nPrio,
			int nNumJobs = 1, int nRange = -1
			);

void 	Jq2Wait(U64 nJob, U32 nWaitFlag = WAITFLAG_EXECUTE_SUCCESSORS | WAITFLAG_BLOCK, U32 usWaitTime = JOB_WAIT_DEFAULT_TIME_USEC);
void 	Jq2WaitAll(U64* pJobs, U32 nNumJobs, U32 nWaitFlag = WAITFLAG_EXECUTE_SUCCESSORS | WAITFLAG_BLOCK, U32 usWaitTime = JOB_WAIT_DEFAULT_TIME_USEC);
void	Jq2ExecuteChildren(U64 nJob);
/// add a non-executing job to group all jobs added between begin/end
U64 	Jq2GroupBegin();
void 	Jq2GroupEnd();
bool 	Jq2IsDone(U64 nJob);

/// Wait for all tasks to complete. Blocks until all scheduled tasks have executed.
void 	Jq2WaitAll();

ERet 	Jq2Start( int nNumWorkers, int threadWorkspaceSize = 0 );
void 	Jq2Stop();
U64		Jq2Self();
U32		Jq2SelfJobIndex();
int 	Jq2GetNumWorkers();
void 	Jq2ConsumeStats(NwJobStats* pStatsOut);

const NwThreadContext& Jq2CurrentThreadContext();

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
