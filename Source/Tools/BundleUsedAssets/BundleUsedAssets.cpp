#include <Base/Base.h>
#pragma hdrstop
#include <Base/Util/LogUtil.h>
#include <Base/Util/PathUtils.h>
#include <Base/Text/TextUtils.h>
#include <Base/Memory/Allocators/FixedBufferAllocator.h>

#include <Core/Memory/MemoryHeaps.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include <Base/Memory/DefaultHeap/DefaultHeap.h>
#include <Engine/Windows/ConsoleWindow.h>

//
#include <bx/bx.h>
#include <bx/commandline.h>
#pragma comment( lib, "bx.lib" )
#pragma comment( lib, "psapi.lib" )	// GetProcessMemoryInfo()


#include "BundleUsedAssets.h"


void showHelp()
{
	fprintf(stderr
		, "Planet heightmap compiler tool/n"
		);
}

ERet compileHeightmapFromBMP(
					  const char* destination_filepath
					  , const char* source_texture_filepath
					  , AllocatorI & allocator
					  , const bool dbg_dump_generated_mipmaps = false
					  )
{
	FileReader	file;
	//mxENSURE(
	//	file.Open( source_texture_filepath ),
	//	ERR_FAILED_TO_OPEN_FILE,
	//	"failed to open '%s'",
	//	source_texture_filepath
	//	);
	mxDO(file.Open( source_texture_filepath ));

	return ALL_OK;
}

int main( int _argc, const char* _argv[] )
{
	bx::CommandLine cmdLine( _argc, _argv );

	// Displays usage information.
	if( cmdLine.hasArg('h', "help") ) {
		showHelp();
		return EXIT_FAILURE;
	}

	//
	CConsole		console;
	WinConsoleLog	console_log;
	StdOut_Log		stdout_log;

	console_log.Initialize();
	// NOTE: stdout log must be run after the win32 console log (which sets the proper colors)
	mxGetLog().Attach(&stdout_log);

	//FileLogUtil		file_log_util;
	//file_log_util.OpenLog("HeightmapCompiler.log");

	//
	NwSetupMemorySystem	setupMemory;

	DefaultHeap	allocator;
	DependsOn_Core	setupCore;




	//
	Resources::CopyUsedAssetsParams	params;
	params.used_assets_manifest_file = "R:/used_assets.son";
	params.src_assets_folder = "R:/NoobWerks/.Build/Assets/";
	params.dst_assets_folder = "R:/temp/";

	mxDO(Resources::CopyOnlyUsedAssets(params, allocator));

	return EXIT_SUCCESS;
}
