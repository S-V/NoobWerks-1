#include <Base/Base.h>
#pragma hdrstop

#define TB_CLIENT_WITH_NETWORK (0)

#if TB_CLIENT_WITH_NETWORK
#include <enet/enet.h>
#pragma comment( lib, "enet.lib" )
#pragma comment( lib, "Ws2_32.lib" )
#endif

// TempAllocator4096
#include <Base/Memory/Allocators/FixedBufferAllocator.h>
#include <Base/Util/LogUtil.h>

#include <Core/ConsoleVariable.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/JobSystem_Jq.h>
#include <Core/Tasking/SlowTasks.h>

#include <Engine/WindowsDriver.h>
#include <Engine/Windows/ConsoleWindow.h>

#include <GPU/Public/graphics_system.h>
#include <GPU/Public/graphics_device.h>

#include <Rendering/Public/Globals.h>

#include <Scripting/Scripting.h>

#include <Engine/Engine.h>
#include <Engine/Engine_ScriptBindings.h>
#include <Engine/Model.h>

// SON::GetPathToConfigFile
#include <Core/Serialization/Text/TxTConfig.h>
#include <Core/Serialization/Text/TxTSerializers.h>


//
#include <globalengineconfig.h>

#define RMT_ENABLED ENGINE_CONFIG_USE_REMOTERY
#define RMT_USE_D3D11 ENGINE_CONFIG_USE_REMOTERY
#include <Remotery/lib/Remotery.h>

namespace NEngine
{

mxDEFINE_CLASS(LaunchConfig );
mxBEGIN_REFLECTION(LaunchConfig )
	mxMEMBER_FIELD(TemporaryHeapSizeMiB),

	mxMEMBER_FIELD(bCreateLogFile),
	mxMEMBER_FIELD(OverrideLogFileName),
	mxMEMBER_FIELD(NumberOfWorkerThreads),
	mxMEMBER_FIELD(worker_thread_workspace_size_MiB),
	mxMEMBER_FIELD(bCreateConsoleWindow),
	mxMEMBER_FIELD(console_buffer_size),

	mxMEMBER_FIELD(path_to_compiled_assets),
	//mxMEMBER_FIELD(path_to_app_cache),

	mxMEMBER_FIELD(window),

	mxMEMBER_FIELD(enable_VSync),
	mxMEMBER_FIELD(screenshots_folder),
	mxMEMBER_FIELD(path_to_save_file),

	mxMEMBER_FIELD(create_debug_render_device),
	mxMEMBER_FIELD(Gfx_Cmd_Buf_Size_MiB),

	mxMEMBER_FIELD_WITH_CUSTOM_NAME(memory_enable_debug_heap, UseDebugMemoryHeap),
	mxMEMBER_FIELD_WITH_CUSTOM_NAME(memory_debug_heap_size, DebugMemoryHeapSizeMiB),

	//mxMEMBER_FIELD(game_UI_mem),
mxEND_REFLECTION

static void* enet_callback_malloc( size_t size )
{
	return MemoryHeaps::network().Allocate( size, 16 );
}
static void enet_callback_free( void * memory )
{
	MemoryHeaps::network().Deallocate( memory );
}

LaunchConfig::LaunchConfig()
{
#if MX_DEBUG
	this->SetDefaultsForDebug();
#else
	this->SetDefaultsForRelease();
#endif
}

void LaunchConfig::_SetDefaultsCommon()
{
	path_to_compiled_assets = "Assets";
}

void LaunchConfig::SetDefaultsForDebug()
{
	_SetDefaultsCommon();

	TemporaryHeapSizeMiB = 64;

	bCreateLogFile = true;

	NumberOfWorkerThreads = 0;
	// large size needed for testing 64^3 chunks which require >5 MiB
	worker_thread_workspace_size_MiB = 8;

	bCreateConsoleWindow = true;
	console_buffer_size = 0;


	enable_VSync = true;

	sleep_msec_if_not_in_focus = 100;

	Gfx_Cmd_Buf_Size_MiB = mxMiB(8);
	create_debug_render_device = true;

	memory_enable_debug_heap = true;
	memory_debug_heap_size = mxMiB(512);
}

void LaunchConfig::SetDefaultsForRelease()
{
	_SetDefaultsCommon();

	TemporaryHeapSizeMiB = 64;

	bCreateLogFile = false;

	NumberOfWorkerThreads = 0;
	// large size needed for testing 64^3 chunks which require >5 MiB
	worker_thread_workspace_size_MiB = 8;

	bCreateConsoleWindow = true;
	console_buffer_size = 0;


	enable_VSync = true;

	sleep_msec_if_not_in_focus = 100;

	Gfx_Cmd_Buf_Size_MiB = mxMiB(8);
	create_debug_render_device = true;

	memory_enable_debug_heap = false;
	memory_debug_heap_size = 0;
}

ERet LaunchConfig::LoadFromFile()
{
	String256 path_to_engine_config;
	mxTRY(SON::GetPathToConfigFile("engine_config.son", path_to_engine_config));

	TempAllocator8192	temp_allocator( &MemoryHeaps::process() );
	mxTRY(SON::LoadFromFile( path_to_engine_config.c_str(), *this, temp_allocator ));

	return ALL_OK;
}

void LaunchConfig::Clamp_To_Reasonable_Values()
{
	// Don't use an unreasonable amount of threads on future hardware.
	NumberOfWorkerThreads = Min( NumberOfWorkerThreads, 128 );
}

}//namespace NEngine




