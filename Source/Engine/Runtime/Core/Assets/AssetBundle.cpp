// Asset Bundles are used for merging asset files into a large files for faster loading.
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Base/Template/Algorithm/Search.h>
#include <Core/Assets/AssetBundle.h>
#include <Core/Serialization/Serialization.h>

#define DBG_ASSET_BUNDLE	(1)

#define DBG_TRACE_FILES_OPENED_LOCALLY	(MX_DEVELOPER)

namespace
{
	AllocatorI& memoryHeap() { return MemoryHeaps::resources(); }
}

/*
-----------------------------------------------------------------------------
	TbAssetBundle
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(TbAssetBundle::ChunkInfo);
mxBEGIN_REFLECTION(TbAssetBundle::ChunkInfo)
	mxMEMBER_FIELD(offset),
	mxMEMBER_FIELD(size),
mxEND_REFLECTION;

mxDEFINE_CLASS(TbAssetBundle::AssetDataRef);
mxBEGIN_REFLECTION(TbAssetBundle::AssetDataRef)
	mxMEMBER_FIELD(type_id),
	mxMEMBER_FIELD(skip),
	mxMEMBER_FIELD(files),
mxEND_REFLECTION;

mxDEFINE_CLASS(TbAssetBundle::MemoryResidentData);
mxBEGIN_REFLECTION(TbAssetBundle::MemoryResidentData)
	mxMEMBER_FIELD(sorted_asset_ids),
	mxMEMBER_FIELD(asset_data_refs),
mxEND_REFLECTION;

mxDEFINE_CLASS(TbAssetBundle);
mxBEGIN_REFLECTION(TbAssetBundle)
mxEND_REFLECTION;

TbAssetBundle::TbAssetBundle()
{
	_num_readers = 0;
}

TbAssetBundle::~TbAssetBundle()
{
	mxASSERT(0 == _num_readers);
	mxASSERT(!_file_stream.IsOpen());
}

ERet TbAssetBundle::mount( const char* filepath )
{
	mxDO(_file_stream.Open( filepath ));

	mxDO(Serialization::LoadMemoryImage(
		_mrd._ptr
		, _file_stream
		, memoryHeap()
		));

	mxDO(Resources::MountPackage(this));

	return ALL_OK;
}

void TbAssetBundle::unmount()
{
	Resources::UnmountPackage(this);

	mxDELETE_AND_NIL( _mrd._ptr, memoryHeap() );

	_file_stream.Close();
}

ERet TbAssetBundle::Open(
	const AssetKey& key,
	AssetReader * stream,
	const U32 subresource /*= 0*/
)
{
	struct CompareAssetIDsLessOrEqual
	{
		mxFORCEINLINE bool operator () ( const AssetID& a, const AssetID& b ) const
		{
			return strcmp( a.d.c_str(), b.d.c_str() ) <= 0;
		}
	};

	const UINT index = LowerBoundAscending(
		key.id
		, _mrd->sorted_asset_ids.raw()
		, _mrd->sorted_asset_ids.num()
		, CompareAssetIDsLessOrEqual()
		);

	if( _mrd->sorted_asset_ids[ index ] == key.id )
	{
		const AssetDataRef& asset_data = _mrd->asset_data_refs[ index ];
		mxENSURE( asset_data.type_id == key.type, ERR_OBJECT_OF_WRONG_TYPE, "" );

		return this->_openStream(
			stream,
			key.id,
			index,
			subresource
			);
	}

	return ERR_OBJECT_NOT_FOUND;
}

void TbAssetBundle::Close(
	AssetReader * stream
)
{
	UNDONE;
}

ERet TbAssetBundle::_openStream(
							   AssetReader * stream
							   , const AssetID& asset_id
							   , const U32 asset_data_index
							   , const U32 subresource
							   )
{
	//const AssetDataRef& asset_data = _mrd->asset_data[ asset_data_index ];

	stream->bundle_reader.file_entry_index = asset_data_index;
	stream->bundle_reader.stream_index = subresource;
	stream->bundle_reader.read_cursor = 0;

	stream->parent = this;

	if( 0 == _num_readers )
	{
		++_num_readers;
	}

	return ALL_OK;
}

