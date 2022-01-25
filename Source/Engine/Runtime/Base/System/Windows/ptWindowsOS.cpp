/*
=============================================================================
	File:	Windows_MVCpp.cpp
	Desc:	Windows-specific code for MVC++ compilers.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <intrin.h>		// __cpuid
#include <inttypes.h>
#include <sys/timeb.h>	// for ftime()
#include <direct.h>		// for chdir()
//#include <string.h>
#include <tchar.h>		// for _tcscat_s()
#include <process.h>	// for system()

// for timeGetTime()
#include <mmsystem.h>
#include <mmreg.h>
#if MX_AUTOLINK
	#pragma comment( lib, "winmm.lib" )
#endif


#include <pdh.h>
#if MX_AUTOLINK
	#pragma comment(lib, "pdh.lib")
#endif

#include <Base/Util/CPUTimer.h>


#if mxARCH_TYPE == mxARCH_64BIT
#define mxARCH_STRING "64-bit"
#elif mxARCH_TYPE == mxARCH_32BIT
#define mxARCH_STRING "32-bit"
#endif

#pragma message("Compiling "mxARCH_STRING" "mxBUILD_MODE_STRING" version with "mxCOMPILER_NAME"...")

// converts backslashes into forward slashes
static void ConvertBackSlashes( TCHAR* s )
{
	while( *s ) {
		if( *s == TEXT('\\') ) {
			*s = TEXT('/');
		}
		++s;
	}
}

template< UINT NUM_CHARS >
void NormalizeDirectory( TCHAR (&buffer)[NUM_CHARS] )
{
	ConvertBackSlashes( buffer );
}

// e.g.
// C:/myprojects/game/game.exe -> C:/myprojects/game/
// C:\basedir\dev\demo.exe -> C:\basedir\dev\
//
static void StripFileNameAndExtension( TCHAR* text, UINT length )
{
	if( !text || !length ) {
		return;
	}
	UINT i = length - 1;
	while( i > 0 )
	{
		if( (text[i-1] == TEXT('/')) || (text[i-1] == TEXT('\\')) )
		{
			break;
		}
		i--;
	}
	text[i]=0;
}

// e.g. "     Hello, world!" -> "Hello, world!"
//
static void RemoveLeadingWhitespaces( char* s )
{
	if( !s ) {
		return;
	}
	while( *s ) {
		if( *s == ' ' ) {
		}
		++s;
	}

	// find the first non-space character
	char * p = s;
	while( *p && *p == ' ' ) {
		p++;
	}
	if( p == s ) {
		return;
	}

	while( *p ) {
		*s = *p;
		p++;
		s++;
	}
	*s = '\0';
}

static void DetectCPU( PtSystemInfo &systemInfo );

struct WindowsSystemData
{
	//mxOPTIMIZE("these waste exe space a bit:")
	PtSystemInfo	systemInfo;

	TCHAR			OSVersion[128];
	TCHAR			OSDirectory[MAX_PATH];
	TCHAR			windowsDirectory[MAX_PATH];
	TCHAR			operatingSystemLanguage[128];

	TCHAR			launchDirectory[MAX_PATH];	// full base path including exe file name
	TCHAR			exeFileName[128];	// fully qualified executable file name including extension

	TCHAR			lauchDriveName[16];		// Optional drive letter, followed by a colon (:) (e.g. "C:").
	TCHAR			lauchBaseDirName[128];	// Optional directory path, including trailing slash (e.g. "\base\data\").
	TCHAR			lauchBaseExeName[128];	// Base filename (no extension) (e.g. "game").
	TCHAR			lauchExeExtension[128];	// Optional filename extension, including leading period (.) (e.g. ".exe").

	TCHAR			userName[32];
	TCHAR			computerName[32];

	HANDLE			hMainThread;
	DWORD			dwMainThreadId;
	HINSTANCE		hAppInstance;	// Handle to the application instance.

	HANDLE			hConsoleInput;
	HANDLE			hConsoleOutput;
	HANDLE			hConsoleError;

	HQUERY			hCpuUsageQuery;
	HCOUNTER		hCpuUsageCounter;

	CPUTimer		timer;	// starts when the app launches

	MEMORYSTATUSEX	memoryStatusOnExeLaunch;	// Initial memory status.
	MEMORYSTATUSEX	memoryStatusOnExeShutdown;

	static bool		bInitialized;

public:
	WindowsSystemData()
	{
		ZeroMemory( this, sizeof(*this) );
		bInitialized = false;
	}
	void OneTimeInit()
	{
		mxASSERT( !bInitialized );

		//NOTE: this should be done before any other variable is initialized!
		ZeroMemory( this, sizeof(*this) );

		timer.Initialize();
		timer.Reset();

		// Initialize platform info.
		systemInfo.platform = EPlatformId::Platform_PC_Windows;

		hMainThread = ::GetCurrentThread();
		dwMainThreadId = ::GetCurrentThreadId();
		Win32_Dbg_SetThreadName( dwMainThreadId, "Main Thread" );

		// Force the main thread to run on CPU 0 to avoid problems with QueryPerformanceCounter().
		::SetThreadAffinityMask( hMainThread, 1 );

		hConsoleInput = ::GetStdHandle( STD_INPUT_HANDLE );
		hConsoleOutput = ::GetStdHandle( STD_OUTPUT_HANDLE );
		hConsoleError = ::GetStdHandle( STD_ERROR_HANDLE );

		DetectCPU( systemInfo );

		// Detect the Windows version.
		{
			OSVERSIONINFOEX osvi;
			BOOL bOsVersionInfoEx;

			::ZeroMemory( &osvi, sizeof(OSVERSIONINFOEX) );
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

			bOsVersionInfoEx = ::GetVersionEx( (OSVERSIONINFO*) &osvi );
			if ( !bOsVersionInfoEx )
			{
				osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				if( !::GetVersionEx((OSVERSIONINFO*) &osvi) ) {
					goto L_End;
				}
			}

			switch( osvi.dwPlatformId )
			{
			case VER_PLATFORM_WIN32_NT :
				{
					if( osvi.dwMajorVersion <= 4 ) {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows NT ") );
					} else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows 2000 ") );
					} else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows XP ") );
					} else if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 ) {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows Vista ") );
					} else if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 ) {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows 7 ") );	// or Windows 2008 R2
					} else {
						_tcscat_s( OSVersion, TEXT("Microsoft Windows ") );
					}
					if( bOsVersionInfoEx )
					{
#ifdef VER_SUITE_ENTERPRISE
						if ( osvi.wProductType == VER_NT_WORKSTATION )
						{
#ifndef __BORLANDC__
							if( osvi.wSuiteMask & VER_SUITE_PERSONAL ) {
								_tcscat_s( OSVersion, TEXT("Personal ") );
							} else {
								_tcscat_s( OSVersion, TEXT("Professional ") );
							}
#endif
						}
						else if ( osvi.wProductType == VER_NT_SERVER )
						{
							if( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
								_tcscat_s( OSVersion, TEXT("DataCenter Server ") );
							}
							else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
								_tcscat_s( OSVersion, TEXT("Advanced Server ") );
							}
							else {
								_tcscat_s( OSVersion, TEXT("Server ") );
							}
						}
#endif
					}
					else
					{
						HKEY hKey;
						TCHAR szProductType[80];
						DWORD dwBufLen;
						::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
							0, KEY_QUERY_VALUE, &hKey );
						::RegQueryValueEx( hKey, TEXT("ProductType"), NULL, NULL,
							(LPBYTE) szProductType, &dwBufLen );
						::RegCloseKey( hKey );
						if( _tcsicmp( TEXT("WINNT"), szProductType) == 0 ) {
							_tcscat_s( OSVersion, TEXT("Professional ") );
						}
						if( _tcsicmp( TEXT("LANMANNT"), szProductType) == 0 ) {
							_tcscat_s( OSVersion, TEXT("Server ") );
						}
						if( _tcsicmp( TEXT("SERVERNT"), szProductType) == 0 ) {
							_tcscat_s( OSVersion, TEXT("Advanced Server ") );
						}
					}

					// Display version, service pack (if any), and build number.
					TCHAR	versionString[512] = { 0 };
					if( osvi.dwMajorVersion <= 4 )
					{
						_tprintf_s
							(
							versionString,
							TEXT("version %ld.%ld %s (Build %ld)"),
							osvi.dwMajorVersion,
							osvi.dwMinorVersion,
							osvi.szCSDVersion,
							osvi.dwBuildNumber & 0xFFFF
							);
					}
					else
					{
						_tprintf_s
							(
							versionString,
							TEXT("%s (Build %ld)"), osvi.szCSDVersion,
							osvi.dwBuildNumber & 0xFFFF
							);
					}
					_tcscat_s( OSVersion, versionString);
				}
				break;

			case VER_PLATFORM_WIN32_WINDOWS:
				if( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0 )
				{
					_tcscat_s( OSVersion, TEXT("Microsoft Windows 95 ") );
					if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' ) {
						_tcscat_s( OSVersion, TEXT("OSR2 ") );
					}
				}
				if( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10 )
				{
					_tcscat_s( OSVersion, TEXT("Microsoft Windows 98 ") );
					if ( osvi.szCSDVersion[1] == 'A' ) {
						_tcscat_s( OSVersion, TEXT("SE ") );
					}
				}
				if( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90 ) {
					_tcscat_s( OSVersion, TEXT("Microsoft Windows Me ") );
				}
				break;

			case VER_PLATFORM_WIN32s:
				_tcscat_s( OSVersion, TEXT("Microsoft Win32s ") );
				break;

			}//switch
		}
L_End:
		systemInfo.OSName = OSVersion;

		// determine OS directory
		{
			//GetSystemWindowsDirectory
			::GetWindowsDirectory( OSDirectory, mxCOUNT_OF(OSDirectory) );
			NormalizeDirectory( OSDirectory );
			systemInfo.OSDirectory = OSDirectory;
		}

		{
			::GetSystemDirectory( windowsDirectory, mxCOUNT_OF(windowsDirectory) );
			NormalizeDirectory( windowsDirectory );
			systemInfo.SysDirectory = windowsDirectory;
		}

		// determine system language
		{
			int ret = GetLocaleInfo(
				LOCALE_SYSTEM_DEFAULT,
				LOCALE_SNATIVELANGNAME | LOCALE_NOUSEROVERRIDE,
				operatingSystemLanguage, mxCOUNT_OF(operatingSystemLanguage)
			);
			mxASSERT(ret != 0);
			systemInfo.OSLanguage = operatingSystemLanguage;
		}

		hAppInstance = ::GetModuleHandle( NULL );

		//
		// Get application executable directory.
		//
/* 
	NOTE: these global vars point to the full path name of the executable file
		_pgmptr
		_get_wpgmptr
*/
		::GetModuleFileName( hAppInstance, launchDirectory, mxCOUNT_OF(launchDirectory) );
		ConvertBackSlashes( launchDirectory );

