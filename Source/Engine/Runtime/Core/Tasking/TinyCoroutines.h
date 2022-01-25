/*
=============================================================================
	
	https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
	https://docs.microsoft.com/en-us/archive/msdn-magazine/2012/august/windows-with-c-lightweight-cooperative-multitasking-with-c
	
	Based on async.hpp - asynchronous, stackless coroutines for C++:
	https://github.com/c-smile/async.hpp
	https://github.com/naasking/async.h
	https://github.com/samguyer/adel

	This is an async/await implementation for C based on Duff's device.
=============================================================================
*/
#pragma once

#include <Base/Memory/ScratchAllocator.h>


struct NwTime;

typedef U64	GlobalTimeMillisecondsT;


/*
 * Copyright (c) 2019, Sandro Magi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the async.h library.
 *
 * Author: Sandro Magi <naasking@gmail.com>
 *
 */

/*
 * = Stackless Async Subroutines =
 *
 * Taking inspiration from protothreads and async/await as found in C#, Rust and JS,
 * this is an async/await implementation for C based on Duff's device.
 *
 * Features:
 *
 * 1. Subroutines can have persistent state that isn't just static state, because
 *    each async subroutine accepts its own struct it uses as a parameter, and
 *    the async state is stored there.
 * 2. Because of the more flexible state handling, async subroutines can be nested
 *    in tree-like fashion which permits fork/join concurrency patterns.
 *
 * Caveats:
 *
 * 1. Due to compile-time bug, MSVC requires changing: 
 *      Project Properties > Configuration Properties > C/C++ > General > Debug Information Format
 *    from "Program Database for Edit And Continue" to "Program Database".
 * 2. As with protothreads, you have to be very careful with switch statements within an async
 *    subroutine. Generally best to avoid them.
 * 3. As with protothreads, you can't make blocking system calls and preserve the async semantics.
 *    These must be changed into non-blocking calls that test a condition.
 */





struct CoTimestamp
{
	GlobalTimeMillisecondsT	time_msec;

public:
	static CoTimestamp GetRealTimeNow()
	{
		CoTimestamp	clock;
		clock.time_msec = tbGetTimeInMilliseconds();//
		return clock;
	}
};



class CoTimer: NonCopyable
{ 
	GlobalTimeMillisecondsT _until_milliseconds;

public:
	mxFORCEINLINE CoTimer()
	{
		this->Reset();
	}

	mxFORCEINLINE void Reset()
	{
		_until_milliseconds = 0;
	}

	mxFORCEINLINE void StartWithDuration(
		unsigned int duration_milliseconds
		, const CoTimestamp& now
		)
	{
		_until_milliseconds = now.time_msec + duration_milliseconds;
	}

	mxFORCEINLINE bool IsExpired(const CoTimestamp& now) const
	{
		return now.time_msec >= _until_milliseconds;
	}
};





/*
 * The async task status.
 * 
 *  All functions return an enum that indicates whether the routine is
 *  done or has more work to do. The various concurrency constructs use
 *  this result to decide whether to continue executing the function, or
 *  move on to the next step.
 */
enum CoTaskStatus: unsigned
{
	// NOTE: the below values must be *invalid* source line numbers!
	Initial = 0,
	Running,
	Completed = ~0u,

	// Other values - the line number where the task is at the moment
};

// 
struct CoTask: NonCopyable
{
	//
	CoTaskStatus	zz__state;
	CoTimer			zz__sleep_timer;

public:
	CoTask()
	{
		Restart();
	}

	void Restart() { zz__state = Initial; }

	/// Return true if the protothread is running or waiting, false if it has ended or exited.
	bool isRunning() const;

	bool isDone() const { return zz__state == Completed; }
};




// NOTE: If you receive error C2051: case expression not constant,
// disable "Edit and Continue" debugging feature.

#define $CoBEGIN	{ switch(zz__state) { case Initial:
#define $CoEND		default: return (zz__state = Completed); } }

/// Stop and exit the coroutine.
#define $EXIT		return (zz__state = Completed);

/// Returns the current status.
#define $CoRETURN		default: return zz__state; } }


#define $CoWAIT_WHILE(cond)	zz__state = CoTaskStatus(__LINE__); case __LINE__: if (cond) return Running
#define $CoWAIT_UNTIL(cond)	$CoWAIT_WHILE(!(cond))

#define $CoYIELD	zz__state = CoTaskStatus(__LINE__); return CoTaskStatus(__LINE__); case __LINE__:



//
#define $CoWAIT_MSEC(MILLISECONDS, CLOCK)	zz__sleep_timer.StartWithDuration((MILLISECONDS), CLOCK); $CoWAIT_UNTIL(zz__sleep_timer.IsExpired(CLOCK));




/*
===============================================================================
	ASYNC TASK SCHEDULER
===============================================================================
*/

/// an abstract async task that can be put into a queue;
/// should be allocated from the task scheduler's heap.
struct CoTaskI
	: AObject
	, CoTask
{
public:
	mxDECLARE_CLASS(CoTaskI,AObject);
	mxDECLARE_REFLECTION;

	virtual CoTaskStatus Run( void* user_data, const CoTimestamp& clock ) = 0;
};

/// for weapon reload anims, etc.
struct CoTaskQueue
{
	TFixedArray< CoTaskI*, 16 >	scheduled_tasks;

	ScratchAllocator	task_allocator;

	BYTE	task_mem[2048];

public:
	CoTaskQueue()
	{
		//
	}

	ERet Init()
	{
		task_allocator.initialize( task_mem, sizeof(task_mem) );
		return ALL_OK;
	}

	void Shutdown()
	{
		task_allocator.shutdown();
	}

	void QueueTask(CoTaskI* new_task)
	{
		scheduled_tasks.add(new_task);
	}

	bool IsBusy() const
	{
		return !scheduled_tasks.IsEmpty();
	}


	void UpdateOncePerFrame(
		const NwTime& args
		);
};
