/*
=============================================================================
	Based on Jonas Meyer's Job Queue: https://bitbucket.org/jonasmeyer/jq
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <windows.h>
#include <winnt.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <Base/Base.h>
#include <Base/Memory/MemoryBase.h>
#include <Base/Memory/ScratchAllocator.h>
#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Tasking/JobSystem_Jq.h>

#define USE_PER_THREAD_HEAPS	(1)

namespace
{
	//static AllocatorI& privateHeap() { return MemoryHeaps::taskScheduler(); }
	static AllocatorI& privateHeap() { return MemoryHeaps::temporary(); }
}//namespace

//static AtomicInt g_frame;

///////////////////////////////////////////////////////////////////////////////////////////
/// Configuration

#ifndef JQ2_MAX_JOBS
#define JQ2_MAX_JOBS (2048)
#endif

#ifndef JQ2_PRIORITY_MAX
#define JQ2_PRIORITY_MAX 4
#endif

#ifndef JQ2_MAX_JOB_STACK
#define JQ2_MAX_JOB_STACK 8
#endif

#ifndef JQ2_CACHE_LINE_SIZE
#define JQ2_CACHE_LINE_SIZE 64
#endif

#ifndef JQ2_API
#define JQ2_API
#endif

#ifndef JQ2_FUNCTION_SIZE
#define JQ2_FUNCTION_SIZE 32
#endif

/// maximum number of worker threads
#ifndef JQ2_MAX_THREADS
#define JQ2_MAX_THREADS 7
#endif


#define JQ2_ASSERT_SANITY 0


#ifndef JQ2_CLEAR_FUNCTION
#define JQ2_CLEAR_FUNCTION(f) do{f = nullptr;}while(0)
#endif

typedef void (*Jq2Worker_pfn)(int nThreadId);

static U32 PASCAL MyThreadFunction( void* _userData );

struct WorkerThread
{
	Thread	m_thread;

	ERet initialize( int index );
	void shutdown();
};

#define JQ2_THREAD WorkerThread

#define JQ2_THREAD_CREATE(pThread) do{} while(0)

#define JQ2_THREAD_DESTROY(pThread) do{} while(0)

#define JQ2_THREAD_START(pThread, entry, index) do{\
		(pThread)->initialize( index );\
	 } while(0)

#define JQ2_THREAD_JOIN(pThread) do{(pThread)->shutdown();}while(0)



#if defined(__APPLE__)
#define JQ2_BREAK() __builtin_trap()
#define JQ2_THREAD_LOCAL __thread
#define JQ2_STRCASECMP strcasecmp
typedef U64 ThreadIdType;
#define JQ2_USLEEP(us) usleep(us);
I64 Jq2TicksPerSecond()
{
	static I64 nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		mach_timebase_info_data_t sTimebaseInfo;	
		mach_timebase_info(&sTimebaseInfo);
		nTicksPerSecond = 1000000000ll * sTimebaseInfo.denom / sTimebaseInfo.numer;
	}
	return nTicksPerSecond;
}
I64 Jq2Tick()
{
	return mach_absolute_time();
}
#elif defined(_WIN32)
#define JQ2_BREAK() __debugbreak()
#define JQ2_THREAD_LOCAL __declspec(thread)
#define JQ2_STRCASECMP _stricmp
typedef U32 ThreadIdType;
#define JQ2_USLEEP(us) Jq2Usleep(us);
#define snprintf _snprintf
#include <windows.h>
#include <Mmsystem.h>
I64 Jq2TicksPerSecond()
{
	static I64 nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
	}
	return nTicksPerSecond;
}
I64 Jq2Tick()
{
	I64 ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
	return ticks;
}
void Jq2Usleep(__int64 usec) 
{ 
	if(usec > 20000)
	{
		Sleep((DWORD)(usec/1000));
	}
	else if(usec >= 1000)
	{
		timeBeginPeriod(1);
		Sleep((DWORD)(usec/1000));
		timeEndPeriod(1);
	}
	else
	{
		Sleep(0);
	}
}
#endif

#ifndef JQ2_THREAD
#include <thread>
#define JQ2_THREAD std::thread
#define JQ2_THREAD_CREATE(pThread) do{} while(0)
#define JQ2_THREAD_DESTROY(pThread) do{} while(0)
#define JQ2_THREAD_START(pThread, entry, index) do{ *pThread = std::thread(entry, index); } while(0)
#define JQ2_THREAD_JOIN(pThread) do{(pThread)->join();}while(0)
#endif

#ifdef JQ2_NO_ASSERT
#define JQ2_ASSERT(a) do{}while(0)
#else
#define JQ2_ASSERT(a) do{if(!(a)){JQ2_BREAK();} }while(0)
#endif

#ifdef JQ2_ASSERT_LOCKS
#define JQ2_ASSERT_LOCKED() do{if(0 == Jq2HasLock){JQ2_BREAK();}}while(0)
#define JQ2_ASSERT_NOT_LOCKED()  do{if(0 != Jq2HasLock){JQ2_BREAK();}}while(0)
#define JQ2_ASSERT_LOCK_ENTER() do{Jq2HasLock++;}while(0)
#define JQ2_ASSERT_LOCK_LEAVE()  do{Jq2HasLock--;}while(0)
#else
#define JQ2_ASSERT_LOCKED() do{}while(0)
#define JQ2_ASSERT_NOT_LOCKED()  do{}while(0)
#define JQ2_ASSERT_LOCK_ENTER() do{}while(0)
#define JQ2_ASSERT_LOCK_LEAVE()  do{}while(0)
#endif

#define JQ2_NUM_JOBS (JQ2_MAX_JOBS-1)

#ifdef JQ2_MICROPROFILE
#define JQ2_MICROPROFILE_SCOPE(a,c) MICROPROFILE_SCOPEI("JQ",a,c)
#else
#define JQ2_MICROPROFILE_SCOPE(a,c) do{}while(0)
#endif

#ifdef JQ2_MICROPROFILE_VERBOSE
#define JQ2_MICROPROFILE_VERBOSE_SCOPE(a,c) MICROPROFILE_SCOPEI("JQ",a,c)
#else
#define JQ2_MICROPROFILE_VERBOSE_SCOPE(a,c) do{}while(0)
#endif

#define JQ2_PRIORITY_SIZE (JQ2_PRIORITY_MAX+1)


struct Jq2MutexLock;

void 		Jq2Start(int nNumWorkers);
void 		Jq2Startop();
void 		Jq2CheckFinished(U64 nJob);
U16 	Jq2IncrementStarted(U64 nJob);
void 		Jq2IncrementFinished(U64 nJob);
void		Jq2AttachChild(U64 nParentJob, U64 nChildJob);
U64 	Jq2DetachChild(U64 nChildJob);
void 		Jq2ExecuteJob(U64 nJob, U16 nJobSubIndex);
U16 	Jq2TakeJob(U16* pJobSubIndex);
U16 	Jq2TakeChildJob(U64 nJob, U16* pJobSubIndex);
void 		Jq2Worker(int nThreadId);
U64 	Jq2NextHandle(U64 nJob);
void 		Jq2PriorityListAdd(U16 nJobIndex);
void 		Jq2PriorityListRemove(U16 nJobIndex);
U64 	Jq2Self();
U32	Jq2SelfJobIndex();
int 		Jq2GetNumWorkers();
bool 		Jq2PendingJobs(U64 nJob);
void 		Jq2SelfPush(U64 nJob, U32 nJobIndex);
void 		Jq2SelfPop(U64 nJob);
U64 	Jq2FindHandle(Jq2MutexLock& Lock);


struct Jq2SelfStack
{
	U64 nJob;
	U32 nJobIndex;
};

JQ2_THREAD_LOCAL Jq2SelfStack Jq2SelfStack[JQ2_MAX_JOB_STACK] = {0};
JQ2_THREAD_LOCAL U32 Jq2SelfPos = 0;
JQ2_THREAD_LOCAL U32 Jq2HasLock = 0;

/// current thread index (0 for the main thread, >0 for worker threads)
JQ2_THREAD_LOCAL U32 TLS_ThreadIndex = 0;


#define JQ2_LT_WRAP(a, b) (((I64)((U64)a - (U64)b))<0)
#define JQ2_LE_WRAP(a, b) (((I64)((U64)a - (U64)b))<=0)

struct Jq2Job
{
	U64 nStartedHandle;
	U64 nFinishedHandle;

	I32 nRange;

	U16 nNumJobs;
	U16 nNumStarted;
	U16 num_finished_jobs;

	//parent/child tree
	U16 nParent;
	U16 nFirstChild;
	U16 nSibling;

	//priority linked list
	U16 nLinkNext;
	U16 nLinkPrev;
	U8 nPrio;
	U8 nWaiters;
	U8 nWaitersWas;
	// int8_t nWaitIndex;

	// U32 nSignalCount;//debug

#ifdef JQ2_ASSERT_SANITY
	int nTag;
#endif

	NwJobData		data;
	F_JobFunction	code;

#if mxARCH_TYPE == mxARCH_64BIT
	char	nameAndPadding[8];
#else
	char	nameAndPadding[11];
#endif
};
//TIncompleteType<sizeof(Jq2Job)> checksize;
mxSTATIC_ASSERT(sizeof(Jq2Job)==256);


#define JQ2_PAD_SIZE(type) (JQ2_CACHE_LINE_SIZE - (sizeof(type)%JQ2_CACHE_LINE_SIZE))
#define JQ2_ALIGN_CACHELINE __declspec(align(JQ2_CACHE_LINE_SIZE))

#ifndef _WIN32
#include <pthread.h>
#endif

//#include <atomic>


struct Jq2Mutex
{
	Jq2Mutex();
	~Jq2Mutex();
	void Lock();
	void Unlock();
#ifdef _WIN32
	CRITICAL_SECTION CriticalSection;
#else
	pthread_mutex_t Mutex;
#endif
};

struct Jq2ConditionVariable
{
	Jq2ConditionVariable();
	~Jq2ConditionVariable();
	void Wait(Jq2Mutex& Mutex);
	void NotifyOne();
	void NotifyAll();
#ifdef _WIN32
	CONDITION_VARIABLE Cond;
#else
	pthread_cond_t Cond;
#endif
};


struct Jq2Semaphore
{
	Jq2Semaphore();
	~Jq2Semaphore();
	void Signal(U32 nCount);
	void Wait();

	void Init(int nMaxCount);

#ifdef _WIN32
	HANDLE Handle;
	LONG nMaxCount;
#else
	Jq2Mutex Mutex;
	Jq2ConditionVariable Cond;
	U32 nReleaseCount;
	U32 nMaxCount;	
#endif
};


struct JQ2_ALIGN_CACHELINE Jq2State_t
{
	Jq2Semaphore Semaphore;
	char pad0[ JQ2_PAD_SIZE(Jq2Semaphore) ]; 
	Jq2Mutex Mutex;
	char pad1[ JQ2_PAD_SIZE(Jq2Mutex) ]; 

	Jq2ConditionVariable WaitCond;
	char pad2[ JQ2_PAD_SIZE(Jq2ConditionVariable) ];

	JQ2_THREAD		WorkerThreads[JQ2_MAX_THREADS];

	int nNumWorkers;	//!< number of created worker threads
	int nStop;
	int nTotalWaiting;
	U64 nNextHandle;
	U32 nFreeJobs;

	/// multiple producers, multiple consumers queue
	Jq2Job Jobs[JQ2_MAX_JOBS];// note: the first item is not used, because zero denotes an invalid handle
	U16 nPrioListHead[JQ2_PRIORITY_SIZE];
	U16 nPrioListTail[JQ2_PRIORITY_SIZE];

	void *	threadWorkspace;	//!< per thread scratch memory, including the heap for the main thread
	U32		threadWorkspaceSize;//!< size of per-thread scratchpad
	NwThreadContext	ThreadContexts[1+JQ2_MAX_THREADS];	//!< the first item is used for the main thread!

	Jq2State_t()
	:nNumWorkers(0)
	{
	}

	NwJobStats Stats;	//!< performance stats
} Jq2State;

/// This is called upon the start of each worker thread.
/// _globalThreadIndex is 0 for the main thread, > 0 for worker threads
static void Create_Thread_Context( const UINT _globalThreadIndex )
{
	TLS_ThreadIndex = _globalThreadIndex;

	NwThreadContext & _threadCtx = Jq2State.ThreadContexts[ _globalThreadIndex ];
	_threadCtx.threadIndex = _globalThreadIndex;

	/// local memory private to this thread
	void *	threadLocalSpace = mxAddByteOffset( Jq2State.threadWorkspace, Jq2State.threadWorkspaceSize * _globalThreadIndex );
	const UINT localSpaceSize = Jq2State.threadWorkspaceSize;

	String64	heapName;
	Str::Format(heapName, "ThreadHeap_%u", _globalThreadIndex);

#if USE_PER_THREAD_HEAPS
	mxSTATIC_ASSERT(sizeof(ScratchAllocator) <= FIELD_SIZE(NwThreadContext,padding));
	ScratchAllocator* perThreadHeap = new(_threadCtx.padding) ScratchAllocator( &privateHeap() );
	perThreadHeap->initialize( threadLocalSpace, localSpaceSize, heapName.c_str() );
	_threadCtx.heap = perThreadHeap;
#else
	UNDONE;
	//_threadCtx.heap = &MemoryHeaps::global();
	mxSTATIC_ASSERT(sizeof(TrackingHeap) <= FIELD_SIZE(NwThreadContext,padding));
	_threadCtx.heap = new(_threadCtx.padding) TrackingHeap( heapName.c_str(), MemoryHeaps::global() );
#endif
}

static void Destroy_Thread_Context( const UINT _globalThreadIndex )
{
	NwThreadContext & _threadCtx = Jq2State.ThreadContexts[ _globalThreadIndex ];
	_threadCtx.threadIndex = ~0;
#if USE_PER_THREAD_HEAPS
	{
		ScratchAllocator* heap = (ScratchAllocator*) _threadCtx.heap;
		heap->~ScratchAllocator();
	}
#else
	{
		TrackingHeap* heap = (TrackingHeap*) _threadCtx.heap;
		heap->~TrackingHeap();
	}
#endif
	_threadCtx.heap = nil;

	TLS_ThreadIndex = ~0;
}

static U32 PASCAL MyThreadFunction( void* _userData )
{
	const WorkerThread* worker = (WorkerThread*) _userData;
	const int threadIndex = worker - Jq2State.WorkerThreads;

#if USE_PROFILER
	char	thread_name[32];
	sprintf(thread_name, "Worker_%d", threadIndex);
	BROFILER_THREAD(thread_name);
#endif

	Jq2Worker( threadIndex );

	return 0;
}



ERet WorkerThread::initialize( int _threadIndex )
{
	Thread::CInfo	threadCInfo;
	threadCInfo.entryPoint = &MyThreadFunction;
	threadCInfo.userPointer = this;
	threadCInfo.priority = ThreadPriority_Normal;
	IF_DEVELOPER String64 threadName;
	IF_DEVELOPER Str::Format(threadName, "Worker_%d", _threadIndex);
	IF_DEVELOPER threadCInfo.debugName = threadName.c_str();
	m_thread.Initialize( threadCInfo );
	return ALL_OK;
}
void WorkerThread::shutdown()
{
	m_thread.Shutdown();
}

struct Jq2MutexLock
{
	Jq2Mutex& Mutex;
	Jq2MutexLock(Jq2Mutex& Mutex)
	:Mutex(Mutex)
	{
		Lock();
	}
	~Jq2MutexLock()
	{
		Unlock();
	}
	void Lock()
	{
		JQ2_MICROPROFILE_VERBOSE_SCOPE("MutexLock", 0x992233);
		Mutex.Lock();
		Jq2State.Stats.num_locks++;
		JQ2_ASSERT_LOCK_ENTER();
	}
	void Unlock()
	{
		JQ2_MICROPROFILE_VERBOSE_SCOPE("MutexUnlock", 0x992233);
		JQ2_ASSERT_LOCK_LEAVE();
		Mutex.Unlock();
	}
};


ERet Jq2Start( int nNumWorkers, int threadWorkspaceSize )
{
#if 0
	//verify macros
	U64 t0 = 0xf000000000000000;
	U64 t1 = 0;
	U64 t2 = 0x1000000000000000;
	JQ2_ASSERT(JQ2_LE_WRAP(t0, t0));
	JQ2_ASSERT(JQ2_LE_WRAP(t1, t1));
	JQ2_ASSERT(JQ2_LE_WRAP(t2, t2));
	JQ2_ASSERT(JQ2_LE_WRAP(t0, t1));
	JQ2_ASSERT(!JQ2_LE_WRAP(t1, t0));
	JQ2_ASSERT(JQ2_LE_WRAP(t1, t2));
	JQ2_ASSERT(!JQ2_LE_WRAP(t2, t1));
	JQ2_ASSERT(JQ2_LE_WRAP(t0, t2));
	JQ2_ASSERT(!JQ2_LE_WRAP(t2, t0));
	
	JQ2_ASSERT(!JQ2_LT_WRAP(t0, t0));
	JQ2_ASSERT(!JQ2_LT_WRAP(t1, t1));
	JQ2_ASSERT(!JQ2_LT_WRAP(t2, t2));
	JQ2_ASSERT(JQ2_LT_WRAP(t0, t1));
	JQ2_ASSERT(!JQ2_LT_WRAP(t1, t0));
	JQ2_ASSERT(JQ2_LT_WRAP(t1, t2));
	JQ2_ASSERT(!JQ2_LT_WRAP(t2, t1));
	JQ2_ASSERT(JQ2_LT_WRAP(t0, t2));
	JQ2_ASSERT(!JQ2_LT_WRAP(t2, t0));
#endif
	JQ2_ASSERT_NOT_LOCKED();

	JQ2_ASSERT( ((JQ2_CACHE_LINE_SIZE-1)&(U64)&Jq2State) == 0);
	JQ2_ASSERT( ((JQ2_CACHE_LINE_SIZE-1)&offsetof(Jq2State_t, Semaphore)) == 0);
	JQ2_ASSERT( ((JQ2_CACHE_LINE_SIZE-1)&offsetof(Jq2State_t, Mutex)) == 0);
	JQ2_ASSERT(Jq2State.nNumWorkers == 0);

	nNumWorkers = smallest( nNumWorkers, JQ2_MAX_THREADS );

	Jq2State.Semaphore.Init(nNumWorkers);
	Jq2State.nTotalWaiting = 0;
	Jq2State.nNumWorkers = nNumWorkers;
	Jq2State.nStop = 0;
	Jq2State.nTotalWaiting = 0;
	for(int i = 0; i < JQ2_MAX_JOBS; ++i)
	{
		Jq2State.Jobs[i].nStartedHandle = 0;
		Jq2State.Jobs[i].nFinishedHandle = 0;	
	}
	memset(&Jq2State.nPrioListHead, 0, sizeof(Jq2State.nPrioListHead));
	Jq2State.nFreeJobs = JQ2_NUM_JOBS;
	Jq2State.nNextHandle = 1;
	Jq2State.Stats.num_finished_jobs = 0;
	Jq2State.Stats.num_locks = 0;
	Jq2State.Stats.num_cond_notifies = 0;
	Jq2State.Stats.num_cond_waits = 0;

	DEVOUT("Creating %u worker threads, with %u KiB of private memory for each", nNumWorkers, threadWorkspaceSize/mxKIBIBYTE);

	// +1 because some jobs may have to run on the main thread
	if( threadWorkspaceSize )
	{
		const U32 totalAllocatedMemory = threadWorkspaceSize * (nNumWorkers + 1);
		Jq2State.threadWorkspace = privateHeap().Allocate( totalAllocatedMemory, EFFICIENT_ALIGNMENT );
		if( !Jq2State.threadWorkspace ) {
			return ERR_OUT_OF_MEMORY;
		}
		DBGOUT("Allocated %u KiB for thread-local storage -> 0x%"PRIXPTR,
			totalAllocatedMemory/mxKIBIBYTE, (uintptr_t)Jq2State.threadWorkspace);
		Jq2State.threadWorkspaceSize = threadWorkspaceSize;
	}
	else
	{
		Jq2State.threadWorkspace = 0;
		Jq2State.threadWorkspaceSize = 0;
	}


	//NwThreadContext & mainThreadCtx = Jq2State.ThreadContexts[ 0 ];
	Create_Thread_Context( 0 );	// must be zero for the main thread


	for(int i = 0; i < nNumWorkers; ++i)
	{
		JQ2_THREAD_CREATE(&Jq2State.WorkerThreads[i]);
		JQ2_THREAD_START(&Jq2State.WorkerThreads[i], Jq2Worker, i);
	}


	//DBGOUT("sizeof(Jq2Job): %d", sizeof(Jq2Job));
	//DBGOUT("sizeof(Jq2State): %d", sizeof(Jq2State));

	return ALL_OK;
}

void Jq2Stop()
{
	DBGOUT("Stopping job queue...");
	Jq2WaitAll();
	Jq2State.nStop = 1;
	Jq2State.Semaphore.Signal(Jq2State.nNumWorkers);
	for(int i = 0; i < Jq2State.nNumWorkers; ++i)
	{
		JQ2_THREAD_JOIN(&Jq2State.WorkerThreads[i]);
		JQ2_THREAD_DESTROY(&Jq2State.WorkerThreads[i]);
	}
	Jq2State.nNumWorkers = 0;

	Destroy_Thread_Context( 0 );

	const U32 allocatedMemorySize = Jq2State.threadWorkspaceSize * (Jq2State.nNumWorkers + 1);
	privateHeap().Deallocate( Jq2State.threadWorkspace/*, allocatedMemorySize*/ );
	Jq2State.threadWorkspace = nil;
	Jq2State.threadWorkspaceSize = 0;
}