#if UNICODE
		::_wsplitpath
#else
		::_splitpath
#endif
		(
			launchDirectory,
			lauchDriveName,
			lauchBaseDirName,
			lauchBaseExeName,
			lauchExeExtension
		);

		// get directory name (without exe file name)
		StripFileNameAndExtension( launchDirectory, _tcslen(launchDirectory) );

		// get exe file name
		_tcscat_s( exeFileName, lauchBaseExeName );
		_tcscat_s( exeFileName, lauchExeExtension );

		// set as current directory
		//::SetCurrentDirectory( launchDirectory );

		{
			DWORD size = mxCOUNT_OF(userName);
			::GetUserName( userName, &size );
		}
		{
			DWORD size = mxCOUNT_OF(computerName);
			::GetComputerName( computerName, &size );
		}

		//
		// Record current memory status.
		//
		memoryStatusOnExeLaunch.dwLength = sizeof(memoryStatusOnExeLaunch);
		::GlobalMemoryStatusEx( & memoryStatusOnExeLaunch );

		// no abort/retry/fail errors
		//::SetErrorMode( SEM_FAILCRITICALERRORS );

		Win32MiniDump::Setup();

		bInitialized = true;
	}
	void Shutdown()
	{
		// We don't have any termination requirements for Win32 at this time.
		if( bInitialized )
		{
			// Record current memory status.
			memoryStatusOnExeShutdown.dwLength = sizeof(memoryStatusOnExeShutdown);
			::GlobalMemoryStatusEx( &memoryStatusOnExeShutdown );
			bInitialized = false;
		}
	}
};

//
//	Global variables.
//
static	WindowsSystemData	g_Win32System;
bool	WindowsSystemData::bInitialized = false;

#define ENSURE_WINSYS_IS_INITIALIZED	mxASSERT( WindowsSystemData::bInitialized );

//
//	mxPlatform_Init()
//
void mxPlatform_Init()
{
	// we're using VLD now
#if 0// MX_DEBUG_MEMORY

	// Send all reports to STDOUT.
	//::_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	//::_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	//::_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	//::_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	//::_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	//::_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

	// Enable run-time memory check for debug builds.
	// By setting the _CRTDBG_LEAK_CHECK_DF flag,
	// we produce a memory leak dump at exit in the visual c++ debug output window.

	int flag = ::_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); // Get current flag

	// Turn on leak-checking bits.
	flag |= _CRTDBG_LEAK_CHECK_DF;
	flag |= _CRTDBG_ALLOC_MEM_DF;

	// Set flag to the new value.
	// That will report any leaks to the output window when upon exiting the application.
	::_CrtSetDbgFlag( flag );
#endif

	g_Win32System.OneTimeInit();
}

//
//	mxPlatform_Shutdown()
//
void mxPlatform_Shutdown()
{
	g_Win32System.Shutdown();
}

HINSTANCE getAppInstanceHandle()
{
	return g_Win32System.hAppInstance;
}

HANDLE getCurrentThreadHandle()
{
	return g_Win32System.hMainThread;
}

//-----------------------------------------------------------------
//		General system info
//-----------------------------------------------------------------

EPlatformId mxCurrentPlatform()
{
	return EPlatformId::Platform_PC_Windows;
}

const char* mxGetPlatformName( EPlatformId platformId )
{
	switch ( platformId )
	{
	default: UNDONE;
	case EPlatformId::Platform_Unknown :
		return "Unknown platform";

	case EPlatformId::Platform_PC_Windows :
		return "Windows platform";
	}
}

const char* mxGetCpuTypeName( ECpuType CpuType )
{
	switch ( CpuType )
	{
	case ECpuType::CPU_x86_32 :
		return "x86-32";
	case ECpuType::CPU_x86_64 :
		return "x86-64";
	case ECpuType::CPU_PowerPC :
		return "PowerPC";
	default:
	case ECpuType::CPU_Unknown :
		return "Unknown CPU type";
	}
}

void mxGetSystemInfo( PtSystemInfo &sys_info_ )
{
	sys_info_ = g_Win32System.systemInfo;
}

void* Sys_Alloc( size_t _size, size_t alignment )
{
	if( _size < 16 ) {
		ptWARN("Small allocation!!!");
	}

	void* mem = ::_aligned_malloc( _size, alignment );

#if MX_DEVELOPER

#if mxARCH_TYPE == mxARCH_64BIT
	ptWARN(
		"[OS] ALLOCATED %I64u bytes (alignment %I64u) -> 0x%p",
		_size, alignment, mem
    );
#elif mxARCH_TYPE == mxARCH_32BIT
	// MVC++ 2008
	ptWARN(
		"[OS] ALLOCATED %"PRIuPTR" bytes (alignment %"PRIuPTR") -> 0x%X",
		_size, alignment, mem
    );
#else
#	error Unknown Arch!
#endif

#endif // MX_DEVELOPER

	return mem;
}

void Sys_Free( void * mem )
{

#if MX_DEVELOPER
	ptWARN(
		"[OS] FREEING 0x%X...",
		mem
		);
#endif // MX_DEVELOPER

	::_aligned_free( mem );
}

CpuCacheType Util_ConvertCpuCacheType( PROCESSOR_CACHE_TYPE e )
{
	switch( e )
	{
	case ::PROCESSOR_CACHE_TYPE::CacheUnified :		return CpuCache_Unified;
	case ::PROCESSOR_CACHE_TYPE::CacheInstruction :	return CpuCache_Instruction;
	case ::PROCESSOR_CACHE_TYPE::CacheData :		return CpuCache_Data;
	case ::PROCESSOR_CACHE_TYPE::CacheTrace :		return CpuCache_Trace;
	}
	mxUNREACHABLE;
	return CpuCache_Unified;
}

//
//	Returns true if the CPUID command is available on the processor.
//
bool Check_if_CPUID_is_available()
{

#if mxARCH_TYPE == mxARCH_64BIT

	INT32 cpuInfo[4] = { 0 };
	// EAX = 0 -> Get vendor ID.
	__cpuid( cpuInfo, 0 );
	mxUNDONE;
	return true;

#else

	int bitChanged = 0;

    // We have to check if we can toggle the flag register bit 21.
    // If we can't the processor does not support the CPUID command.

    __asm
	{
        push	EAX
        push	EBX
        pushfd
        pushfd
        pop		EAX
        mov		EBX, EAX
        xor		EAX, 0x00200000 
        push	EAX
        popfd
        pushfd
        pop		EAX
        popfd
        xor		EAX, EBX 
        mov		bitChanged, EAX
        pop		EBX
        pop		EAX
    }

    return ( bitChanged != 0 );

#endif

}

UINT64 EstimateProcessorFrequencyHz()
{
	const DWORD oldPriorityClass = ::GetPriorityClass( ::GetCurrentProcess() );
	const int oldThreadPriority = ::GetThreadPriority( ::GetCurrentThread() );

	LARGE_INTEGER t1,t2,tf;
	UINT64 c1,c2;

	::QueryPerformanceFrequency( &tf );
	::QueryPerformanceCounter( &t1 );
	c1 = __rdtsc();	// "__asm rdtsc" - reads time stamp into EDX:EAX

	for( volatile int i = 0; i < 1000000; i++ )
		;

	::QueryPerformanceCounter( &t2 );
	c2 = __rdtsc();

	::SetPriorityClass( ::GetCurrentProcess(), oldPriorityClass );
	::SetThreadPriority( ::GetCurrentThread(), oldThreadPriority );

	return (c2 - c1) * tf.QuadPart / (t2.QuadPart - t1.QuadPart);
}

