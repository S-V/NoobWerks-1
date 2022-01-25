#include "stdafx.h"
#pragma hdrstop
#include <bx/bx.h>
#include <bx/commandline.h>
#pragma comment( lib, "bx.lib" )
#pragma comment( lib, "psapi.lib" )	// GetProcessMemoryInfo()

#include <Base/Util/LogUtil.h>
#include <Base/Util/PathUtils.h>
#include <Base/Text/TextUtils.h>
#include <Base/Memory/Allocators/FixedBufferAllocator.h>
#include <Engine/Windows/ConsoleWindow.h>

#include <Engine/Engine.h>

#include <Developer/EditorSupport/FileWatcher.h>
#include <Developer/EditorSupport/DevAssetFolder.h>

#include <Core/Serialization/Text/TxTSerializers.h>
#include <Utility/DemoFramework/DemoFramework.h>
#include <Core/Serialization/Text/TxTConfig.h>

#include <AssetCompiler/AssetPipeline.h>
#include <AssetCompiler/AssetCompilers/Doom3/Doom3_AssetUtil.h>






//#define DBG_SRC_ASSET_FILENAME	"D:/dev/___FPS_Game/models/weapons/plasmagun_view/weapon_plasmagun.def"

// weapon_fists
//#define DBG_SRC_ASSET_FILENAME	"D:/dev/___FPS_Game/models/weapons/fists_view/weapon_fists.def"




// fix for Assimp linking error:
// error LNK2001: unresolved external symbol "void __cdecl boost::throw_exception(class std::exception const &)" (?throw_exception@boost@@YAXAEBVexception@std@@@Z)
// https://stackoverflow.com/a/33691561/4232223

#if ( _MSC_VER < 1600 ) // if earlier than Microsoft Visual C++ 11.0	// 2012

namespace boost
{
#ifdef BOOST_NO_EXCEPTIONS
void throw_exception( std::exception const & e ){
    throw 11; // or whatever
};
#endif
}// namespace boost

#endif






bool g_debug_rebuildAllAssets = 
	false
	//::IsDebuggerPresent()
	;

void showHelp(const char* _error = NULL)
{
	if (NULL != _error)
	{
		fprintf(stderr, "Error:\n%s\n\n", _error);
	}
mxTEMP
	fprintf(stderr
		, "shaderc, bgfx shader compiler tool\n"
		  "Copyright 2011-2015 Branimir Karadzic. All rights reserved.\n"
		  "License: http://www.opensource.org/licenses/BSD-2-Clause\n\n"
		);

	fprintf(stderr
		, "Usage: shaderc -f <in> -o <out> --type <v/f> --platform <platform>\n"

		  "\n"
		  "Options:\n"
		  "  -f <file path>                Input file path.\n"
		  "  -i <include path>             Include path (for multiple paths use semicolon).\n"
		  "  -o <file path>                Output file path.\n"
		  "      --bin2c <file path>       Generate C header file.\n"
		  "      --depends                 Generate makefile style depends file.\n"
		  "      --platform <platform>     Target platform.\n"
		  "           android\n"
		  "           asm.js\n"
		  "           ios\n"
		  "           linux\n"
		  "           nacl\n"
		  "           osx\n"
		  "           windows\n"
		  "      --preprocess              Preprocess only.\n"
		  "      --raw                     Do not process shader. No preprocessor, and no glsl-optimizer (GLSL only).\n"
		  "      --type <type>             Shader type (vertex, fragment)\n"
		  "      --varyingdef <file path>  Path to varying.def.sc file.\n"
		  "      --verbose                 Verbose.\n"

		  "\n"
		  "Options (DX9 and DX11 only):\n"

		  "\n"
		  "      --debug                   Debug information.\n"
		  "      --disasm                  Disassemble compiled shader.\n"
		  "  -p, --profile <profile>       Shader model (f.e. ps_3_0).\n"
		  "  -O <level>                    Optimization level (0, 1, 2, 3).\n"
		  "      --Werror                  Treat warnings as errors.\n"

		  "\n"
		  "For additional information, see https://github.com/bkaradzic/bgfx\n"
		);
}

struct AppConfig : AssetBaking::AssetPipelineConfig
{
	/// Monitor file changes and automatically rebuild changed assets?
	bool		monitor_file_changes;

	bool		use_debug_memory_heap;

	//String64	pathToLastOpenedProject;
	//bool		bLoadLastProjectOnStartup;
	//int			updateIntervalMilliseconds;
	//String64	srcPathToLastSelectedAsset;
	//String64	dstPathToLastSelectedAsset;
public:
	mxDECLARE_CLASS( AppConfig, AssetBaking::AssetPipelineConfig );
	mxDECLARE_REFLECTION;
	AppConfig();
};