void Jq2ConsumeStats(NwJobStats* pStats)
{
	Jq2MutexLock lock(Jq2State.Mutex);
	*pStats = Jq2State.Stats;
	Jq2State.Stats.num_finished_jobs  = 0;
	Jq2State.Stats.num_locks = 0;
	Jq2State.Stats.num_cond_notifies = 0;
	Jq2State.Stats.num_cond_waits = 0;
//InterlockedIncrement(&g_frame);
}

const NwThreadContext& Jq2CurrentThreadContext()
{
	return Jq2State.ThreadContexts[ TLS_ThreadIndex ];
}

void Jq2CheckFinished(U64 nJob)
{
	JQ2_ASSERT_LOCKED();
	U32 nIndex = nJob % JQ2_MAX_JOBS; 
	JQ2_ASSERT(JQ2_LE_WRAP(Jq2State.Jobs[nIndex].nFinishedHandle, nJob));
	JQ2_ASSERT(nJob == Jq2State.Jobs[nIndex].nStartedHandle);
	if(0 == Jq2State.Jobs[nIndex].nFirstChild && Jq2State.Jobs[nIndex].num_finished_jobs == Jq2State.Jobs[nIndex].nNumJobs)
	{
		U16 nParent = Jq2State.Jobs[nIndex].nParent;
		if(nParent)
		{
			U64 nParentHandle = Jq2DetachChild(nJob);
			Jq2State.Jobs[nIndex].nParent = 0;
			Jq2CheckFinished(nParentHandle);
		}
		Jq2State.Jobs[nIndex].nFinishedHandle = Jq2State.Jobs[nIndex].nStartedHandle;
		JQ2_CLEAR_FUNCTION(Jq2State.Jobs[nIndex].code);

		//kick waiting threads.
		int8_t nWaiters = Jq2State.Jobs[nIndex].nWaiters;
		if(nWaiters != 0)
		{
			Jq2State.Stats.num_cond_notifies++;
			Jq2State.WaitCond.NotifyAll();
			Jq2State.Jobs[nIndex].nWaiters = 0;
			Jq2State.Jobs[nIndex].nWaitersWas = nWaiters;
		}
		else
		{
			Jq2State.Jobs[nIndex].nWaitersWas = 0xff;
		}
		Jq2State.nFreeJobs++;
	}
}

