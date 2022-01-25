// high-level engine interface
#pragma once

#include <Core/Core.h>
#include <Core/Event.h>

#include <Engine/WindowsDriver.h>
#include <Engine/ForwardDecls.h>


namespace NEngine
{
///
/// This is a temporary, short-lived structure used for initializing the engine.
/// It's typically loaded from a human-readable config file
/// and discarded after initialization.
///
struct LaunchConfig : CStruct
{
	//=== MEMORY MANAGEMENT =====================

	// The size of the thread-safe ring buffer allocator that can be used for passing data between threads.
	// Set to 0 to disable the global locking heap for temporary allocations.
	U32		TemporaryHeapSizeMiB;

public:	//=== DEVELOPER =====================
	/// Set to 1 to use a debug memory heap for all allocations to break on memory access errors.
	/// This memory heap is implemented via Insomniac Games' DebugHeap.
	/// NOTE: this heap is terribly slow, and wastes tons of memory.
	bool	memory_enable_debug_heap;

	/// the size of the DebugHeap, in megabytes (max 512 MiB on 32-bit)
	U32		memory_debug_heap_size;





	bool	bCreateLogFile;
	String32	OverrideLogFileName;



public:	//=== THREADING =====================

	/// the number of threads that may be used for parallel task execution
	/// 0 = auto, max = 8, high number doesn't make sense because of contention
	int		NumberOfWorkerThreads;

	/// size of local memory private to each worker thread;
	/// thread-local heaps are used to avoid manipulating the global heap
	U32		worker_thread_workspace_size_MiB;



	/// only under Windows OS
	bool	bCreateConsoleWindow;

	int		console_buffer_size;	// 0 = don't create console

	//=== APPLICATION =====================

	/// the assets directory containing compiled asset data
	String64	path_to_compiled_assets;

	//String64	path_to_app_cache;	//!< cached compile data blobs (e.g. BSP trees)

	// rendering
	NwWindowSettings	window;

	bool		enable_VSync;
	String64	screenshots_folder;

	// game state
	String64	path_to_save_file;

	int	sleep_msec_if_not_in_focus;	//!< 0 == don't sleep

	//=== RENDERER =====================

	bool	create_debug_render_device;
	U32	Gfx_Cmd_Buf_Size_MiB;

	//=== GAME UI =====================
	//U32		game_UI_mem;

public:
	mxDECLARE_CLASS(LaunchConfig, CStruct);
	mxDECLARE_REFLECTION;

	LaunchConfig();

	void SetDefaultsForDebug();
	void SetDefaultsForRelease();

	ERet LoadFromFile();

	void Clamp_To_Reasonable_Values();

private:
	void _SetDefaultsCommon();
};

}//namespace NEngine



class NwEngine: TSingleInstance<NwEngine>
{
public:
	TPtr<NwScriptingEngineI>	scripting;


public:
	///
	ERet Initialize(
		const NEngine::LaunchConfig & engine_launch_config = NEngine::LaunchConfig ()
		);

	///
	void Shutdown();

	//bool isExitRequested();

	////
	//void setGamePaused( bool pause_game );
	//bool isGamePaused();

	//void SaveScreenShot( EImageFileFormat::Enum _format );

};


//
extern NwEngine	g_engine;


///
class NwSimpleGameLoop: public NwGameLoopI
{
	// in microseconds
	Microseconds64T	_real_time;
	Microseconds64T	_prev_real_time;
	Microseconds64T	_delta_real_time;

	U64				_frame_number;

	// left over time from the last game frame
	//Microseconds64T	_game_time_residual;

public:
	NwSimpleGameLoop();

	virtual void update(
		NwAppI* game
		) override;
};
