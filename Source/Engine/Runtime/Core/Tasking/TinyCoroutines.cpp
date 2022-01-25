//

#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Tasking/TinyCoroutines.h>

#include <Core/Client.h>	// NwTime


mxDEFINE_ABSTRACT_CLASS(CoTaskI);
mxBEGIN_REFLECTION(CoTaskI)
mxMEMBER_FIELD_OF_TYPE(zz__state, T_DeduceTypeInfo<int>()),
mxEND_REFLECTION



void CoTaskQueue::UpdateOncePerFrame(
	const NwTime& args
	)
{
	const CoTimestamp	clock = CoTimestamp::GetRealTimeNow();//

	//
	U32	completed_tasks_mask = 0;

	//
	nwFOR_LOOP(UINT, i, scheduled_tasks.num())
	{
		const CoTaskStatus task_status = scheduled_tasks._data[i]->Run(nil, clock);
		if( task_status == Completed ) {
			completed_tasks_mask |= BIT(i);
		} else {
			break; // stop after the first running task
		}
	}

	//
	for(int i = scheduled_tasks.num(); i >= 0; --i)
	{
		if( completed_tasks_mask & BIT(i) )
		{
			CoTaskI* task_to_delete = scheduled_tasks._data[i];

			DBGOUT("Deleting completed task '%s' at [%d].",
				typeid(task_to_delete).name(),
				i
				);

			scheduled_tasks.RemoveAt(i);
			//
			task_to_delete->~CoTaskI();

			//
			task_allocator.Deallocate(task_to_delete);
		}
	}
}



/*
Coroutines:

#include <boost/coroutine/all.hpp>
https://www.boost.org/doc/libs/1_61_0/boost/asio/coroutine.hpp

https://github.com/samguyer/adel

https://hackaday.com/2019/09/24/asynchronous-routines-for-c/
https://github.com/naasking/async.h

https://terrainformatica.com/2008/06/25/generators-in-c-revisited/
https://github.com/c-smile/async.hpp


https://github.com/obiwanjacobi/atl/blob/master/Source/Code/ArduinoTemplateLibrary/Task.h
https://github.com/obiwanjacobi/Zingularity/blob/master/Source/Jacobi.CpuZ80/Async.h


https://github.com/hnes/libaco
http://cvs.schmorp.de/libcoro/coro.h

http://wiki.c2.com/?ContinuationImplementation

// Coroutine-ish feature implemented using a thread.
http://www.ilikebigbits.com/2016_03_20_coroutines.html
https://github.com/emilk/emilib/blob/master/emilib/coroutine.hpp

https://github.com/crazybie/co
https://github.com/jamboree/co2
https://github.com/vmilea/CppAsync
https://github.com/r-lyeh-archived/duty/blob/master/duty.hpp

https://github.com/topics/coroutines?l=c%2B%2B

https://www.gamedev.net/forums/topic/374644-thoughts-on-coroutines-for-game-scripting/


Protothreads:

How protothreads really work
http://dunkels.com/adam/pt/expansion.html

https://github.com/adamdunkels/uip/blob/master/uip/pt.h
*/