U16 Jq2IncrementStarted(U64 nJob)
{
	JQ2_ASSERT_LOCKED();
	U16 nIndex = nJob % JQ2_MAX_JOBS; 
	JQ2_ASSERT(Jq2State.Jobs[nIndex].nNumJobs > Jq2State.Jobs[nIndex].nNumStarted);
	U16 nSubIndex = Jq2State.Jobs[nIndex].nNumStarted++;
	if(Jq2State.Jobs[nIndex].nNumStarted == Jq2State.Jobs[nIndex].nNumJobs)
	{
		Jq2PriorityListRemove(nIndex);
	}
	return nSubIndex;
}
void Jq2IncrementFinished(U64 nJob)
{
	JQ2_ASSERT_LOCKED();
	JQ2_MICROPROFILE_VERBOSE_SCOPE("Increment Finished", 0xffff);
	U16 nIndex = nJob % JQ2_MAX_JOBS; 
	Jq2State.Jobs[nIndex].num_finished_jobs++;
	Jq2State.Stats.num_finished_jobs++;
	Jq2CheckFinished(nJob);
}

void Jq2AttachChild(U64 nParentJob, U64 nChildJob)
{
	JQ2_ASSERT_LOCKED();
	U16 nParentIndex = nParentJob % JQ2_MAX_JOBS;
	U16 nChildIndex = nChildJob % JQ2_MAX_JOBS;
	U16 nFirstChild = Jq2State.Jobs[nParentIndex].nFirstChild;
	Jq2State.Jobs[nChildIndex].nParent = nParentIndex;
	Jq2State.Jobs[nChildIndex].nSibling = nFirstChild;
	Jq2State.Jobs[nParentIndex].nFirstChild = nChildIndex;

	JQ2_ASSERT(Jq2State.Jobs[nParentIndex].nFinishedHandle != nParentJob);
	JQ2_ASSERT(Jq2State.Jobs[nParentIndex].nStartedHandle == nParentJob);
}

