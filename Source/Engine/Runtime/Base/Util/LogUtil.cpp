/*
=============================================================================
	File:	LogUtil.cpp
	Desc:
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Text/TextUtils.h>
#include "LogUtil.h"

void F_Util_ComposeLogFileName( String256 & log_file_name )
{
	String256	exeFileName;

	Str::CopyS( exeFileName, mxCHARS_AS_ANSI(mxGetExeFileName()) );
	Str::StripFileExtension( exeFileName );

	Str::Format( log_file_name, "%s.log", exeFileName.raw() );
}

void mxUtil_StartLogging( ALog* logger )
{
	DEVOUT( "Engine init: %u microseconds.", (UINT)mxGetTimeInMicroseconds() );

	// log current time
	{
		CalendarTime	localTime( CalendarTime::GetCurrentLocalTime() );

		String256	timeOfDay;
		GetTimeOfDayString( timeOfDay, localTime.hour, localTime.minute, localTime.second );

		logger->PrintF( LL_Info, "=====================================================");
		logger->PrintF( LL_Info,  "Log started on %s, %s %u%s, at %s.",
			CalendarTime::WeekdayToStr( localTime.weekday ),
			CalendarTime::MonthToStr( localTime.month ),
			localTime.day, GetNumberExtension( localTime.day ),
			timeOfDay.raw()
			);
		logger->PrintF( LL_Info, "=====================================================");
	}

	// log engine version and game build time stamp

	logger->PrintF( LL_Info, "Engine version: %s (%s", mxENGINE_VERSION_STRING, (sizeof(void*) == 4) ? "32-bit" : "64-bit" );
	logger->PrintF( LL_Info, "Build time: %s", mxGetBaseBuildTimestamp());
	logger->PrintF( LL_Info, "Compiled with %s (%s)", mxCOMPILER_NAME, MX_DEBUG ? "DEBUG" : "RELEASE" );
	logger->PrintF( LL_Info, "Machine endianness: %s", (mxCPU_ENDIANNESS == mxCPU_ENDIAN_LITTLE) ? "Little-endian" : "Big-endian" );
	logger->PrintF( LL_Info, "Character set: %s", sizeof(TCHAR)==1 ? ("ANSI") : ("Unicode") );

//	logger->Logf( LL_Info,  "\n==================================\n" );


	// log system info

	PtSystemInfo	sysInfo;
	mxGetSystemInfo( sysInfo );

	logger->PrintF( LL_Info,  "OS: %s", mxCHARS_AS_ANSI( sysInfo.OSName ) );
	extern bool IsWow64();
	logger->PrintF( LL_Info,  "OS type: %s Operating System", IsWow64() ? "64-bit" : "32-bit" );
	logger->PrintF( LL_Info,  "OS language: \"%s\"", mxCHARS_AS_ANSI( sysInfo.OSLanguage ) );
	logger->PrintF( LL_Info,  "OS directory: \"%s\"", mxCHARS_AS_ANSI( sysInfo.OSDirectory ) );
	logger->PrintF( LL_Info,  "System directory: \"%s\"", mxCHARS_AS_ANSI( sysInfo.SysDirectory ) );
	logger->PrintF( LL_Info,  "Launch directory: \"%s\"", mxCHARS_AS_ANSI( mxGetLauchDirectory() ) );
	logger->PrintF( LL_Info,  "Executable file name: \"%s\"", mxCHARS_AS_ANSI( mxGetExeFileName() ) );
	logger->PrintF( LL_Info,  "Command line parameters: %s", mxCHARS_AS_ANSI( mxGetCmdLine() ) );
	logger->PrintF( LL_Info,  "Computer name: %s", mxCHARS_AS_ANSI( mxGetComputerName() ) );
	logger->PrintF( LL_Info,  "User name: %s", mxCHARS_AS_ANSI( mxGetUserName() ) );

//	logger->Logf( LL_Info,  "\n==================================\n" );

	const mxCpuInfo & cpuInfo = sysInfo.cpu;

	logger->PrintF( LL_Info,  "CPU name: %s (%s), vendor: %s, family: %u, model: %u, stepping: %u",
		cpuInfo.brandName, mxGetCpuTypeName( cpuInfo.type ), cpuInfo.vendor, cpuInfo.family, cpuInfo.model, cpuInfo.stepping );

	const UINT numCores = cpuInfo.numPhysicalCores;
	logger->PrintF( LL_Info,  "CPU cores: %u, clock frequency: %u MHz (estimated: ~%u MHz), page size = %u bytes",
				numCores, (UINT)cpuInfo.readFreqMHz, UINT(cpuInfo.estimatedFreqHz/(1000*1000)), (UINT)cpuInfo.pageSize );

	logger->PrintF( LL_Info, "Cache line size: %u bytes", cpuInfo.cacheLineSize );
	logger->PrintF( LL_Info, "L1 Data Cache: %u x %u KiB, %u-way set associative", numCores, cpuInfo.L1_DCache_Size/numCores, cpuInfo.L1_Assoc );
	logger->PrintF( LL_Info, "L1 Instruction Cache: %u x %u KiB, %u-way set associative", numCores, cpuInfo.L1_ICache_Size/numCores, cpuInfo.L1_Assoc );

	logger->PrintF( LL_Info, "L2 Cache: %u x %u KiB, %u-way set associative", numCores, cpuInfo.L2_Cache_Size/numCores, cpuInfo.L2_Assoc );

	if( cpuInfo.L3_Cache_Size && cpuInfo.L3_Assoc )
	{
		logger->PrintF( LL_Info, "L3 Cache: %u KiB, %u-way set associative", cpuInfo.L3_Cache_Size, cpuInfo.L3_Assoc );
	}

	logger->PrintF( LL_Info, "CPU features:");
	//if( cpuInfo.has_CPUID ) {
	//	logger->Logf( LL_Info, "CPUID supported");
	//}
	if( cpuInfo.has_RDTSC ) {
		logger->PrintF( LL_Info, "	RDTSC");
	}
	if( cpuInfo.has_CMOV ) {
		logger->PrintF( LL_Info, "	CMOV");
	}
	if( cpuInfo.has_MOVBE ) {
		logger->PrintF( LL_Info, "	MOVBE");
	}
	if( cpuInfo.has_FPU ) {
		logger->PrintF( LL_Info, "	FPU: On-Chip");
	}
	if( cpuInfo.has_MMX ) {
		logger->PrintF( LL_Info, "	MMX");
	}
	if( cpuInfo.has_MMX_AMD ) {
		logger->PrintF( LL_Info, "	MMX (AMD extensions)");
	}
	if( cpuInfo.has_3DNow ) {
		logger->PrintF( LL_Info, "	3D Now!\n");
	}
	if( cpuInfo.has_3DNow_Ext ) {
		logger->PrintF( LL_Info, "	3D Now! (AMD extensions)");
	}
	if( cpuInfo.has_SSE_1 ) {
		logger->PrintF( LL_Info, "	SSE");
	}
	if( cpuInfo.has_SSE_2 ) {
		logger->PrintF( LL_Info, "	SSE2");
	}
	if( cpuInfo.has_SSE_3 ) {
		logger->PrintF( LL_Info, "	SSE3");
	}
	if( cpuInfo.has_SSE_4_1 ) {
		logger->PrintF( LL_Info, "	SSE4.1");
	}
	if( cpuInfo.has_SSE_4_2 ) {
		logger->PrintF( LL_Info, "	SSE4.2");
	}
	if( cpuInfo.has_SSE_4a ) {
		logger->PrintF( LL_Info, "	SSE4a");
	}
	if( cpuInfo.has_POPCNT ) {
		logger->PrintF( LL_Info, "	POPCNT");
	}
	if( cpuInfo.has_FMA ) {
		logger->PrintF( LL_Info, "	FMA");
	}
	if( cpuInfo.has_AES ) {
		logger->PrintF( LL_Info, "	AES");
	}
	if( cpuInfo.has_AVX ) {
		logger->PrintF( LL_Info, "	AVX");
	}


	if( cpuInfo.has_HTT ) {
		logger->PrintF( LL_Info, "detected: Hyper-Threading Technology");
	}
	if( cpuInfo.has_EM64T ) {
		logger->PrintF( LL_Info, "64-bit Memory Extensions supported");
	}


	logger->PrintF( LL_Info,  "Logical CPU cores: %u",
		(UINT)cpuInfo.numLogicalCores );

//	logger->Logf( LL_Info,  "\n==================================\n" );

	// log memory stats

	mxMemoryStatus initialMemoryStatus;
	mxGetExeLaunchMemoryStatus(initialMemoryStatus);
	logger->PrintF( LL_Info,  "%u MB physical memory installed, %u MB available, %u MB virtual memory installed, %u percent memory in use",
		initialMemoryStatus.totalPhysical,
		initialMemoryStatus.availPhysical,
		initialMemoryStatus.totalVirtual,
		initialMemoryStatus.memoryLoad
	);
	logger->PrintF( LL_Info,  "Page file: %u MB used, %u MB available",
		initialMemoryStatus.totalPageFile - initialMemoryStatus.availPageFile,
		initialMemoryStatus.availPageFile
	);

	UINT	currScreenWidth, currScreenHeight;
	mxGetCurrentDeckstopResolution( currScreenWidth, currScreenHeight );
	logger->PrintF( LL_Info,  "Current deckstop resolution: %u x %u",
		currScreenWidth, currScreenHeight
	);

	mxPERM("determine bus speed");

//	logger->Logf( LL_Info,  "\n==================================\n" );

	// Log basic info.

	logger->PrintF( LL_Info,  "Initialization took %u milliseconds.", UINT(mxGetTimeInMicroseconds()/1000) );
}

void mxUtil_EndLogging( ALog* logger )
{
#if MX_DEBUG_MEMORY
	// Dump memory leaks if needed.
	if(!IsInDebugger())
	{
		//mxMemoryStatistics	globalMemStats;
		//F_DumpGlobalMemoryStats( *logger );
	}
#endif

	// Dump stats if needed.

#if MX_ENABLE_PROFILING
	
	logger->PrintF( LL_Info, "--- BEGIN METRICS --------------------");
	PtProfileManager::dumpAll( logger );
	logger->PrintF( LL_Info, "--- END METRICS ----------------------");

#endif // MX_ENABLE_PROFILING



	// log current time
	{
		CalendarTime	localTime( CalendarTime::GetCurrentLocalTime() );

		String256	timeOfDay;
		GetTimeOfDayString( timeOfDay, localTime.hour, localTime.minute, localTime.second );

		logger->PrintF( LL_Info, "=====================================================\n");
		logger->PrintF( LL_Info, "Log ended on %s, %s %u%s, at %s.\n",
			CalendarTime::WeekdayToStr( localTime.weekday ),
			CalendarTime::MonthToStr( localTime.month ),
			localTime.day, GetNumberExtension( localTime.day ),
			timeOfDay.raw()
		);

		const UINT64 totalRunningTime = mxGetTimeInMicroseconds();
		UINT32 hours, minutes, seconds;
		ConvertMicrosecondsToHoursMinutesSeconds( totalRunningTime, hours, minutes, seconds );
		logger->PrintF( LL_Info, "Program running time: %u hours, %u minutes, %u seconds.\n",
			hours, minutes, seconds
		);

		logger->PrintF( LL_Info, "=====================================================\n\n");
	}
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