namespace
{
	struct EngineData
	{
		double g_lastFrameTimeSeconds;

		bool	is_game_paused;

		int	sleep_msec_if_not_in_focus;

		NwJobStats g_job_sytem_stats;

		//
		FileLogUtil		file_log;

		// optional
		CConsole		console_output;
		WinConsoleLog	console_log;
		StdOut_Log		stdout_log;

#if TB_CLIENT_WITH_NETWORK
		TPtr< ENetHost >	net_client;
		TPtr< ENetPeer >	net_peer;
#endif

		String64	screenshots_folder;

	public:
		EngineData()
		{
			g_lastFrameTimeSeconds = 0.0f;
			is_game_paused = false;
			sleep_msec_if_not_in_focus = 0;
		}
	};
	static TPtr< EngineData >	gs_data;
	static Remotery *			gs_remotery;


	static ERet _check_SSE_Support()
	{
		PtSystemInfo	systemInfo;
		mxGetSystemInfo( systemInfo );

		// needed for OZZ animation
		if( !systemInfo.cpu.has_SSE_4_2 ) {
			MessageBoxA(NULL, "SSE4.2 support required!", "Error", MB_OK);
			return ERR_UNSUPPORTED_FEATURE;
		}
		//if( !systemInfo.cpu.has_SSE_2 ) {
		//	MessageBoxA( NULL, "SSE2 support required!", "Error", MB_OK );
		//	return ERR_UNSUPPORTED_FEATURE;
		//}

		return ALL_OK;
	}

	static ERet _create_Log_File(
		const NEngine::LaunchConfig& engine_launch_config
		)
	{
		String256	log_file_name;
		Str::Copy(log_file_name, engine_launch_config.OverrideLogFileName);

		if( log_file_name.IsEmpty() ) {
			F_Util_ComposeLogFileName( log_file_name );
		}

		gs_data->file_log.OpenLog( log_file_name.raw() );
		return ALL_OK;
	}

	static ERet _create_Console_Window()
	{
		gs_data->console_output.Create("N00bWerks Engine Debug Console");

		gs_data->console_log.Initialize();
		mxGetLog().Attach( &gs_data->console_log );

		//
		HWND console_window_handle = ::GetConsoleWindow();
		SetWindowPos(
			console_window_handle	// HWND hWnd
			, NULL	// hWndInsertAfter

			// move to the right
#if MX_DEBUG
			, 1024, 0, 1324, 1024
#else
			, 0, 0, 1024, 1024
#endif
			, // UINT uFlags
			//SWP_NOSIZE|
			SWP_NOZORDER	
			);

		return ALL_OK;
	}

	static ERet _initialize_Low_Level_Graphics(
		const NEngine::LaunchConfig& engine_launch_config
		)
	{
		//
		mxDO(NGraphics::Initialize(MemoryHeaps::graphics()));

		//
		NGpu::Settings graphics_settings;
		graphics_settings.window_handle = WindowsDriver::GetNativeWindowHandle();
		graphics_settings.create_debug_device = 0;//MX_DEBUG || MX_DEVELOPER;

		graphics_settings.device.use_fullscreen_mode = engine_launch_config.window.fullscreen;
		graphics_settings.device.enable_VSync = engine_launch_config.enable_VSync;

		graphics_settings.cmd_buf_size = mxMiB(engine_launch_config.Gfx_Cmd_Buf_Size_MiB);

		mxDO(NGpu::Initialize(graphics_settings));

		return ALL_OK;
	}
}



NwEngine	g_engine;