U64 Jq2DetachChild(U64 nChildJob)
{
	U16 nChildIndex = nChildJob % JQ2_MAX_JOBS;
	U16 nParentIndex = Jq2State.Jobs[nChildIndex].nParent;
	JQ2_ASSERT(Jq2State.Jobs[nChildIndex].num_finished_jobs == Jq2State.Jobs[nChildIndex].nNumJobs);
	JQ2_ASSERT(Jq2State.Jobs[nChildIndex].num_finished_jobs == Jq2State.Jobs[nChildIndex].nNumStarted);
	if(0 == nParentIndex)
	{
		return 0;
	}

	JQ2_ASSERT(nParentIndex != 0);
	JQ2_ASSERT(Jq2State.Jobs[nChildIndex].nFirstChild == 0);
	U16* pChildIndex = &Jq2State.Jobs[nParentIndex].nFirstChild;
	JQ2_ASSERT(Jq2State.Jobs[nParentIndex].nFirstChild != 0); 
	while(*pChildIndex != nChildIndex)
	{
		JQ2_ASSERT(Jq2State.Jobs[*pChildIndex].nSibling != 0);
		pChildIndex = &Jq2State.Jobs[*pChildIndex].nSibling;
	}
	JQ2_ASSERT(pChildIndex);
	*pChildIndex = Jq2State.Jobs[nChildIndex].nSibling;
	return Jq2State.Jobs[nParentIndex].nStartedHandle;
}