static void DetectCPU( PtSystemInfo & sysInfo )
{
	mxZERO_OUT( sysInfo );

	// Used to specify buffer sizes for holding strings with processor names, miscellaneous info, etc.
	enum { MAX_MISC_INFO_CHARS = 512 };


	//
	// CPU detection.
	//

	// Portions of CPU detection code were lifted and modified from MSDN.
	{
		SYSTEM_INFO	win32SysInfo;
		::GetSystemInfo( & win32SysInfo );

		sysInfo.cpu.numLogicalCores = win32SysInfo.dwNumberOfProcessors;
		sysInfo.cpu.pageSize = win32SysInfo.dwPageSize;

		// Change the granularity of the windows media timer down to 1ms.
		::timeBeginPeriod( 1 );



		const bool hasCPUID = Check_if_CPUID_is_available();

		if( !hasCPUID ) {
			ptERROR("Engine requires a processor with CPUID support!\n");
		}

		sysInfo.cpu.has_CPUID = hasCPUID;



		enum CPUID_FUNCTION
		{
			CPUID_VENDOR_ID              = 0x00000000,
			CPUID_PROCESSOR_FEATURES     = 0x00000001,
			CPUID_NUM_CORES              = 0x00000004,
			CPUID_GET_HIGHEST_FUNCTION   = 0x80000000,
			CPUID_EXTENDED_FEATURES      = 0x80000001,

			CPUID_BRAND_STRING_0         = 0x80000002,
			CPUID_BRAND_STRING_1         = 0x80000003,
			CPUID_BRAND_STRING_2         = 0x80000004,

			CPUID_CACHE_INFO             = 0x80000006,
		};

		INT32 cpuInfo[4] = { 0 };

		// __cpuid( int cpuInfo[4], int infoType ) generates the CPUID instruction available on x86 and x64,
		// which queries the processor for information about the supported features and CPU type.

		// EAX = 0 -> Get vendor ID.
		__cpuid( cpuInfo, CPUID_VENDOR_ID );

		/*
		----------------------------------------------------------------------------------
		When the InfoType argument is 0, the following table describes the output.

		InfoType[0]	- Maximum meaningful value for the InfoType parameter.
		InfoType[1] - Identification String (part 1)
		InfoType[2]	- Identification String (part 3)
		InfoType[3]	- Identification String (part 2)
		----------------------------------------------------------------------------------
		*/
		*((INT32*)sysInfo.cpu.vendor + 0) = cpuInfo[1];
		*((INT32*)sysInfo.cpu.vendor + 1) = cpuInfo[3];
		*((INT32*)sysInfo.cpu.vendor + 2) = cpuInfo[2];

		/*
		----------------------------------------------------------------------------------
		When the InfoType argument is 1, the following table describes the output.

		InfoType[0, bits 0-3]	- Stepping ID.
		InfoType[0, bits 4-7]	- Model.
		InfoType[0, bits 8-11]	- Family.
		InfoType[0, bits 12-13]	- Processor Type.
		InfoType[0, bits 14-15]	- Reserved.
		InfoType[0, bits 16-19]	- Extended model.
		InfoType[0, bits 20-27]	- Extended family.
		InfoType[0, bits 28-31]	- Reserved.
		----------------------------------------------------------------------------------
		*/

		struct SCpuFeatureMask0
		{
			BITFIELD stepping		: 4;
			BITFIELD model			: 4;
			BITFIELD family			: 4;

			BITFIELD cpuType		: 2;
			BITFIELD reserved0		: 2;

			BITFIELD extendedModel	: 4;
			BITFIELD extendedFamily	: 8;

			BITFIELD reserved1		: 4;
		};
		
		/*
		----------------------------------------------------------------------------------
		InfoType[1, bits 0-7]	- Brand Index.
		InfoType[1, bits 8-15]	- CLFLUSH cache line size / 8.
		InfoType[1, bits 16-23]	- Reserved.
		InfoType[1, bits 24-31]	- APIC Physical ID.
		----------------------------------------------------------------------------------
		*/
		struct SCpuFeatureMask1
		{
			BITFIELD brandIndex		: 8;
			BITFIELD clflush		: 8;
			BITFIELD reserved		: 8;
			BITFIELD apicId			: 8;
		};

		/*
		----------------------------------------------------------------------------------
		InfoType[2, bits 0]	- SSE3 New Instructions.
		InfoType[2, bits 1-2]	- Reserved.
		InfoType[2, bits 3]	- MONITOR/MWAIT.
		InfoType[2, bits 4]	- CPL Qualified Debug Store.
		InfoType[2, bits 5-7]	- Reserved.
		InfoType[2, bits 8]	- Thermal Monitor 2.
		InfoType[2, bits 9]	- Reserved.
		InfoType[2, bits 10]	- L1 Context ID.
		InfoType[2, bits 11-31]	- Reserved.
		----------------------------------------------------------------------------------
		*/
		struct SCpuFeatureMask2
		{
			BITFIELD sse3			: 1;
			BITFIELD reserved0		: 2;
			BITFIELD mwait			: 1;
			BITFIELD cpl			: 1;
			BITFIELD reserved1		: 3;
			BITFIELD thermalMonitor2: 1;
			BITFIELD reserved2		: 1;
			BITFIELD L1				: 1;
			BITFIELD reserved3		: 21;
		};
			
		/*
		----------------------------------------------------------------------------------
		InfoType[3, bits 0-31]	- Feature Information (see below).

		The following table shows the meaning of the Feature Information value,
		the value of EDX which is written to cpuInfo[3],
		when the InfoType argument is 1.

		Bit 0 --- FPU --- x87 FPU on Chip

		Bit 1 --- VME --- Virtual-8086 Mode Enhancement

		Bit 2 --- DE --- Debugging Extensions

		Bit 3 --- PSE --- Page Size Extensions

		Bit 4 --- TSC --- Time Stamp Counter

		Bit 5 --- MSR --- RDMSR and WRMSR Support

		Bit 6 --- PAE --- Physical Address Extensions

		Bit 7 --- MCE --- Machine Check Exception

		Bit 8 --- CX8 --- CMPXCHG8B Inst.

		Bit 9 --- APIC --- APIC on Chip

		Bit 10 --- n/a --- Reserved

		Bit 11 --- SEP --- SYSENTER and SYSEXIT

		Bit 12 --- MTRR --- Memory Type Range Registers

		Bit 13 --- PGE --- PTE Global Bit

		Bit 14 --- MCA --- Machine Check Architecture

		Bit 15 --- CMOV --- Conditional Move/Compare Instruction

		Bit 16 --- PAT --- Page Attribute Table

		Bit 17 --- PSE --- Page Size Extension

		Bit 18 --- PSN --- Processor Serial Number

		Bit 19 --- CLFSH --- CFLUSH Instruction

		Bit 20 --- n/a --- Reserved

		Bit 21 --- DS --- Debug Store

		Bit 22 --- ACPI	 --- Thermal Monitor and Clock Ctrl

		Bit 23 --- MMX --- MMX Technology

		Bit 24 --- FXSR --- FXSAVE/FXRSTOR

		Bit 25 --- SSE --- SSE Extensions

		Bit 26 --- SSE2 --- SSE2 Extensions

		Bit 27 --- SS --- Self Snoop

		Bit 28 --- HTT --- Hyper-threading technology

		Bit 29 --- TM --- Thermal Monitor

		Bit 30 --- n/a --- Reserved

		Bit 31 --- PBE --- Pend. Brk. En.
		----------------------------------------------------------------------------------
		*/


		__cpuid( cpuInfo, CPUID_PROCESSOR_FEATURES );

	//			SCpuFeatureMask0 	featuresPart0 = *(SCpuFeatureMask0*) &cpuInfo[0];
	//			SCpuFeatureMask1 	featuresPart1 = *(SCpuFeatureMask1*) &cpuInfo[1];
	//			SCpuFeatureMask2 	featuresPart2 = *(SCpuFeatureMask2*) &cpuInfo[2];
		INT32 				featuresPart3 = cpuInfo[3];


	#pragma warning( push, 3 )
	#pragma warning( disable: 4189 ) // local variable is initialized but not referenced

	mxCOMPILER_PROBLEM("it seems that the above pragma warning doesn't work with MVC++ 9.0 so i commented out the stuff below:")

		int nSteppingID = cpuInfo[0] & 0xf;
		int nModel = (cpuInfo[0] >> 4) & 0xf;
		int nFamily = (cpuInfo[0] >> 8) & 0xf;
	//			int nProcessorType = (cpuInfo[0] >> 12) & 0x3;
		int nExtendedmodel = (cpuInfo[0] >> 16) & 0xf;
		int nExtendedfamily = (cpuInfo[0] >> 20) & 0xff;
		int nBrandIndex = cpuInfo[1] & 0xff;
	//			int nCLFLUSHcachelinesize = ((cpuInfo[1] >> 8) & 0xff) * 8;
	//			int nLogicalProcessors = ((cpuInfo[1] >> 16) & 0xff);
	//			int nAPICPhysicalID = (cpuInfo[1] >> 24) & 0xff;
		bool bSSE3Instructions = (cpuInfo[2] & 0x1) || false;
	//			bool bMONITOR_MWAIT = (cpuInfo[2] & 0x8) || false;
	//			bool bCPLQualifiedDebugStore = (cpuInfo[2] & 0x10) || false;
	//			bool bVirtualMachineExtensions = (cpuInfo[2] & 0x20) || false;
	//			bool bEnhancedIntelSpeedStepTechnology = (cpuInfo[2] & 0x80) || false;
	//			bool bThermalMonitor2 = (cpuInfo[2] & 0x100) || false;
	//			bool bSupplementalSSE3 = (cpuInfo[2] & 0x200) || false;
	//			bool bL1ContextID = (cpuInfo[2] & 0x300) || false;
	//			bool bCMPXCHG16B= (cpuInfo[2] & 0x2000) || false;
	//			bool bxTPRUpdateControl = (cpuInfo[2] & 0x4000) || false;
	//			bool bPerfDebugCapabilityMSR = (cpuInfo[2] & 0x8000) || false;
		bool bSSE41Extensions = (cpuInfo[2] & 0x80000) || false;
		bool bSSE42Extensions = (cpuInfo[2] & 0x100000) || false;
	//			bool bPOPCNT= (cpuInfo[2] & 0x800000) || false;
	//			int nFeatureInfo = cpuInfo[3];
	//			bool bMultithreading = (nFeatureInfo & (1 << 28)) || false;

		bool bFMA = (cpuInfo[2] & (1UL << 12)) || false;// 256-bit FMA (Intel)
		bool bMOVBE = (cpuInfo[2] & (1UL << 22)) || false;	// MOVBE (Intel)
		bool bPOPCNT = (cpuInfo[2] & (1UL << 23)) || false;	// POPCNT
		bool bAES = (cpuInfo[2] & (1UL << 25)) || false;	// AES support (Intel)
		bool bAVX = (cpuInfo[2] & (1UL << 28)) || false;	// 256-bit AVX (Intel)


		__cpuid( cpuInfo, CPUID_EXTENDED_FEATURES );

	//			bool bLAHF_SAHFAvailable = (cpuInfo[2] & 0x1) || false;
	//			bool bCmpLegacy = (cpuInfo[2] & 0x2) || false;
	//			bool bSVM = (cpuInfo[2] & 0x4) || false;
	//			bool bExtApicSpace = (cpuInfo[2] & 0x8) || false;
	//			bool bAltMovCr8 = (cpuInfo[2] & 0x10) || false;
	//			bool bLZCNT = (cpuInfo[2] & 0x20) || false;
		bool bSSE4A = (cpuInfo[2] & 0x40) || false;
	//			bool bMisalignedSSE = (cpuInfo[2] & 0x80) || false;
	//			bool bPREFETCH = (cpuInfo[2] & 0x100) || false;
	//			bool bSKINITandDEV = (cpuInfo[2] & 0x1000) || false;
	//			bool bSYSCALL_SYSRETAvailable = (cpuInfo[3] & 0x800) || false;
	//			bool bExecuteDisableBitAvailable = (cpuInfo[3] & 0x10000) || false;
	//			bool bMMXExtensions = (cpuInfo[3] & 0x40000) || false;
		bool bMMXExtensions_AMD = (cpuInfo[3] & (1<<22)) || false;
	//			bool bFFXSR = (cpuInfo[3] & 0x200000) || false;
	//			bool b1GBSupport = (cpuInfo[3] & 0x400000) || false;
	//			bool bRDTSCP = (cpuInfo[3] & 0x8000000) || false;
		bool b64Available = (cpuInfo[3] & 0x20000000) || false;
		bool b3DNowExt = (cpuInfo[3] & 0x40000000) || false;
		bool b3DNow = (cpuInfo[3] & 0x80000000) || false;



		// Interpret CPU brand string and cache information.

		__cpuid( cpuInfo, CPUID_BRAND_STRING_0 );
		memcpy( sysInfo.cpu.brandName, cpuInfo, sizeof(cpuInfo) );

		__cpuid( cpuInfo, CPUID_BRAND_STRING_1 );
		memcpy( sysInfo.cpu.brandName + 16, cpuInfo, sizeof(cpuInfo) );

		__cpuid( cpuInfo, CPUID_BRAND_STRING_2 );
		memcpy( sysInfo.cpu.brandName + 32, cpuInfo, sizeof(cpuInfo) );

		__cpuid( cpuInfo, CPUID_CACHE_INFO );
		sysInfo.cpu.cacheLineSize = cpuInfo[2] & 0xff;
	//	sysInfo.cpu.cacheAssoc = (cpuInfo[2] >> 12) & 0xf;	// L2 associativity
	//	sysInfo.cpu.cacheSize = (cpuInfo[2] >> 16) & 0xffff;	// L2 cache size in 1K units

		RemoveLeadingWhitespaces( sysInfo.cpu.brandName );

		sysInfo.cpu.brandId = nBrandIndex;
		sysInfo.cpu.family = nFamily;
		sysInfo.cpu.model = nModel;
		sysInfo.cpu.stepping = nSteppingID;

		sysInfo.cpu.extendedFamily = nExtendedfamily;
		sysInfo.cpu.extendedModel = nExtendedmodel;

		sysInfo.cpu.type = b64Available ? ECpuType::CPU_x86_64 : ECpuType::CPU_x86_32;


		sysInfo.cpu.has_RDTSC		= ((featuresPart3 & (1<<4)) != 0);
		sysInfo.cpu.has_FPU			= ((featuresPart3 & (1<<0)) != 0);
		sysInfo.cpu.has_MMX			= (::IsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE ) != FALSE);
		sysInfo.cpu.has_MMX_AMD		= bMMXExtensions_AMD;
		sysInfo.cpu.has_3DNow		= b3DNow;
		sysInfo.cpu.has_3DNow_Ext	= b3DNowExt;
		sysInfo.cpu.has_SSE_1		= ((featuresPart3 & (1<<25)) != 0);
		sysInfo.cpu.has_SSE_2		= ((featuresPart3 & (1<<26)) != 0);
		sysInfo.cpu.has_SSE_3		= bSSE3Instructions;
		sysInfo.cpu.has_SSE_4_1		= bSSE41Extensions;
		sysInfo.cpu.has_SSE_4_2		= bSSE42Extensions;
		sysInfo.cpu.has_SSE_4a		= bSSE4A;
		sysInfo.cpu.has_HTT			= ((featuresPart3 & (1<<28)) != 0);
		sysInfo.cpu.has_CMOV		= ((featuresPart3 & (1<<15)) != 0);
		sysInfo.cpu.has_EM64T		= b64Available;
		sysInfo.cpu.has_MOVBE		= bMOVBE;
		sysInfo.cpu.has_FMA			= bFMA;
		sysInfo.cpu.has_POPCNT		= bPOPCNT;
		sysInfo.cpu.has_AES			= bAES;
		sysInfo.cpu.has_AVX			= bAVX;

		// With the AMD chipset, all multi-core AMD CPUs set bit 28
		// of the feature information bits to indicate
		// that the chip has more than one core.
		// This is the case even though AMD does not support hyper-threading.

		if( !strcmp( sysInfo.cpu.vendor, "AuthenticAMD" ) ) {
			sysInfo.cpu.has_HTT	= false;
		}


		mxPLATFORM_PROBLEM("__cpuidex undefined");
	#if 0
		int nCores = 0;
		int nCacheType = 0;
		int nCacheLevel = 0;
		int nMaxThread = 0;
		int nSysLineSize = 0;
		int nPhysicalLinePartitions = 0;
		int nWaysAssociativity = 0;
		int nNumberSets = 0;

		bool    bSelfInit = false;
		bool    bFullyAssociative = false;

		for( UINT i = 0; ; i++ )
		{
			__cpuidex( cpuInfo, 0x4, i );

			if( !(cpuInfo[0] & 0xf0) ) {
				break;
			}

			if( i == 0 )
			{
				nCores = cpuInfo[0] >> 26;
				printf_s("\n\nNumber of Cores = %d\n", nCores + 1);
			}

			nCacheType = (cpuInfo[0] & 0x1f);
			nCacheLevel = (cpuInfo[0] & 0xe0) >> 5;
			bSelfInit = ((cpuInfo[0] & 0x100) >> 8) || false;
			bFullyAssociative = ((cpuInfo[0] & 0x200) >> 9) || false;
			nMaxThread = (cpuInfo[0] & 0x03ffc000) >> 14;
			nSysLineSize = (cpuInfo[1] & 0x0fff);
			nPhysicalLinePartitions = (cpuInfo[1] & 0x03ff000) >> 12;
			nWaysAssociativity = (cpuInfo[1]) >> 22;
			nNumberSets = cpuInfo[2];

			printf_s("\n");

			printf_s("ECX Index %d\n", i);
			switch (nCacheType)
			{
			case 0:
				printf_s("   Type: Null\n");
				break;
			case 1:
				printf_s("   Type: Data Cache\n");
				break;
			case 2:
				printf_s("   Type: Instruction Cache\n");
				break;
			case 3:
				printf_s("   Type: Unified Cache\n");
				break;
			default:
				printf_s("   Type: Unknown\n");
			}

			printf_s("   Level = %d\n", nCacheLevel + 1); 
			if (bSelfInit)
			{
				printf_s("   Self Initializing\n");
			}
			else
			{
				printf_s("   Not Self Initializing\n");
			}
			if (bFullyAssociative)
			{
				printf_s("   Is Fully Associative\n");
			}
			else
			{
				printf_s("   Is Not Fully Associative\n");
			}
			printf_s("   Max Threads = %d\n", 
				nMaxThread+1);
			printf_s("   System Line Size = %d\n", 
				nSysLineSize+1);
			printf_s("   Physical Line Partitions = %d\n", 
				nPhysicalLinePartitions+1);
			printf_s("   Ways of Associativity = %d\n", 
				nWaysAssociativity+1);
			printf_s("   Number of Sets = %d\n", 
				nNumberSets+1);
		}
	#endif

	#pragma warning( pop )


	//	RemoveLeadingWhitespaces( sysInfo.cpu.brandName );


		sysInfo.cpu.estimatedFreqHz = EstimateProcessorFrequencyHz();
		//extern UINT64 CalculateClockSpeed( ULONG cpu );
		//sysInfo.cpu.estimatedFreqHz = CalculateClockSpeed( 0 );


		::timeEndPeriod( 1 );

		//***********************************

	#if MX_COMPILE_WITH_XNA_MATH
		if( ! XMVerifyCPUSupport() ) {
			ptERROR("XNA math is not supported on this system!\n");
		}
	#endif //MX_COMPILE_WITH_XNA_MATH

	#if MX_USE_SSE
		if( ! sysInfo.cpu.has_SSE_1 || ! sysInfo.cpu.has_SSE_2 ) {
			ptERROR("Engine requires a processor with SSE/SSE2 support!\n"
				"This program was not built to run on the processor in your system!\n");
		}
	#endif

	}

	// Retrieve information about logical processors and related hardware.

	mxPLATFORM_PROBLEM("GetLogicalProcessorInformation() is not supported on WinXP without SP3");

	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION	buffer = NULL;

		DWORD returnLength = 0;
		
		HANDLE heap = ::GetProcessHeap();

		BOOL done = FALSE;
		while( !done )
		{
			DWORD dwResult = ::GetLogicalProcessorInformation(
				buffer,
				&returnLength
			);
			if( FALSE == dwResult ) 
			{
				if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
				{
					if( buffer ) {
						::HeapFree( heap, 0, buffer );
					}
					buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)::HeapAlloc(
						heap,
						0,
						returnLength
					);
					if( !buffer )
					{
						ptERROR( "Allocation failure\n" );
					}
				}
				else
				{
					ptERROR( "Error code: %d\n", GetLastError() );
				}
			}
			else done = TRUE;
		}//while


		enum { MAX_CPU_CACHES = 32 };
		CpuCacheInfo	cacheInfos[MAX_CPU_CACHES] = {0};
		UINT	numCaches = 0;


		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION	ptr = buffer;
		UINT numaNodeCount = 0, processorCoreCount = 0;
		UINT processorL1CacheCount = 0, processorL2CacheCount = 0, processorL3CacheCount = 0;
		UINT processorPackageCount = 0;

		DWORD byteOffset = 0;
		while( byteOffset < returnLength ) 
		{
			switch( ptr->Relationship ) 
			{
			case RelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				numaNodeCount++;
				break;

			case RelationProcessorCore:
				processorCoreCount++;
				break;

			case RelationCache:
				{
					// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
					CACHE_DESCRIPTOR * pCacheDesc = &ptr->Cache;
					if (pCacheDesc->Level == 1)
					{
						processorL1CacheCount++;
					}
					else if (pCacheDesc->Level == 2)
					{
						processorL2CacheCount++;
					}
					else if (pCacheDesc->Level == 3)
					{
						processorL3CacheCount++;
					}

					mxSTATIC_ASSERT(TIsPowerOfTwo<MAX_CPU_CACHES>::value);
					UINT index = (numCaches++) & (MAX_CPU_CACHES-1);
					CpuCacheInfo & newCacheInfo = cacheInfos[index];

					newCacheInfo.level = pCacheDesc->Level;
					newCacheInfo.associativity = pCacheDesc->Associativity;
					newCacheInfo.lineSize = pCacheDesc->LineSize;
					newCacheInfo.size = pCacheDesc->Size;
					newCacheInfo.type = Util_ConvertCpuCacheType( pCacheDesc->Type );
				}
				break;

			case RelationProcessorPackage:
				// Logical processors share a physical package.
				processorPackageCount++;
				break;

			default:
				break;
			}

			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr++;
		}

		::HeapFree( heap, 0, buffer );


		sysInfo.cpu.numPhysicalCores = processorCoreCount;

		{
			UINT cacheSizes[ 4 /*level*/ ][ NumCpuCacheTypes ] = {0};
			UINT cacheAssoc[ 4 /*level*/ ] = {0};


			for( UINT iCpuCore = 0; iCpuCore < numCaches; iCpuCore++ )
			{
				const CpuCacheInfo & cacheInfo = cacheInfos[ iCpuCore ];

				UINT level = cacheInfo.level & 3;
				cacheSizes[ level ][ cacheInfo.type ] += cacheInfo.size;
				cacheAssoc[ level ] = cacheInfo.associativity;
			}

			sysInfo.cpu.L1_ICache_Size = cacheSizes[1][CpuCache_Instruction] / 1024;
			sysInfo.cpu.L1_DCache_Size = cacheSizes[1][CpuCache_Data] / 1024;
			sysInfo.cpu.L2_Cache_Size = (cacheSizes[2][CpuCache_Instruction] + cacheSizes[2][CpuCache_Data] + cacheSizes[2][CpuCache_Unified]) / 1024;
			sysInfo.cpu.L3_Cache_Size = (cacheSizes[3][CpuCache_Instruction] + cacheSizes[3][CpuCache_Data] + cacheSizes[3][CpuCache_Unified]) / 1024;

			sysInfo.cpu.L1_Assoc = cacheAssoc[1];
			sysInfo.cpu.L2_Assoc = cacheAssoc[2];
			sysInfo.cpu.L3_Assoc = cacheAssoc[3];
		}
	}

	// can also use IsProcessorFeaturePresent()

	// Read CPU info from OS registry.
