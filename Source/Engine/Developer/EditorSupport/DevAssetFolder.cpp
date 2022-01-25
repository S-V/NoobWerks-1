#include "EditorSupport_PCH.h"
#pragma hdrstop
#include <Base/Util/PathUtils.h>
#include <EditorSupport/FileWatcher.h>
#include <EditorSupport/DevAssetFolder.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <GlobalEngineConfig.h>

namespace
{
	AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }
	AllocatorI& defaultMemHeap() { return MemoryHeaps::global(); }
}

/*
-----------------------------------------------------------------------------
	DevAssetFolder
-----------------------------------------------------------------------------
*/
DevAssetFolder::DevAssetFolder()
{
}

DevAssetFolder::~DevAssetFolder()
{
	//mxASSERT(!Assets::IsPackageMounted(this));
	this->Unmount();
}

ERet DevAssetFolder::Initialize()
{
#if MX_DEVELOPER
	mxDO(m_fileWatcher.Initialize());
#endif
	return ALL_OK;
}

void DevAssetFolder::Shutdown()
{
#if MX_DEVELOPER
	m_fileWatcher.Shutdown();
#endif
}

ERet DevAssetFolder::Mount( const char* path )
{
	mxASSERT(!Resources::IsPackageMounted(this));

	//mxDO(LocalFileSystem::Mount( path ));
	Str::CopyS( m_path, path );
	Str::NormalizePath( m_path );
	mxENSURE(OS::IO::PathExists(m_path.c_str()), ERR_FILE_OR_PATH_NOT_FOUND, "Couldn't locate asset path: '%s'", path);

	//
	String256	asset_bundle_list_file_path;
	Str::ComposeFilePath(
		asset_bundle_list_file_path, path, TB_ASSET_BUNDLE_LIST_FILENAME
		);

	TbAssetBundle::IDList	asset_bundle_ids;
	/*mxDO*/(SON::LoadFromFile(
		asset_bundle_list_file_path.c_str(),
		asset_bundle_ids,
		tempMemHeap()
		));

	ptPRINT("Mounting folder: '%s'", path);

	//
	mxDO(Resources::MountPackage(this));

	//
	nwFOR_EACH( const AssetID& asset_bundle_id, asset_bundle_ids )
	{
		String256	asset_bundle_file_path;

		Str::ComposeFilePath(
			asset_bundle_file_path
			, path
			, AssetId_ToChars(asset_bundle_id)
			, TbDevAssetBundle::metaClass().GetTypeName()
			);

		//
		//NwAssetBundleT * new_asset_bundle = new NwAssetBundleT(
		//	defaultMemHeap()
		//	, asset_bundle_id
		//	);

		NwAssetBundleT * new_asset_bundle = new NwAssetBundleT();

		mxDO(new_asset_bundle->Mount( asset_bundle_file_path.c_str() ));

		mxDO(_mounted_asset_bundles.add( new_asset_bundle ));
	}

	//
#if MX_DEVELOPER
	m_fileWatcher.AddFileWatcher( m_path.c_str(), true /*recursive*/ );
	m_fileWatcher.StartWatching();
#endif

	return ALL_OK;
}

void DevAssetFolder::Unmount()
{
	//
	nwFOR_EACH( NwAssetBundleT * asset_bundle, _mounted_asset_bundles )
	{
		asset_bundle->Unmount();
	}
	_mounted_asset_bundles.RemoveAll();

	//
#if MX_DEVELOPER
	m_fileWatcher.RemoveFileWatchers();
#endif

	Resources::UnmountPackage(this);

	m_path.empty();
}

void DevAssetFolder::_moundAssetBundlesInFolder( const char* folder )
{
	//
}

