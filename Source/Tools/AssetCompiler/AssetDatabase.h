/*

	NOTE: string asset IDs are source file names (without the path),
	e.g. "D:/gamedev/mygame/asset/sky.dds" -> "sky".
	Asset IDs are case-insensitive (i.e. "sky" is the same as "SKY").

*/
#pragma once

#include <Base/Template/Containers/Blob.h>

#include <Core/Assets/AssetManagement.h>
#include <Core/Assets/AssetBundle.h>
//#include <Core/NetProtocol.h>
#include <Core/Serialization/Serialization.h>

// SQLite Database Connection Handle
struct sqlite3;

typedef U32 AssetNameHash;

typedef U64 AssetDataHash;

struct SourceFileInfo
{
	/// e.g.: "C:/game/assets/models/weapons/shotgun.blend"
	/// Empty is the asset was procedurally generated (e.g. a shader permutation).
	FilePathStringT	path;

	/// length of the source file, in bytes
	U64				length;	// should always be valid

	/// last time when this asset was compiled (for all platforms)
	FileTimeT		timestamp;

	/// the hash of the source file's contents (for change detection)
	AssetDataHash	checksum;

public:
	SourceFileInfo()
	{
		length = 0;
		timestamp = FileTimeT::CurrentTime();
		checksum = 0;
	}

	bool isOk() const
	{
		return length > 0;
	}

	void clear()
	{
		path.empty();
		length = 0;
		mxZERO_OUT(timestamp);
		checksum = 0;
	}
};

///
struct AssetInfo
{
	/// unique identifier
	AssetID			id;

	/// The name of the asset's class
	String32		class_name;

	/// The '.' delimited list of group names in which the asset is found.
	//String64	Tags;

	///
	SourceFileInfo	source_file;

	/// never decreases
	//U32		version;

	/// The path to intermediate asset file relative to root folder.
	/// This is a portable path, it should contain enough information
	/// to construct an entire path given the configuration setting, platform, etc.
	/// (it's platform-independent, p with additional info, metadata, etc.)
	//String128		path;

	/// type of asset, null if unknown type
	//TbMetaClass *	type;

public:
	AssetInfo()
	{
		//Version = 0;
	}

	bool IsValid() const
	{
		return AssetId_IsValid(id) && source_file.isOk();
	}

	void setInvalid()
	{
		source_file.clear();
	}
};


/// the asset's data that may be loaded at run-time
struct CompiledAssetData: NonCopyable
{
	/// usually memory-resident data, e.g. a memory image (for in-place loading), texture headers
	NwBlob	object_data;

	/// [optional] everything that exists only on disk (e.g. shader bytecode, texture mipmaps)
	NwBlob	stream_data;

	/// the asset's data that is used only during development (e.g. for compiling dependent assets):
	NwBlob	metadata;

public:
	CompiledAssetData( AllocatorI & blob_storage = MemoryHeaps::temporary() )
		: object_data( blob_storage )
		, stream_data( blob_storage )
		, metadata( blob_storage )
	{}

	//NOTE: doesn't compare metadata!
	bool equals( const CompiledAssetData& other ) const
	{
		return this->object_data == other.object_data
			&& this->stream_data == other.stream_data
			;
	}

	bool isOk() const
	{
		// some assets (e.g. sounds) can only have streaming data
		return !object_data.IsEmpty() || !stream_data.IsEmpty();
	}

	NO_COMPARES(CompiledAssetData);

public:
	ERet SaveAssetDataToDisk(
		const AssetID& asset_id
		, const char* asset_class_name
		, const char* asset_data_root_folder
	) const;

	static void ComposeAssetDataFilePaths(
		FilePathStringT &object_data_filepath_
		, FilePathStringT &stream_data_filepath_
		, FilePathStringT &asset_metadata_filepath_
		, const AssetID& asset_id
		, const char* destination_folder
		);
};

/*
-----------------------------------------------------------------------------
	AssetBundleList
-----------------------------------------------------------------------------
*/
struct AssetBundleList: NonCopyable
{
	TbAssetBundle::IDList	ids;
	mutable bool			is_dirty_and_must_be_flushed_to_disk;

public:
	AssetBundleList();

	ERet loadFromFolder( const char* folder );

	void clear();

	ERet addAssetBundleIfNeeded( const AssetID& asset_bundle_id );

	ERet saveInFolderIfNeeded( const char* folder ) const;

public:
	static ERet loadAssetIDsInFolder(
		TbAssetBundle::IDList &ids_
		, const char* folder
		);
};

/*
-----------------------------------------------------------------------------
	AssetDatabase
	stores information about all assets, including inter-dependencies;
	allows for fast mapping human-readable names to numeric ids and vice versa.

	NOTE: its methods are *NOT* thread-safe!
-----------------------------------------------------------------------------
*/
class AssetDatabase: CStruct
{
public_internal:
	/// where SQL DB is stored
	FilePathStringT		_database_filepath;