void TbAssetBundle::_closeStream(
								AssetReader * stream
								)
{
	mxASSERT( stream->parent == this );
	mxASSERT( _num_readers > 0 );

	if( _num_readers > 0 )
	{
		--_num_readers;
	}

	stream->bundle_reader.file_entry_index = ~0;
	stream->bundle_reader.stream_index = ~0;
	stream->bundle_reader.read_cursor = ~0;

	stream->parent = nil;
}

ERet TbAssetBundle::read( AssetReader * reader, void *buffer, size_t size )
{
	const AssetDataRef& asset_data = _mrd->asset_data_refs[ reader->bundle_reader.file_entry_index ];
	const ChunkInfo& asset_file_info = asset_data.files[ reader->bundle_reader.stream_index ];

	_file_stream.Seek( asset_file_info.offset + reader->bundle_reader.read_cursor );
	mxDO(_file_stream.Read( buffer, size ));
	reader->bundle_reader.read_cursor += size;

	mxASSERT( reader->bundle_reader.read_cursor <= asset_file_info.size );

	return ALL_OK;
}

size_t TbAssetBundle::length( const AssetReader * reader ) const
{
	const AssetDataRef& asset_data = _mrd->asset_data_refs[ reader->bundle_reader.file_entry_index ];
	const ChunkInfo& asset_file_info = asset_data.files[ reader->bundle_reader.stream_index ];

	return asset_file_info.size;
}

size_t TbAssetBundle::tell( const AssetReader * reader ) const
{
	return reader->bundle_reader.read_cursor;
}

/*
-----------------------------------------------------------------------------
	TbDevAssetBundle
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS_NO_DEFAULT_CTOR(TbDevAssetBundle);

TbDevAssetBundle::TbDevAssetBundle(
								   AllocatorI & allocator
								   , const AssetID& name
								   )
	: _assets( allocator )
{
	mxZERO_OUT(_header);
	_name = name;
}

TbDevAssetBundle::~TbDevAssetBundle()
{
	mxASSERT(!_file_stream.IsOpen());
}

ERet TbDevAssetBundle::initialize( U32 expected_asset_count )
{
	mxDO(_assets.Initialize(
		CeilPowerOfTwo( expected_asset_count * 2 )
		, expected_asset_count
		));
	return ALL_OK;
}

bool TbDevAssetBundle::containsAsset(
				   const AssetID& asset_id
				   , const TbMetaClass& asset_class
				   ) const
{
	const TbAssetBundle::AssetDataRef* existing_entry = _assets.FindValue( asset_id );
	return existing_entry && existing_entry->type_id == asset_class.GetTypeGUID();
}

ERet TbDevAssetBundle::mount( const char* filepath )
{
	DEVOUT("Mounting bundle: '%s'", filepath);

	//
	mxDO(_file_stream.openForReadingOnly( filepath ));

	mxDO(_file_stream.Get( _header ));

	//
	mxDO(this->initialize( _header.num_entries ));

#if DBG_ASSET_BUNDLE
DebugParam	dbgparm;
if(strstr(filepath, "doom3_models"))
{
	dbgparm.flag = true;
}
if(dbgparm.flag) ptWARN("=== Started Reading bundle at '%s' (%u entries, offset = %u)",
	   filepath, _header.num_entries, _file_stream.Tell());
#endif
	//
	for( U32 i = 0; i < _header.num_entries; i++ )
	{
		mxDO(Reader_Align_Forward(
			_file_stream
			, TbAssetBundle::FILE_BLOCK_ALIGNMENT
			));
#if DBG_ASSET_BUNDLE
if(dbgparm.flag) ptWARN("=== Reading asset id at offset %u...", _file_stream.Tell());
#endif
		// Read Asset ID.
		AssetID	asset_id;
		mxDO(readAssetID( asset_id, _file_stream ));

#if DBG_ASSET_BUNDLE
if(dbgparm.flag) ptWARN("=== Read asset id: %s", AssetId_ToChars(asset_id));
#endif
		//
		mxDO(Reader_Align_Forward(
			_file_stream
			, TbAssetBundle::FILE_BLOCK_ALIGNMENT
			));

const U32 offs = _file_stream.Tell();
		//
		TbAssetBundle::AssetDataRef	asset_data_ref;
		mxDO(_file_stream.Get(asset_data_ref));

		//
		const TbMetaClass* asset_class = TypeRegistry::FindClassByGuid( asset_data_ref.type_id );
		mxASSERT2( asset_class, "Class with id = 0x%X not found!", asset_data_ref.type_id );
#if DBG_ASSET_BUNDLE
if(dbgparm.flag) ptWARN("=== Read asset data at offset %u, skip = %u", offs, asset_data_ref.skip);
#endif
		// move to the next entry
		mxDO(_file_stream.Seek( _file_stream.Tell() + asset_data_ref.skip ));

		//
		_assets.Insert(
			asset_id,
			asset_data_ref
			);

	}//for each record

	mxDO(Resources::MountPackage(this));

	return ALL_OK;
}

void TbDevAssetBundle::unmount()
{
	Resources::UnmountPackage(this);

	_assets.RemoveAll();

	_file_stream.Close();
}

ERet TbDevAssetBundle::Open(
	const AssetKey& key,
	AssetReader * stream,
	const U32 subresource /*= 0*/
)
{
	const TbAssetBundle::AssetDataRef* entry = _assets.FindValue( key.id );

	if( entry && entry->type_id == key.type )
	{
#if DBG_ASSET_BUNDLE
		const TbMetaClass* asset_class = TypeRegistry::FindClassByGuid( key.type );
		LogStream(LL_Info)<<"[TbDevAssetBundle] Opened '",key.id,"' of type '",asset_class->GetTypeName(),"', skip = ",entry->skip;
#endif

		stream->dev_bundle_reader.file_entry_ptr = (void*) entry;
		stream->dev_bundle_reader.stream_index = subresource;
		stream->dev_bundle_reader.read_cursor = 0;

		stream->parent = this;

		return ALL_OK;
	}

	return ERR_OBJECT_NOT_FOUND;
}