//splits range evenly to jobs
int Jq2GetRangeStart(int nIndex, int nFraction, int nRemainder)
{
	int nStart = 0;
	if(nIndex > 0)
	{
		nStart = nIndex * nFraction;
		if(nRemainder <= nIndex)
			nStart += nRemainder;
		else
			nStart += nIndex;
	}
	return nStart;
}

void Jq2SelfPush(U64 nJob, U32 nSubIndex)
{	
	Jq2SelfStack[Jq2SelfPos].nJob = nJob;
	Jq2SelfStack[Jq2SelfPos].nJobIndex = nSubIndex;
	Jq2SelfPos++;
}

void Jq2SelfPop(U64 nJob)
{
	JQ2_ASSERT(Jq2SelfPos != 0);
	Jq2SelfPos--;
	JQ2_ASSERT(Jq2SelfStack[Jq2SelfPos].nJob == nJob);	
}

U32 Jq2SelfJobIndex()
{
	JQ2_ASSERT(Jq2SelfPos != 0);
	return Jq2SelfStack[Jq2SelfPos-1].nJobIndex;
}

int Jq2GetNumWorkers()
{
	return Jq2State.nNumWorkers;
}

void Jq2ExecuteJob(U64 nJob, U16 nSubIndex)
{
	JQ2_MICROPROFILE_SCOPE("Execute", 0xc0c0c0);
	JQ2_ASSERT_NOT_LOCKED();
	JQ2_ASSERT(Jq2SelfPos < JQ2_MAX_JOB_STACK);
	Jq2SelfPush(nJob, nSubIndex);
	const U16 nWorkIndex = nJob % JQ2_MAX_JOBS;
	Jq2Job & rJob = Jq2State.Jobs[ nWorkIndex ];
	const U16 nNumJobs = rJob.nNumJobs;
	const int nRange = rJob.nRange;
	const int nFraction = nRange / nNumJobs;
	const int nRemainder = nRange - nFraction * nNumJobs;	
	const int nStart = Jq2GetRangeStart(nSubIndex, nFraction, nRemainder);
	const int nEnd = Jq2GetRangeStart(nSubIndex+1, nFraction, nRemainder);
	const bool is_last_job = (nEnd == nRange);

//static volatile int BBB = 0;
//if(BBB) {
//	char	buf[256];
//	sprintf(buf,"{%d}[Thread:%d] Executing job(%d/%d): nNumJobs=%d, nRange=%d, nFraction=%d, nRemainder=%d, nStart=%d, nEnd=%d, CRAP%d\n"
//		,g_frame,TLS_ThreadIndex,(int)nJob,nSubIndex,nNumJobs,nRange,nFraction,nRemainder,nStart,nEnd,777);
//	OutputDebugStringA(buf);
//	mxASSERT(nStart<nEnd && nEnd>0);
//}
	NwThreadContext & threadCtx = Jq2State.ThreadContexts[ TLS_ThreadIndex ];
	(*rJob.code)( threadCtx, rJob.data, nStart, nEnd, is_last_job );

	Jq2SelfPop(nJob);
}

U16 Jq2TakeJob(U16* pSubIndex)
{
	JQ2_ASSERT_LOCKED();
	for(int i = 0; i < JQ2_PRIORITY_SIZE; ++i)
	{
		U16 nIndex = Jq2State.nPrioListHead[i];
		if(nIndex)
		{
			*pSubIndex = Jq2IncrementStarted(Jq2State.Jobs[nIndex].nStartedHandle);
			return nIndex;
		}
	}
	return 0;
}
#ifdef JQ2_ASSERT_SANITY
void Jq2TagChildren(U16 nRoot)
{
	for(int i = 1; i < JQ2_MAX_JOBS; ++i)
	{
		JQ2_ASSERT(Jq2State.Jobs[i].nTag == 0);
		if(nRoot == i)
		{
			Jq2State.Jobs[i].nTag = 1;
		}
		else
		{
			int nParent = Jq2State.Jobs[i].nParent;
			while(nParent)
			{
				if(nParent == nRoot)
				{
					Jq2State.Jobs[i].nTag = 1;
					break;
				}
				nParent = Jq2State.Jobs[nParent].nParent;
			}
		}
	}

}
void Jq2CheckTagChildren(U16 nRoot)
{
	for(int i = 1; i < JQ2_MAX_JOBS; ++i)
	{
		JQ2_ASSERT(Jq2State.Jobs[i].nTag == 0);
		bool bTagged = false;
		if(nRoot == i)
			bTagged = false;
		else
		{
			int nParent = Jq2State.Jobs[i].nParent;
			while(nParent)
			{
				if(nParent == nRoot)
				{
					bTagged = true;
					break;
				}
				nParent = Jq2State.Jobs[nParent].nParent;
			}
		}
		JQ2_ASSERT(bTagged == (1==Jq2State.Jobs[i].nTag));
	}
}

void Jq2LoopChildren(U16 nRoot)
{
	int nNext = Jq2State.Jobs[nRoot].nFirstChild;
	while(nNext != nRoot && nNext)
	{
		while(Jq2State.Jobs[nNext].nFirstChild)
			nNext = Jq2State.Jobs[nNext].nFirstChild;
		JQ2_ASSERT(Jq2State.Jobs[nNext].nTag == 1);
		Jq2State.Jobs[nNext].nTag = 0;
		if(Jq2State.Jobs[nNext].nSibling)
			nNext = Jq2State.Jobs[nNext].nSibling;
		else
		{
			//search up
			nNext = Jq2State.Jobs[nNext].nParent;
			while(nNext != nRoot)
			{
				JQ2_ASSERT(Jq2State.Jobs[nNext].nTag == 1);
				Jq2State.Jobs[nNext].nTag = 0;
				if(Jq2State.Jobs[nNext].nSibling)
				{
					nNext = Jq2State.Jobs[nNext].nSibling;
					break;
				}
				else
				{
					nNext = Jq2State.Jobs[nNext].nParent;
				}

			}
		}
	}

	JQ2_ASSERT(Jq2State.Jobs[nRoot].nTag == 1);
	Jq2State.Jobs[nRoot].nTag = 0;
}
#endif


//Depth first. Once a node is visited all child nodes have been visited
U16 Jq2TreeIterate(U64 nJob, U16 nCurrent)
{
	JQ2_ASSERT(nJob);
	U16 nRoot = nJob % JQ2_MAX_JOBS;
	if(nRoot == nCurrent)
	{
		while(Jq2State.Jobs[nCurrent].nFirstChild)
			nCurrent = Jq2State.Jobs[nCurrent].nFirstChild;
	}
	else
	{
		//once here all child nodes _have_ been processed.
		if(Jq2State.Jobs[nCurrent].nSibling)
		{
			nCurrent = Jq2State.Jobs[nCurrent].nSibling;
			while(Jq2State.Jobs[nCurrent].nFirstChild) //child nodes first.
				nCurrent = Jq2State.Jobs[nCurrent].nFirstChild;
		}
		else
		{
			nCurrent = Jq2State.Jobs[nCurrent].nParent;
		}
	}
	return nCurrent;
}