mxDEFINE_CLASS( AppConfig );
mxBEGIN_REFLECTION( AppConfig )
	mxMEMBER_FIELD( monitor_file_changes ),
	mxMEMBER_FIELD( use_debug_memory_heap ),
	//mxMEMBER_FIELD( pathToLastOpenedProject ),
	//mxMEMBER_FIELD( bLoadLastProjectOnStartup ),
	//mxMEMBER_FIELD( updateIntervalMilliseconds ),
	//mxMEMBER_FIELD( srcPathToLastSelectedAsset ),
	//mxMEMBER_FIELD( dstPathToLastSelectedAsset ),
mxEND_REFLECTION
AppConfig::AppConfig()
{
	monitor_file_changes = false;
	use_debug_memory_heap = false;
}

ERet GetAssetDatabaseFilePath( const char* folder, String &pathToAssetDatabase )
{
	Str::CopyS(pathToAssetDatabase, folder );
	Str::NormalizePath( pathToAssetDatabase );
	Str::AppendS(pathToAssetDatabase, "AssetDatabase.son");
	return ALL_OK;
}

static
ERet loadConfigFromFile( AppConfig &config_ )
{
	FilePathStringT	path_to_config;
	mxDO(SON::GetPathToConfigFile( "asset_compiler.son", path_to_config ));

	mxBUG("memory corruption when TempAllocator4096 attempts to free mem in dtor")
	//TempAllocator4096	temp_allocator( &MemoryHeaps::process() );
	mxDO(SON::LoadFromFile( path_to_config.c_str(), config_, MemoryHeaps::process() ));

	config_.fixPathsAfterLoading();

	return ALL_OK;
}

