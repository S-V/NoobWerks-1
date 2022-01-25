/*
=============================================================================
	File:	Core.h
	Desc:	
=============================================================================
*/
#pragma once

class ConfigFile;
class FileSystem;
class AssetCache;
class AsyncLoader;
class AssetPackage;
class AssetInfo;

class NwObjectList;
class NwClump;
class Entity;
class EntityComponent;

class AEditorInterface;

ERet SetupCoreSubsystem();
void ShutdownCoreSubsystem();
bool CoreSubsystemIsOpen();

struct DependsOn_Core : DependsOn_Base {
	DependsOn_Core() { SetupCoreSubsystem(); }
	~DependsOn_Core() { ShutdownCoreSubsystem(); }
};


/*
=============================================================================
	Core global variables
=============================================================================
*/
namespace Testbed
{
	struct Time
	{
		U64	real_time;

		/// Virtual time helps with pausing the game
		U64	virtual_time;
	};

	///
	U64	realTime();

	/// time in microsec of last frame
	U64	realTimeElapsed();

	/// Virtual time helps with pausing the game
	U64	gameTime();

	U64	gameTimeElapsed();


	/// Updates time and processes queued events.
	/// If 'update_game_time' is true, game ('virtual') time is updated as well.
	void update( bool update_game_time );

}//namespace Testbed

mxFORCEINLINE float microsecondsToSeconds( U64 microseconds ) {
	return microseconds * 1e-6f;
}


/*
--------------------------------------------------------------
	single point of access to all core globals
--------------------------------------------------------------
*/
namespace Testbed
{
	//STimeStamp		startTime;	// time of engine initialization

	// ultra-fast stack-like allocator for short-lived (scratch) per-frame memory on the main thread
	//LinearAllocator	frameAlloc;

	//String256	basePath;
}//namespace Testbed

//--------------------------------------------------------------//
//	Reflection.
//--------------------------------------------------------------//

mxDECLARE_STRUCT(FileTimeT);

/*
--------------------------------------------------------------
	AConfigFile
--------------------------------------------------------------
*/
struct AConfigFile
{
	virtual const char* FindString( const char* _key ) const = 0;
	virtual bool FindInteger( const char* _key, int &_value ) const = 0;
	virtual bool FindSingle( const char* _key, float &_value ) const = 0;
	virtual bool FindBoolean( const char* _key, bool &_value ) const = 0;

	bool GetInteger( const char* _key, int &_value, int min = MIN_INT32, int max = MAX_INT32 ) const;
	bool GetSingle( const char* _key, float &_value, float min = MIN_FLOAT32, float max = MAX_FLOAT32 ) const;
	bool GetBoolean( const char* _key, bool &_value ) const;

public:
	// returns the default string if the key doesn't exist
	const char* GetString( const char* _key, const char* _default = "" ) const;
	// doesn't touch the string if the key doesn't exist
	bool GetStringOptional( const char* key, String &_value ) const;

protected:
	virtual ~AConfigFile() {}
};

enum ENamedThreadIDs
{
	MAIN_THREAD_INDEX = 0,
	ASYNC_IO_THREAD_INDEX,
	RENDER_THREAD_INDEX,
	FIRST_WORKER_THREAD_INDEX,
};



/// controlled by GUI
IF_DEBUG extern bool g_DebugBreakEnabled;



mxREMOVE_THIS
#if 0


/*
--------------------------------------------------------------
	ANetwork
--------------------------------------------------------------
*/
struct ANetwork
{
	virtual void TickFrame( FLOAT deltaSeconds ) = 0;

protected:
	virtual ~ANetwork() {}
};

void GetRootFolder( String & _basePath );
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