ERet NwEngine::Initialize(
	const NEngine::LaunchConfig & engine_launch_config
	)
{
	// Create the main instance of Remotery.
	// You need only do this once per program.
	if( RMT_ERROR_NONE != rmt_CreateGlobalInstance( &gs_remotery ) ) {
		return ERR_UNKNOWN_ERROR;
	}
	rmt_SetCurrentThreadName("MainThread");


	//
	Testbed::g_InitMemorySettings.temporaryHeapSizeMiB = engine_launch_config.TemporaryHeapSizeMiB;

	//
	Testbed::g_InitMemorySettings.debugHeapSizeMiB = engine_launch_config.memory_enable_debug_heap
		? engine_launch_config.memory_debug_heap_size
		: 0
		;

	//
	mxDO(mxInitializeBase());

	mxDO(_check_SSE_Support());

	mxDO(SetupCoreSubsystem());

	//
	mxDO(NwScriptingEngineI::Create(
		scripting._ptr
		, MemoryHeaps::scripting()
		));

	mxDO(Engine::ExposeToLua());

	//
	gs_data.ConstructInPlace();

	//
	if( engine_launch_config.bCreateLogFile ) {
		_create_Log_File(
			engine_launch_config
			);
	}

	//
	if( engine_launch_config.bCreateConsoleWindow ) {
		_create_Console_Window();
	}

	// NOTE: stdout log must be run after the win32 console log (which sets the proper colors)
	mxGetLog().Attach( &gs_data->stdout_log );

	//g_sleep_msec_if_not_in_focus = engine_launch_config.sleep_msec_if_not_in_focus;

	// Initialize background loader.
	mxDO(SlowTasks::Initialize(
		SlowTasks::Settings(),
		MemoryHeaps::backgroundQueue()
		));

	// Initialize job scheduler.
	{
		PtSystemInfo	systemInfo;
		mxGetSystemInfo(systemInfo);

		const bool hyperthreading = (systemInfo.cpu.numLogicalCores > systemInfo.cpu.numPhysicalCores);

		// account for the main thread and the background thread (for 'slow tasks')
		//const int numBackgroundThreads = 2;
		const int numBackgroundThreads = 1;	// 'slow tasks' thread is nearly always idle

		const int numWorkerThreadsToCreate = engine_launch_config.NumberOfWorkerThreads ?
			engine_launch_config.NumberOfWorkerThreads : (systemInfo.cpu.numPhysicalCores - numBackgroundThreads);
		//const int numWorkerThreadsToCreate =
		//	engine_launch_config.NumberOfWorkerThreads ?
		//	engine_launch_config.NumberOfWorkerThreads :
		//	(systemInfo.cpu.numPhysicalCores - numBackgroundThreads) + (hyperthreading ? 1 : 0)
		//	;

#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_MY_SHIT
		// Initialize parallel job manager.
		{
			TbJobManagerSettings jobManSettings;

			jobManSettings.numWorkerThreads = engine_launch_config.NumberOfWorkerThreads;
			if( !jobManSettings.numWorkerThreads ) {
				jobManSettings.CalculateOptimalValues(3);	// main, asyncIO, render
			}

			JobMan::Initialize( jobManSettings );
		}
#endif // ENGINE_CONFIG_USE_MY_JOB_SYSTEM

#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_ENKITS
		{
			// Set the callbacks BEFORE Initialize or we will get no threadstart nor first waitStart calls
			m_jobMan.GetProfilerCallbacks()->threadStart    = enki::threadStartCallback;
			m_jobMan.GetProfilerCallbacks()->waitStart      = enki::waitStartCallback;
			m_jobMan.GetProfilerCallbacks()->waitStop       = enki::waitStopCallback;

			PtSystemInfo	systemInfo;
			mxGetSystemInfo(systemInfo);

			// account for the main and background loading ('slow tasks') threads
			const UINT numWorkerThreadsToCreate = systemInfo.cpu.numPhysicalCores - 2;

			m_jobMan.Initialize( numWorkerThreadsToCreate );
		}
#endif

#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_JQ
		Jq2Start( numWorkerThreadsToCreate, mxMiB(engine_launch_config.worker_thread_workspace_size_MiB) );
#endif

		//mxZERO_OUT(g_job_sytem_stats);
	}

#if TB_CLIENT_WITH_NETWORK
	ENetCallbacks enet_callbacks;
	mxZERO_OUT(enet_callbacks);
	enet_callbacks.malloc = &enet_callback_malloc;
	enet_callbacks.free = &enet_callback_free;

	mxENSURE(enet_initialize_with_callbacks(ENET_VERSION, &enet_callbacks) == 0,
		ERR_UNKNOWN_ERROR, "enet_initialize_with_callbacks() failed");
	//mxENSURE(enet_initialize() == 0, ERR_UNKNOWN_ERROR, "enet_initialize() failed");

	gs_data->net_client = enet_host_create (NULL /* create a client host */,
				1 /* only allow 1 outgoing connection */,
				2 /* allow up 2 channels to be used, 0 and 1 */,
				57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
				14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);
	mxENSURE( gs_data->net_client != NULL, ERR_UNKNOWN_ERROR, "An error occurred while trying to create an ENet client host.");


	ENetAddress address;
	//address.host = ENET_HOST_ANY;
	enet_address_set_host (& address, "localhost");
	address.port = 1234;

	/* Initiate the connection, allocating the two channels 0 and 1. */
	gs_data->net_peer = enet_host_connect (gs_data->net_client, & address, 2, 0);
	mxENSURE( gs_data->net_peer != NULL, ERR_UNKNOWN_ERROR, "No available peers for initiating an ENet connection.");

	const enet_uint32 timeout_milliseconds = 0;
	ENetEvent event;
	if (enet_host_service (gs_data->net_client, & event, timeout_milliseconds) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		DBGOUT ("Connection to the server succeeded.");
	}
	else
	{
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset (gs_data->net_peer);
		ptWARN ("Connection to the server failed.");
	}
#endif

	gs_data->screenshots_folder = engine_launch_config.screenshots_folder;

	//
	{
		WindowsDriver::Settings	windows_drv_settings;
		windows_drv_settings.window = engine_launch_config.window;

		mxDO(WindowsDriver::Initialize(
			windows_drv_settings
			));
	}

	//
	mxDO(_initialize_Low_Level_Graphics( engine_launch_config ));


	return ALL_OK;
}