ERet DevAssetFolder::Open(
	const AssetKey& _key,
	AssetReader *stream_,
	const U32 _subresource
)
{
	mxENSURE(AssetId_IsValid(_key.id), ERR_INVALID_PARAMETER, "Invalid asset id");

	// figure out the filename and extension
	const TbMetaClass* asset_class = TypeRegistry::FindClassByGuid( _key.type );
	mxENSURE(asset_class, ERR_OBJECT_NOT_FOUND, "no class with id=0x%X", _key.type);

	String512 full_file_path;
	mxDO(Str::Copy(
		full_file_path,
		m_path
		));
	mxDO(Str::AppendS(
		full_file_path,
		asset_class->m_name
		));
	Str::AppendSlash(full_file_path);
	//
	const int len_without_filename = full_file_path.length();
	mxDO(Str::AppendS(
		full_file_path,
		_key.id.d.c_str()
		));

	// Fix for Doom 3 paths, e.g. "models/weapons/chaingun/w_chaingun_invis"
	Str::ReplaceChar(
		full_file_path
		, '/', '_'
		, len_without_filename
		);

	//
	mxASSERT(_subresource == OBJECT_DATA || _subresource == STREAM_DATA0);
	if( _subresource != OBJECT_DATA ) {
		mxDO(Str::AppendS(full_file_path, ".0" ));
	}

	FileReader* file_reader = new(stream_->file) FileReader();
	mxTRY(file_reader->Open( full_file_path.c_str(), FileRead_NoErrors/*allow empty files*/ ));
	stream_->parent = this;

	if( DEBUG_PRINT_OPENED_FILES ) {
		DBGOUT( "Opened '%s' for reading", full_file_path.c_str() );
	}

	mxTEMP
#if 0
	const FileHandleT fileHandle = OS::IO::Create_File(
		full_file_path.c_str(),
		OS::IO::AccessMode::ReadAccess
	);

	if( fileHandle == INVALID_HANDLE_VALUE ) {
		DBGOUT("Failed to open '%s'", full_file_path.c_str());
		return ERR_FAILED_TO_OPEN_FILE;
	}

	//LogStream(LL_Debug) << "found '" << fileInfo.fileNameOnly << "' in '" << fileInfo.relativePath << "'";

	//stream->DbgSetName( AssetId_ToChars(fileId) );
	mxSTATIC_ASSERT(FIELD_SIZE(AssetPackage::Stream,fileHandle) >= sizeof(fileHandle));
	stream->fileHandle = (UINT64)fileHandle;
	stream->dataSize = OS::IO::Get_File_Size(fileHandle);
	stream->offset = 0;
#endif

	return ALL_OK;
}

void DevAssetFolder::Close(
	AssetReader * stream
)
{
	FileReader* file_reader = (FileReader*) stream->file;
	file_reader->~FileReader();
	file_reader->Close();
	stream->parent = nil;
}

ERet DevAssetFolder::read( AssetReader * reader, void *buffer, size_t size )
{
	FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Read( buffer, size );
}

size_t DevAssetFolder::length( const AssetReader * reader ) const
{
	const FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Length();
}

size_t DevAssetFolder::tell( const AssetReader * reader ) const
{
	const FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Tell();
}

