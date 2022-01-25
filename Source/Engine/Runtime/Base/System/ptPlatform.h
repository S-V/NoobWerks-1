/*
=============================================================================
	File:	Platform.h
	Desc:	System-level non-portable services,
			preprocessor definitions for platform, compiler and atomic types.
	Note:	some code parts swiped from bx library by Branimir Karadzic
	Info on platform abstraction:
	Platform Abstraction with C++ Templates
	http://www.altdevblogaday.com/2011/06/27/platform-abstraction-with-cpp-templates/
	#include <rules>
	http://zeuxcg.org/2010/11/15/include-rules/
=============================================================================
*/
#pragma once

/*
	mxCOMPILER
	mxCOMPILER_NAME
*/
#define mxCOMPILER_GCC		0
#define mxCOMPILER_MSVC		1
#define mxCOMPILER_INTEL	2
#define mxCOMPILER_CLANG	3
#define mxCOMPILER_MWERKS	4
/*
	mxPLATFORM
*/
#define mxPLATFORM_WINDOWS		0
#define mxPLATFORM_LINUX		1
#define mxPLATFORM_NACL			2
#define mxPLATFORM_OSX			3
#define mxPLATFORM_QNX			4
#define mxPLATFORM_ANDROID		5
#define mxPLATFORM_MACOS		6
#define mxPLATFORM_IOS			7
#define mxPLATFORM_XBOX360		8
#define mxPLATFORM_PS3			9
#define mxPLATFORM_PS4			10
#define mxPLATFORM_WII			11
#define mxPLATFORM_PSP2			12
#define mxPLATFORM_EMSCRIPTEN	13
/*
	mxCPU_TYPE
*/
#define mxCPU_X86	0
#define mxCPU_ARM	1
#define mxCPU_PPC	2
/*
	mxARCH_TYPE
*/
#define mxARCH_32BIT	0
#define mxARCH_64BIT	1
/*
	mxCPU_ENDIANNESS
*/
#define mxCPU_ENDIAN_LITTLE	0
#define mxCPU_ENDIAN_BIG	1

//
// Identify the current platform.
//
// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_XBOX_VER)
#	define mxPLATFORM mxPLATFORM_XBOX360
#elif defined(_WIN32) || defined(_WIN64)
#	define mxPLATFORM mxPLATFORM_WINDOWS
#elif defined(__native_client__)
// NaCl compiler defines __linux__
#	define mxPLATFORM mxPLATFORM_NACL
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#	define mxPLATFORM mxPLATFORM_ANDROID
#elif defined(__linux__)
#	define mxPLATFORM mxPLATFORM_LINUX
#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#	define mxPLATFORM mxPLATFORM_IOS
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#	define mxPLATFORM mxPLATFORM_OSX
#elif defined(EMSCRIPTEN)
#	define mxPLATFORM mxPLATFORM_EMSCRIPTEN
#elif defined(__QNX__)
#	define mxPLATFORM mxPLATFORM_QNX
#elif defined(SN_TARGET_PSP2)
#	define mxPLATFORM mxPLATFORM_PSP2
#elif defined(SN_TARGET_PS3) || defined(SN_TARGET_PS3_SPU)
#	define mxPLATFORM mxPLATFORM_PS3
#else
#	error "Unknown platform!"
#endif //

