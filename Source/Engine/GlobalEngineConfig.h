/*
=============================================================================
	COMPILE-TIME CONFIGURATION
=============================================================================
*/
#pragma once

#define RELEASE_BUILD	(0)

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//	BASE
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// If you compile for minimum code size, it's recommended to use native OS services.
#define ENGINE_CONFIG_USE_C_RUNTIME_LIBRARY	(1)

/// Use name hashes, don't store strings.
/// Reduces code size, but breaks many debugging features and text serialization.
#define ENGINE_CONFIG_REFLECTION_NO_STRINGS	(0)

/// https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus
#define ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP	(0)

//==--	Memory

/// Use MemTrace ("ig-memtrace") from Insomniac Games for debugging and profiling memory management.
#define ENGINE_CONFIG_USE_IG_MEMTRACE		(0)

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	#include <ig-memtrace/MemTrace.h>
	#pragma comment( lib, "ig-memtrace.lib" )
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//	CORE
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// numerical asset ids (usually, name hashes) are much faster
/// and consume less memory than string-based ones, but make debugging harder
#define ENGINE_CONFIG_USE_INTEGER_ASSET_IDs	(0)

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//	MULTITHREADED JOB SYSTEM
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// use my own (lame & buggy) multi-threaded job/task system/manager/scheduler
#define ENGINE_CONFIG_JOB_SYSTEM_IMPL_MY_SHIT	(0)
/// Jonas Meyer's Job Queue: https://bitbucket.org/jonasmeyer/jq
#define ENGINE_CONFIG_JOB_SYSTEM_IMPL_JQ		(1)
/// Doug Binks's enki::TaskScheduler
#define ENGINE_CONFIG_JOB_SYSTEM_IMPL_ENKITS	(2)

#define ENGINE_CONFIG_JOB_SYSTEM_IMPL	(ENGINE_CONFIG_JOB_SYSTEM_IMPL_JQ)


/// number of threads for short-lived jobs
//#define ENGINE_CONFIG_USE_JOB_SYSTEM_WORKER_THREADS	(0)

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//	RENDERER
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// Use Branimir Karadzic's excellent rendering backend.
#define ENGINE_CONFIG_USE_BGFX	(0)


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//	PROFILING
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// Use Brofiler?
#define USE_PROFILER (0)
//#include <Brofiler.h>






/// Use Remotery for thread profiling.
#define ENGINE_CONFIG_USE_REMOTERY	(0)

//#define RMT_ENABLED ENGINE_CONFIG_USE_REMOTERY
//#define RMT_USE_D3D11 ENGINE_CONFIG_USE_REMOTERY
//#include <Remotery/lib/Remotery.h>

#if ENGINE_CONFIG_USE_REMOTERY
	#pragma comment( lib, "Remotery.lib" )

	#define tbREMOTERY_PROFILE_FUNCTION		rmt_ScopedCPUSample(__FUNCTION__, RMTSF_None)
#else
	#define tbREMOTERY_PROFILE_FUNCTION
#endif	//#if ENGINE_CONFIG_USE_REMOTERY


#if USE_PROFILER || ENGINE_CONFIG_USE_REMOTERY
	#define tbPROFILE_FUNCTION	tbREMOTERY_PROFILE_FUNCTION;\
								PROFILE
#else
	#define tbPROFILE_FUNCTION
#endif //#if USE_PROFILER || ENGINE_CONFIG_USE_REMOTERY

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DEBUGGING
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// maximum allowed array size (for debugging)
static const size_t DBG_MAX_ARRAY_CAPACITY = (1<<28);	// TArray<>
//static const size_t DBG_MAX_ARRAY_ELEMENTS = (1<<16);

/// e.g. "Opened 'R:/testbed/.Build/Assets/GUI.TbShader.stream' for reading"
const bool DEBUG_PRINT_OPENED_FILES = false;

/// render scenes for printing in papers
extern const bool SCREENSHOTS_FOR_PAPERS;

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