void NwEngine::Shutdown()
{
	//
	NGpu::Shutdown();
	NGraphics::Shutdown();

	//
	WindowsDriver::Shutdown();


#if TB_CLIENT_WITH_NETWORK
	if( gs_data->net_peer != NULL ) {
		enet_peer_disconnect(gs_data->net_peer, 0);
	}
	enet_host_destroy(gs_data->net_client);
	gs_data->net_client = NULL;
	enet_deinitialize();
#endif


	{
#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_MY_SHIT
		//if( JobMan::NumWorkerThreads() > 0 )
		{
			JobMan::Shutdown();
		}
#endif // ENGINE_CONFIG_USE_MY_JOB_SYSTEM


#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_ENKITS
		m_jobMan.WaitforAllAndShutdown();
#endif

#if ENGINE_CONFIG_JOB_SYSTEM_IMPL == ENGINE_CONFIG_JOB_SYSTEM_IMPL_JQ
		Jq2Stop();
#endif
	}

	//
	SlowTasks::Shutdown();

	//
	mxGetLog().Detach( &gs_data->console_log );
	mxGetLog().Detach( &gs_data->stdout_log );

	gs_data->console_output.DisableClose();

	gs_data->file_log.CloseLog();

	//
	gs_data.Destruct();

	//
	NwScriptingEngineI::Destroy(scripting);
	scripting = nil;

	ShutdownCoreSubsystem();

	mxShutdownBase();

	// Destroy the main instance of Remotery.
	rmt_DestroyGlobalInstance( gs_remotery );
	gs_remotery = nil;
}