//this code is O(n) where n is the no. of child nodes.
//I wish this could be written in a simpler way
U16 Jq2TakeChildJob(U64 nJob, U16* pSubIndexOut)
{
	JQ2_MICROPROFILE_VERBOSE_SCOPE("Jq2TakeChildJob", 0xff);
	JQ2_ASSERT_LOCKED();
	#if JQ2_ASSERT_SANITY
	{
		//verify that the child iteration is sane
		for(int i = 0; i < JQ2_MAX_JOBS; ++i)
		{
			Jq2State.Jobs[i].nTag = 0;
		}
		Jq2TagChildren(nJob%JQ2_MAX_JOBS);
		U16 nIndex = nJob%JQ2_MAX_JOBS;
		U16 nRootIndex = nJob % JQ2_MAX_JOBS;
		do
		{
			nIndex = Jq2TreeIterate(nJob, nIndex);
			JQ2_ASSERT(Jq2State.Jobs[nIndex].nTag == 1);
			Jq2State.Jobs[nIndex].nTag = 0;
		}while(nIndex != nRootIndex);
		for(int i = 0; i < JQ2_MAX_JOBS; ++i)
		{
			JQ2_ASSERT(Jq2State.Jobs[i].nTag == 0);
		}
	}
	#endif

	U16 nRoot = nJob % JQ2_MAX_JOBS;
	U16 nIndex = nRoot;

	do
	{
		nIndex = Jq2TreeIterate(nJob, nIndex);
		JQ2_ASSERT(nIndex);

		if(Jq2State.Jobs[nIndex].nNumStarted < Jq2State.Jobs[nIndex].nNumJobs)
		{
			*pSubIndexOut = Jq2IncrementStarted(Jq2State.Jobs[nIndex].nStartedHandle);
			return nIndex;
		}
	}while(nIndex != nRoot);
	
	return 0;
}

void Jq2Worker(int nThreadId)
{
#ifdef JQ2_MICROPROFILE
	char sWorker[32];
	snprintf(sWorker, sizeof(sWorker)-1, "Jq2Worker %d", nThreadId);
	MicroProfileOnThreadCreate(&sWorker[0]);
#endif

	const int globalThreadIndex = nThreadId + 1;	// zero is reserved for the main thread
	Create_Thread_Context( globalThreadIndex );

	while(0 == Jq2State.nStop)
	{
		U16 nWork = 0;
		do
		{
			U16 nSubIndex = 0;
			{
				Jq2MutexLock lock(Jq2State.Mutex);
				if(nWork)
				{
					Jq2IncrementFinished(Jq2State.Jobs[nWork].nStartedHandle);
				}
				nSubIndex = 0;
				nWork = Jq2TakeJob(&nSubIndex);
			}
			if(!nWork)
			{
				break;
			}
			Jq2ExecuteJob(Jq2State.Jobs[nWork].nStartedHandle, nSubIndex);
		}while(1);
		Jq2State.Semaphore.Wait();
	}
#ifdef JQ2_MICROPROFILE
	MicroProfileOnThreadExit();
#endif

	Destroy_Thread_Context( globalThreadIndex );
}
U64 Jq2NextHandle(U64 nJob)
{
	nJob++;
	if(0 == (nJob%JQ2_MAX_JOBS))
	{
		nJob++;
	}
	return nJob;
}
U64 Jq2FindHandle(Jq2MutexLock& Lock)
{
	JQ2_ASSERT_LOCKED();
	while(!Jq2State.nFreeJobs)
	{
		if(Jq2SelfPos < JQ2_MAX_JOB_STACK)
		{
			JQ2_MICROPROFILE_SCOPE("AddExecute", 0xff); // if this starts happening the job queue size should be increased...
			U16 nSubIndex = 0;
			U16 nIndex = Jq2TakeJob(&nSubIndex);
			if(nIndex)
			{
				Lock.Unlock();
				Jq2ExecuteJob(Jq2State.Jobs[nIndex].nStartedHandle, nSubIndex);
				Lock.Lock();
				Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);
			}
		}
		else
		{
			JQ2_BREAK(); //out of job queue space. increase JQ2_MAX_JOBS or create fewer jobs
		}
	}
	JQ2_ASSERT(Jq2State.nFreeJobs != 0);
	U64 nNextHandle = Jq2State.nNextHandle;
	U16 nCount = 0;
	while(Jq2PendingJobs(nNextHandle))
	{
		nNextHandle = Jq2NextHandle(nNextHandle);
		JQ2_ASSERT(nCount++ < JQ2_MAX_JOBS);
	}
	Jq2State.nNextHandle = Jq2NextHandle(nNextHandle);
	Jq2State.nFreeJobs--;	
	return nNextHandle;
}

U64 Jq2Add( F_JobFunction JobFunc, const NwJobData& _data, U8 nPrio, int nNumJobs, int nRange )
{
	JQ2_ASSERT(nPrio < JQ2_PRIORITY_SIZE);
	JQ2_ASSERT(Jq2State.nNumWorkers);
	JQ2_ASSERT(nNumJobs);
	if(nRange < 0)
	{
		nRange = nNumJobs;
	}
	if(nNumJobs < 0)
	{
		nNumJobs = Jq2State.nNumWorkers;
	}
	U64 nNextHandle = 0;
	{
		Jq2MutexLock Lock(Jq2State.Mutex);
		nNextHandle = Jq2FindHandle(Lock);
		U16 nIndex = nNextHandle % JQ2_MAX_JOBS;
		
		Jq2Job* pEntry = &Jq2State.Jobs[nIndex];
		JQ2_ASSERT(JQ2_LT_WRAP(pEntry->nFinishedHandle, nNextHandle));
		pEntry->nStartedHandle = nNextHandle;
		JQ2_ASSERT(nNumJobs <= 0xffff);
		pEntry->nNumJobs = (U16)nNumJobs;
		pEntry->nNumStarted = 0;
		pEntry->num_finished_jobs = 0;
		pEntry->nRange = nRange;
		U64 nParentHandle = Jq2Self();
		pEntry->nParent = nParentHandle % JQ2_MAX_JOBS;
		if(pEntry->nParent)
		{
			Jq2AttachChild(nParentHandle, nNextHandle);
		}
		pEntry->code = JobFunc;
		pEntry->data = _data;
		pEntry->nPrio = nPrio;
		pEntry->nWaiters = 0;
		Jq2PriorityListAdd(nIndex);
	}

	Jq2State.Semaphore.Signal(nNumJobs);
	return nNextHandle;
}

