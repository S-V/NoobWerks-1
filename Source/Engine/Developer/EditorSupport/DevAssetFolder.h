#pragma once

#include <Core/Filesystem.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Assets/AssetBundle.h>
#include <Developer/EditorSupport/EditorSupport.h>

#if MX_DEVELOPER
#include <Developer/EditorSupport/FileWatcher.h>
#endif

/// An asset package for loading assets from a local folder in development mode.
/// Monitors the assets folder for file changes and automatically, on-the-fly reloads modified assets.
///
class DevAssetFolder: public AssetPackage, public AFileSystem, DependsOn_Core
{
	String256	m_path;	// normalized directory name (ends with '/')

	TInplaceArray< TRefPtr< NwAssetBundleT >, 32 >	_mounted_asset_bundles;

#if MX_DEVELOPER
	// for detecting file changes and hot-reloading changed assets
	MyFileWatcher	m_fileWatcher;
#endif

public:
	DevAssetFolder();
	~DevAssetFolder();

	ERet Initialize();
	void Shutdown();

	//@ AFileSystem
	virtual ERet Mount( const char* path ) override;
	virtual void Unmount();

	//@ AssetPackage

	virtual ERet Open(
		const AssetKey& _key,
		AssetReader *stream_,
		const U32 _subresource = 0
	) override;

	virtual void Close(
		AssetReader * stream
	) override;

	virtual ERet read( AssetReader * reader, void *buffer, size_t size ) override;
	virtual size_t length( const AssetReader * reader ) const override;
	virtual size_t tell( const AssetReader * reader ) const override;

#if MX_DEVELOPER
	void ProcessChangedAssets( AFileWatchListener* callback );

	MyFileWatcher& GetFileWatcher() { return m_fileWatcher; }
#endif

private:
	void _moundAssetBundlesInFolder( const char* folder );
};

#if MX_DEVELOPER
///
class AssetHotReloader : public AFileWatchListener
{
public:
	virtual void ProcessChangedFile( const efswData& _data ) override;
};
#endif

extern const char* TB_ASSET_BUNDLE_LIST_FILENAME;