#if 0
	void UpdateNetwork()
	{
#if TB_CLIENT_WITH_NETWORK
		ENetEvent net_event;
		while (enet_host_service (gs_data->net_client, & net_event, 0) > 0)
		{
			switch (net_event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				DBGOUT ("A new client connected from %x:%u.\n", 
					net_event.peer -> address.host,
					net_event.peer -> address.port);
				/* Store any relevant client information here. */
				net_event.peer -> data = "Client information";
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				DBGOUT ("A packet of length %u containing %s was received from %s on channel %u.\n",
					net_event.packet -> dataLength,
					net_event.packet -> data,
					net_event.peer -> data,
					net_event.channelID);
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy (net_event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				DBGOUT ("%s disconnected.\n", net_event.peer -> data);
				/* Reset the peer's client information. */
				net_event.peer -> data = NULL;
			}
		}
#endif // TB_CLIENT_WITH_NETWORK
	}

	//void setGamePaused( bool pause_game )
	//{
	//	gs_data->is_game_paused = pause_game;
	//}

	//bool isGamePaused()
	//{
	//	return gs_data->is_game_paused;
	//}

	//void SaveScreenShot( EImageFileFormat::Enum _format )
	//{
	//	String64	szCurrDateTime;
	//	GetDateTimeOfDayString( szCurrDateTime );

	//	String128	szScreenShotFilePath;
	//	Str::ComposeFilePath(
	//		szScreenShotFilePath,
	//		gs_data->screenshots_folder.c_str(), szCurrDateTime.c_str(), EImageFileFormat::GetFileExtension( _format )
	//	);

	//	NGpu::SaveScreenshot( szScreenShotFilePath.c_str(), _format );
	//}

#endif



NwSimpleGameLoop::NwSimpleGameLoop()
{
	_real_time = 0;
	_prev_real_time = 0;
	_delta_real_time = 0;

	_frame_number = 0;

	//_game_time_residual = 0;
}

/*
On Game Loops:
https://gafferongames.com/post/fix_your_timestep/
https://gamedev.stackexchange.com/questions/651/how-should-i-write-a-main-game-loop
https://gameprogrammingpatterns.com/game-loop.html
https://dewitters.com/dewitters-gameloop/

Fixed-Time-Step Implementation
By L. Spiro • March 31, 2012 • General, Tutorial • 64 Comments 
http://lspiroengine.com/?p=378
*/
void NwSimpleGameLoop::update(
							NwAppI* game
							)
{
	if( G_isExitRequested() ) {
		WindowsDriver::requestExit();
		return;
	}

	//const Microseconds64T TARGET_FRAME_TIME = (1.0 / 30.0) * 1e6f;
	const Microseconds64T FIXED_TIME_STEP = (1.0 / 30.0) * 1e6f;

	//
	const Microseconds64T real_time_now = mxGetTimeInMicroseconds();
	const Microseconds64T real_delta_time = real_time_now - _prev_real_time;
	_prev_real_time = real_time_now;
	_delta_real_time = real_delta_time;



	//
	//runStuffOncePerFrame();
	SlowTasks::Tick();

	//Engine::UpdateNetwork();

	EventSystem::processEvents();





	//
	// From "Game Engine Architecture" [2009], "7.5. Measuring and Dealing with Time", P.321:
	// If dt is too large, we must have resumed from a break point.
	const Microseconds64T MAX_FRAME_TIME = 100 * 1000;	// 0.1 seconds

	const Microseconds64T clamped_delta_time = Min( real_delta_time, MAX_FRAME_TIME );


	//
	const double total_elapsed_time_seconds = real_time_now * 1e-6f;
	const double delta_time_seconds = real_delta_time * 1e-6f;
	const double clamped_delta_time_seconds = clamped_delta_time * 1e-6f;



	//
	NwTime	game_time_args;
	game_time_args.real.delta_seconds = clamped_delta_time_seconds;
	game_time_args.real.total_msec = real_time_now / 1000;
	game_time_args.real.total_elapsed = total_elapsed_time_seconds;
	game_time_args.real.frame_number = _frame_number++;


	// note: max frame time to avoid spiral of death:
	// if there was a large gap in time since the last frame, or the frame
	// rate is very very low, limit the number of frames we will run

	const Microseconds64T game_frame_time = Min( real_delta_time, MAX_FRAME_TIME );

	//_game_time_residual += game_frame_time;

	//while( _game_time_residual >= FIXED_TIME_STEP )
	//{
	//	//game->updateWithFixedTimestep( FIXED_TIME_STEP );
	//	_game_time_residual -= FIXED_TIME_STEP;
	//}

	//
	g_engine.scripting->TickOncePerFrame(game_time_args);



	//
	//const float visual_interpolation_factor = (float) ( (double)_game_time_residual / (double)FIXED_TIME_STEP );
	//State state = currentState * visual_interpolation_factor
	//			  + previousState * ( 1.0 - visual_interpolation_factor );

	//
	game->RunFrame( game_time_args );

	//
	NGpu::NextFrame();

	//
	//Jq2ConsumeStats( &g_job_sytem_stats );

#if ENGINE_CONFIG_USE_IG_MEMTRACE
	MemTrace::Flush();
#endif // ENGINE_CONFIG_USE_IG_MEMTRACE


	//
//	YieldToOSAndSleep();


	//if( WindowsDriver::HasFocus() )
	//{
	//	/// Yield execution to another thread.
	//	/// Offers the operating system the opportunity to schedule another thread
	//	/// that is ready to run on the current processor.
	//	YieldSoftwareThread();
	//}
	//else
	//{
	//	YieldToOSAndSleep();	// sleep for 1 msec
	//}

	mxPROFILE_INCREMENT_FRAME_COUNTER;
}