void TbDevAssetBundle::Close(
	AssetReader * stream
	)
{
	mxASSERT( stream->parent == this );

	stream->dev_bundle_reader.file_entry_ptr = nil;
	stream->dev_bundle_reader.stream_index = ~0;
	stream->dev_bundle_reader.read_cursor = ~0;

	stream->parent = nil;
}

ERet TbDevAssetBundle::read( AssetReader * reader, void *buffer, size_t size )
{
	const TbAssetBundle::AssetDataRef* entry = (TbAssetBundle::AssetDataRef*) reader->dev_bundle_reader.file_entry_ptr;
	const TbAssetBundle::ChunkInfo& chunk_info = entry->files[ reader->dev_bundle_reader.stream_index ];

	_file_stream.Seek( chunk_info.offset + reader->dev_bundle_reader.read_cursor );
	mxDO(_file_stream.Read( buffer, size ));
	reader->dev_bundle_reader.read_cursor += size;

	mxASSERT( reader->dev_bundle_reader.read_cursor <= chunk_info.size );

	return ALL_OK;
}

size_t TbDevAssetBundle::length( const AssetReader * reader ) const
{
	const TbAssetBundle::AssetDataRef* entry = (TbAssetBundle::AssetDataRef*) reader->dev_bundle_reader.file_entry_ptr;
	return entry->files[ reader->dev_bundle_reader.stream_index ].size;
}

size_t TbDevAssetBundle::tell( const AssetReader * reader ) const
{
	const TbAssetBundle::AssetDataRef* entry = (TbAssetBundle::AssetDataRef*) reader->dev_bundle_reader.file_entry_ptr;
	return reader->dev_bundle_reader.read_cursor;
}