bool Jq2IsDone(U64 nJob)
{
	U64 nIndex = nJob % JQ2_MAX_JOBS;
	U64 nFinishedHandle = Jq2State.Jobs[nIndex].nFinishedHandle;
	U64 nStartedHandle = Jq2State.Jobs[nIndex].nStartedHandle;
	JQ2_ASSERT(JQ2_LE_WRAP(nFinishedHandle, nStartedHandle));
	I64 nDiff = (I64)(nFinishedHandle - nJob);
	JQ2_ASSERT((nDiff >= 0) == JQ2_LE_WRAP(nJob, nFinishedHandle));
	return JQ2_LE_WRAP(nJob, nFinishedHandle);
}

bool Jq2PendingJobs(U64 nJob)
{
	U64 nIndex = nJob % JQ2_MAX_JOBS;
	JQ2_ASSERT(JQ2_LE_WRAP(Jq2State.Jobs[nIndex].nFinishedHandle, Jq2State.Jobs[nIndex].nStartedHandle));
	return Jq2State.Jobs[nIndex].nFinishedHandle != Jq2State.Jobs[nIndex].nStartedHandle;
}


void Jq2WaitAll()
{
	U16 nIndex = 0;
	while(Jq2State.nFreeJobs != JQ2_NUM_JOBS)
	{
		U16 nSubIndex = 0;
		{
			Jq2MutexLock lock(Jq2State.Mutex);
			if(nIndex)
			{
				Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);		
			}
			nIndex = Jq2TakeJob(&nSubIndex);
		}
		if(nIndex)
		{
			Jq2ExecuteJob(Jq2State.Jobs[nIndex].nStartedHandle, nSubIndex);
		}
		else
		{
			JQ2_USLEEP(1000);
		}	
	}
	if(nIndex)
	{
		Jq2MutexLock lock(Jq2State.Mutex);
		Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);		
	}
}

JQ2_THREAD_LOCAL int Jq2Spinloop = 0; //prevent optimizer from removing spin loop

void Jq2Wait(U64 nJob, U32 nWaitFlag, U32 nUsWaitTime)
{
	U16 nIndex = 0;
	if(Jq2IsDone(nJob))
	{
		return;
	}
	while(!Jq2IsDone(nJob))
	{
		
		U16 nSubIndex = 0;
		if(nWaitFlag & WAITFLAG_EXECUTE_SUCCESSORS)
		{
			Jq2MutexLock lock(Jq2State.Mutex);
			if(nIndex)
				Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);
			if(Jq2IsDone(nJob)) 
				return;
			nIndex = Jq2TakeChildJob(nJob, &nSubIndex);
		}
		else if(0 != (nWaitFlag & WAITFLAG_EXECUTE_ANY))
		{
			Jq2MutexLock lock(Jq2State.Mutex);
			if(nIndex)
				Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);
			if(Jq2IsDone(nJob)) 
				return;
			nIndex = Jq2TakeJob(&nSubIndex);
		}
		else
		{
			JQ2_BREAK();
		}
		if(!nIndex)
		{
			JQ2_ASSERT(0 != (nWaitFlag & (WAITFLAG_SLEEP|WAITFLAG_BLOCK|WAITFLAG_SPIN)));
			if(nWaitFlag & WAITFLAG_SPIN)
			{
				U64 nTick = Jq2Tick();
				U64 nTicksPerSecond = Jq2TicksPerSecond();
				do
				{
					U32 result = 0;
					for(U32 i = 0; i < 1000; ++i)
					{
						result |= i << (i&7); //do something.. whatever
					}
					Jq2Spinloop |= result; //write it somewhere so the optimizer can't remote it
				}while( (1000000ull*(Jq2Tick()-nTick)) / nTicksPerSecond < nUsWaitTime);


			}
			else if(nWaitFlag & WAITFLAG_SLEEP)
			{
				JQ2_USLEEP(nUsWaitTime);
			}
			else
			{
				Jq2MutexLock lock(Jq2State.Mutex);
				if(Jq2IsDone(nJob))
				{
					return;
				}
				U16 nJobIndex = nJob % JQ2_MAX_JOBS;
				Jq2State.Jobs[nJobIndex].nWaiters++;
				Jq2State.Stats.num_cond_waits++;
				Jq2State.WaitCond.Wait(Jq2State.Mutex);
				Jq2State.Jobs[nJobIndex].nWaiters--;
			}
		}
		else
		{
			Jq2ExecuteJob(Jq2State.Jobs[nIndex].nStartedHandle, nSubIndex);
		}
	}
	if(nIndex)
	{
		Jq2MutexLock lock(Jq2State.Mutex);
		Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);
	}
}

void Jq2ExecuteChildren(U64 nJob)
{
	if(Jq2IsDone(nJob))
	{
		return;
	}
	
	U16 nIndex = 0;
	do 
	{		
		U16 nSubIndex = 0;
		{
			Jq2MutexLock lock(Jq2State.Mutex);
			if(nIndex)
				Jq2IncrementFinished(Jq2State.Jobs[nIndex].nStartedHandle);

			if(Jq2IsDone(nJob)) 
				return;

			nIndex = Jq2TakeChildJob(nJob, &nSubIndex);
		}
		if(nIndex)
		{
			Jq2ExecuteJob(Jq2State.Jobs[nIndex].nStartedHandle, nSubIndex);
		}

	}while(nIndex);
}


void Jq2WaitAll(U64* pJobs, U32 nNumJobs, U32 nWaitFlag, U32 nUsWaitTime)
{
	for(U32 i = 0; i < nNumJobs; ++i)
	{
		if(!Jq2IsDone(pJobs[i]))
		{
			Jq2Wait(pJobs[i], nWaitFlag, nUsWaitTime);
		}
	}
}

U64 Jq2GroupBegin()
{
	Jq2MutexLock Lock(Jq2State.Mutex);
	U64 nNextHandle = Jq2FindHandle(Lock);
	U16 nIndex = nNextHandle % JQ2_MAX_JOBS;
	Jq2Job* pEntry = &Jq2State.Jobs[nIndex];
	JQ2_ASSERT(JQ2_LE_WRAP(pEntry->nFinishedHandle, nNextHandle));
	pEntry->nStartedHandle = nNextHandle;
	pEntry->nNumJobs = 1;
	pEntry->nNumStarted = 1;
	pEntry->num_finished_jobs = 0;
	pEntry->nRange = 0;
	U64 nParentHandle = Jq2Self();
	pEntry->nParent = nParentHandle % JQ2_MAX_JOBS;
	if(pEntry->nParent)
	{
		Jq2AttachChild(nParentHandle, nNextHandle);
	}
	JQ2_CLEAR_FUNCTION(pEntry->code);
	#ifdef JQ2_NO_LAMBDA
	pEntry->pArg = 0;
	#endif
	pEntry->nPrio = 7;
	pEntry->nWaiters = 0;
	Jq2SelfPush(nNextHandle, 0);
	return nNextHandle;
}

