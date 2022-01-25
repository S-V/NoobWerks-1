// code samples:
// https://developer.valvesoftware.com/wiki/Using_a_SQLite_database_with_Source
// implementation: mtwilliams/00-meta.md:
// https://gist.github.com/mtwilliams/497e940284b5516dc633
// for asset thumbnails:
// http://www.sqlite.org/cvstrac/wiki?p=BlobExample
// worth reading:
// http://cowboyprogramming.com/2008/12/29/some-uses-of-sql-databases-in-game-development/
// https://coronalabs.com/blog/2012/04/03/tutorial-database-access-in-corona/


#include "stdafx.h"
#pragma hdrstop

#include <SQLite/sqlite3.h>
#pragma comment( lib, "SQLite.lib" )

#include <Base/IO/FileIO.h>

#include <AssetCompiler/AssetDatabase.h>
#include <AssetCompiler/AssetPipeline.h>
#include <Core/Serialization/Text/TxTSerializers.h>

extern const char* TB_ASSET_BUNDLE_LIST_FILENAME;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

mxTODO("convert SQLite error code to ERet");
#define SQLITE_DO(X)\
	mxMACRO_BEGIN\
		const int __error_code = (X);\
		if( __error_code != SQLITE_OK )\
		{\
			ptERROR( "'%s' failed with error '%s' (%d)", #X, sqlite3_errstr(__error_code), __error_code );\
			return ERR_UNKNOWN_ERROR;\
		}\
	mxMACRO_END

#define SQLITE_EXEC( CONNECTION, SQL_TEXT )\
	mxMACRO_BEGIN\
		char* __error_message = NULL;\
		const int __error_code = sqlite3_exec( (CONNECTION), (SQL_TEXT), NULL, NULL, &__error_message );\
		if( __error_code != SQLITE_OK )\
		{\
			ptERROR( "SQL exec() failed: '%s', reason: '%s'", __error_message, sqlite3_errstr(__error_code) );\
			return ERR_UNKNOWN_ERROR;\
		}\
	mxMACRO_END

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
-----------------------------------------------------------------------------
	Columns
-----------------------------------------------------------------------------
*/

/// asset id - file name without extension, always in lowercase
#define SqlAssetName			"AssetName"

/// 32-bit hash of the file name
#define SqlAssetNameHash		"AssetNameHash"

/// always increasing
#define SqlSequentialId			"SequentialId"

//

/// asset type - the name of the class
#define SqlAssetTypeName		"AssetTypeName"

/// the absolute path of the source file
#define SqlSourceFilePath		"SourceFilePath"
/// the length of the source file
#define SqlSourceFileLength		"SourceFileLength"
/// the source file's timestamp obtained from OS (for change detection)
#define SqlSourceFileTimeStamp	"SourceFileTimeStamp"
/// the hash of the source file's contents (for change detection)
#define SqlSourceFileHash		"SourceFileHash"

/*
-----------------------------------------------------------------------------
	AssetInfo
-----------------------------------------------------------------------------
*/
namespace
{
	AllocatorI& tempMemHeap() { return MemoryHeaps::temporary(); }

	I64 AssetFileHashToI64( const AssetDataHash assetFileHash )
	{
		return always_cast<I64>( assetFileHash );	// SQLite natively supports only signed 64-bit integers.
	}


	ERet SQL_readAssetInfos(
		sqlite3 * database
		, const String& commandText	// SQL statement, UTF-8 encoded
		, DynamicArray< AssetInfo > &assetInfosArray_
		)
	{
		// Compile an SQL statement.

		sqlite3_stmt* stmt;
		SQLITE_DO(sqlite3_prepare_v2(
			database	// Database handle
			, commandText.c_str()	// SQL statement, UTF-8 encoded
			, commandText.length()	// Maximum length of zSql in bytes.
			, &stmt	// OUT: Statement handle
			, NULL	// OUT: Pointer to unused portion of zSql
			));

		const int column_count = sqlite3_column_count(stmt);
		mxUNUSED(column_count);

		int return_code = SQLITE_ERROR;
		do
		{
			return_code = sqlite3_step(stmt);

			switch( return_code )
			{
			case SQLITE_OK:		// Successful result
			case SQLITE_DONE:	// sqlite3_step() has finished executing
				break;

			case SQLITE_ROW:	// sqlite3_step() has another row ready
				{
//for( int column_index = 0; column_index < column_count; column_index++ )
//{
//	const char* column_name = sqlite3_column_name( stmt, column_index );
//	ptPRINT("Column [%d] is '%s'", column_index, column_name);
//}

					AssetInfo	newAssetInfo;

					//

					const unsigned char* szAssetName = sqlite3_column_text( stmt, 0 );
					newAssetInfo.id = AssetID_from_FilePath( (const char*) szAssetName );

					// Skip:
					// name hash
					// sequential id

					const unsigned char* szFileType = sqlite3_column_text( stmt, 3 );
					Str::CopyS( newAssetInfo.class_name, (char*)szFileType );


					// Read Source File info:

					const unsigned char* szSrcFilePath = sqlite3_column_text( stmt, 4 );
					Str::CopyS( newAssetInfo.source_file.path, (char*)szSrcFilePath );

					newAssetInfo.source_file.length = (U64)sqlite3_column_int64( stmt, 5 );

					const FILETIME timestamp = always_cast<FILETIME>(sqlite3_column_int64( stmt, 6 ));
					newAssetInfo.source_file.timestamp.time = timestamp;

					//char temp[256];
					//OS::IO::Win32_FileTimeStampToDateTimeString(timestamp, temp, 255);

					newAssetInfo.source_file.checksum = (U64)sqlite3_column_int64( stmt, 7 );
					//newAssetInfo.Version = (U64)sqlite3_column_int64( stmt, 7 );


					//
					assetInfosArray_.add( newAssetInfo );
				}
				continue;

			default:
				ptERROR("SQL error: '%s' (%d)", sqlite3_errstr(return_code), return_code);
				break;
			}
		} while( return_code == SQLITE_ROW );

		SQLITE_DO(sqlite3_finalize(stmt));

		return ALL_OK;
	}
}//namespace


/*
-----------------------------------------------------------------------------
	CompiledAssetData
-----------------------------------------------------------------------------
*/
ERet CompiledAssetData::SaveAssetDataToDisk(
	const AssetID& asset_id
	, const char* asset_class_name
	, const char* asset_data_root_folder
	) const
{
	const CompiledAssetData& asset_data = *this;

	mxASSERT(check_AssetID_string( asset_id.d.c_str() ) == ALL_OK);
	mxASSERT(asset_data.isOk());
	mxASSERT(strlen(asset_class_name) > 0);

	// Ensure there's a folder with the name of the asset's class.
	FilePathStringT	destination_folder;
	{
		Str::CopyS(destination_folder, asset_data_root_folder);
		Str::NormalizePath(destination_folder);
		Str::AppendS(destination_folder, asset_class_name);
	}
	OS::IO::MakeDirectoryIfDoesntExist( destination_folder.c_str() );

	//
	FilePathStringT	object_data_filepath;
	FilePathStringT	stream_data_filepath;
	FilePathStringT	asset_metadata_filepath;

	ComposeAssetDataFilePaths(
		object_data_filepath
		, stream_data_filepath
		, asset_metadata_filepath
		, asset_id
		, destination_folder.c_str()
		);

	//
	if( !asset_data.object_data.IsEmpty() )
	{
		LogStream(LL_Info) << GetCurrentTimeString<String32>(':')
			<< ": Saving '" << asset_class_name
			<< "' -> " << Str::GetFileName(object_data_filepath.c_str())
			;

		mxDO(NwBlob_::saveBlobToFile(
			asset_data.object_data,
			object_data_filepath.c_str()
			));
	}

	//
	if( !asset_data.stream_data.IsEmpty() )
	{
		mxTRY(NwBlob_::saveBlobToFile(
			asset_data.stream_data,
			stream_data_filepath.c_str()
			));
	}

	//
	if( !asset_data.metadata.IsEmpty() )
	{
		mxTRY(NwBlob_::saveBlobToFile(
			asset_data.metadata,
			asset_metadata_filepath.c_str()
			));
	}

	return ALL_OK;
}

void CompiledAssetData::ComposeAssetDataFilePaths(
	FilePathStringT &object_data_filepath_
	, FilePathStringT &stream_data_filepath_
	, FilePathStringT &asset_metadata_filepath_
	, const AssetID& asset_id
	, const char* destination_folder
	)
{
	mxASSERT(check_AssetID_string(asset_id.d.c_str()) == ALL_OK);

	Str::ComposeFilePath(
		object_data_filepath_
		, destination_folder
		, asset_id.d.c_str()
		);

	//
	Str::Copy( stream_data_filepath_, object_data_filepath_ );
	Str::AppendS( stream_data_filepath_, ".0" );

	//
	Str::Copy( asset_metadata_filepath_, object_data_filepath_ );
	Str::AppendS( asset_metadata_filepath_, ".meta" );
}

/*
-----------------------------------------------------------------------------
	AssetBundleList
-----------------------------------------------------------------------------
*/
AssetBundleList::AssetBundleList()
{
	this->clear();
}

ERet AssetBundleList::loadFromFolder( const char* folder )
{
	mxASSERT(this->ids.IsEmpty());

	String256	asset_bundle_list_file_path;
	Str::ComposeFilePath(
		asset_bundle_list_file_path, folder, TB_ASSET_BUNDLE_LIST_FILENAME
		);

	if( OS::IO::FileExists( asset_bundle_list_file_path.c_str() ) )
	{
		mxDO(loadAssetIDsInFolder( this->ids, folder ));
	}

	return ALL_OK;
}

void AssetBundleList::clear()
{
	this->ids.RemoveAll();
	this->is_dirty_and_must_be_flushed_to_disk = false;
}

ERet AssetBundleList::addAssetBundleIfNeeded( const AssetID& asset_bundle_id )
{
	if( !this->ids.contains( asset_bundle_id ) )
	{
		mxDO(this->ids.add( asset_bundle_id ));
		this->is_dirty_and_must_be_flushed_to_disk = true;
	}
	return ALL_OK;
}

ERet AssetBundleList::saveInFolderIfNeeded( const char* folder ) const
{
	if( this->is_dirty_and_must_be_flushed_to_disk )
	{
		String256	asset_bundle_list_file_path;
		Str::ComposeFilePath(
			asset_bundle_list_file_path, folder, TB_ASSET_BUNDLE_LIST_FILENAME
			);

		mxDO(SON::SaveToFile(
			this->ids,
			asset_bundle_list_file_path.c_str()
			));

		this->is_dirty_and_must_be_flushed_to_disk = false;
	}

	return ALL_OK;
}

ERet AssetBundleList::loadAssetIDsInFolder(
	TbAssetBundle::IDList &ids_
	, const char* folder
	)
{
	String256	asset_bundle_list_file_path;
	Str::ComposeFilePath(
		asset_bundle_list_file_path, folder, TB_ASSET_BUNDLE_LIST_FILENAME
		);

	mxDO(SON::LoadFromFile(
		asset_bundle_list_file_path.c_str(),
		ids_,
		tempMemHeap()
		));

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
	AssetDatabase
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( AssetDatabase );
mxBEGIN_REFLECTION( AssetDatabase )
	//mxMEMBER_FIELD( m_config ),
	//mxMEMBER_FIELD( m_assets ),
mxEND_REFLECTION
AssetDatabase::AssetDatabase()
{
	_transaction_depth = 0;
}

AssetDatabase::~AssetDatabase()
{
	mxASSERT2(_sql_db_connection.IsNil(), "Did you forget to close the database?");
	mxASSERT(_transaction_depth == 0);
}

ERet AssetDatabase::_EnsureTablesExist()
{
	//// This command will cause SQLite to not wait on data to reach the disk surface,
	//// which will make write operations appear to be much faster.
	//// But if you lose power in the middle of a transaction, your database file might go corrupt.
	//SQLITE_EXEC(_sql_db_connection,
	//	"PRAGMA synchronous=OFF"
	//);

	mxDO(this->beginTransaction());

	//NOTE: SQLite understands the column type of "VARCHAR(N)" to be the same as "TEXT", regardless of the value of N.
	//NOTE: SQLite natively supports only _signed_ 64-bit integers.
	SQLITE_EXEC(_sql_db_connection,
		"CREATE TABLE IF NOT EXISTS Assets ("
			""SqlAssetName" TEXT"	// asset id - file name without extension, always in lowercase
			","SqlAssetNameHash" INT32 NOT NULL"	// 32-bit hash of the file name
			","SqlSequentialId" INT32 AUTO_INCREMENT PRIMARY KEY"// always increasing

			// asset type - the name of the class
			","SqlAssetTypeName" TEXT"

			// source file info
			","SqlSourceFilePath" TEXT"
			","SqlSourceFileLength" INT64 NOT NULL DEFAULT '0'"
			","SqlSourceFileTimeStamp" INT64 NOT NULL DEFAULT '0'"	// the source file's timestamp obtained from OS
			","SqlSourceFileHash" INT64 NOT NULL DEFAULT '0'"
			//",Version INT64 NOT NULL DEFAULT '0'"
		")"
	);

	// The 'Dependencies' table stores full (absolute) file paths of dependent assets.
	SQLITE_EXEC(_sql_db_connection,
		"CREATE TABLE IF NOT EXISTS Dependencies (SourceFile TEXT, DependentFile TEXT)"
	);

	SQLITE_EXEC(_sql_db_connection,
		"CREATE TABLE IF NOT EXISTS BuildScript ("SqlAssetName" TEXT PRIMARY key, ScriptText TEXT)"
	);

	mxDO(this->commitTransaction());

	return ALL_OK;
}

ERet AssetDatabase::_DropTablesIfExist()
{
	SQLITE_EXEC(_sql_db_connection,
		"DROP TABLE IF EXISTS Assets"
	);
	SQLITE_EXEC(_sql_db_connection,
		"DROP TABLE IF EXISTS BuildScript"
	);
	SQLITE_EXEC(_sql_db_connection,
		"DROP TABLE IF EXISTS Dependencies"
	);
	return ALL_OK;
}

ERet AssetDatabase::CreateNew( const char* databaseFilePath )
{
	this->Close();

	mxDO(Str::CopyS( _database_filepath, databaseFilePath ));

	mxDO(Str::CopyS(_database_folder, databaseFilePath));
	Str::StripFileName(_database_folder);

	SQLITE_DO(sqlite3_open_v2(
		databaseFilePath,
		&_sql_db_connection._ptr,
		SQLITE_OPEN_NOMUTEX|SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,
		NULL
	));

	ptPRINT("Creating a new asset database '%s', Using SQLite '%s'",
		databaseFilePath, sqlite3_libversion()
		);

	this->_EnsureTablesExist();

	return ALL_OK;
}

ERet AssetDatabase::OpenExisting( const char* databaseFilePath )
{
	mxENSURE(mxIsValidHeapPointer(databaseFilePath), ERR_INVALID_PARAMETER,
		"Invalid string pointer: %P", databaseFilePath
		);

	this->Close();

	mxDO(Str::CopyS( _database_filepath, databaseFilePath ));

	mxDO(Str::CopyS(_database_folder, databaseFilePath));
	Str::StripFileName(_database_folder);

	SQLITE_DO(sqlite3_open_v2(
		databaseFilePath,
		&_sql_db_connection._ptr,
		SQLITE_OPEN_NOMUTEX|SQLITE_OPEN_READWRITE,
		NULL
	));

	ptPRINT("Opening asset database '%s', SQLite version: '%s', %d records",
		databaseFilePath, sqlite3_libversion(), getTotalNumRecords()
		);

	this->_EnsureTablesExist();

	//
	mxDO(_asset_bundles.loadFromFolder( _database_folder.c_str() ));

	return ALL_OK;
}

void AssetDatabase::clearDB()
{
	this->_DropTablesIfExist();
	this->_EnsureTablesExist();
}

void AssetDatabase::Close()
{
	mxASSERT(_transaction_depth == 0);
	_transaction_depth = 0;

	if( _sql_db_connection != NULL )
	{
		sqlite3_close_v2(_sql_db_connection);
		_sql_db_connection = NULL;
	}

	_database_filepath.empty();

	_asset_bundles.clear();

	_database_folder.empty();
}

const char* AssetDatabase::pathToContainingFolder() const
{
	mxASSERT(this->IsOpen());
	mxASSERT(!_database_folder.IsEmpty());
	return _database_folder.c_str();
}

U64 AssetDatabase::getLastRowId() const
{
	return sqlite3_last_insert_rowid( _sql_db_connection );
}

int AssetDatabase::getTotalNumRecords() const
{
	struct CallbackWrapper
	{
		static int callback(void *count, int argc, char **argv, char **azColName)
		{
			int *c = (int*) count;
			*c = atoi(argv[0]);
			return 0;
		}
	};

	int count = 0;

	char *zErrMsg = nil;
	const int rc = sqlite3_exec(
		_sql_db_connection,
		"SELECT COUNT(*) FROM Assets", CallbackWrapper::callback,
		&count,
		&zErrMsg
		);
	mxASSERT( rc == SQLITE_OK );
	if( rc != SQLITE_OK )
	{
		ptWARN( "SQL error: %s\n", zErrMsg );
		sqlite3_free( zErrMsg );
	}

	return count;
}

const AssetID AssetDatabase::generateNextAssetId()
{
	String32	buf;
	Str::Format( buf, "%x", this->getTotalNumRecords() );
	const AssetID nextAssetId = MakeAssetID( buf.c_str() );

#if MX_DEBUG || MX_DEVELOPER
	mxASSERT(!this->containsAssetWithId( nextAssetId ));
#endif

	return nextAssetId;
}

bool AssetDatabase::containsAssetWithId( const AssetID& asset_id ) const
{
	AssetInfo	existingAssetInfo;

	DynamicArray< AssetInfo >	found_assets( tempMemHeap() );
	const ERet ret = this->findAllAssetsWithId(
		asset_id
		, nil
		, found_assets
		);
	mxASSERT( ret == ALL_OK );

	return !found_assets.IsEmpty();
}

bool AssetDatabase::containsAssetWithId(
						 const AssetID& asset_id
						 , const TbMetaClass& asset_class
						 ) const
{
	return this->containsAssetWithId(
		asset_id,
		asset_class.GetTypeName()
		);
}

bool AssetDatabase::containsAssetWithId(
										const AssetID& asset_id
										, const char* asset_class_name
										) const
{
	FilePathStringT	commandText;
	Str::Format(commandText,
		"SELECT * FROM Assets"
		" WHERE "
		SqlAssetName " = lower('%s')"
		" AND "
		SqlAssetTypeName " = '%s'"
		";"
		, AssetId_ToChars(asset_id)
		, asset_class_name
	);

	DynamicArrayWithLocalBuffer< AssetInfo, 4 >	found_assets( tempMemHeap() );

	/*mxDO*/(SQL_readAssetInfos(
		_sql_db_connection
		, commandText
		, found_assets
		));

	return !found_assets.IsEmpty();
}

ERet AssetDatabase::findAssetWithId(
	const AssetID& asset_id
	, const TbMetaClass& asset_class
	, AssetInfo &found_asset_info_
	) const
{
	DynamicArray< AssetInfo >	existing_assets_with_this_id( tempMemHeap() );

	mxTRY(this->findAllAssetsWithId(
		asset_id
		, &asset_class
		, existing_assets_with_this_id
		));

	// If we found several assets with the same ID and of the same type, there must be an error:
	if( !existing_assets_with_this_id.IsEmpty() )
	{
		mxENSURE(existing_assets_with_this_id.num() == 1, ERR_UNKNOWN_ERROR,
			"There must be only one asset '%s'of type '%s', but got %d",
			AssetId_ToChars(asset_id), asset_class.GetTypeName(), existing_assets_with_this_id.num()
			);
		found_asset_info_ = existing_assets_with_this_id.getFirst();
	}
	else
	{
		found_asset_info_.setInvalid();
	}

	return ALL_OK;
}

ERet AssetDatabase::findAllAssetsWithId(
										const AssetID& asset_id
										, const TbMetaClass* asset_class
										, DynamicArray< AssetInfo > &found_assets_
										) const
{
	FilePathStringT	commandText;

	if( asset_class )
	{
		const char* asset_class_name = asset_class->GetTypeName();

		Str::Format(commandText,
			"SELECT * FROM Assets"
			" WHERE "
			SqlAssetName " = lower('%s')"
			" AND "
			SqlAssetTypeName " = '%s'"
			";"
			, AssetId_ToChars(asset_id)
			, asset_class_name
			);
	}
	else
	{
		Str::Format(commandText,
			"SELECT * FROM Assets"
			" WHERE "
			SqlAssetName " = lower('%s')"
			";",
			AssetId_ToChars(asset_id)
			);
	}

	//
	mxDO(SQL_readAssetInfos(
		_sql_db_connection
		, commandText
		, found_assets_
		));

	return ALL_OK;
}

ERet AssetDatabase::addNewAsset(
	const AssetInfo& asset
	, const CompiledAssetData& asset_data
	, const bool save_asset_data /*= true*/
)
{
	mxASSERT(asset.IsValid());

#if MX_DEBUG || MX_DEVELOPER
	mxASSERT(!this->containsAssetWithId(
		asset.id
		, asset.class_name.c_str()
		));
#endif

	DBGOUT("AssetDatabase: inserting '%s' of type '%s'...",
		asset.id.d.c_str(), asset.class_name.c_str()
		);

	if( save_asset_data )
	{
		mxDO(asset_data.SaveAssetDataToDisk(
			asset.id
			, asset.class_name.c_str()
			, _database_folder.c_str()
			));
	}

	const AssetNameHash asset_name_hash = MurmurHash32( asset.id.d.c_str(), asset.id.d.size() );

	// SQLite natively supports only signed 64-bit integers.
	const I64 timestamp = always_cast<I64>(asset.source_file.timestamp.time);

	//
	FilePathStringT	commandText;
	Str::Format(commandText,
		"INSERT INTO Assets (" SqlAssetName "," SqlAssetNameHash "," SqlSequentialId
			"," SqlAssetTypeName
			"," SqlSourceFilePath "," SqlSourceFileLength "," SqlSourceFileTimeStamp "," SqlSourceFileHash
			")"
			"VALUES( lower('%s'), %u, NULL"
			", '%s'"
			", '%s', %lld, %lld, %lld"
			");"
			, asset.id.d.c_str(), asset_name_hash
			, asset.class_name.c_str()
			, asset.source_file.path.c_str(), (I64)asset.source_file.length, (I64)timestamp, (I64)asset.source_file.checksum
	);

	SQLITE_EXEC( _sql_db_connection, commandText.c_str() );

	return ALL_OK;
}

ERet AssetDatabase::updateAsset(
								const AssetInfo& asset
								, const CompiledAssetData& asset_data
								)
{
	mxASSERT(asset.IsValid());

#if MX_DEBUG || MX_DEVELOPER
	mxASSERT(this->containsAssetWithId(
		asset.id
		, asset.class_name.c_str()
		));
#endif

	mxDO(asset_data.SaveAssetDataToDisk(
		asset.id
		, asset.class_name.c_str()
		, _database_folder.c_str()
		));

	// SQLite natively supports only signed 64-bit integers.
	const I64 timestamp = always_cast<I64>(asset.source_file.timestamp.time);

	FilePathStringT	commandText;

	Str::Format(commandText,
		"UPDATE Assets"
		" SET"
		"  " SqlAssetTypeName" = '%s'"
		", " SqlSourceFilePath" = '%s'"
		", " SqlSourceFileLength " = %lld"
		", " SqlSourceFileTimeStamp " = %lld"
		", " SqlSourceFileHash" = %lld"

		" WHERE " SqlAssetName " = lower('%s')"
		" AND " SqlAssetTypeName " = '%s'"
		";"

		,      asset.class_name.c_str()
		,      asset.source_file.path.c_str()
		, (I64)asset.source_file.length
		, (I64)timestamp
		, (I64)asset.source_file.checksum

		, AssetId_ToChars(asset.id)
		, asset.class_name.c_str()
	);

	SQLITE_EXEC(_sql_db_connection,
		commandText.c_str()
	);

	return ALL_OK;
}

ERet AssetDatabase::addOrUpdateGeneratedAsset(
	const AssetID& suggested_asset_id	// pass empty Asset ID to generate an Asset ID automatically
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	, AssetID *returned_asset_id_ /*= nil*/
	)
{
	// Try to find an existing asset and re-use its ID, if possible.
	AssetInfo	existing_asset_info;

	// If the user has provided a valid asset ID:
	if( AssetId_IsValid( suggested_asset_id ) )
	{
		mxDO(this->findAssetWithId(
			suggested_asset_id
			, asset_class
			, existing_asset_info
			));

		const bool asset_with_given_id_already_exists = existing_asset_info.IsValid();

		if( asset_with_given_id_already_exists )
		{
			mxDO(this->updateAsset(
				existing_asset_info
				, asset_data
				));
		}
		else
		{
			AssetBaking::Utils::fillInAssetInfo(
				&existing_asset_info
				, suggested_asset_id
				, asset_class
				, asset_data
				);

			mxDO(this->addNewAsset(
				existing_asset_info
				, asset_data
				));
		}

		if( returned_asset_id_ ) {
			*returned_asset_id_ = suggested_asset_id;
		}
	}
	else
	{
		mxDO(this->findExistingAssetWithSameContents(
			existing_asset_info
			, asset_class
			, asset_data
			));

		if( existing_asset_info.IsValid() )
		{
			//DBGOUT("Duplicate prevention: found existing asset '%s' of type '%s'.",
			//	AssetId_ToChars(existing_asset_info.id), existing_asset_info.class_name.c_str()
			//	);

			mxDO(this->updateAsset(
				existing_asset_info
				, asset_data
				));

			if( returned_asset_id_ ) {
				*returned_asset_id_ = existing_asset_info.id;
			}
		}
		else
		{
			const AssetID new_asset_id = this->generateNextAssetId();

			AssetBaking::Utils::fillInAssetInfo(
				&existing_asset_info
				, new_asset_id
				, asset_class
				, asset_data
				);

			mxDO(this->addNewAsset(
				existing_asset_info
				, asset_data
				));

			if( returned_asset_id_ ) {
				*returned_asset_id_ = new_asset_id;
			}
		}
	}

	return ALL_OK;
}

ERet AssetDatabase::addAssetInBundle(
	const AssetID& asset_id
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	)
{
	AssetInfo	asset;
	AssetBaking::Utils::fillInAssetInfo( &asset, asset_id, asset_class, asset_data );

	mxDO(this->addNewAsset(
		asset
		, asset_data
		, false	// save_asset_data
		));

	return ALL_OK;
}

ERet AssetDatabase::addDependency(
	const char* source_file_path,
	const char* dependent_file_path
)
{
//	DBGOUT("AssetDatabase: '%s' depends on '%s'", dependent_file_path, source_file_path);

	FilePathStringT commandText;

	Str::Format(commandText,
		"DELETE FROM Dependencies WHERE SourceFile=lower('%s') AND DependentFile=lower('%s')",
		source_file_path, dependent_file_path
	);
	SQLITE_EXEC(_sql_db_connection,
		commandText.c_str()
	);

	Str::Format(commandText,
		"INSERT INTO Dependencies (SourceFile, DependentFile) VALUES(lower('%s'), lower('%s'))",
		source_file_path, dependent_file_path
	);
	SQLITE_EXEC(_sql_db_connection,
		commandText.c_str()
	);

	return ALL_OK;
}

ERet AssetDatabase::getDependentFileNames(
	const char* source_file_path,
	TArray< String64 > &dependentFileNames_
) const
{
	FilePathStringT	commandText;

	Str::Format(commandText,
		"SELECT DependentFile FROM Dependencies WHERE SourceFile = lower('%s');", source_file_path);

	sqlite3_stmt* stmt;
	SQLITE_DO(sqlite3_prepare_v2(_sql_db_connection,
		commandText.c_str(),
		commandText.length(),
		&stmt, NULL
	));

	dependentFileNames_.RemoveAll();

	int return_code = SQLITE_ERROR;
	do
	{
		return_code = sqlite3_step(stmt);
		switch( return_code )
		{
		case SQLITE_OK:
		case SQLITE_DONE:
			break;
		case SQLITE_ROW:
			{
				const char* dependentFile = (const char*) sqlite3_column_text( stmt, 0 );

				// If the shader code is described within the corresponding *.fx file,
				// then the *.fx file becomes dependent on itself.

				// If the source file differs from the dependent file:
				if( 0 != stricmp( source_file_path, dependentFile ) )
				{
					dependentFileNames_.AddNew().Copy(Chars((char*)dependentFile,_InitSlow));
				}
			}
			break;

		default:
			ptERROR("SQL error: '%s' (%d)", sqlite3_errstr(return_code), return_code);
			break;
		}
	} while( return_code == SQLITE_ROW );

	SQLITE_DO(sqlite3_finalize(stmt));

	return ALL_OK;
}

ERet AssetDatabase::beginTransaction()
{
	_transaction_depth++;
	if( _transaction_depth == 1 )
	{
		SQLITE_EXEC( _sql_db_connection, "BEGIN" );
	}
	return ALL_OK;
}

ERet AssetDatabase::commitTransaction()
{
	mxASSERT(_transaction_depth > 0);
	_transaction_depth--;
	if( _transaction_depth == 0 )
	{
		SQLITE_EXEC( _sql_db_connection, "COMMIT" );
	}
	return ALL_OK;
}

ERet AssetDatabase::loadCompiledAssetData(
	CompiledAssetData &data_
	,const AssetID& asset_id
	,const TbMetaClass& asset_class
	)
{
	const char* asset_class_name = asset_class.GetTypeName();

	if( !this->containsAssetWithId( asset_id, asset_class_name ) )
	{
		return ERR_OBJECT_NOT_FOUND;
	}

	//
	FilePathStringT	destination_folder;
	{
		Str::CopyS(destination_folder, _database_folder.c_str());
		Str::NormalizePath(destination_folder);
		Str::AppendS(destination_folder, asset_class_name);
	}

	//
	FilePathStringT	object_data_filepath;
	FilePathStringT	stream_data_filepath;
	FilePathStringT	asset_metadata_filepath;

	CompiledAssetData::ComposeAssetDataFilePaths(
		object_data_filepath
		, stream_data_filepath
		, asset_metadata_filepath
		, asset_id
		, destination_folder.c_str()
		);

	//
	/*mxDO*/(NwBlob_::loadBlobFromFile(
		data_.object_data,
		object_data_filepath.c_str()
		));

	/*mxDO*/(NwBlob_::loadBlobFromFile(
		data_.stream_data,
		stream_data_filepath.c_str()
		));

	/*mxDO*/(NwBlob_::loadBlobFromFile(
		data_.metadata,
		asset_metadata_filepath.c_str()
		));

	return data_.isOk() ? ALL_OK : ERR_FAILED_TO_OPEN_FILE;
}

ERet AssetDatabase::findExistingAssetWithSameContents(
	AssetInfo &found_asset_
	, const TbMetaClass& asset_class
	, const CompiledAssetData& asset_data
	)
{
	AllocatorI & scratchpad = tempMemHeap();

	//
	const AssetDataHash asset_data_hash = AssetBaking::computeHashFromAssetData( asset_data );

	const char* asset_class_name = asset_class.GetTypeName();

	//
	FilePathStringT	commandText;

	Str::Format(commandText,
		"SELECT * FROM Assets"
		" WHERE "
		SqlAssetTypeName " = '%s'"
		" AND "
		SqlSourceFileHash" = %lld;"
		";"
		, asset_class_name
		, always_cast<I64>( asset_data_hash )	// SQLite natively supports only signed 64-bit integers.
		);

	//
	DynamicArrayWithLocalBuffer< AssetInfo, 16 >	found_assets( scratchpad );

	mxDO(SQL_readAssetInfos( _sql_db_connection, commandText, found_assets ));

	//
	if( !found_assets.IsEmpty() )
	{
		nwFOR_EACH( const AssetInfo& existing_asset, found_assets )
		{
			CompiledAssetData	existing_asset_data( scratchpad );
			mxDO(loadCompiledAssetData( existing_asset_data, existing_asset.id, asset_class ));

			if( asset_data.equals( existing_asset_data ) )
			{
				found_asset_ = existing_asset;
				return ALL_OK;
			}
		}
	}

	return ALL_OK;
}

//QString AssetDatabase::GetOpenFileName( QString& folder, const QString& fileName )
//{
//	const QString result = qtGetOpenFileName(
//		folder,
//		"Resource Database Files (*.rdb)",
//		qtSetFileExtension(fileName, AssetDatabase::FILE_EXTENSION)
//		);
//	return result;
//}

// References:
//
// "Some Uses of SQL Databases in Game Development" By Mick West
// http://cowboyprogramming.com/2008/12/29/some-uses-of-sql-databases-in-game-development/
//
