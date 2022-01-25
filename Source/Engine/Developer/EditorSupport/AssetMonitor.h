// watching changed asset files and initiating their hot-reloading
#pragma once

#if MX_AUTOLINK
#pragma comment( lib, "EditorSupport.lib" )
#endif //MX_AUTOLINK

ERet AssetMonitor_Initialize();
void AssetMonitor_Shutdown();

ERet AssetMonitor_AddPath( const char* directory );
void AssetMonitor_RemovePath( const char* directory );

//ERet AssetMonitor_AddPathsFromConfig();

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
