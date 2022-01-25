// watching changed asset files and initiating their hot-reloading
#include "EditorSupport_PCH.h"
#pragma hdrstop
#include <EditorSupport/AssetMonitor.h>


#if MX_DEVELOPER

#include <efsw/efsw.hpp>
#if MX_AUTOLINK
	#pragma comment( lib, "efsw.lib" )
#endif

#include <Core/Assets/AssetManagement.h>

class CAssetFileListener : public efsw::FileWatchListener
{
public:
	virtual void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "" ) override
	{
		//NOTE: this code is *NOT* executing in the main thread!
		if( action == efsw::Actions::Modified )
		{
			char	extension[8];
			V_ExtractFileExtension(filename.c_str(), extension, mxCOUNT_OF(extension));

			//const TbMetaClass* assetType = FindAssetTypeByExtension(extension);
			//if( assetType )
			//{
			//	const AssetID assetId = GetAssetIdFromFilePath(filename.c_str());
			//	UNDONE;
			//	//				Assets::ReloadAsset(AssetKey(assetId, assetType));
			//}
		}
	}
};

static TPtr< efsw::FileWatcher >	g_pFileWatcher;
static CAssetFileListener			g_listenerCallback;

ERet AssetMonitor_Initialize()
{
	g_pFileWatcher.ConstructInPlace();
	return ALL_OK;
}
void AssetMonitor_Shutdown()
{
	g_pFileWatcher.Destruct();
}
ERet AssetMonitor_AddPath( const char* directory )
{
	// NOTE: the name of the watched folder must not end with a path separator for efsw to work
	String512	tmp;
	Str::CopyS(tmp, directory);
	Str::NormalizePath(tmp);
	Str::StripTrailing(tmp, '/');	

	const efsw::WatchID watchId = g_pFileWatcher->addWatch(tmp.raw(), &g_listenerCallback, true/*recursive*/);

	if( watchId < 0 ) {
		ptPRINT("Failed to install file monitor, reason: '%s'\n", efsw::Errors::Log::getLastErrorLog().c_str());
	}

	if( watchId == efsw::Errors::FileNotFound ) {
		return ERR_OBJECT_NOT_FOUND;
	}
	if( watchId == efsw::Errors::FileRepeated ) {
		return ERR_DUPLICATE_OBJECT;
	}
	if( watchId == efsw::Errors::FileOutOfScope ) {
		return ERR_INVALID_PARAMETER;
	}
	if( watchId == efsw::Errors::FileNotReadable ) {
		return ERR_FAILED_TO_READ_FILE;
	}
	if( watchId == efsw::Errors::Unspecified ) {
		return ERR_UNKNOWN_ERROR;
	}

	g_pFileWatcher->watch();

	return ALL_OK;
}
void AssetMonitor_RemovePath( const char* directory )
{
	g_pFileWatcher->removeWatch(directory);
}
//ERet AssetMonitor_AddPathsFromConfig()
//{
//	const char* pathToAssets = gINI->FindString("PathToAssets");
//	if( pathToAssets ) {
//		mxDO(AssetMonitor_AddPath( pathToAssets ));
//	}
//	return ALL_OK;
//}
#endif