#if 1
	{
		HKEY key;

		if ( FAILED( ::RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
			0, KEY_READ, &key ) ) )
		{
			ptWARN( "Failed to get processor info" );
			return;
		}

		// CPU name.

		ANSICHAR	cpuName[ 512 ];

		DWORD dwSize;
		DWORD dwType;
		if( SUCCEEDED( ::RegQueryValueEx( key, TEXT("ProcessorNameString"), NULL, &dwType, NULL, &dwSize ) ) )
		{
			if( dwSize < MAX_MISC_INFO_CHARS )
			{
				if( FAILED( ::RegQueryValueEx(
					key,
					TEXT("ProcessorNameString"),
					NULL,
					&dwType,
					(LPBYTE) cpuName,
					&dwSize ) ) )
				{
					ptWARN( "Failed to get processor name" );
				}
			}
			else {
				ptWARN( "Processor name is too long (CPU name length: %d chars)", dwSize );
			}
		}
		else
		{
			ptWARN( "Failed to get processor name" );
		}

		// Clock frequency.

		DWORD dwClockFreq = 0;
		DWORD dwSizeOf = sizeof( dwClockFreq );

		if ( FAILED( ::RegQueryValueEx( key, TEXT("~MHz"), nil, nil,
			(LPBYTE) &dwClockFreq, (LPDWORD) &dwSizeOf ) ) )
		{
			ptERROR( "Failed to get processor clock frequency" );
			return;
		}

		::RegCloseKey( key );

		sysInfo.cpu.readFreqMHz = (UINT) dwClockFreq;
	}