/*
-----------------------------------------------------------------------------
	TbAssetBundle_LocalFolder
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(TbAssetBundle_LocalFolder);

TbAssetBundle_LocalFolder::TbAssetBundle_LocalFolder(
	AllocatorI & allocator
	)
	: _file_name_by_asset_id( allocator )
{
}

TbAssetBundle_LocalFolder::TbAssetBundle_LocalFolder(
	AllocatorI & allocator
	, const AssetID& asset_bundle_id
	)
	: _file_name_by_asset_id( allocator )
	, _asset_bundle_id( asset_bundle_id )
{
	Str::CopyS( _path, asset_bundle_id.d.c_str() );
}

TbAssetBundle_LocalFolder::~TbAssetBundle_LocalFolder()
{
	mxASSERT(!Resources::IsPackageMounted(this));
}

ERet TbAssetBundle_LocalFolder::Mount( const char* folder )
{
	mxASSERT2(_path.IsEmpty(), "Did you forget to call Unmount()?");
	mxASSERT(!Resources::IsPackageMounted(this));

	//mxDO(LocalFileSystem::Mount( path ));
	Str::CopyS( _path, folder );
	Str::NormalizePath( _path );
	mxENSURE(OS::IO::PathExists(_path.c_str()), ERR_FILE_OR_PATH_NOT_FOUND, "Couldn't locate asset path: '%s'", folder);

	//
	ptPRINT("Mounting local folder: '%s'", folder);

	//
	mxDO(Resources::MountPackage(this));

	return ALL_OK;
}

void TbAssetBundle_LocalFolder::Unmount()
{
	Resources::UnmountPackage(this);

	_path.empty();
}

ERet TbAssetBundle_LocalFolder::Open(
	const AssetKey& asset_key,
	AssetReader *stream_,
	const U32 _subresource
)
{
	mxENSURE(AssetId_IsValid(asset_key.id), ERR_INVALID_PARAMETER, "Invalid asset id");

	const TbMetaClass* asset_class = TypeRegistry::FindClassByGuid( asset_key.type );
	mxENSURE(asset_class, ERR_OBJECT_NOT_FOUND, "No class with id=0x%X", asset_key.type);

	//
	String512 full_file_path;
	mxDO(Str::Copy(
		full_file_path,
		_path
		));
	mxDO(Str::AppendS(
		full_file_path,
		asset_class->m_name
		));
	Str::AppendSlash(full_file_path);
	mxDO(Str::AppendS(
		full_file_path,
		asset_key.id.d.c_str()
		));

	//
	FileReader* file_reader = new(stream_->file) FileReader();
	mxTRY(file_reader->Open( full_file_path.c_str(), FileRead_NoErrors/*allow empty files*/ ));
	stream_->parent = this;

#if DBG_TRACE_FILES_OPENED_LOCALLY
	DBGOUT( "Opened file '%s' for reading.", full_file_path.c_str() );
#endif

	return ALL_OK;
}

void TbAssetBundle_LocalFolder::Close(
	AssetReader * stream
)
{
	FileReader* file_reader = (FileReader*) stream->file;
	file_reader->~FileReader();
	file_reader->Close();
	stream->parent = nil;
}

ERet TbAssetBundle_LocalFolder::read( AssetReader * reader, void *buffer, size_t size )
{
	FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Read( buffer, size );
}

size_t TbAssetBundle_LocalFolder::length( const AssetReader * reader ) const
{
	const FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Length();
}

size_t TbAssetBundle_LocalFolder::tell( const AssetReader * reader ) const
{
	const FileReader* file_reader = (FileReader*) reader->file;
	return file_reader->Tell();
}

ERet TbAssetBundle_LocalFolder::CreateFolderInFolder( const char* containing_folder )
{
	FilePathStringT	dst_folder_path;
	mxDO(Str::ComposeFilePath(
		dst_folder_path
		, containing_folder
		, _path.c_str()
		));

	// make sure the destination directory exists

	if( !OS::IO::FileOrPathExists( dst_folder_path.c_str() ) )
	{
		return OS::IO::MakeDirectory( dst_folder_path.c_str() )
			? ALL_OK
			: ERR_FAILED_TO_CREATE_FILE;
	}

	return ALL_OK;
}
