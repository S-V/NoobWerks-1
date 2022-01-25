#include "stdafx.h"
#pragma hdrstop

#include <Base/Template/Algorithm/Sorting/InsertionSort.h>
#include <Core/Assets/AssetBundle.h>
#include <Core/Serialization/Serialization.h>

#include <Rendering/Public/Core/Mesh.h>

#include <AssetCompiler/AssetBundleBuilder.h>

#define DBG_ASSET_BUNDLE_BUILDER	(1)

namespace AssetBaking
{

AssetBundleBuilder::AssetBundleBuilder( AllocatorI & allocator )
	: _assets( allocator )
{
}

AssetBundleBuilder::~AssetBundleBuilder()
{

}

ERet AssetBundleBuilder::initialize( U32 expected_asset_count )
{
	mxDO(_assets.Initialize(
		CeilPowerOfTwo( expected_asset_count * 2 )
		, expected_asset_count
		));
	return ALL_OK;
}

ERet AssetBundleBuilder::addAsset(
			  const AssetID& asset_id
			  , const TbMetaClass& asset_class
			  , const CompiledAssetData& asset_data
			  )
{
	DEVOUT("AssetBundleBuilder::addAsset: %s",
		AssetId_ToChars(asset_id)
		);

	mxASSERT(AssetId_IsValid(asset_id));
	mxASSERT(asset_data.isOk());
	mxASSERT(!_assets.Contains( asset_id ));

	_assets.Insert(
		asset_id,
		new StoredAssetData( asset_data, asset_class )
		);

	return ALL_OK;
}

ERet AssetBundleBuilder::addAssetIfNeeded(
	const AssetID& asset_id
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	)
{
	const TRefPtr<StoredAssetData>* existing_entry = _assets.FindValue( asset_id );
	if( existing_entry )
	{
		mxASSERT( (*existing_entry)->asset_class == asset_class );
		mxASSERT( (*existing_entry)->equals( asset_data ) );
		//
	}
	else
	{
		mxDO(this->addAsset(
			asset_id
			, asset_class
			, asset_data
			));
	}
	return ALL_OK;
}

static
U32 measureSizeOnDisk( const TbAssetBundle::MemoryResidentData& mrd )
{
	NullWriter	dummy_writer;
	Serialization::SaveMemoryImage(
		mrd
		, dummy_writer
		);

	return dummy_writer.bytes_written;
}

ERet AssetBundleBuilder::saveToBlob( NwBlob & blob_ )
{
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	const AssetsMap::PairsArray& pairs = _assets.GetPairs();

	const U32 num_assets = pairs.num();
	mxASSERT(num_assets > 0);

	//
	TbAssetBundle::MemoryResidentData	mrd;
	mxDO(mrd.sorted_asset_ids.setNum( num_assets ));
	mxDO(mrd.asset_data_refs.setNum( num_assets ));

	//
	DynamicArray< U32 >		sorted_indices( scratchpad );
	mxDO(sorted_indices.setNum( num_assets ));
	for( U32 i = 0; i < num_assets; i++ ) { sorted_indices[i] = i; }

	// Fill asset IDs.
	{
		U32	i = 0;
		nwFOR_EACH( const AssetsMap::Pair& pair, pairs ) {
			const AssetID& asset_id = pair.key;
			mrd.sorted_asset_ids[ i++ ] = asset_id;
		}
	}

	// Sort asset ids and asset data indices.

	struct ByIncreasingAssetIDs
	{
		mxFORCEINLINE bool AreInOrder( const AssetID& a, const AssetID& b ) const
		{
			return strcmp( a.d.c_str(), b.d.c_str() ) <= 0;
		}
	};

	//DEVOUT("\nBEFORE SORTING:");
	//for( U32 i = 0; i < num_assets; i++ ) {
	//	DEVOUT("[%i]: %s", i, AssetId_ToChars(mrd.sorted_asset_ids[i]));
	//}

	Sorting::InsertionSortKeysAndValues(
		mrd.sorted_asset_ids.raw()
		, num_assets
		, sorted_indices.raw()
		, ByIncreasingAssetIDs()
		);

	//DEVOUT("\nAFTER SORTING:");
	//for( U32 i = 0; i < num_assets; i++ ) {
	//	DEVOUT("[%i]: %s", i, AssetId_ToChars(mrd.sorted_asset_ids[i]));
	//}

	//
	const U32	size_of_serialized_header = measureSizeOnDisk( mrd );
	const U32	aligned_size_of_header = AlignUp(
		size_of_serialized_header,
		TbAssetBundle::FILE_BLOCK_ALIGNMENT
		);

	//
	mxDO(blob_.reserve( aligned_size_of_header ));

	NwBlobWriter	blob_writer( blob_ );


	// Reserve space for the dictionary.
	mxDO(Stream_WriteZeros(
		blob_writer
		, aligned_size_of_header
		));


	// Save data blocks.

	for( U32 i = 0; i < num_assets; i++ )
	{
		// asset ids are already in sorted order
		const AssetID& asset_id = mrd.sorted_asset_ids[ i ];

		const StoredAssetData& asset_data = *pairs[ sorted_indices[ i ] ].value;

		TbAssetBundle::AssetDataRef &dst_data = mrd.asset_data_refs[ i ];

		//
		dst_data.type_id = asset_data.asset_class.GetTypeGUID();

		// Save object data.

		mxDO(blob_writer.alignBy( TbAssetBundle::FILE_BLOCK_ALIGNMENT ));

		dst_data.files[AssetPackage::OBJECT_DATA].offset = blob_writer.Tell();
		dst_data.files[AssetPackage::OBJECT_DATA].size = asset_data.object_data.rawSize();

		mxDO(blob_writer.writeBlob( asset_data.object_data ));


		// Save streaming data, if any.
		if( !asset_data.stream_data.IsEmpty() )
		{
			mxDO(blob_writer.alignBy( TbAssetBundle::FILE_BLOCK_ALIGNMENT ));

			dst_data.files[AssetPackage::STREAM_DATA0].offset = blob_writer.Tell();
			dst_data.files[AssetPackage::STREAM_DATA0].size = asset_data.stream_data.rawSize();

			mxDO(blob_writer.writeBlob( asset_data.stream_data ));
		}
	}//for

	const size_t bundle_size_in_bytes = blob_.num();

	// Save the dictionary.

	blob_writer.Rewind( 0 );

	mxDO(Serialization::SaveMemoryImage(
		mrd
		, blob_writer
		));

	//
	mxDO(blob_.setNum( bundle_size_in_bytes ));

	return ALL_OK;
}

namespace TbDevAssetBundleUtil
{

bool assetMustBeCompiled(
	const AssetID& asset_id
	, const TbMetaClass& asset_class
	, AssetDatabase & asset_database
	)
{
	return true;
}


#if !nwUSE_LOOSE_ASSETS

static
ERet composeAssetBundleFilePath(
	String & asset_bundle_filepath_
	, const char* destination_folder
	, const char* asset_bundle_filename
	)
{
	return Str::ComposeFilePath(
		asset_bundle_filepath_
		, destination_folder
		, asset_bundle_filename
		, NwAssetBundleT::metaClass().GetTypeName()
		);
}

void checkAssetBundleFileHeader(
								NwAssetBundleT & asset_bundle
								)
{
	//mxASSERT( asset_bundle._header.filesize >= sizeof(asset_bundle._header) );
}

static
ERet openFileIfNeeded(
	NwAssetBundleT & asset_bundle
	, AssetDatabase & asset_database
	)
{
	//
	if( !asset_bundle._file_stream.IsOpen() )
	{
		bool	created_new_file = false;

		//
		String256	asset_bundle_filepath;
		mxDO(composeAssetBundleFilePath(
			asset_bundle_filepath
			, asset_database.pathToContainingFolder()
			, AssetId_ToChars( asset_bundle._name )
			));

		mxDO(asset_bundle._file_stream.openForReadingAndWriting(
			asset_bundle_filepath.c_str()
			, &created_new_file
			));

		//
		if( created_new_file )
		{
			// write the header
			asset_bundle._header.signature = NwAssetBundleT::SIGNATURE;
			asset_bundle._header.version = 0;
			asset_bundle._header.num_entries = 0;
			//asset_bundle._header.filesize = sizeof(asset_bundle._header);

			mxDO(asset_bundle._file_stream.Put( asset_bundle._header ));
		}
		else
		{
			// read the header
			mxDO(asset_bundle._file_stream.Get( asset_bundle._header ));

			checkAssetBundleFileHeader( asset_bundle );
		}
	}
	else
	{
		checkAssetBundleFileHeader( asset_bundle );
	}

	// seek to the end of the file
	//mxDO(asset_bundle._file_stream.Seek( asset_bundle._header.filesize ));
	mxDO(asset_bundle._file_stream.SeekToEnd());

	return ALL_OK;
}

#endif // #if !nwUSE_LOOSE_ASSETS

ERet addAsset_and_Save(
	NwAssetBundleT & asset_bundle
	, const AssetID& asset_id
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	, AssetDatabase & asset_database
	)
{

#if !nwUSE_LOOSE_ASSETS
	if( AssetBaking::LogLevelEnabled( LL_Debug ) )
	{
		DEVOUT("'%s'::addAsset '%s' of type '%s' (were %d entries)",
			asset_bundle.dbgName(), AssetId_ToChars(asset_id), asset_class.GetTypeName(), asset_bundle._header.num_entries
			);
	}
#endif

	mxASSERT(AssetId_IsValid(asset_id));
	mxASSERT(asset_data.isOk());


#if nwUSE_LOOSE_ASSETS

	//
	FilePathStringT	dst_folder_path;
	mxDO(Str::ComposeFilePath(
		dst_folder_path
		, asset_database.pathToContainingFolder()
		, asset_bundle.GetFolder()
		));

	//
	mxDO(asset_data.SaveAssetDataToDisk(
		asset_id
		, asset_class.m_name
		, dst_folder_path.c_str()
		));

	//
	//asset_bundle._file_name_by_asset_id.Set(asset_id, );

#else


	//
	if( asset_bundle.containsAsset(
		asset_id
		, asset_class
		) )
	{
		// The asset has already been added to this bundle.
		// Append the data to the end of the file.

		DEVOUT("Asset '%s' of type '%s' has already been added to bundle '%s', appending new data"
			, AssetId_ToChars(asset_id), asset_class.GetTypeName(), AssetId_ToChars(asset_bundle._name)
			);
	}

	mxASSERT2(
		!asset_database.containsAssetWithId( asset_id, asset_class )
		, "Doom3's bundled assets must not be stored in asset database"
		);



#if MX_DEBUG
//QQQ
bool is_dbg_bundle = strstr(
							asset_bundle._name.d.c_str()
							, "doom3_meshes"
							);
//bool is_dbg_bundle = strstr(
//							AssetId_ToChars(asset_id)
//							, "trite.md5mesh"
//							);

if(is_dbg_bundle)
{
	printf("");
}
#endif


	//
	mxDO(openFileIfNeeded( asset_bundle, asset_database ));



	// Write Asset ID.

	mxDO(Writer_AlignForward(
		asset_bundle._file_stream
		, asset_bundle._file_stream.Tell()
		, TbAssetBundle::FILE_BLOCK_ALIGNMENT
		));

#if DBG_ASSET_BUNDLE_BUILDER && MX_DEBUG
if(is_dbg_bundle) ptWARN("=== Write asset id '%s' at offset %u", AssetId_ToChars(asset_id), asset_bundle._file_stream.Tell());
#endif

	mxDO(writeAssetID( asset_id, asset_bundle._file_stream ));
	mxDO(asset_bundle._file_stream.flushToOutput());


	// Write asset data chunk.

	TbAssetBundle::AssetDataRef	saved_asset_data;
	mxZERO_OUT(saved_asset_data);
	{
		mxDO(Writer_AlignForward(
			asset_bundle._file_stream
			, asset_bundle._file_stream.Tell()
			, TbAssetBundle::FILE_BLOCK_ALIGNMENT
			));

		// Reserve space for asset data chunk (AssetDataRef).
		const size_t	data_chunk_header_offset = asset_bundle._file_stream.Tell();

#if DBG_ASSET_BUNDLE_BUILDER && MX_DEBUG
if(is_dbg_bundle) ptWARN("=== Write asset data chunk at offset %u", data_chunk_header_offset);
#endif

		mxDO(asset_bundle._file_stream.Put( saved_asset_data ));

		//
		saved_asset_data.type_id = asset_class.GetTypeGUID();


		// Write object data.

		mxDO(Writer_AlignForward(
			asset_bundle._file_stream
			, asset_bundle._file_stream.Tell()
			, TbAssetBundle::FILE_BLOCK_ALIGNMENT
			));

		saved_asset_data.files[AssetPackage::OBJECT_DATA].offset = asset_bundle._file_stream.Tell();
		saved_asset_data.files[AssetPackage::OBJECT_DATA].size = asset_data.object_data.rawSize();

#if DBG_ASSET_BUNDLE_BUILDER && MX_DEBUG
if(is_dbg_bundle) ptWARN("=== Object data starts at offset %u (size = %u)",
						 saved_asset_data.files[AssetPackage::OBJECT_DATA].offset, saved_asset_data.files[AssetPackage::OBJECT_DATA].size
						 );
#endif

		mxDO(asset_bundle._file_stream.Write(
			asset_data.object_data.raw(),
			asset_data.object_data.rawSize()
			));


		// Write streaming data, if any.
		if( !asset_data.stream_data.IsEmpty() )
		{
			mxDO(Writer_AlignForward(
				asset_bundle._file_stream
				, asset_bundle._file_stream.Tell()
				, TbAssetBundle::FILE_BLOCK_ALIGNMENT
				));

			saved_asset_data.files[AssetPackage::STREAM_DATA0].offset = asset_bundle._file_stream.Tell();
			saved_asset_data.files[AssetPackage::STREAM_DATA0].size = asset_data.stream_data.rawSize();

			mxDO(asset_bundle._file_stream.Write(
				asset_data.stream_data.raw(),
				asset_data.stream_data.rawSize()
				));
		}

		mxDO(asset_bundle._file_stream.flushToOutput());

		//for( UINT i = 0; i < mxCOUNT_OF(); i++ ) {
		//	saved_asset_data.size += saved_asset_data.files[i].size;
		//}

		//
		const size_t	total_num_bytes_written_so_far = asset_bundle._file_stream.Tell();

		// the total size of data following the header
		saved_asset_data.skip = total_num_bytes_written_so_far - ( data_chunk_header_offset + sizeof(saved_asset_data) );

#if DBG_ASSET_BUNDLE_BUILDER && MX_DEBUG
if(is_dbg_bundle) ptWARN("=== Written asset data (skip = %u, total file size = %u)",
						 saved_asset_data.skip, total_num_bytes_written_so_far
						 );
#endif


		// Update chunk header (AssetDataRef).
		mxDO(asset_bundle._file_stream.Seek( data_chunk_header_offset ));
		mxDO(asset_bundle._file_stream.Put( saved_asset_data ));

		// Update the file header.
		asset_bundle._header.num_entries++;
		//asset_bundle._header.filesize = total_num_bytes_written_so_far;

		mxDO(asset_bundle._file_stream.seekToBeginning());

		// NOTE: writing the whole header may also overwrite the immediately following byte with zero.
		//mxDO(asset_bundle._file_stream.Put( asset_bundle._header ));
		mxDO(asset_bundle._file_stream.Write( &asset_bundle._header, sizeof(asset_bundle._header) - sizeof(U32) ));

		mxDO(asset_bundle._file_stream.flushToOutput());

		//
		mxDO(asset_bundle._file_stream.Seek( total_num_bytes_written_so_far ));
	}

	//
	asset_bundle._assets.Set(
		asset_id,
		saved_asset_data
		);

	//

	//mxDO(asset_database.addAssetInBundle(
	//	asset_id
	//	, asset_class
	//	, asset_data
	//	));

	mxDO(asset_database._asset_bundles.addAssetBundleIfNeeded(
		asset_bundle._name
		));

	mxDO(asset_database._asset_bundles.saveInFolderIfNeeded(
		asset_database.pathToContainingFolder()
		));

	//
	asset_bundle._file_stream.Close();//QQQ

	////
	//{
	//	NwAssetBundleT	temp(MemoryHeaps::global(), asset_bundle._name);

	//	String256	asset_bundle_filepath;
	//	mxDO(composeAssetBundleFilePath(
	//		asset_bundle_filepath
	//		, asset_database.pathToContainingFolder()
	//		, AssetId_ToChars( asset_bundle._name )
	//		));
	//	temp.mount(asset_bundle_filepath.c_str());
	//	temp.unmount();
	//}

#if 0//MX_DEBUG
	if(is_dbg_bundle)
	{
		printf("");
	}
	if(is_dbg_bundle)
	{
		NwAssetBundleT	temp(MemoryHeaps::global(), asset_bundle._name);
	
		//
		String256	asset_bundle_filepath;
		mxDO(composeAssetBundleFilePath(
			asset_bundle_filepath
			, asset_database.pathToContainingFolder()
			, AssetId_ToChars( asset_bundle._name )
			));
		temp.mount(asset_bundle_filepath.c_str());

		//
		AssetKey	key;
		key.id = asset_id;
		key.type = asset_class.GetTypeGUID();

		AssetReader	stream;
		mxDO(temp.Open(key, &stream));

		Rendering::NwMesh	mesh;
		Testbed::TbMeshLoader::loadFromStream( mesh, stream, AssetId_ToChars(key.id) );

		temp.unmount();
	}
#endif


#endif // #if !nwUSE_LOOSE_ASSETS

	return ALL_OK;
}

ERet addAssetIfNotExists_and_SaveToDisk(
	NwAssetBundleT & asset_bundle
	, const AssetID& asset_id
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	, AssetDatabase & asset_database
	)
{
	UNDONE;
	return ALL_OK;
}

ERet saveAndClose(
				  NwAssetBundleT & asset_bundle
				  )
{
	// The file is now saved immediately after adding an asset.

	//if( asset_bundle._file_stream.IsOpen() )
	//{
	//	checkAssetBundleFileHeader(asset_bundle);
	//	mxASSERT( asset_bundle._header.num_entries > 0 );

	//	//
	//	mxDO(asset_bundle._file_stream.seekToBeginning());
	//	mxDO(asset_bundle._file_stream.Put( asset_bundle._header ));

	//	mxDO(asset_bundle._file_stream.flushToOutput());
	//}

	return ALL_OK;
}

}//namespace TbDevAssetBundleUtil

/*
----------------------------------------------------------
	DevAssetBundler
----------------------------------------------------------
*/
DevAssetBundler::DevAssetBundler(
								 NwAssetBundleT & asset_bundle
								 , const TbMetaClass* accepted_asset_class /*= nil*/
								 )
								 : _asset_bundle( asset_bundle )
								 , _accepted_asset_class( accepted_asset_class )
{
	//
}

DevAssetBundler::~DevAssetBundler()
{
	//
}

//
ERet DevAssetBundler::addAsset(
							   const AssetID& asset_id
							   , const TbMetaClass& asset_class
							   , const CompiledAssetData& asset_data
							   , AssetDatabase & asset_database
							   )
{
	if( _accepted_asset_class )
	{
		mxENSURE( &asset_class == _accepted_asset_class,
			ERR_OBJECT_OF_WRONG_TYPE,
			"Expected assets of type '%s', but got '%s'",
			_accepted_asset_class->GetTypeName(), asset_class.GetTypeName()
			);
	}

	mxDO(TbDevAssetBundleUtil::addAsset_and_Save(
		_asset_bundle
		, asset_id
		, asset_class
		, asset_data
		, asset_database
		));

	return ALL_OK;
}

/*
----------------------------------------------------------
	AssetBundler_None
----------------------------------------------------------
*/
AssetBundler_None::AssetBundler_None(
									 const TbMetaClass* accepted_asset_class /*= nil*/
									 )
									 : _accepted_asset_class( accepted_asset_class )
{
	//
}

AssetBundler_None::~AssetBundler_None()
{
	//
}

//
ERet AssetBundler_None::addAsset(
							   const AssetID& asset_id
							   , const TbMetaClass& asset_class
							   , const CompiledAssetData& asset_data
							   , AssetDatabase & asset_database
							   )
{
	if( _accepted_asset_class )
	{
		mxENSURE( &asset_class == _accepted_asset_class,
			ERR_OBJECT_OF_WRONG_TYPE,
			"Expected assets of type '%s', but got '%s'",
			_accepted_asset_class->GetTypeName(), asset_class.GetTypeName()
			);
	}

	mxDO(check_AssetID_string( AssetId_ToChars(asset_id) ));

	return asset_database.addOrUpdateGeneratedAsset( asset_id, asset_class, asset_data );
}

}//namespace AssetBaking