#endif
}


bool OperatingOnVista()
{
	OSVERSIONINFO osver;
	memset(&osver, 0, sizeof(OSVERSIONINFO));
	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

	if (!GetVersionEx( &osver ))
		return FALSE;

	if (   osver.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& osver.dwMajorVersion >= 6  )
		return TRUE;

	return FALSE;
}
//----------------------------------------------------------------------------
bool OperatingOnXP()
{
	OSVERSIONINFO osver;
	memset(&osver, 0, sizeof(OSVERSIONINFO));
	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

	if (!GetVersionEx( &osver ))
		return FALSE;

	if (   osver.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& osver.dwMajorVersion >= 5  )
		return TRUE;

	return FALSE;
}
//----------------------------------------------------------------------------

/*
64-bit operating systems of the Windows family can execute 32-bit programs
with the help of the WoW64 (Windows in Windows 64) subsystem
that emulates the 32-bit environment due to an additional layer
between a 32-bit application and 64-bit Windows API.
A 32-bit program can find out if it is launched in WoW64
with the help of the IsWow64Process function.
The program can get additional information about the processor
through the GetNativeSystemInfo function.
Keep in mind that the IsWow64Process function is included only in 64-bit Windows versions.
You can use the GetProcAddress and GetModuleHandle functions to know
if the IsWow64Process function is present in the system and to access it.
*/
bool IsWow64()
{
  BOOL bIsWow64 = FALSE;

  typedef BOOL (APIENTRY *LPFN_ISWOW64PROCESS)
    (HANDLE, PBOOL);

  LPFN_ISWOW64PROCESS fnIsWow64Process;

  HMODULE module = GetModuleHandle(_T("kernel32"));
  const char funcName[] = "IsWow64Process";
  fnIsWow64Process = (LPFN_ISWOW64PROCESS)
    GetProcAddress(module, funcName);

  if(NULL != fnIsWow64Process)
  {
    if (!fnIsWow64Process(GetCurrentProcess(),
                          &bIsWow64))
      throw std::exception("Unknown error");
  }
  return bIsWow64 != FALSE;
}