#define mxPLATFORM_POSIX\
	(  (mxPLATFORM == mxPLATFORM_LINUX)\
	|| (mxPLATFORM == mxPLATFORM_OSX)\
	|| (mxPLATFORM == mxPLATFORM_IOS)\
	|| (mxPLATFORM == mxPLATFORM_MACOS)\
	|| (mxPLATFORM == mxPLATFORM_ANDROID)

//
// find the current compiler.
//
// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(_MSC_VER)
#	define mxCOMPILER mxCOMPILER_MSVC
#elif defined(__clang__)
// clang defines __GNUC__
#	define mxCOMPILER mxCOMPILER_CLANG
#elif defined(__GNUC__)
#	define mxCOMPILER mxCOMPILER_GCC
#elif defined(__MWERKS__)
#	define mxCOMPILER mxCOMPILER_MWERKS
#else
#	error "Unsupported compiler!"
#endif //

//
// find CPU architecture type.
//
// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__)
#	define mxCPU_TYPE mxCPU_ARM
#	define mxCACHE_LINE_SIZE 64
#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
#	define mxCPU_TYPE mxCPU_PPC
#	define mxCACHE_LINE_SIZE 128
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#	define mxCPU_TYPE mxCPU_X86
#	define mxCACHE_LINE_SIZE 64
#else
#	error "Unknown processor architecture!"
#endif //

#if defined(__x86_64__) || defined(_M_X64) || defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#	define mxARCH_TYPE		mxARCH_64BIT
#	define mxPOINTERSIZE	8
#else
#	define mxARCH_TYPE		mxARCH_32BIT
#	define mxPOINTERSIZE	4
#endif //

#if (mxCPU_TYPE == mxCPU_PPC)
#	define mxCPU_ENDIANNESS mxCPU_ENDIAN_BIG
#else
#	define mxCPU_ENDIANNESS mxCPU_ENDIAN_LITTLE
#endif // mxPLATFORM_

//---------------------------------------------------------------------------
// Include platform-specific headers.
//---------------------------------------------------------------------------

#if (mxPLATFORM == mxPLATFORM_WINDOWS) && (mxCOMPILER == mxCOMPILER_MSVC)
#	include <Base/System/Windows/ptWindowsOS.h>
#else
#	error "Unsupported compiler/platform!"
#endif

#include "ptMacros.h"

// These functions are used to initialize/shutdown platform layer.
void	mxPlatform_Init();
void	mxPlatform_Shutdown();

//-----------------------------------------------------------------
//		General platform & system info
//-----------------------------------------------------------------

enum EPlatformId
{
	Platform_PC_Windows,
	Platform_PC_Linux,
	Platform_Mac,
	Platform_IPhone,
	Platform_PS3,
	Platform_XBox360,
	Platform_Wii,
	Platform_GameCube,
	Platform_Unknown,
};
EPlatformId mxCurrentPlatform();
const char* mxGetPlatformName( EPlatformId platformId );

//-----------------------------------------------------------------
//		Processor information
//-----------------------------------------------------------------
enum ECpuType
{
	CPU_x86_32,
	CPU_x86_64,
	CPU_PowerPC,
	CPU_Unknown,	// Any unrecognized processor.
};
const char* mxGetCpuTypeName( ECpuType CpuType );

enum ECpuVendor
{
	CpuVendor_AMD,
	CpuVendor_Intel,
	CpuVendor_Unknown,
};
enum ECpuFamily
{
	CpuFamily_Unknown		= 0,
	CpuFamily_Pentium		= 5,
	CpuFamily_PentiumII		= 6,
	CpuFamily_PentiumIII	= 6,
	CpuFamily_PentiumIV		= 15,
};
enum CpuCacheType
{
	CpuCache_Unified,
	CpuCache_Instruction,
	CpuCache_Data,
	CpuCache_Trace,
	NumCpuCacheTypes
};
enum ELogicalProcessorRelationship
{
	CpuRelation_ProcessorCore	= BIT(0),
	CpuRelation_NumaNode		= BIT(1),
	CpuRelation_Cache			= BIT(2),
	CpuRelation_ProcessorPackage= BIT(3),
	CpuRelation_Group			= BIT(4),
	CpuRelation_All				= BITS_ALL
};
struct CpuCacheInfo
{
	UINT32			size;		// in bytes
	UINT32			lineSize;	// in bytes
	BYTE			level;
	BYTE			associativity;	// 0xFF if fully associative
	CpuCacheType	type;
};
struct mxCpuInfo
{
	char	brandName[49];	// Processor Brand String.
	char	vendor[13];		// CPU vendor string.
	INT		brandId;		// CPU brand index.
	INT		family;			// ECpuFamily.
	INT		model;			// CPU model.
	INT		stepping;		// Stepping ID.

	INT		extendedFamily;
	INT		extendedModel;

	ECpuType	type;

	UINT	numLogicalCores;// number of CPU cores
	UINT	numPhysicalCores;

	UINT	pageSize;		// CPU page size, in bytes
	UINT	cacheLineSize;	// cache line size, in bytes

	// cache sizes in 1K units
	UINT	L1_ICache_Size, L1_DCache_Size;
	UINT	L2_Cache_Size, L3_Cache_Size;

	// cache associativity
	UINT	L1_Assoc, L2_Assoc, L3_Assoc;

	// CPU clock frequency
	UINT32	readFreqMHz;	// value read from OS registry, in megahertz
	UINT64	estimatedFreqHz;// roughly estimated CPU clock speed, in hertz

	// CPU features
	bool	has_CPUID;
	bool	has_RDTSC;
	bool	has_FPU;	// On-Chip FPU
	bool	has_MMX;	// Multimedia Extensions
	bool	has_MMX_AMD;// AMD Extensions to the MMX instruction set
	bool	has_3DNow;	// 3DNow!
	bool	has_3DNow_Ext;// AMD Extensions to the 3DNow! instruction set
	bool	has_SSE_1;	// Streaming SIMD Extensions
	bool	has_SSE_2;	// Streaming SIMD Extensions 2
	bool	has_SSE_3;	// Streaming SIMD Extensions 3 aka Prescott's New Instructions
	bool	has_SSE_4_1;// Streaming SIMD Extensions 4.1
	bool	has_SSE_4_2;// Streaming SIMD Extensions 4.2
	bool	has_SSE_4a;	// AMD's SSE4a extensions
	bool	has_HTT;	// Hyper-Threading Technology
	bool	has_CMOV;	// Conditional Move and fast floating point comparison (FCOMI) instructions
	bool	has_EM64T;	// 64-bit Memory Extensions (64-bit registers, address spaces, RIP-relative addressing mode)
	bool	has_MOVBE;	// MOVBE (this instruction is unique to Intel Atom processor)
	bool	has_FMA;	// 256-bit FMA (Intel)
	bool	has_POPCNT;	// POPCNT
	bool	has_AES;	// AES support (Intel)
	bool	has_AVX;	// 256-bit AVX (Intel)
};

UINT mxGetNumCpuCores();	// shortcut for convenience

struct PtSystemInfo
{
	EPlatformId		platform;
	const TCHAR *	OSName;
	const TCHAR *	OSDirectory;
	const TCHAR *	SysDirectory;
	const TCHAR *	OSLanguage;
	mxCpuInfo		cpu;
};
void mxGetSystemInfo( PtSystemInfo &OutInfo );

///
///	System memory management functions.
///
void* Sys_Alloc( size_t _size, size_t alignment );
void Sys_Free( void * _memory );

//-----------------------------------------------------------------
//		Memory usage statistics
//-----------------------------------------------------------------

/// Returns the total amount of system (physical) RAM, in megabytes.
size_t mxGetSystemRam();

struct mxMemoryStatus
{
	/// Number between 1 and 100 that specifies
	/// the approximate percentage of physical memory  that is in use.
	UINT 	memoryLoad;

	size_t 	totalPhysical;	//!< Total size of physical memory, in megabytes.
	size_t 	availPhysical;	//!< Size of physical memory available, in megabytes.
	size_t 	totalPageFile;	//!< Size of commited memory limit, in megabytes.
	size_t 	availPageFile;

	/// Total size of the user mode portion
	/// of the virtual address space of the calling process, inmega bytes.
	size_t 	totalVirtual;
	
	/// Size of unreserved and uncommitted memory in the user mode portion
	/// of the virtual address space of the calling process, in megabytes.
	size_t 	availVirtual;
};

/// Returns memory stats.
void mxGetCurrentMemoryStatus( mxMemoryStatus & OutStats );
void mxGetExeLaunchMemoryStatus( mxMemoryStatus & OutStats );

/// This function is provided for convenience.
void mxGetMemoryInfo( size_t &TotalBytes, size_t &AvailableBytes );

//-----------------------------------------------------------------
//		DLL-related functions.
//-----------------------------------------------------------------

void * mxDLLOpen( const char* moduleName );
void   mxDLLClose( void* module );
void * mxDLLGetSymbol( void* module, const char* symbolName );

//-----------------------------------------------------------------
//		System time
//-----------------------------------------------------------------

/// Returns the number of milliseconds elapsed since the last reset.
/// NOTE: Should only be used for profiling, not for game timing!
UINT32	mxGetTimeInMilliseconds();

// These functions return the amount of time elapsed since the application started.
UINT64 mxGetTimeInMicroseconds();
unsigned long int tbGetTimeInMilliseconds();
FLOAT64 mxGetTimeInSeconds();

// These functions can be used for accurate performance testing,
// but not for in-game timing.
//
/// Retrieves the number of milliseconds that have elapsed since the system was started, up to 49.7 days.
UINT32	mxGetClockTicks();
/// CPU frequency in Hertz
UINT32	mxGetClockTicksPerSecond();

float	mxGetClockTicks_F();
float	mxGetClockTicksPerSecond_F();

///
///	CalendarTime
///
class CalendarTime {
public:
    enum Month 
    {
        January = 1,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December,
    };
    enum Weekday
    {
        Sunday = 0,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
    };
	static const char * MonthToStr( Month e );
	static const char * WeekdayToStr( Weekday e );
public:
	UINT		year;		/* years since 1900 */
    Month		month;		/* months since January - [1,12] */
    Weekday		weekday;	/* days since Sunday - [0,6] */
    UINT		day;		/* day of the month - [1,31] */
    UINT		hour;		/* hours since midnight - [0,23] */
    UINT		minute;		/* minutes after the hour - [0,59] */
    UINT		second;		/* seconds after the minute - [0,59] */
    UINT		milliSecond;
public:
	/// Obtains the current system time. This does not depend on the current time zone.
	static CalendarTime	GetCurrentSystemTime();
	/// Obtains the current local time (with time-zone adjustment).
	static CalendarTime	GetCurrentLocalTime();
};

// Returns the exact local time.
//
void mxGetTime( UINT & year, UINT & month, UINT & dayOfWeek,
			 UINT & day, UINT & hour, UINT & minute,
			 UINT & second, UINT & milliseconds );

// Read-Write memory barriers do not translate to any instructions,
// but prevent the compiler from re-ordering memory accesses across the barrier
// (all memory reads and writes will be finished before the barrier is crossed).

extern "C" void _WriteBarrier();
extern "C" void _ReadWriteBarrier();

#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)

// MemoryBarrier is defined in WinNT.h.

/// Platform-specific atomic integer
typedef LONG AtomicInt;

AtomicInt AtomicLoad( const AtomicInt& var );

/// returns the resulting incremented value
AtomicInt AtomicIncrement( AtomicInt* var );

/// returns the resulting decremented value
AtomicInt AtomicDecrement( AtomicInt* var );

/// returns the resulting incremented value
AtomicInt AtomicAdd( AtomicInt& var, int add );

/// Sets a 32-bit 'dest' variable to the specified value as an atomic operation.
/// The function sets this variable to 'value', and returns its prior value.
///
AtomicInt AtomicExchange( AtomicInt* dest, int value );

///
/// Atomic Compare and Swap.
/// Performs an atomic compare-and-exchange operation on the specified values.
/// The function compares two specified 32-bit values and exchanges
/// with another 32-bit value based on the outcome of the comparison.
///
/// Arguments:
/// 'dest' - A pointer to the destination value.
/// 'comparand' - The value to compare to 'dest'.
/// 'exchange' - The exchange value.
/// 
/// Returns
/// the previous value of 'dest'
///
AtomicInt AtomicCompareExchange( AtomicInt* dest, AtomicInt comparand, AtomicInt exchange );

/// Atomic Compare and Swap.
/// Performs an atomic compare-and-exchange operation on the specified values.
/// The function compares two specified 32-bit values and exchanges
/// with another 32-bit value based on the outcome of the comparison.
///
/// Returns
/// 'true' if swap operation has occurred
///
bool AtomicCAS( AtomicInt* valuePtr, int oldValue, int newValue );

/// Description:
/// Atomically increments a value.
/// Arguments:
/// value - The pointer to the value.
/// incAmount - The amount to increment by.
///
void AtomicIncrement( AtomicInt* value, int incAmount );

/// Description:
/// Atomically decrements a value.
/// Arguments:
/// value - The pointer to the value.
/// decAmount - The amount to decrement by.
///
void AtomicDecrement( AtomicInt* value, int decAmount );

// A very simple spin lock for 'busy waiting'.
// The calling process obtains the lock if the old value was 0.
// It spins writing 1 to the variable until this occurs.
//
void SpinLock( AtomicInt* valuePtr,int oldValue,int newValue );
void WaitLock( AtomicInt* valuePtr, int value );

/// Scoped spin lock.
class AtomicLock {
	AtomicInt *	m_valuePtr;
public:
	inline AtomicLock( AtomicInt* theValue )
		: m_valuePtr( theValue )
	{
		SpinLock( m_valuePtr, 0, 1 );
	}
	inline ~AtomicLock()
	{
		SpinLock( m_valuePtr, 1, 0 );
	}
};

///
///	ThreadSafeFlag - A thread-safe flag variable.
///
class ThreadSafeFlag {
public:
			ThreadSafeFlag();

			/// set the flag
	void	Set();

			/// clear the flag
	void	clear();

			/// test if the flag is set
	bool	Test() const;

			/// test if flag is set, if yes, clear flag
	bool	TestAndClearIfSet();

private:
    AtomicInt	flag;
};

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
struct EventFlag
{
	HANDLE	m_handle;	//!< OS event handle

public:
	EventFlag();
	~EventFlag();

	enum Flags
	{
		CreateEvent_AutoReset = BIT(0),	//!< This event flag is reset automatically after signaling.
		CreateEvent_Signalled = BIT(1),	//!< This event flag is initialized in the signaled state.
		CreateEvent_DefaultFlags = 0
	};
	bool Initialize( UINT flags = CreateEvent_DefaultFlags );
	void Shutdown();

	/// signal the event
	bool Signal() const;

	/// reset the event (only if manual reset)
	bool Reset() const;

	/// wait for the event to become signaled
	bool Wait() const;

	/// wait for the event with timeout in millisecs
	bool WaitTimeOut( int ms ) const;

	/// check if event is signaled
    bool Peek() const;
};

class SpinWait;

///
///	System Condition variable
///
struct ConditionVariable
{
	CONDITION_VARIABLE	m_CV;

public:
	ConditionVariable();
	~ConditionVariable();

	void Initialize();
	void Shutdown();

	void Wait( SpinWait & CS );

	void NotifyOne();

	/// Wake all threads waiting on the specified condition variable.
	void Broadcast();
};

/*
-------------------------------------------------------------------------
	A spin wait object class similar to a critical section.
-------------------------------------------------------------------------
*/
struct SpinWait
{
	CRITICAL_SECTION	m_CS;

public:
	SpinWait();
	~SpinWait();

	bool Initialize( UINT spinCount = 1024 );
	void Shutdown();

	void Enter();
	bool TryEnter();
	void Leave();

public:
	class Lock: NonCopyable
	{
		SpinWait &	m_spinWait;
	public:
		Lock( SpinWait & spinWait );
		~Lock();
	};
};

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
class Semaphore
{
	HANDLE	m_handle;

public:
	Semaphore();

	bool Initialize( UINT initialCount, UINT maximumCount );
	void Shutdown();

	bool Wait();
	bool Signal( UINT count = 1, INT *oldCount = NULL );
};


/// The signature of a thread start function.
typedef UINT32 PASCAL F_ThreadEntryPoint( void* userPointer );

enum EThreadPriority
{
	ThreadPriority_Low,			//!< The thread has low priority.
	ThreadPriority_Normal,		//!< The thread has normal priority.
	ThreadPriority_High,		//!< The thread has high priority. If this priority is used, then the thread should block often so that lower-priority threads can run.
	ThreadPriority_Critical		//!< The thread has critical priority. If this priority is used, then the thread should be in the blocked state most of the time.
};

// Thread identifier.
typedef UINT32 THREAD_ID;
static const THREAD_ID	INVALID_THREAD_ID = 0xFFFFFFFF;

THREAD_ID ptGetMainThreadID();
THREAD_ID ptGetCurrentThreadID();

#define mxTHIS_IS_MAIN_THREAD	(ptGetCurrentThreadID() == ptGetMainThreadID())
#define mxASSERT_MAIN_THREAD	mxASSERT2(mxTHIS_IS_MAIN_THREAD, "must be called from the main thread")

/// The default stack size for a thread.
enum { mxDEFAULT_THREAD_STACK_SIZE = 64 * 1024 };	// 64 KiB

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
struct Thread
{
	HANDLE	m_handle;	// OS thread handle

public:
	Thread();
	~Thread();

	struct CInfo
	{
		F_ThreadEntryPoint *	entryPoint;	//!< The entry point for the new thread.
		void *					userPointer;//!< The pointer to user data for the new thread.
		int						userData;	//!< This can be used to pass the thread index.
		UINT					stackSize;	//!< The size of the stack for the new thread. 0 implies that the size of the stack is standard for the operating system. Defaults to mxDEFAULT_THREAD_STACK_SIZE.
		EThreadPriority			priority;	//!< The priority of created thread relative to the current thread. Defaults to ThreadPriority_Normal.
		const char *			debugName;	//!< Optional. The name for thread, which can be up to 27 characters excluding the NULL terminator. Defaults to NULL.
		bool					bCreateSuspended;
	public:
		CInfo();
	};

	bool Initialize( const CInfo& cInfo );
	void Shutdown();

	void Join( UINT *exitCode = NULL );	// wait for completion
	bool JoinTimeOut( UINT timeOut = INFINITE, UINT *exitCode = NULL );	// wait for completion

	bool Suspend();
	bool Resume();
	bool Kill();	// [Dangerous function]

	void SetAffinity( UINT affinity );
	void SetAffinityMask( UINT64 affinityMask );
	void SetProcessor( UINT processorIndex );
	void SetPriority( EThreadPriority newPriority );

	bool IsRunning() const;
};

extern void Win32_Dbg_SetThreadName( THREAD_ID dwThreadID, const char* threadName );

/// Switch to another hardware thread (on the same core)
void YieldHardwareThread();
/// Switch to another thread
void YieldSoftwareThread();
///Switch to the OS kernel.
void YieldToOS();
///Switch to the OS kernel and block for at least 1ms.
void YieldToOSAndSleep();

//-----------------------------------------------------------------
//		Miscellaneous
//-----------------------------------------------------------------

void ptDebugPrint( const char* message );
bool IsInDebugger();

/// Returns the absolute file path name of the application executable in UTF-8 encoding
/// (including the trailing slash, e.g. "D:/MyCoolGame/Bin/").
const TCHAR* mxGetLauchDirectory();

/// Returns the fully qualified filename for the currently running executable.
const TCHAR* mxGetExeFileName();

const TCHAR* mxGetCmdLine();

const TCHAR* mxGetComputerName();
const TCHAR* mxGetUserName();

/// Sleep for a specified amount of seconds.
void	mxSleep( float seconds );

///Causes the current thread to sleep for usec microseconds.
void	mxSleepMicroseconds( UINT64 usec );

///Causes the current thread to sleep for msecs milliseconds.
void	mxSleepMilliseconds( UINT32 msecs );

// Returns the percentage of current CPU load.
//UINT	mxGetCpuUsage();

///
///	Emits a simple sound through the speakers.
///	(This can be used for debugging or waking you up when the whole thing compiles...)
///
///	Param[In]: delayMs - duration of the sound, in milliseconds.
///	Param[In]: frequencyInHertz - frequency of the sound, in hertz
///
void	mxBeep( UINT delayMs, UINT frequencyInHertz = 600 );

//
// Mouse cursor control.
//
/// show/hide mouse cursor
void	mxSetMouseCursorVisible( bool bVisible );
/// coordinates are absolute, not relative!
void	mxSetMouseCursorPosition( UINT x, UINT y );

void	mxGetCurrentDeckstopResolution( UINT &ScreenWidth, UINT &ScreenHeight );

void	mxWaitForKeyPress();

// these are safeS to call from multiple threads:
void G_RequestExit();
/// this is used to break long-running tasks during shutdown
bool G_isExitRequested();

#if MX_DEBUG
/// Causes a crash to occur. Use only for testing & debugging!
void	mxCrash();
#endif // MX_DEBUG

struct AExceptionCallback
{
	virtual void AddStackEntry( const char* text ) = 0;
	virtual void OnException() = 0;
};

extern AExceptionCallback* g_exceptionCallback;

///	Error return code.
///  An enum of success/failure return codes (very much like Windows System Error Codes).
enum ERet : unsigned	// Rslt
{
#define mxDECLARE_ERROR_CODE( ENUM, NUMBER, STRING )	ENUM,
	#include <Base/System/ptErrorCodes.inl>
#undef mxDECLARE_ERROR_CODE
	ERR_MAX,				//!< Marker, don't use!
	ERR_FORCE32BIT = 0xFFFFFFFF,	//!< Makes sure this enum is 32-bit. Don't use!
};

/// converts an error code into a human-readable string
const char* EReturnCode_To_Chars( ERet returnCode );
