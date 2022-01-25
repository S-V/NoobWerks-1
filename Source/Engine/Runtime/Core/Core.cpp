/*
=============================================================================
	File:	Core.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/Event.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
//#include <Core/Kernel.h>
//#include <Core/IO/IOSystem.h>
//#include <Core/Resources.h>
//#include <Core/System/StringTable.h>
//#include <Core/Serialization/Serialization.h>
//#include <Core/Entity/System.h>
#include <Core/Text/NameTable.h>
//#include <Core/Util/Tweakable.h>
#include <Core/Util/Tweakable.h>
#include <Input/Input.h>
#include <Core/Client.h>
#include <Core/ObjectModel/Clump.h>

IF_DEBUG bool g_DebugBreakEnabled = false;

bool AConfigFile::GetInteger( const char* _key, int &_value, int min, int max ) const
{
	bool found = this->FindInteger(_key, _value);
	if( found )
	{
		if( _value < min || _value > max ) {
			ptWARN("'%s'=%d should be in range [%d..%d]\n", _key, _value, min, max);
			_value = Clamp(_value, min, max);
		}
		return true;
	}
	else {
		ptWARN("Couldn't find integer '%s'\n", _key);
	}
	return false;
}
bool AConfigFile::GetSingle( const char* _key, float &_value, float min, float max ) const
{
	bool found = this->FindSingle(_key, _value);
	if( found )
	{
		if( _value < min || _value > max ) {
			ptWARN("'%s'=%d should be in range [%f..%f]\n", _key, _value, min, max);
			_value = Clamp(_value, min, max);
		}
		return true;
	}
	else {
		ptWARN("Couldn't find single '%s'\n", _key);
	}
	return false;
}
bool AConfigFile::GetBoolean( const char* _key, bool &_value ) const
{
	bool found = this->FindBoolean(_key, _value);
	if( !found ) {
		ptWARN("Couldn't find boolean '%s'\n", _key);
	}
	return found;
}

const char* AConfigFile::GetString( const char* _key, const char* _default ) const
{
	const char* existing = this->FindString(_key);
	return existing ? existing : _default;
}
bool AConfigFile::GetStringOptional( const char* _key, String &_value ) const
{
	const char* existing = this->FindString(_key);
	if( existing ) {
		Str::CopyS( _value, existing );
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------

static NiftyCounter	gs_CoreSystemRefCounter;


namespace Testbed
{
	namespace
	{
		ERet registerResourceLoaders( ProxyAllocator & allocator )
		{
			NwClump::metaClass().loader = mxNEW( allocator, ClumpLoader, allocator );

			NwInputBindings::metaClass().loader = Resources::memoryImageAssetLoader();

			return ALL_OK;
		}

		void tearDownResourceLoaders( ProxyAllocator & allocator )
		{
			NwInputBindings::metaClass().loader = nil;

			mxDELETE( NwClump::metaClass().loader, allocator );
			NwClump::metaClass().loader = nil;
		}
	}
}//namespace Testbed


ERet SetupCoreSubsystem()
{
	if( gs_CoreSystemRefCounter.IncRef() )
	{
		mxInitializeBase();

		mxDO(MemoryHeaps::initialize());

		NameID::StaticInitialize();

#if MX_DEVELOPER
		{
			ANSICHAR	timeStampStr[ 512 ];
			STimeStamp::GetCurrent().ToString( timeStampStr, mxCOUNT_OF(timeStampStr) );
//			gCore.startTime.ToPtr( timeStampStr, mxCOUNT_OF(timeStampStr) );
			DBGOUT("Timestamp: %s\n", timeStampStr );
		}
		{
			DevBuildOptionsList	buildOptions;

			if( MX_DEBUG ) {
				buildOptions.add("MX_DEBUG");
			}
			if( MX_DEVELOPER ) {
				buildOptions.add("MX_DEVELOPER");
			}
			if( MX_ENABLE_PROFILING ) {
				buildOptions.add("MX_ENABLE_PROFILING");
			}
			if( MX_ENABLE_REFLECTION ) {
				buildOptions.add("MX_ENABLE_REFLECTION");
			}
			if( MX_EDITOR ) {
				buildOptions.add("MX_EDITOR");
			}
			if( MX_DEMO_BUILD ) {
				buildOptions.add("MX_DEMO_BUILD");
			}

			ANSICHAR	buildOptionsStr[ 1024 ];
			buildOptions.ToChars( buildOptionsStr, mxCOUNT_OF(buildOptionsStr) );

			DBGOUT("Core library build settings:\n%s\n", buildOptionsStr);
		}
#endif // MX_DEVELOPER
 

//#if MX_EDITOR
//		mxASSERT2( gCore.editorApp.IsValid(), "Editor system must be initialized" );
//#endif // MX_EDITOR

		// Initialize Object system.

		//STATIC_IN_PLACE_CTOR_X( gCore.objectSystem, mxObjectSystem );



//#if MX_EDITOR
//		gCore.tweaks.ConstructInPlace();
//#endif // MX_EDITOR

		EventSystem::Initialize();

		// Initialize Input/Output system.
		//gCore.ioSystem.ConstructInPlace();

		//foundation::memory_globals::init();

		// Initialize frame allocator.
#if 0
		{
			int frameMemorySizeMb = 32;
			const int frameMemorySizeBytes = frameMemorySizeMb * mxMEBIBYTE;
			void* frameMemory = mxALLOC( frameMemorySizeBytes );
			mxDO(gCore.frameAlloc.Initialize( frameMemory, frameMemorySizeBytes ));
		}
#endif

		DBGOUT("sizeof(NwClump) = %d\n", sizeof(NwClump) );

		// Initialize the resource system.

#if 0
		mxHACK("auto initialization doesn't work:")
		{
			const AssetType* asset_type = Clump::StaticAssetType();
			TbMetaClass & clump_class = Clump::metaClass();
			clump_class.ass = asset_type;
		}
#endif

		mxDO(Resources::Initialize());

		mxDO(Testbed::registerResourceLoaders( MemoryHeaps::resources() ));
		
mxTEMP
#if 0
		// Setup asset loaders.
		{
			Resources::Meta& callbacks = Resources::gs_metaTypes[AssetTypes::CLUMP];
			callbacks.Allocate = &Clump::Z_New;
			callbacks.loadData = &Clump::Z_Load;
			callbacks.finalize = &Clump::Z_Online;
			callbacks.bringOut = &Clump::Z_Offline;
			callbacks.freeData = &Clump::Z_Destruct;
		}
#endif
#if MX_DEVELOPER
		Tweaking::Setup();
#endif // MX_EDITOR
	}
	return ALL_OK;
}
//---------------------------------------------------------------------------
void ShutdownCoreSubsystem()
{
	if( gs_CoreSystemRefCounter.DecRef() )
	{
		//DBGOUT("Shutting Down Core...\n");

		Testbed::tearDownResourceLoaders( MemoryHeaps::resources() );

		// Shutdown resource system.
		Resources::Shutdown();

		// Shutdown frame allocator.
#if 0
		{
			mxFREE( gCore.frameAlloc.GetBufferPtr() );
			gCore.frameAlloc.Shutdown();
		}
#endif

		//foundation::memory_globals::shutdown();

		//EntitySystem_Close();

		// Shutdown parallel job manager.
		{
//			Async::Shutdown();
		}

#if MX_DEVELOPER
		Tweaking::Close();
#endif // MX_EDITOR

		EventSystem::Shutdown();

		//Kernel::Shutdown();

		NameID::StaticShutdown();

		MemoryHeaps::shutdown();

		mxShutdownBase();
	}
}

bool CoreSubsystemIsOpen()
{
	return gs_CoreSystemRefCounter.IsOpen();
}

//--------------------------------------------------------------//
//	Reflection.
//--------------------------------------------------------------//

mxBEGIN_STRUCT(FileTimeT)
	mxMEMBER_FIELD_OF_TYPE(time, T_DeduceTypeInfo<U64>()),
mxEND_REFLECTION


namespace Testbed
{
	// Fixed-Time-Step Implementation
	// By L. Spiro • March 31, 2012 • General, Tutorial
	// http://lspiroengine.com/?p=378

	static U64	gs_real_time;
	static U64	gs_prev_real_time;
	static U64	gs_delta_real_time;

	static U64	gs_virtual_time;
	static U64	gs_prev_virtual_time;
	static U64	gs_delta_game_time;

	U64	realTime()
	{
		return gs_real_time;
	}

	U64	realTimeElapsed()
	{
		return gs_delta_real_time;
	}

	/// Virtual time helps with pausing the game
	U64	gameTime()
	{
		return gs_virtual_time;
	}

	U64	gameTimeElapsed()
	{
		return gs_delta_game_time;
	}

	/// If update_virtual_time is true, virtual values are updated as well.
	void update( bool update_virtual_time )
	{
		const U64 real_time_now = mxGetTimeInMicroseconds();
		const U64 real_time_diff = real_time_now - gs_prev_real_time;
		gs_prev_real_time = real_time_now;
		gs_delta_real_time = real_time_diff;

		// Wrapping handled implicitly.

		if( update_virtual_time )
		{
			gs_virtual_time += gs_delta_real_time;
			gs_delta_game_time = gs_virtual_time - gs_prev_virtual_time;
			gs_prev_virtual_time = gs_virtual_time;
		}
		else
		{
			gs_delta_game_time = 0;
		}

		EventSystem::processEvents();
	}

}//namespace Testbed