//-----------------------------------------------------------------
//		DLL-related functions.
//-----------------------------------------------------------------

void * mxDLLOpen( const char* moduleName )
{
	mxASSERT_PTR(moduleName);
	return ::LoadLibraryA( moduleName );
}

void   mxDLLClose( void* module )
{
	mxASSERT_PTR(module);
	::FreeLibrary( (HMODULE)module );
}

void * mxDLLGetSymbol( void* module, const char* symbolName )
{
	mxASSERT_PTR(module);
	mxASSERT_PTR(symbolName);
	return ::GetProcAddress( (HMODULE)module, symbolName );
}

//-----------------------------------------------------------------
//		System time
//-----------------------------------------------------------------

UINT32 mxGetTimeInMilliseconds()
{
	return ::timeGetTime();
}

UINT64 mxGetTimeInMicroseconds()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.timer.GetTimeMicroseconds();
}

unsigned long int tbGetTimeInMilliseconds()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.timer.GetTimeMilliseconds();
}

FLOAT64 mxGetTimeInSeconds()
{
	const UINT64 microseconds = mxGetTimeInMicroseconds();
	return 1e-06 * microseconds;
}

UINT32 mxGetClockTicks()
{
	return (UINT)::GetTickCount();
}

UINT32 mxGetClockTicksPerSecond()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.systemInfo.cpu.estimatedFreqHz;
}

float mxGetClockTicks_F()
{
	return (float) (INT)mxGetClockTicks();
}

float mxGetClockTicksPerSecond_F()
{
	return (float) (INT)mxGetClockTicksPerSecond();
}

//--------------------------------------------------------------------------

static CalendarTime FromWin32SystemTime( const SYSTEMTIME& t )
{
    CalendarTime calTime;
    calTime.year    = t.wYear;
	calTime.month   = (CalendarTime::Month) t.wMonth;
    calTime.weekday = (CalendarTime::Weekday) t.wDayOfWeek;
    calTime.day     = t.wDay;
    calTime.hour    = t.wHour;
    calTime.minute  = t.wMinute;
    calTime.second  = t.wSecond;
    calTime.milliSecond = t.wMilliseconds;
    return calTime;
}

//--------------------------------------------------------------------------
/*
static SYSTEMTIME ToWin32SystemTime( const CalendarTime& calTime )
{
    SYSTEMTIME t;
    t.wYear         = (WORD) calTime.year;
    t.wMonth        = (WORD) calTime.month;
    t.wDayOfWeek    = (WORD) calTime.weekday;
    t.wDay          = (WORD) calTime.day;
    t.wHour         = (WORD) calTime.hour;
    t.wMinute       = (WORD) calTime.minute;
    t.wSecond       = (WORD) calTime.second;
    t.wMilliseconds = (WORD) calTime.milliSecond;
    return t;
}
*/
//--------------------------------------------------------------------------

// OF_ains the current system time. This does not depend on the current time zone.
CalendarTime CalendarTime::GetCurrentSystemTime()
{
    SYSTEMTIME t;
    ::GetSystemTime( &t );
    return FromWin32SystemTime( t );
}

// OF_ains the current local time (with time-zone adjustment).
CalendarTime CalendarTime::GetCurrentLocalTime()
{
    SYSTEMTIME t;
    ::GetLocalTime( &t );
    return FromWin32SystemTime( t );
}

const char * CalendarTime::MonthToStr( Month e )
{
	switch( e )
	{
		case January:   return "January";
		case February:  return "February";
		case March:     return "March";
		case April:     return "April";
		case May:       return "May";
		case June:      return "June";
		case July:      return "July";
		case August:    return "August";
		case September: return "September";
		case October:   return "October";
		case November:  return "November";
		case December:  return "December";
		mxNO_SWITCH_DEFAULT;
	}
	return mxSTRING_Unknown;
}

const char * CalendarTime::WeekdayToStr( Weekday e )
{
	switch( e )
	{
		case Sunday:    return "Sunday";
		case Monday:    return "Monday";
		case Tuesday:   return "Tuesday";
		case Wednesday: return "Wednesday";
		case Thursday:  return "Thursday";
		case Friday:    return "Friday";
		case Saturday:  return "Saturday";
		mxNO_SWITCH_DEFAULT;
	}
	return mxSTRING_Unknown;
}

//--------------------------------------------------------------------------

void mxGetTime( UINT & year, UINT & month, UINT & dayOfWeek,
			UINT & day, UINT & hour, UINT & minute, UINT & second, UINT & milliseconds )
{
	SYSTEMTIME	sysTime;

	::GetLocalTime( & sysTime );

	year		= sysTime.wYear;
	month		= sysTime.wMonth;
	dayOfWeek	= sysTime.wDayOfWeek;
	day			= sysTime.wDay;
	hour		= sysTime.wHour;
	minute		= sysTime.wMinute;
	second		= sysTime.wSecond;
	milliseconds = sysTime.wMilliseconds;
}

//-----------------------------------------------------------------
//		CPU information
//-----------------------------------------------------------------

UINT mxGetNumCpuCores()
{
	SYSTEM_INFO	win32SysInfo;
	::GetSystemInfo( & win32SysInfo );

	return (UINT) win32SysInfo.dwNumberOfProcessors;
}

//-----------------------------------------------------------------
//		Memory stats
//-----------------------------------------------------------------

size_t mxGetSystemRam()
{
#if 0
	ULONGLONG	TotalMemoryInKilobytes;
	BOOL bOk = ::GetPhysicallyInstalledSystemMemory(
		&TotalMemoryInKilobytes
	);
	mxASSERT(bOk != FALSE);
	return (size_t) (TotalMemoryInKilobytes / 1024);

#elif 1

	MEMORYSTATUSEX  memStatus;
	memStatus.dwLength = sizeof( memStatus );

	::GlobalMemoryStatusEx( &memStatus );

	return (size_t) (memStatus.ullTotalPhys / (1024*1024));

#else
	MEMORYSTATUS  memStatus;
	memStatus.dwLength = sizeof( memStatus );

	::GlobalMemoryStatus( &memStatus );

	return ((size_t) memStatus.dwTotalPhys) / (1024*1024);
#endif
}

void FromWinMemoryStatus( const MEMORYSTATUS& memStatus, mxMemoryStatus & OutStats )
{
	OutStats.memoryLoad		= memStatus.dwMemoryLoad;
	OutStats.totalPhysical	= memStatus.dwTotalPhys / (1024*1024);
	OutStats.availPhysical	= memStatus.dwAvailPhys / (1024*1024);
	OutStats.totalPageFile	= memStatus.dwTotalPageFile / (1024*1024);
	OutStats.availPageFile	= memStatus.dwAvailPageFile / (1024*1024);
	OutStats.totalVirtual	= memStatus.dwTotalVirtual / (1024*1024);
	OutStats.availVirtual	= memStatus.dwAvailVirtual / (1024*1024);
}
void FromWinMemoryStatus( const MEMORYSTATUSEX& memStatus, mxMemoryStatus & OutStats )
{
	OutStats.memoryLoad		= memStatus.dwMemoryLoad;
	OutStats.totalPhysical	= memStatus.ullTotalPhys / (1024*1024);
	OutStats.availPhysical	= memStatus.ullAvailPhys / (1024*1024);
	OutStats.totalPageFile	= memStatus.ullTotalPageFile / (1024*1024);
	OutStats.availPageFile	= memStatus.ullAvailPageFile / (1024*1024);
	OutStats.totalVirtual	= memStatus.ullTotalVirtual / (1024*1024);
	OutStats.availVirtual	= memStatus.ullAvailVirtual / (1024*1024);
}
void mxGetCurrentMemoryStatus( mxMemoryStatus & OutStats )
{
	ENSURE_WINSYS_IS_INITIALIZED;
	MEMORYSTATUS  memStatus;
	memStatus.dwLength = sizeof( memStatus );

	::GlobalMemoryStatus( &memStatus );

	FromWinMemoryStatus( memStatus, OutStats );
}

void mxGetExeLaunchMemoryStatus( mxMemoryStatus & OutStats )
{
	ENSURE_WINSYS_IS_INITIALIZED;
	FromWinMemoryStatus( g_Win32System.memoryStatusOnExeLaunch, OutStats );
}

void mxGetMemoryInfo( size_t &TotalBytes, size_t &AvailableBytes )
{
#if 0
	MEMORYSTATUS  memStatus;
	memStatus.dwLength = sizeof( memStatus );

	::GlobalMemoryStatus( &memStatus );

	TotalBytes = memStatus.dwTotalPhys;
	AvailableBytes = memStatus.dwAvailPhys;
#else
	MEMORYSTATUSEX  memStatus;
	memStatus.dwLength = sizeof( memStatus );

	::GlobalMemoryStatusEx( &memStatus );

	TotalBytes = (size_t) memStatus.ullTotalPhys;
	AvailableBytes = (size_t) memStatus.ullAvailPhys;
#endif
}

//-----------------------------------------------------------------
//		File system
//-----------------------------------------------------------------

const TCHAR* mxGetLauchDirectory()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.launchDirectory;
}

const TCHAR* mxGetExeFileName()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.exeFileName;
}