class App
	: public AFileWatchListener
	, public ADirectoryWalker
	, DependsOn_Core
{
	const AppConfig &	_config;

	MyFileWatcher					m_fileWatcher;

	AssetDatabase					m_assetDatabase;
	AssetBaking::AssetProcessor		_asset_processor;	

#if WITH_NETWORK
	TPtr< ENetHost >	m_server;
#endif

public:
	App( const AppConfig& config )
		: _config( config )
	{
	}

	~App()
	{
#if WITH_NETWORK
		enet_host_destroy(m_server);
		m_server = NULL;

		enet_deinitialize();
#endif
		m_fileWatcher.Shutdown();

		_asset_processor.Shutdown();

		m_assetDatabase.Close();
	}

	//void MakePlatformPath()
	//{

	//}

	ERet Main( const bx::CommandLine& cmdLine )
	{
		//Str::StripTrailingSlashes( _config.output_path );
		//if( mxARCH_TYPE == mxARCH_64BIT ) {
		//	Str::Append( _config.output_path, "-x64/" );
		//} else {
		//	Str::Append( _config.output_path, "-x86/" );
		//}


		// make sure the destination directory exists
		mxDO(OS::IO::MakeDirectoryIfDoesntExist2( _config.output_path.c_str() ));

		if( OS::IO::FileExists(_config.asset_database.c_str()) )
		{
			m_assetDatabase.OpenExisting(_config.asset_database.c_str());
		}

		if( !m_assetDatabase.IsOpen() )
		{
			mxDO(m_assetDatabase.CreateNew(_config.asset_database.c_str()));
			//mxDO(m_assetDatabase.SaveToFile(_config.asset_database.c_str()));
		}


		mxDO(m_fileWatcher.Initialize());

		mxDO(_asset_processor.Initialize(_config));

		//
		Doom3_AssetBundles	doom3_asset_bundles( MemoryHeaps::global() );
		mxDO(doom3_asset_bundles.initialize(m_assetDatabase));







		//
		const char*	source_file_path;
		if( cmdLine.hasArg( source_file_path, 'f', "file" ) )
		{
			_CompileSingleSourceFile(source_file_path);
			return ALL_OK;
		}




#ifdef DBG_SRC_ASSET_FILENAME
_CompileSingleSourceFile(
	 //"D:/dev/___FPS_Game/materials/characters.mtr"
	 	//"R:/test.mtr"
		DBG_SRC_ASSET_FILENAME

	//"R:/NoobWerks/Source/Engine/Runtime/Rendering/Private/Shaders/HLSL/Model/doom3_mesh.fx"
	//"D:/dev/___FPS_Game/models/weapons/rocketlauncher_view/weapon_rocketlauncher.NwWeaponDef"

	 );
return ALL_OK;
#endif







		// Sets the input directory.
		//const char* sourcePath = cmdLine.findOption('i', "input");
		// Sets the output directory.
		//const char* outputPath = cmdLine.findOption('o', "output");

		// Performs a non incremental build.
		bool rebuildAllAssets = cmdLine.hasArg('r', "rebuild");
	
if( g_debug_rebuildAllAssets )
	rebuildAllAssets = true;
	
		if( rebuildAllAssets )
		{
			ptPRINT("Performing a non incremental build...");
			m_assetDatabase.clearDB();
		}

		mxDO(_asset_processor.BuildAssets(
			_config,
			m_assetDatabase
			));
		//mxDO(m_assetDatabase.SaveToFile(pathToAssetDatabase.c_str()));

		if( _config.monitor_file_changes )
		{
			for( UINT i = 0; i < _config.source_paths.num(); i++ )
			{
				m_fileWatcher.AddFileWatcher(
					_config.source_paths[i].c_str()
					, true /*recursive*/
					);
			}

			//DON'T ADD TO WATCHER, BECAUSE FILES ARE REPEATED
			//// automatically rebuild modified shaders
			//for( int i = 0; i < _config.shader_compiler.search_paths.num(); i++ )
			//{
			//	m_fileWatcher.AddFileWatcher(
			//		_config.shader_compiler.search_paths[i].c_str()
			//		, false/*recursive*/
			//	);
			//}
		}

		m_fileWatcher.StartWatching();

		while( true )
		{
			if( _config.monitor_file_changes )
			{
				m_fileWatcher.ProcessNotifications( this );
			}

			mxSleepMilliseconds(500);
		}

		m_assetDatabase.Close();

		return ALL_OK;
	}

	virtual void ProcessChangedFile( const efswData& _data ) override
	{
		const char* extension = Str::FindExtensionS( _data.filename.c_str() );
		if( !extension || (strlen( extension ) == 0)
			|| (stricmp( extension, "tmp") == 0)
			)
		{
			return;
		}

		//DBGOUT("Folder: '%s', FileName: '%s', Action: '%s', OldFileName: '%s'",
		//	_data.folder.c_str(), _data.filename.c_str(), Action_To_Chars(_data.action), _data.oldFilename.c_str());

		if( _data.action == efsw::Action::Modified )
		{
			FilePathStringT sourceFolder;
			Str::CopyS( sourceFolder, _data.folder.c_str() );
			Str::NormalizePath( sourceFolder );

			FilePathStringT sourceFileName;
			Str::CopyS( sourceFileName, _data.filename.c_str() );

			FilePathStringT sourceFilePath;
			Str::Copy( sourceFilePath, sourceFolder );
			Str::Append( sourceFilePath, sourceFileName );
			Str::FixBackSlashes( sourceFilePath );

			_asset_processor.ProcessChangedFile(
				sourceFilePath.c_str()
				, _config
				, m_assetDatabase
				);
		}
	}

	// return true to exit immediately
	virtual bool Found_File( const SFindFileInfo& file ) override
	{
		_asset_processor.ProcessChangedFile(
			file.fullFileName.c_str()
			, _config
			, m_assetDatabase
			);
		return false;
	}

private:
	void _CompileSingleSourceFile(const char* source_file_path)
	{
		_asset_processor.CompileAsset2(
			source_file_path
			, _config
			, m_assetDatabase
			);
	}
};


int main( int _argc, const char* _argv[] )
{
	bx::CommandLine cmdLine( _argc, _argv );

	// Displays usage information.
	if( cmdLine.hasArg('h', "help") ) {
		showHelp();
		return EXIT_FAILURE;
	}

	//SetupMemoryHeapsUtil	setupMemoryHeaps;
	//SetupScratchHeap		setupScratchHeap( MemoryHeaps::global(), 64 );

	CConsole		console;
	WinConsoleLog	console_log;
	StdOut_Log		stdout_log;

	console_log.Initialize();
	// NOTE: stdout log must be run after the win32 console log (which sets the proper colors)
	mxGetLog().Attach(&stdout_log);

	FileLogUtil		file_log_util;
	file_log_util.OpenLog("AssetCompiler.log");

	//
	NwSetupMemorySystem	setupMemory;

	//
	AppConfig	config;
	mxDO(loadConfigFromFile( config ));

	//
	Testbed::g_InitMemorySettings.debugHeapSizeMiB = config.use_debug_memory_heap ? 1024 : 0;

	//
	NwGlobals::canLoadAssets = false;

	//
	DBGOUT("Cmd Line Args:");
	nwFOR_LOOP(int, i, _argc) {
		DBGOUT("[%d]: %s", i, _argv[i]);
	}


	//
	App	app( config );
	app.Main( cmdLine );

	mxGetLog().Detach(&stdout_log);

	return EXIT_SUCCESS;
}
