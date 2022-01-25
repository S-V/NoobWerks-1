// https://www.enkisoftware.com/devlogpost-20200202-1-Runtime-Compiled-C++-Dear-ImGui-and-DirectX11-Tutorial
#include <Base/Base.h>
#pragma hdrstop

//
#pragma comment( lib, "RuntimeCompiler.lib" )
#pragma comment( lib, "RuntimeObjectSystem.lib" )

//
#include <iostream>

//
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/IObjectFactorySystem.h>
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/RuntimeObjectSystem.h>
#include <RuntimeCompiledCPlusPlus/RuntimeCompiler/Compiler.h>
#include <RuntimeCompiledCPlusPlus/RuntimeCompiler/ICompilerLogger.h>

//
#include <GlobalEngineConfig.h>

//
#include <Developer/RunTimeCompiledCpp.h>


// StdioLogSystem for compiler

const size_t LOGSYSTEM_MAX_BUFFER = 4096;

class StdioLogSystem : public ICompilerLogger
{
public:	
	virtual void LogError(const char * format, ...);
	virtual void LogWarning(const char * format, ...);
    virtual void LogInfo(const char * format, ...);

protected:
	void LogInternal(const char * format, va_list args);
	char m_buff[LOGSYSTEM_MAX_BUFFER];
};

void StdioLogSystem::LogError(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	LogInternal(format, args);
}

void StdioLogSystem::LogWarning(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	LogInternal(format, args);
}

void StdioLogSystem::LogInfo(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	LogInternal(format, args);
}
void StdioLogSystem::LogInternal(const char * format, va_list args)
{
	int result = vsnprintf(m_buff, LOGSYSTEM_MAX_BUFFER-1, format, args);
	// Make sure there's a limit to the amount of rubbish we can output
	m_buff[LOGSYSTEM_MAX_BUFFER-1] = '\0';

	std::cout << m_buff;
#ifdef _WIN32
	OutputDebugStringA( m_buff );
#endif
}






// RCC++ Data
static IRuntimeObjectSystem*    g_pRuntimeObjectSystem;
static StdioLogSystem           g_Logger;


namespace RunTimeCompiledCpp
{

	static ListenerI::Head	gs_listeners;

	static struct NwObjectFactoryListener: IObjectFactoryListener
	{
		virtual void OnConstructorsAdded() override
		{
			ListenerI* curr = gs_listeners;
			while( curr )
			{
				ListenerI* next = curr->next;
				curr->RTCPP_OnConstructorsAdded();
				curr = next;
			}
		}
	} s_object_factory_listener;

ERet initialize( SystemTable* system_table /*= NULL*/ )
{
	g_pRuntimeObjectSystem = new RuntimeObjectSystem();
	if( !g_pRuntimeObjectSystem->Initialise( &g_Logger, system_table ) )
	{
		delete g_pRuntimeObjectSystem;
		g_pRuntimeObjectSystem = NULL;
		return ERR_UNKNOWN_ERROR;
	}

	// ensure include directories are set - use location of this file as starting point

	const FileSystemUtils::Path filePath = g_pRuntimeObjectSystem->FindFile( __FILE__ );

	const FileSystemUtils::Path base_path = filePath
		.ParentPath()	// Engine
		.ParentPath()	// Runtime
		.ParentPath()	// Engine
		.ParentPath()	// Source
		// R:/NoobWerks/
		;

	const FileSystemUtils::Path source_path = base_path / "Source";

	// #include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/RuntimeInclude.h>
	const FileSystemUtils::Path External_IncludeDir = base_path / "External";
	g_pRuntimeObjectSystem->AddIncludeDir( External_IncludeDir.c_str() );

	const FileSystemUtils::Path OzzAnimation_IncludeDir = External_IncludeDir / "ozz-animation" / "include";
	g_pRuntimeObjectSystem->AddIncludeDir( OzzAnimation_IncludeDir.c_str() );

	// e.g. #include <GlobalEngineConfig.h>
	const FileSystemUtils::Path Engine_IncludeDir = source_path / "Engine";
	g_pRuntimeObjectSystem->AddIncludeDir( Engine_IncludeDir.c_str() );

	// e.g. #include <Base/Base.h>
	const FileSystemUtils::Path EngineRuntime_IncludeDir = source_path / "Engine" / "Runtime";
	g_pRuntimeObjectSystem->AddIncludeDir( EngineRuntime_IncludeDir.c_str() );

	//
	const FileSystemUtils::Path RuntimeEngine_IncludeDir = source_path / "Tests" / "RTSGame";
	g_pRuntimeObjectSystem->AddIncludeDir( RuntimeEngine_IncludeDir.c_str() );


	//
	enum ePROJECTIDS
	{
		PROJECTID_DEFAULT = 0,

		PROJECTID_MODULE_Base,
		PROJECTID_MODULE_Core,

		PROJECTID_MODULE_AI,
		PROJECTID_MODULE_GAME,
		PROJECTID_MODULE_RENDERING,
		PROJECTID_MODULE_AUDIO,
		PROJECTID_MODULE_UI,
	};


#if mxARCH_TYPE == mxARCH_64BIT

	g_pRuntimeObjectSystem->AddLibraryDir("R:/NoobWerks/Binaries/x64" );

	// FMOD
	g_pRuntimeObjectSystem->AddLibraryDir("R:/NoobWerks/External/_libs64" );

#else

	g_pRuntimeObjectSystem->AddLibraryDir("R:/NoobWerks/Binaries/x86" );

	// FMOD
	g_pRuntimeObjectSystem->AddLibraryDir("R:/NoobWerks/External/_libs32" );

#endif


	// DirectX libraries

#if mxARCH_TYPE == mxARCH_64BIT
	g_pRuntimeObjectSystem->AddLibraryDir("D:/sdk/dx_sdk/Lib/x64" );
#else
	g_pRuntimeObjectSystem->AddLibraryDir("D:/sdk/dx_sdk/Lib/x86" );
#endif

	//

	g_pRuntimeObjectSystem->SetIntermediateDir("R:/temp" );

	//

	g_pRuntimeObjectSystem->GetObjectFactorySystem()
		->AddListener(&s_object_factory_listener)
		;

	return ALL_OK;
}

void shutdown()
{
	delete g_pRuntimeObjectSystem;
	g_pRuntimeObjectSystem = NULL;
}

void update( float delta_seconds_elapsed )
{
    //check status of any compile
    if( g_pRuntimeObjectSystem->GetIsCompiledComplete() )
    {
        // load module when compile complete
        g_pRuntimeObjectSystem->LoadCompiledModule();
    }

    if( !g_pRuntimeObjectSystem->GetIsCompiling() )
    {
		// Note that a time delta is passed to RCC++.
		// This is used to delay compilation of any files
		// until 0.1 seconds have passed so that RCC++ captures all files saved
		// if multiple files are saved at once.

		g_pRuntimeObjectSystem->GetFileChangeNotifier()->Update( delta_seconds_elapsed );
    }
}

void AddListener(ListenerI* new_listener)
{
	new_listener->PrependSelfToList(&gs_listeners);
}

void RemoveListener(ListenerI* listener)
{
	listener->RemoveSelfFromList();
}

}//namespace RunTimeCompiledCpp