const TCHAR* mxGetCmdLine()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return ::GetCommandLine();
}

const TCHAR* mxGetComputerName()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.computerName;
}

const TCHAR* mxGetUserName()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.userName;
}

//-----------------------------------------------------------------
//		Miscellaneous
//-----------------------------------------------------------------

void mxSleep( float seconds )
{
	::Sleep( DWORD(seconds * 1000.0f) );
}
void mxSleepMicroseconds( UINT64 usec )
{
	if(usec > 20000)
	{
		::Sleep((DWORD)(usec/1000));
	}
	else if(usec >= 1000)
	{
		::timeBeginPeriod(1);
		::Sleep((DWORD)(usec/1000));
		::timeEndPeriod(1);
	}
	else
	{
		::Sleep(0);
	}
}
void mxSleepMilliseconds( UINT32 msecs )
{
	::Sleep( msecs );
}

//UINT mxGetCpuUsage()
//{
//	ENSURE_WINSYS_IS_INITIALIZED;
//	PDH_STATUS	status;
//	status = ::PdhCollectQueryData( g_Win32System.hCpuUsageQuery );
//	if( ERROR_SUCCESS != status ) {
//		ptERROR("PdhCollectQueryData() failed.\n");
//	}
//	PDH_FMT_COUNTERVALUE value;
//	status = ::PdhGetFormattedCounterValue( g_Win32System.hCpuUsageCounter, PDH_FMT_LONG, NULL, &value );
//	if( ERROR_SUCCESS != status ) {
//		ptERROR("PdhCollectQueryData() failed.\n");
//	}
//	return value.longValue;
//}

THREAD_ID ptGetMainThreadID()
{
	ENSURE_WINSYS_IS_INITIALIZED;
	return g_Win32System.dwMainThreadId;
}

void mxBeep( UINT delayMs, UINT frequencyInHertz )
{
	::Beep( frequencyInHertz, delayMs );
}

void mxSetMouseCursorVisible( bool bVisible )
{
#if 0
	static HCURSOR hCursor = null;	// application cursor
	if ( !hCursor ) {
		hCursor = ::GetCursor();
	}

	if ( bVisible ) {
		::SetCursor( hCursor );
	} else {
		::SetCursor( null );
	}
#else
	::ShowCursor( bVisible );
#endif
}

void mxSetMouseCursorPosition( UINT x, UINT y )
{
	POINT point;
	point.x = x;
	point.y = y;
	::SetCursorPos( point.x, point.y );
}

void mxGetCurrentDeckstopResolution( UINT &ScreenWidth, UINT &ScreenHeight )
{
	ScreenWidth  = (UINT) ::GetSystemMetrics( SM_CXSCREEN );
	ScreenHeight = (UINT) ::GetSystemMetrics( SM_CYSCREEN );
}

void mxWaitForKeyPress()
{
	//FIXME: this is too dull and expensive
	::system("PAUSE");
}

static AtomicInt s_ExitRequested = FALSE;

void G_RequestExit()
{
	AtomicExchange( &s_ExitRequested, TRUE );
}
bool G_isExitRequested()
{
	return AtomicLoad( s_ExitRequested ) == TRUE;
}

AtomicInt AtomicLoad( const AtomicInt& var )
{
	// Simple reads to properly-aligned 32-bit variables are atomic operations.
	return var;
}

AtomicInt AtomicIncrement( AtomicInt* var )
{
	return ::InterlockedIncrementAcquire( var );
}

AtomicInt AtomicDecrement( AtomicInt* var )
{
	return ::InterlockedDecrementRelease( var );
}

AtomicInt AtomicAdd( AtomicInt& var, int add )
{
	return ::InterlockedExchangeAdd( (AtomicInt*)&var, add ) + add;
}

AtomicInt AtomicExchange( AtomicInt* dest, int value )
{
	// Sets a 32-bit variable to the specified value as an atomic operation.
	return ::InterlockedExchange( dest, value );	// returns the previous value.
}

AtomicInt AtomicCompareExchange( AtomicInt* dest, AtomicInt comparand, AtomicInt exchange )
{
	return ::InterlockedCompareExchange( dest, exchange, comparand );
}



mxTEMP
bool AtomicCAS( AtomicInt* valuePtr, int oldValue, int newValue )
{
	return ::InterlockedCompareExchange( valuePtr, newValue, oldValue ) == oldValue;
}

void AtomicIncrement( AtomicInt* value, int incAmount )
{
	AtomicInt count = *value;
	while( !AtomicCAS( value, count, count + incAmount ) )
	{
		count = *value;
	}
}

void AtomicDecrement( AtomicInt* value, int decAmount )
{
	AtomicInt count = *value;
	while( !AtomicCAS( value, count, count - decAmount ) )
	{
		count = *value;
	}
}

void SpinLock( AtomicInt* valuePtr,int oldValue,int newValue )
{
	while( !AtomicCAS( valuePtr, oldValue, newValue ) )
		;
}