	/// where compiled asset files are stored
	FilePathStringT		_database_folder;

	//
	AssetBundleList		_asset_bundles;

	//

	TPtr< sqlite3 >		_sql_db_connection;

	U32					_transaction_depth;

public:
	mxDECLARE_CLASS( AssetDatabase, CStruct );
	mxDECLARE_REFLECTION;

	AssetDatabase();
	~AssetDatabase();

	/// creates a new asset database
	ERet CreateNew( const char* databaseFilePath );

	/// loads asset database from the given file
	ERet OpenExisting( const char* databaseFilePath );

	void clearDB();

	bool IsOpen() const { return _sql_db_connection != NULL; }

	void Close();

	const char* pathToContainingFolder() const;

public:	// Asset compiler

	U64 getLastRowId() const;

	int getTotalNumRecords() const;

	/// for shader/program permutations
	const AssetID generateNextAssetId();

	/// Is object an asset?
	bool containsAssetWithId(
		const AssetID& asset_id
		) const;

	bool containsAssetWithId(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		) const;

	/// assets can have the same ID, but different types, e.g.:
	/// "material_default.fx" and "material_default.material"
	bool containsAssetWithId(
		const AssetID& asset_id
		, const char* asset_class_name
		) const;

	ERet findAssetWithId(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, AssetInfo &found_asset_info_
		) const;

	/// assets can have the same ID, but different types, e.g.:
	/// "material_default.fx" and "material_default.material"
	ERet findAllAssetsWithId(
		const AssetID& asset_id
		, const TbMetaClass* asset_class	// optional
		, DynamicArray< AssetInfo > &found_assets_
	) const;

	/// Registers a new asset in the database and saves the asset's data in the output folder.
	ERet addNewAsset(
		const AssetInfo& asset
		, const CompiledAssetData& asset_data
		, const bool save_asset_data = true
	);

	ERet updateAsset(
		const AssetInfo& asset
		, const CompiledAssetData& asset_data
	);

	/// Used for automatically generated assets (e.g. shader permutations/programs).
	/// this is used to prevent creation of duplicate shader/program assets
	ERet addOrUpdateGeneratedAsset(
		const AssetID& suggested_asset_id	// pass empty Asset ID to generate an Asset ID automatically
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		, AssetID *returned_asset_id_ = nil
		);

	ERet addAssetInBundle(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		);

	/// Adds a dependency to the specified file.
	/// This causes a rebuild of the file, when modified, on subsequent incremental builds.
	/// (A change to the source file will trigger a recompile of a dependent file.)
	/// NOTE: filenames are case-insensitive!
	ERet addDependency(
		const char* source_file_path,
		const char* dependent_file_path
	);

	//// Returns the list of all assets that this asset depends on.
	//ERet GetDependencies(
	//	const StringAssetID& _dependentAsset,
	//	TArray< StringAssetID > &dependentFileNames_
	//) const;

	/// Returns the list of all assets that depend on this asset.
	/// NOTE: the filename is case-insensitive!
	ERet getDependentFileNames(
		const char* source_file_path,
		TArray< String64 > &dependent_files_
	) const;

	// optimization to reduce write time
	ERet beginTransaction();
	ERet commitTransaction();

public:
	//mxDEPRECATED
	//ERet loadCompiledAssetIntoBlob(
	//	Blob &blob_
	//	,const AssetID& asset_id
	//	,const TbMetaClass& asset_class
	//	);

	ERet loadCompiledAssetData(
		CompiledAssetData &data_
		,const AssetID& asset_id
		,const TbMetaClass& asset_class
		);

#if 0
	ERet saveAssetDataToDisk(
		const Blob& assetData
		, const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const Blob* streamingData = nil
		);

	/// Registers a new asset in the database and saves it in the output folder.
	/// Useful for automatically generated assets (e.g. shader permutations/programs).
	///
	mxDEPRECATED
	ERet addOrUpdateGeneratedAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const Blob& assetData
		, const Blob* streamingData = nil
		);

	ERet addOrUpdateGeneratedAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_type
		, const CompiledAssetData& asset_data
		);
#endif


private:
	ERet _EnsureTablesExist();
	ERet _DropTablesIfExist();

	/// this is used to prevent creation of duplicate shader/program assets
	ERet findExistingAssetWithSameContents(
		AssetInfo &found_asset_
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		);
};

/* References:

https://docs.unity3d.com/ScriptReference/AssetDatabase.html

https://seanmiddleditch.com/resource-pipelines-part-4-dependencies/

An asset management & processing framework for game engines. 
https://github.com/amethyst/atelier-assets
*/
