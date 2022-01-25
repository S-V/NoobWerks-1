/*
=============================================================================
	File:	SlowTasks.h
	Desc:	Scheduler for slow tasks which usually last more than one frame
	(e.g. streaming, background resource loading, AI).
	(Most often used for communicating between the loading thread and the main thread.)
	NOTE: slow tasks can spawn 'fast' tasks (jobs).
=============================================================================
*/
#pragma once

#include <Brofiler.h>

struct TaskContext
{
	AllocatorI &	heap;	//!< thread-local, scratch allocator for temporary storage
	//U64	taskId;
public:
	TaskContext( AllocatorI & _heap )
		: heap( _heap )
	{
	}
};

/// Base class for background tasks (e.g. background resource loading).
/// Tasks are organized in priority-based linked lists to avoid allocating arrays for storing them.
struct ASlowTask: TDoublyLinkedList< ASlowTask >
{
	/// This is where the real work should be done (called in the background thread).
	virtual ERet Execute_InBackgroundThread( const TaskContext& _context ) = 0;

	/// Executes completion callback (called in the main thread).
	virtual void Finalize_InMainThread() {};

	/// Destroy_InMainThread() is called by the client when she is finished with this object.
	/// NOTE: This requires the object to delete itself using whatever heap it was allocated in.
	virtual void Destroy_InMainThread() {};

protected:
	virtual ~ASlowTask() {}
};

/// callback to execute code on a background thread
struct ABackgroundRunnable: TDoublyLinkedList< ABackgroundRunnable >
{
	virtual void Tick_BackgroundThread() = 0;

protected:	virtual ~ABackgroundRunnable() {}
};

namespace SlowTasks
{
	/// Bounded priority queue - each priority level is represented by its own FIFO queue.
	enum Priorities
	{
		PriorityHighest = 0,
		PriorityNormal,
		PriorityLow,
		PriorityLowest,
		PriorityMAX		//!< Marker, don't use!
	};

	/// NOTE: the linked list must be set up before launching the background thread!
	extern ABackgroundRunnable::Head g_callbacks;

	/// The construction info for the thread pool
	struct Settings
	{
		//int ringBufferSize;

		/// You may want to set the background thread to a lower priority than the thread that created it.
		EThreadPriority	thread_priority;

		U32		thread_local_space_bytes;

	public:
		Settings()
		{
			thread_priority = ThreadPriority_Normal;
			thread_local_space_bytes = mxMiB(16);
		}
	};

	/// Creates a background thread with provided parameters.
	ERet Initialize( const Settings& _settings, AllocatorI & _allocator );
	/// Waits for all tasks to complete. Blocks until all tasks have executed.
	void Shutdown();

	/// Adds a new task to the LIFO queue. Executes the given task at a later time.
	void add( ASlowTask* _newTask, Priorities _priority = PriorityNormal );

	/// Dispatches synchronous callbacks
	void Tick();

	//void AddDependency( TaskID _parent, TaskID _child );
	//bool IsFinished( TaskID _taskId );
	//void Cancel( TaskID _taskId );

	//void WaitFor( TaskID _taskId );
	//void WaitForAll();

	// Returns the number of pending tasks.
	U32 NumPendingTasks();

}//namespace SlowTasks

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
