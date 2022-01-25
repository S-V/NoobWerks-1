# enkiTS

## enki Task Scheduler

A permissively licensed C and C++ Task Scheduler for creating parallel programs.

* [C API via src/TaskScheduler_c.h](src/TaskScheduler_c.h)
* [C++ API via src/TaskScheduler.h](src/TaskScheduler.h)
* [C++ 11 version  on Branch C++11](https://github.com/dougbinks/enkiTS/tree/C++11)
* [User thread version  on Branch UserThread](https://github.com/dougbinks/enkiTS/tree/UserThread) for running enkiTS on other tasking / threading systems, so it can be used as in other engines as well as standalone for example.
* [C++ 11 version of user threads on Branch UserThread_C++11](https://github.com/dougbinks/enkiTS/tree/UserThread_C++11)

The user thread versions may get rolled into the C++11 and standard branches with defines controlling whether to include User Thread API.

Note - this is a work in progress conversion from my code for [enkisoftware's](http://www.enkisoftware.com/) Avoyd codebase, with [RuntimeCompiledC++](https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus) removed along with the removal of profiling code.

As this was originally written before widespread decent C++11 support for atomics and threads, these are implemented here per-platform only supporting Windows, Linux and OSX on Intel x86 / x64. [A separate C++11 branch exists](https://github.com/dougbinks/enkiTS/tree/C++11) for those who would like to use it, but this currently has slightly slower performance under very high task throughput when there is low work per task.

The example code requires C++ 11 for chrono (and for [C++ 11 features in the C++11 branch C++11](https://github.com/dougbinks/enkiTS/tree/C++11) )

For further examples, see https://github.com/dougbinks/enkiTSExamples

## Building

On Windows / Mac OS X / Linux with cmake installed, open a prompt in the enkiTS directory and:

1. `mkdir build`
2. `cmake ..`
3. either run `make` or open `enkiTS.sln`

## Project Goals

1. *Lightweight* - enkiTS is designed to be lean so you can use it anywhere easily, and understand it.
1. *Fast, then scalable* - enkiTS is designed for consumer devices first, so performance on a low number of threads is important, followed by scalability.
1. *Braided parallelism* - enkiTS can issue tasks from another task as well as from the thread which created the Task System.
1. *Up-front Allocation friendly* - enkiTS is designed for zero allocations during scheduling.

 
## Usage

C usage:
```C
#include "TaskScheduler_c.h"

enkiTaskScheduler*	g_pTS;

void ParalleTaskSetFunc( uint32_t start_, uint32_t end, uint32_t threadnum_, void* pArgs_ ) {
   /* Do something here, can issue tasks with g_pTS */
}

int main(int argc, const char * argv[]) {
   enkiTaskSet* pTask;
   g_pTS = enkiCreateTaskScheduler();
	
   // create a task, can re-use this to get allocation occuring on startup
   pTask	= enkiCreateTaskSet( g_pTS, ParalleTaskSetFunc );

   enkiAddTaskSetToPipe( g_pTS, pTask, NULL, 1); // NULL args, setsize of 1

   // wait for task set (running tasks if they exist) - since we've just added it and it has no range we'll likely run it.
   enkiWaitForTaskSet( g_pTS, pTask );
   return 0;
}
```

C++ usage:
```C
#include "TaskScheduler.h"

enki::TaskScheduler g_TS;

// define a task set, can ignore range if we only do one thing
struct ParallelTaskSet : enki::ITaskSet {
   virtual void    ExecuteRange( TaskSetPartition range, uint32_t threadnum ) {
      // do something here, can issue tasks with g_TS
   }
};

int main(int argc, const char * argv[]) {
   g_TS.Initialize();
   enki::ParallelTask task; // default constructor has a set size of 1
   g_TS.AddTaskSetToPipe( &task );

   // wait for task set (running tasks if they exist) - since we've just added it and it has no range we'll likely run it.
   g_TS.WaitforTaskSet( &task );
   return 0;
}
```

C++ 11 usage (currently requires [C++11 branch](https://github.com/dougbinks/enkiTS/tree/C++11), or define own lambda wrapper taskset interface.
```C
#include "TaskScheduler.h"

enki::TaskScheduler g_TS;

int main(int argc, const char * argv[]) {
   g_TS.Initialize();

   enki::TaskSet task( 1, []( enki::TaskSetPartition range, uint32_t threadnum  ) {
         // do something here
      }  );

   g_TS.AddTaskSetToPipe( &task );
   g_TS.WaitforTaskSet( &task );
   return 0;
}
```

## To Do

* Documentation.
* Add a profile header for support of various profiling libraries such as [ITT](https://software.intel.com/en-us/articles/intel-itt-api-open-source), [Remotery](https://github.com/dougbinks/Remotery), [InsightProfile](https://github.com/kayru/insightprofiler), [MicroProfile](https://bitbucket.org/jonasmeyer/microprofile) and potentially [Telemetry](http://www.radgametools.com/telemetry.htm).
* Benchmarking?


## License (zlib)

Copyright (c) 2013 Doug Binks

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.