#if 0
ERet DevAssetFolder::OpenFile( const AssetKey& key, Stream *stream )
{
	chkRET_X_IF_NIL(AssetId_IsValid(key.id), ERR_INVALID_PARAMETER);
	mxASSERT2(NULL == Str::FindExtensionS(key.id.d.c_str()), "Asset IDs must not have an extension: '%s'", key.id.d.c_str());
	chkRET_X_IF_NIL(stream, ERR_NULL_POINTER_PASSED);
	//DBGOUT("searching '%s' in '%s'",AssetId_ToChars(fileId),m_path.c_str());

	mxTEMP
	const TbMetaClass* asset_class = TypeRegistry::FindClassByGuid( key.type );
	mxENSURE(asset_class, ERR_OBJECT_NOT_FOUND, "no class with id=0x%X", key.type);
	mxENSURE(asset_class->ass, ERR_INVALID_PARAMETER, "class %s is not an asset", asset_class->GetTypeName());

#if 0
	SFindFileInfo	fileInfo;
	if( !Win32_FindFileInDirectory( m_path.c_str(), fileId.d.c_str(), fileInfo ) )
	{
		return ERR_OBJECT_NOT_FOUND;
	}
#endif
	String512 full_file_path;
	mxDO(Str::Copy(full_file_path, m_path));
	mxDO(Str::AppendS(full_file_path, key.id.d.c_str() ));
	mxDO(Str::SetFileExtension( full_file_path, asset_class->GetTypeName() ));

	const FileHandleT fileHandle = OS::IO::Create_File(
		full_file_path.c_str(),
		OS::IO::AccessMode::ReadAccess
	);

	if( fileHandle == INVALID_HANDLE_VALUE ) {
		DBGOUT("Failed to open '%s'", full_file_path.c_str());
		return ERR_FAILED_TO_OPEN_FILE;
	}

	//LogStream(LL_Debug) << "found '" << fileInfo.fileNameOnly << "' in '" << fileInfo.relativePath << "'";

	//stream->DbgSetName( AssetId_ToChars(fileId) );
	mxSTATIC_ASSERT(FIELD_SIZE(AssetPackage::Stream,fileHandle) >= sizeof(fileHandle));
	stream->fileHandle = (UINT64)fileHandle;
	stream->dataSize = OS::IO::Get_File_Size(fileHandle);
	stream->offset = 0;

	return ALL_OK;
}
ERet DevAssetFolder::CloseFile( Stream * stream )
{
	chkRET_X_IF_NIL(stream, ERR_NULL_POINTER_PASSED);

	const FileHandleT fileHandle = (FileHandleT)stream->fileHandle;
	chkRET_X_IF_NOT(fileHandle != INVALID_HANDLE_VALUE, ERR_INVALID_PARAMETER);
	OS::IO::Close_File( fileHandle );
	return ALL_OK;
}
ERet DevAssetFolder::ReadFile( Stream * stream, void *buffer, UINT bytesToRead )
{
	chkRET_X_IF_NIL(stream, ERR_NULL_POINTER_PASSED);
	chkRET_X_IF_NIL(buffer, ERR_NULL_POINTER_PASSED);
	chkRET_X_IF_NOT(bytesToRead > 0, ERR_INVALID_PARAMETER);

	const FileHandleT fileHandle = (FileHandleT) stream->fileHandle;
	const size_t numReadBytes = OS::IO::Read_File( fileHandle, buffer, bytesToRead );
	mxASSERT(numReadBytes == bytesToRead);
	stream->offset += numReadBytes;
	return ALL_OK;
}
size_t DevAssetFolder::TellPosition( const Stream& stream ) const
{
	const FileHandleT fileHandle = (FileHandleT) stream.fileHandle;
	return OS::IO::Tell_File_Position( fileHandle );
}
#endif


#if MX_DEVELOPER
void DevAssetFolder::ProcessChangedAssets( AFileWatchListener* callback )
{
	m_fileWatcher.ProcessNotifications( callback );
}
#endif


#if MX_DEVELOPER

void AssetHotReloader::ProcessChangedFile( const efswData& _data )
{
	if( _data.action == efsw::Actions::Modified )
	{
		const char* extension = Str::FindExtensionS(_data.filename.c_str());

		const char* asset_class_name = extension;
		if( !asset_class_name )
		{
			// e.g. "NwRenderState\idtech4_fp_weapon"
			FilePathStringT	temp;
			Str::CopyS(temp, _data.filename.c_str());

			Str::StripFileName(temp);
			asset_class_name = Str::GetFileName(temp.c_str());
		}

		if( asset_class_name )
		{
			const TbMetaClass* asset_class = TypeRegistry::FindClassByName( asset_class_name );
			if( !asset_class ) {
				DBGOUT("Skipping changed file, because couldn't find asset class: '%s'",
				_data.filename.c_str()
				);
				return;
			}

			//
			String256	stripped_filename;
			Str::CopyS( stripped_filename, _data.filename.c_str() );
			Str::StripFileExtension( stripped_filename );

			//
			AssetKey key;
			key.id = AssetID_from_FilePath( stripped_filename.c_str() );
			key.type = asset_class->GetTypeGUID();

			Resources::Reload(key);
		}
		else
		{
			DBGOUT("Skipping changed file, because it's of unknown asset type: '%s'",
				_data.filename.c_str()
				);
		}
	}
}

#endif

const char* TB_ASSET_BUNDLE_LIST_FILENAME = "asset_bundles.lst";