void Jq2GroupEnd()
{
	U64 nJob = Jq2Self();
	Jq2SelfPop(nJob);
	
	Jq2MutexLock lock(Jq2State.Mutex);
	Jq2IncrementFinished(nJob);
}

U64 Jq2Self()
{
	return Jq2SelfPos ? Jq2SelfStack[Jq2SelfPos-1].nJob : 0;
}

void Jq2PriorityListAdd(U16 nIndex)
{
	U8 nPrio = Jq2State.Jobs[nIndex].nPrio;
	JQ2_ASSERT(Jq2State.Jobs[nIndex].nLinkNext == 0);
	JQ2_ASSERT(Jq2State.Jobs[nIndex].nLinkPrev == 0);
	U16 nTail = Jq2State.nPrioListTail[nPrio];
	if(nTail != 0)
	{
		JQ2_ASSERT(Jq2State.Jobs[nTail].nLinkNext == 0);
		Jq2State.Jobs[nTail].nLinkNext = nIndex;
		Jq2State.Jobs[nIndex].nLinkPrev = nTail;
		Jq2State.Jobs[nIndex].nLinkNext = 0;
		Jq2State.nPrioListTail[nPrio] = nIndex;
	}
	else
	{
		JQ2_ASSERT(Jq2State.nPrioListHead[nPrio] == 0);
		Jq2State.nPrioListHead[nPrio] = nIndex;
		Jq2State.nPrioListTail[nPrio] = nIndex;
		Jq2State.Jobs[nIndex].nLinkNext = 0;
		Jq2State.Jobs[nIndex].nLinkPrev = 0;
	}
}
void Jq2PriorityListRemove(U16 nIndex)
{
	U8 nPrio = Jq2State.Jobs[nIndex].nPrio;
	U16 nNext = Jq2State.Jobs[nIndex].nLinkNext;
	U16 nPrev = Jq2State.Jobs[nIndex].nLinkPrev;
	Jq2State.Jobs[nIndex].nLinkNext = 0;
	Jq2State.Jobs[nIndex].nLinkPrev = 0;

	if(nNext != 0)
	{
		Jq2State.Jobs[nNext].nLinkPrev = nPrev;
	}
	else
	{
		JQ2_ASSERT(Jq2State.nPrioListTail[nPrio] == nIndex);
		Jq2State.nPrioListTail[nPrio] = nPrev;
	}
	if(nPrev != 0)
	{
		Jq2State.Jobs[nPrev].nLinkNext = nNext;
	}
	else
	{
		JQ2_ASSERT(Jq2State.nPrioListHead[nPrio] == nIndex);
		Jq2State.nPrioListHead[nPrio] = nNext;
	}

}

#ifdef _WIN32
Jq2Mutex::Jq2Mutex()
{
	InitializeCriticalSection(&CriticalSection);
}

Jq2Mutex::~Jq2Mutex()
{
	DeleteCriticalSection(&CriticalSection);
}

void Jq2Mutex::Lock()
{
	EnterCriticalSection(&CriticalSection);
}

void Jq2Mutex::Unlock()
{
	LeaveCriticalSection(&CriticalSection);
}


Jq2ConditionVariable::Jq2ConditionVariable()
{
	InitializeConditionVariable(&Cond);
}

Jq2ConditionVariable::~Jq2ConditionVariable()
{
	//?
}

void Jq2ConditionVariable::Wait(Jq2Mutex& Mutex)
{
	SleepConditionVariableCS(&Cond, &Mutex.CriticalSection, INFINITE);
}

void Jq2ConditionVariable::NotifyOne()
{
	WakeConditionVariable(&Cond);
}

void Jq2ConditionVariable::NotifyAll()
{
	WakeAllConditionVariable(&Cond);
}

Jq2Semaphore::Jq2Semaphore()
{
	Handle = 0;
}
Jq2Semaphore::~Jq2Semaphore()
{
	if(Handle)
	{
		CloseHandle(Handle);
	}

}
void Jq2Semaphore::Init(int nCount)
{
	if(Handle)
	{
		CloseHandle(Handle);
		Handle = 0;
	}
	nMaxCount = nCount;
	Handle = CreateSemaphoreEx(NULL, 0, nCount*2, NULL, 0, SEMAPHORE_ALL_ACCESS);	
}


void Jq2Semaphore::Signal(U32 nCount)
{
	if(nCount > (U32)nMaxCount)
		nCount = nMaxCount;
	BOOL r = ReleaseSemaphore(Handle, nCount, 0);
}

void Jq2Semaphore::Wait()
{
	JQ2_MICROPROFILE_SCOPE("Wait", 0xc0c0c0);
	DWORD r = WaitForSingleObject((HANDLE)Handle, INFINITE);
	JQ2_ASSERT(WAIT_OBJECT_0 == r);
}
#else
Jq2Mutex::Jq2Mutex()
{
	pthread_mutex_init(&Mutex, 0);
}

Jq2Mutex::~Jq2Mutex()
{
	pthread_mutex_destroy(&Mutex);
}

void Jq2Mutex::Lock()
{
	pthread_mutex_lock(&Mutex);
}

void Jq2Mutex::Unlock()
{
	pthread_mutex_unlock(&Mutex);
}


Jq2ConditionVariable::Jq2ConditionVariable()
{
	pthread_cond_init(&Cond, 0);
}

Jq2ConditionVariable::~Jq2ConditionVariable()
{
	pthread_cond_destroy(&Cond);
}

void Jq2ConditionVariable::Wait(Jq2Mutex& Mutex)
{
	pthread_cond_wait(&Cond, &Mutex.Mutex);
}

void Jq2ConditionVariable::NotifyOne()
{
	pthread_cond_signal(&Cond);
}

void Jq2ConditionVariable::NotifyAll()
{
	pthread_cond_broadcast(&Cond);
}

Jq2Semaphore::Jq2Semaphore()
{
	nMaxCount = 0xffffffff;
	nReleaseCount = 0;
}
Jq2Semaphore::~Jq2Semaphore()
{

}
void Jq2Semaphore::Init(int nCount)
{
	nMaxCount = nCount;
}
void Jq2Semaphore::Signal(U32 nCount)
{
	{
		Jq2MutexLock l(Mutex);
		U32 nCurrent = nReleaseCount;
		if(nCurrent + nCount > nMaxCount)
			nCount = nMaxCount - nCurrent;
		nReleaseCount += nCount;
		JQ2_ASSERT(nReleaseCount <= nMaxCount);
		if(nReleaseCount == nMaxCount)
		{
			Cond.NotifyAll();
		}
		else
		{
			for(U32 i = 0; i < nReleaseCount; ++i)
			{
				Cond.NotifyOne();
			}
		}
	}
}

void Jq2Semaphore::Wait()
{
	JQ2_MICROPROFILE_SCOPE("Wait", 0xc0c0c0);
	Jq2MutexLock l(Mutex);
	while(!nReleaseCount)
	{
		Cond.Wait(Mutex);
	}
	nReleaseCount--;
}

#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
