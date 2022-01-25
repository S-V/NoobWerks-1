/*
=============================================================================
	File:	Win32_Debug.cpp
	Desc:
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <string>
#include <Base/Base.h>
#include "StackWalker.h"

// headers needed for working with minidumps

#pragma pack( push, 8 )
	#include <DbgHelp.h>
#pragma pack( pop )

#if MX_AUTOLINK
	#pragma comment( lib, "Dbghelp.lib" )
#endif

#include <errno.h>

void ptDebugPrint( const char* message )
{
	::OutputDebugStringA( message );
}

bool IsInDebugger()
{
	return ::IsDebuggerPresent();
}

mxSTOLEN("Nebula3")

//--------------------------------------------------------------------------

static LONG WINAPI ExceptionCallback( EXCEPTION_POINTERS* exceptionInfo );

/**
    This static method registers our own exception handler with Windows.
*/
void /*static*/
Win32MiniDump::Setup()
{
	:: SetUnhandledExceptionFilter( &ExceptionCallback );
}

//--------------------------------------------------------------------------

/// build a filename for the dump file
static bool BuildMiniDumpFilename( char *outDumpFileName, UINT numChars )
{
	// get our module filename directly from windows
	char exeFileName[256] = {0};
	char driveName[32] = {0};
	char folderName[256] = {0};
	char pureFileName[256] = {0};
	char fileExtension[32] = {0};
	
	DWORD numBytes = ::GetModuleFileNameA( NULL, exeFileName, sizeof(exeFileName) - 1 );
	if( numBytes > 0 )
	{
		int ret = _splitpath_s(
			exeFileName,
			driveName,
			folderName,
			pureFileName,
			fileExtension
		);
		if( ret != 0 ) {
			ptWARN( "_splitpath_s() failed with error code '%d'.\n", ret );
		}

		// get the current calender time
		SYSTEMTIME t;
		::GetLocalTime( &t );

		char timeStr[256];
		tMy_sprintfA( timeStr, "%04d-%02d-%02d_%02d-%02d-%02d",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond );

		// build the dump filename
		sprintf_s( outDumpFileName, numChars, "%s%s_%s.dmp", folderName, pureFileName, timeStr );

		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
/**
    Private method to write a mini-dump with extra exception info. This 
    method is either called from the public WriteMiniDump() method or
    from the ExceptionCallback() function.
*/
static bool WriteMiniDumpInternal( EXCEPTION_POINTERS* exceptionInfo )
{
	char dumpFilename[512] = {0};
    if( BuildMiniDumpFilename( dumpFilename, mxCOUNT_OF(dumpFilename) ) )
    {
		char msgBoxText[1024] = {0};
		tMy_sprintfA( msgBoxText,
			"An unhandled error occurred.\nDo you wish to dump debug info into file '%s'?",
			dumpFilename
		);
		const int ret = ::MessageBoxA( NULL, msgBoxText, "Fatal error", MB_OKCANCEL );
		if( IDCANCEL == ret ) {
			return false;
		}

        HANDLE hFile = 
			::CreateFileA(
				dumpFilename,             // lpFileName
				GENERIC_WRITE,            // dwDesiredAccess
				FILE_SHARE_READ,          // dwShareMode
				0,                        // lpSecurityAttributes
				CREATE_ALWAYS,            // dwCreationDisposition,
				FILE_ATTRIBUTE_NORMAL,    // dwFlagsAndAttributes
				NULL                      // hTemplateFile
		);
        if( NULL != hFile )
        {
			HANDLE hProc = ::GetCurrentProcess();
            DWORD procId = ::GetCurrentProcessId();
            BOOL res = FALSE;
            if( NULL != exceptionInfo )
            {
                // extended exception info is available
                MINIDUMP_EXCEPTION_INFORMATION extInfo = { 0 };
                extInfo.ThreadId = ::GetCurrentThreadId();
                extInfo.ExceptionPointers = exceptionInfo;
                extInfo.ClientPointers = TRUE;
                res = ::MiniDumpWriteDump( hProc, procId, hFile, MiniDumpNormal, &extInfo, NULL, NULL );
            }
            else
            {
                // extended exception info is not available
                res = ::MiniDumpWriteDump( hProc, procId, hFile, MiniDumpNormal, NULL, NULL, NULL );
            }
            ::CloseHandle( hFile );
            return true;
        }
    }

    return false;
}

struct WindowsExceptionCallback: AExceptionCallback
{
	std::string	m_stackTrace;
mxREMOVE_THIS
	virtual void AddStackEntry( const char* text ) override
	{
		m_stackTrace.append( text );
	}
	virtual void OnException() override
	{
		if( m_stackTrace.size() > 0 )
		{
			mxGetLog().VWrite(
				LL_Error,
				m_stackTrace.c_str(),
				m_stackTrace.size()
				);

			::MessageBoxA(
				NULL,
				"[Stack Trace dumped to Log]",//m_stackTrace.c_str(),
				"ERROR",
				MB_OK
				);
		}
	}
};
WindowsExceptionCallback g_defaultExceptionCallback;

AExceptionCallback* g_exceptionCallback = nil;//&g_defaultExceptionCallback;

/**
    Exception handler function called back by Windows when something
    unexpected happens.
*/
static LONG WINAPI ExceptionCallback( EXCEPTION_POINTERS* exceptionInfo )
{
	WriteMiniDumpInternal(exceptionInfo);

	if( g_exceptionCallback )
	{
		// collect stack trace
		struct MyStackWalker: public StackWalker
		{
			MyStackWalker( int options )
				: StackWalker( options )
			{}
			virtual void OnOutput(LPCSTR szText)
			{
				g_exceptionCallback->AddStackEntry(szText);
			}
		};

		int options = StackWalker::RetrieveLine;	// too verbose if StackWalker::RetrieveSymbol option is on

		MyStackWalker sw( options );
		sw.ShowCallstack(
			::GetCurrentThread(),
			exceptionInfo->ContextRecord
			);

		g_exceptionCallback->OnException();
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

/**
    This method is called by assert() and error() to write out
    a minidump file.
*/
bool /*static*/
Win32MiniDump::WriteMiniDump()
{
    return WriteMiniDumpInternal( 0 );
}