void WaitLock( AtomicInt* valuePtr, int value )
{
	while( !AtomicCAS( valuePtr, value, value ) )
		;
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
ThreadSafeFlag::ThreadSafeFlag()
	: flag( 0 )
{}

void ThreadSafeFlag::Set()
{
    AtomicExchange( &flag, 1 );
}

void ThreadSafeFlag::clear()
{
    AtomicExchange( &flag, 0 );
}

bool ThreadSafeFlag::Test() const
{
    return (0 != flag);
}

bool ThreadSafeFlag::TestAndClearIfSet()
{
    return (1 == AtomicCompareExchange( &flag, 0, 1 ));
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
EventFlag::EventFlag()
{
	m_handle = NULL;
}

EventFlag::~EventFlag()
{

}

bool EventFlag::Initialize( UINT flags )
{
	const bool bManualReset			= (flags & CreateEvent_AutoReset) == 0;
	const bool bInitiallySignalled	= (flags & CreateEvent_Signalled) != 0;

	m_handle = ::CreateEvent(
		NULL,						// Security attributes
		bManualReset,				// Manual reset ?
		bInitiallySignalled,		// Initially signaled ?
		NULL						// Name
	);
	chkRET_FALSE_IF_NOT( m_handle != NULL );

	return true;
}

void EventFlag::Shutdown()
{
	::CloseHandle( m_handle );
	m_handle = NULL;
}

bool EventFlag::Signal() const
{
	if( ::SetEvent( m_handle ) ) {
		return true;
	}
	return false;
}

bool EventFlag::Reset() const
{
	if( ::ResetEvent( m_handle ) ) {
		return true;
	}
	return false;
}

bool EventFlag::Wait() const
{
	const DWORD dwWaitResult = ::WaitForSingleObject( m_handle, INFINITE );
	if( dwWaitResult == WAIT_ABANDONED ) {
		return false;
	}
	return true;
}

// Waits for the event to become signaled with a specified timeout
// in milliseconds. If the method times out it will return false,
// if the event becomes signaled within the timeout it will return 
// true.
bool EventFlag::WaitTimeOut( int ms ) const
{
	const DWORD dwWaitResult = ::WaitForSingleObject( m_handle, ms );
    return (WAIT_TIMEOUT == dwWaitResult) ? false : true;
}

// This checks if the event is signaled and returns immediately.
bool EventFlag::Peek() const
{
	const DWORD dwWaitResult = ::WaitForSingleObject( m_handle, 0 );
    return (WAIT_TIMEOUT == dwWaitResult) ? false : true;
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
ConditionVariable::ConditionVariable()
{

}

ConditionVariable::~ConditionVariable()
{

}

void ConditionVariable::Initialize()
{
	InitializeConditionVariable( &m_CV );
}

void ConditionVariable::Shutdown()
{
	// nothing
}

void ConditionVariable::Wait( SpinWait & CS )
{
	::SleepConditionVariableCS( &m_CV, &CS.m_CS, INFINITE );
}

void ConditionVariable::NotifyOne()
{
	::WakeConditionVariable( &m_CV );
}

void ConditionVariable::Broadcast()
{
	// The WakeAllConditionVariable wakes all waiting threads
	// while the WakeConditionVariable wakes only a single thread.
	// Waking one thread is similar to setting an auto-reset event,
	// while waking all threads is similar to pulsing a manual reset event
	// but more reliable (see PulseEvent for details).
	::WakeAllConditionVariable( &m_CV );
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
SpinWait::SpinWait()
{

}

SpinWait::~SpinWait()
{

}

bool SpinWait::Initialize( UINT spinCount )
{
	// IMPLEMENTATION NOTE
	// In cases when there is no oversubscription the reasonable value of the spin 
	// count would be 50000 - 100000 (25 - 50 microseconds on modern CPUs).
	// Unfortunately the spinning in Windows critical section is power inefficient
	// so the value is a traditional 1000 (approx. the cost of kernel mode transition)
	//
	// To achieve maximal locking efficiency use TBB spin_mutex (which employs 
	// exponential backoff technique, and supports cooperative behavior in case 
	// of oversubscription)
	const BOOL bResult = ::InitializeCriticalSectionAndSpinCount( &m_CS, spinCount );
	return bResult;

	//::InitializeCriticalSection( &m_CS );
	//return true;
}

void SpinWait::Shutdown()
{
	::DeleteCriticalSection( &m_CS );
}

// Locks the critical section.
void SpinWait::Enter()
{
	::EnterCriticalSection( &m_CS );
}

// Attempts to acquire exclusive ownership of a mutex.
bool SpinWait::TryEnter()
{
	return ::TryEnterCriticalSection( &m_CS ) != FALSE;
}

// Releases the lock on the critical section.
void SpinWait::Leave()
{
	::LeaveCriticalSection( &m_CS );
}

SpinWait::Lock::Lock( SpinWait & spinWait )
	: m_spinWait( spinWait )
{
	m_spinWait.Enter();
}

SpinWait::Lock::~Lock()
{
	m_spinWait.Leave();
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
Semaphore::Semaphore()
{
	m_handle = NULL;
}

bool Semaphore::Initialize( UINT initialCount, UINT maximumCount )
{
	//enum { MAX_ALLOWED_COUNT = MAX_UINT32 };
	//maximumCount = smallest( maximumCount, MAX_ALLOWED_COUNT );

	m_handle = ::CreateSemaphore(
		NULL,			// default security attributes
		initialCount,
		maximumCount,
		NULL			// name
	);

	chkRET_FALSE_IF_NOT( m_handle != NULL );

	return true;
}

void Semaphore::Shutdown()
{
	::CloseHandle( m_handle );
}

bool Semaphore::Wait()
{
	const DWORD dwWaitResult = ::WaitForSingleObject( m_handle, INFINITE );
	mxASSERT( WAIT_OBJECT_0 == dwWaitResult );
	return dwWaitResult != WAIT_ABANDONED;
}

// Increases the count of the specified semaphore object by a specified amount.
bool Semaphore::Signal( UINT count, INT *oldCount )
{
	mxASSERT( count >= 1 );

	LONG	ulOldCount;

	if( ::ReleaseSemaphore( m_handle, count, &ulOldCount ) )
	{
		if( oldCount ) {
			*oldCount = ulOldCount;
		}
		return true;
	}
	else
	{
		return false;
	}
}

THREAD_ID ptGetCurrentThreadID()
{
	return ::GetCurrentThreadId();
}

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
int EThreadPriority_To_Win32ThreadPriority( EThreadPriority e )
{
	switch( e )
	{
	case ThreadPriority_Low:
		return THREAD_PRIORITY_LOWEST;

	case ThreadPriority_Normal:
		return THREAD_PRIORITY_NORMAL;

	case ThreadPriority_High:
		return THREAD_PRIORITY_HIGHEST;

	case ThreadPriority_Critical:
		return THREAD_PRIORITY_TIME_CRITICAL;

	default:
		mxUNREACHABLE;
		return -1;
	}
}

Thread::CInfo::CInfo()
{
	entryPoint = NULL;
	userPointer = NULL;
	userData = -1;
	stackSize = mxDEFAULT_THREAD_STACK_SIZE;
	priority = ThreadPriority_Normal;
	bCreateSuspended = false;
	debugName = NULL;
}

ASSERT_SIZEOF(THREAD_ID, sizeof(DWORD));

/*
-------------------------------------------------------------------------
-------------------------------------------------------------------------
*/
Thread::Thread()
{
	m_handle = NULL;
}

Thread::~Thread()
{
	if( m_handle != NULL )
	{
		this->Shutdown();
	}
}

bool Thread::Initialize( const CInfo& cInfo )
{
	mxASSERT( m_handle == NULL );

	DWORD	dwCreationFlags = 0;
	if( cInfo.bCreateSuspended ) {
		dwCreationFlags |= CREATE_SUSPENDED;
	}

	DWORD	threadID;

	mxSTOLEN("comments from Phyre Engine");
	m_handle = ::CreateThread(
		NULL,										// default security attributes
		cInfo.stackSize,							// use default stack size
		(LPTHREAD_START_ROUTINE)cInfo.entryPoint,	// thread function
		cInfo.userPointer,							// argument to thread function
		dwCreationFlags,							// creation flags
		&threadID									// returns the thread identifier
	);
	chkRET_FALSE_IF_NOT(m_handle != NULL);

	if( threadID && cInfo.debugName != NULL )
	{
		Win32_Dbg_SetThreadName( threadID, cInfo.debugName );
	}

	this->SetPriority( cInfo.priority );

	return true;
}

void Thread::Shutdown()
{
	mxASSERT( m_handle != NULL );
	::WaitForSingleObject( m_handle, INFINITE );
	::CloseHandle( m_handle );
	m_handle = NULL;
}

void Thread::Join( UINT *exitCode )
{
	this->JoinTimeOut( INFINITE, exitCode );
}

bool Thread::JoinTimeOut( UINT timeOut, UINT *exitCode )
{
	mxASSERT( m_handle != NULL );

	const DWORD dwWaitResult = ::WaitForSingleObject( m_handle, timeOut );
	chkRET_FALSE_IF_NOT( dwWaitResult == WAIT_OBJECT_0 );

	if( exitCode != NULL )
	{
		DWORD	result;
		::GetExitCodeThread( m_handle, &result );

		*exitCode = result;
	}

	return true;
}

bool Thread::Suspend()
{
	mxASSERT( m_handle != NULL );
	return ::SuspendThread( m_handle ) != (DWORD) -1;
}

bool Thread::Resume()
{
	mxASSERT( m_handle != NULL );
	return ::ResumeThread( m_handle ) != (DWORD) -1;
}

bool Thread::Kill()
{
	mxASSERT( m_handle != NULL );
	const DWORD dwExitCode = 1;
	//NOTE: Using TerminateThread() does not allow proper thread clean up.
	return ::TerminateThread( m_handle, dwExitCode ) != 0;
}

void Thread::SetAffinity( UINT affinity )
{
	mxASSERT( m_handle != NULL );
	const UINT affinityMask = (1 << affinity);
	::SetThreadAffinityMask( m_handle, (DWORD_PTR)affinityMask );
}

void Thread::SetAffinityMask( UINT64 affinityMask )
{
	mxASSERT( m_handle != NULL );
	::SetThreadAffinityMask( m_handle, (DWORD_PTR)affinityMask );
}

void Thread::SetProcessor( UINT processorIndex )
{
	mxASSERT( m_handle != NULL );
	::SetThreadIdealProcessor( m_handle, processorIndex );
}

void Thread::SetPriority( EThreadPriority newPriority )
{
	mxASSERT( m_handle != NULL );
	::SetThreadPriority( m_handle, EThreadPriority_To_Win32ThreadPriority( newPriority ) );
}

bool Thread::IsRunning() const
{
	if( NULL != m_handle )
	{
		DWORD dwExitCode = 0;
		if( ::GetExitCodeThread( m_handle, &dwExitCode ) )
		{
			if( STILL_ACTIVE == dwExitCode )
			{
				return true;
			}
		}
	}
	return false;
}

/*
Most of windows Debuggers supports thread names.
This is really helpful to identify the purpose of threads especially
when you’re having multiple threads in your application.

In Visual Studio  you can see the details of threads including it’s name in the “Threads” window.
There are no direction functions to set the thread names for a thread.

But it’s possible to implement by exploiting exception handling mechanism of a debugger.
The debugger will get the first chance to handle the exception when an application is being debugged.
Using this concept we will be explicitly raising an exception with parameters which specifies the thread name and other information.
The particular exception will be specially handled by debugger and thus it will update the name of corresponding thread. 

When application running without debugger, raising exceptions lead to application crash.
So we’ve to enclose the RaiseException code inside a try-except block.
*/

// The following function can be used either by specifying the thread ID
// of the particular thread to be named or passing -1
// as thread ID will set the name for the current thread which is calling the function.
//
// Usage: SetThreadName (-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void Win32_Dbg_SetThreadName( THREAD_ID dwThreadID, const char* threadName )
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		::RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// printf(“Not under debugger. Swallowing exception”);
	}
}

void YieldHardwareThread()
{
	YieldProcessor();
}

void YieldSoftwareThread()
{
	// Causes the calling thread to yield execution to another thread
	// that is ready to run on the current processor.
	// The operating system selects the next thread to be executed.
	::SwitchToThread();
}

void YieldToOS()
{
	::Sleep( 0 );
}

void YieldToOSAndSleep()
{
	::Sleep( 1 );
}

//-----------------------------------------------------------------
//		Debug output and error handling
//-----------------------------------------------------------------

#if MX_DEBUG

void mxCrash()
{
#if MX_DEVELOPER
	MessageBox(NULL, TEXT("Info"), TEXT("Crashing at your desire!"), MB_OK);
	*((int*)NULL) = 1;
#endif // MX_DEVELOPER
}

#endif // MX_DEBUG

// Leave error strings in developer mode.
#if MX_DEBUG || MX_DEVELOPER

const char* EReturnCode_To_Chars( ERet returnCode )
{
	static const char* errorCodeToString[] = {
#define mxDECLARE_ERROR_CODE( ENUM, NUMBER, STRING )	STRING,
	#include <Base/System/ptErrorCodes.inl>
#undef mxDECLARE_ERROR_CODE
	};

	mxASSERT( returnCode < ERR_MAX );

	returnCode = (ERet) smallest( returnCode, ERR_MAX-1 );

	return errorCodeToString[ returnCode ];
}

#else

const char* EReturnCode_To_Chars( ERet returnCode )
{
	return "Unknown error";
}

#endif // !MX_DEBUG
