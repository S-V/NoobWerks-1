/*
=============================================================================
	File:	SlowTasks.cpp
	Desc:	Scheduler for slow tasks which usually last more than one frame
	(e.g. streaming, background resource loading, AI)
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop

//#include <Remotery/lib/Remotery.h>

#include <Base/Memory/Stack/StackAlloc.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Core/Core.h>
#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Tasking/SlowTasks.h>

namespace SlowTasks
{

ABackgroundRunnable::Head g_callbacks = nil;

typedef TLinkedDequeue< ASlowTask >	TaskQueue;

struct SlowTasksManager
{
	ConditionVariable	taskQueueNonEmptyCV;	//!< for signaling that the task queue is not empty

	SpinWait		pendingTasksCS;	//!< for serializing access to the pending tasks queue
	// 'bounded height priority queue'
	TaskQueue		pendingTasks[ PriorityMAX ];	//!< linked list of outstanding/enqueued tasks
	unsigned		totalPendingTasks;	//!< the total number of pending tasks
	bool			isRunningFlag;	//!< for exiting the background thread

	TaskQueue		completedTasks;	//!< executed, finished tasks (they'll be finalized in the main thread)
	SpinWait		completedTasksCS;

	Thread	backgroundThread;
	LinearAllocator	threadLocalHeap;

	AllocatorI & allocator;

public:
	SlowTasksManager( AllocatorI & _allocator )
		: allocator( _allocator )
	{
	}

	ERet Initialize( const Settings& _settings )
	{
		mxASSERT_MAIN_THREAD;

		// Create the queue critical sections
		pendingTasksCS.Initialize();
		completedTasksCS.Initialize();
		taskQueueNonEmptyCV.Initialize();

		totalPendingTasks = 0;
		isRunningFlag = true;

		DEVOUT("Allocating %u bytes for 'slow' tasks", _settings.thread_local_space_bytes);
		mxDO(threadLocalHeap.Initialize(
			allocator.Allocate( _settings.thread_local_space_bytes, EFFICIENT_ALIGNMENT )
			, _settings.thread_local_space_bytes
		));

		// Create a thread to handle all background processing.

		/// entry point for the thread, convenience mostly
		struct Wrapper {
			static U32 PASCAL ThreadFunction( void* _userData ) {
				SlowTasksManager* taskManager = static_cast< SlowTasksManager* >( _userData );
				taskManager->ThreadFunction();
				return 0;
			}
		};
		Thread::CInfo	threadCInfo;
		threadCInfo.entryPoint = &Wrapper::ThreadFunction;
		threadCInfo.userPointer = this;
		threadCInfo.priority = _settings.thread_priority;
		IF_DEVELOPER threadCInfo.debugName = "SlowTasks";
		backgroundThread.Initialize( threadCInfo );

		return ALL_OK;
	}

	void Shutdown()
	{
		mxASSERT_MAIN_THREAD;

		// Wake up and exit the background thread.

		// add a special ExitTask to the queue
		struct ExitTask : ASlowTask {
			virtual ERet Execute_InBackgroundThread( const TaskContext& _context ) {return ALL_OK;}
		} dummyTask;

		// If there are no background tasks, the background thread will wait forever.
		// no pending tasks => the thread is waiting
		{
			SpinWait::Lock	scopedLock( pendingTasksCS );
			isRunningFlag = false;	//!<= must be done before signalling!
			this->add( &dummyTask );
		}
		// notify the thread and wait for it to finish
		taskQueueNonEmptyCV.NotifyOne();

		backgroundThread.Shutdown();	// 'join'

		for( int i = 0; i < mxCOUNT_OF(pendingTasks); i++ ) {
			pendingTasks[i].SetEmpty();
		}
		completedTasks.SetEmpty();

		totalPendingTasks = 0;

		taskQueueNonEmptyCV.Shutdown();
		pendingTasksCS.Shutdown();
		completedTasksCS.Shutdown();

		allocator.Deallocate(
			threadLocalHeap.GetBufferPtr()
		);
		threadLocalHeap.Shutdown();
	}

	void add( ASlowTask* _newTask, Priorities _priority = PriorityNormal )
	{
		// add a work item to the queue of work items.
		bool queueWasEmpty;
		{
			SpinWait::Lock	scopedLock( pendingTasksCS );
			queueWasEmpty = (totalPendingTasks == 0);
			pendingTasks[ _priority ].Append( _newTask );
			++totalPendingTasks;
		}
		// Wake up the background thread if it isn't already running.
		if( queueWasEmpty ) {
			taskQueueNonEmptyCV.NotifyOne();
		}
	}

	void ThreadFunction()
	{
		//rmt_SetCurrentThreadName( "SlowTasks" );
		//BROFILER_THREAD("SlowTasks");

		DBGOUT("SlowTasks thread started.");

		while( isRunningFlag )
		{
			// run background tasks
			{
				ABackgroundRunnable* current = g_callbacks;
				while( current ) {
					current->Tick_BackgroundThread();
					current = current->next;
				}
			}

			ASlowTask* taskTaken = nil;
			{
				SpinWait::Lock	scopedLock( pendingTasksCS );
				// Wait on the condition variable until there are items in the queue.
				while( !totalPendingTasks ) {
					// no tasks, go to sleep until one arrives
					taskQueueNonEmptyCV.Wait( pendingTasksCS );
				}
				// Pop a work item off of the queue.
				for( int priority = PriorityHighest; priority < PriorityMAX; priority++ ) {
					taskTaken = pendingTasks[ priority ].TakeFirst();
					if( taskTaken ) {
						--totalPendingTasks;
						break;
					}
				}
			}
			// Handle the work item.
			mxASSERT_PTR( taskTaken );

			StackAllocator	scratch( threadLocalHeap, MemoryHeaps::global() );

			const TaskContext	context( scratch );
			taskTaken->Execute_InBackgroundThread( context );

			// add it to the 'finished-items' queue
			{
				SpinWait::Lock	scopedLock( completedTasksCS );
				completedTasks.Append( taskTaken );
			}

			if( G_isExitRequested() ) {
				DBGOUT("!SlowTasks thread interrupted!");
				break;
			}
		}

		DBGOUT("SlowTasks thread exiting...");
	}
};
static TPtr< SlowTasksManager >	g_me;


static U32 ThreadFunction( void* _userData );

ERet Initialize( const Settings& _settings, AllocatorI & _allocator )
{
	mxASSERT_MAIN_THREAD;

	g_me = mxNEW( _allocator, SlowTasksManager, _allocator );
//	mxENSURE( g_me != nil, ERR_OUT_OF_MEMORY );

	g_me->Initialize( _settings );

	return ALL_OK;
}

void Shutdown()
{
	mxASSERT_MAIN_THREAD;
	DBGOUT("Slow tasks: shutting down...");
	g_me->Shutdown();
	mxDELETE( g_me._ptr, g_me->allocator );
	g_me = nil;
}

void add( ASlowTask* _newTask, Priorities _priority )
{
	g_me->add( _newTask, _priority );
}

void Tick()
{
	SpinWait::Lock	scopedLock( g_me->completedTasksCS );

	ASlowTask* current = g_me->completedTasks.head;
	while( current )
	{
		ASlowTask* next = current->next;

		current->Finalize_InMainThread();
		current->Destroy_InMainThread();

		current = next;
	}
	g_me->completedTasks.SetEmpty();
}

U32 NumPendingTasks()
{
	SpinWait::Lock	scopedLock( g_me->pendingTasksCS );
	return g_me->totalPendingTasks;
}

}//namespace SlowTasks

/*
Streaming:
http://qdn.qubesoft.com/docs/1.1/doc/qserver/streaming.html

Highly Detailed Continuous Worlds: Streaming Game Resources From Slow Media
http://www.gdcvault.com/play/1022677/Highly-Detailed-Continuous-Worlds-Streaming

http://docs.aws.amazon.com/lumberyard/latest/developerguide/system-streaming.html

Job systems:
https://github.com/SFTtech/openage/tree/master/libopenage/job

###
https://github.com/nasa/libSPRITE/blob/master/SRTX/Base_ring_buffer.cpp
https://github.com/nasa/libSPRITE/blob/master/SRTX/Scheduler.h
KeInitializeDpc
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
