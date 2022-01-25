/*
=============================================================================
	File:	AssetID.cpp
	Desc:	Asset/Resource management.
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Base/Math/Hashing/HashFunctions.h>
#include <Base/Util/Sorting.h>
#include <Core/Core.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory.h>
#include <Core/ObjectModel/Clump.h>
#include <Core/Serialization/Serialization.h>

#include <Core/Assets/AssetID.h>

/*
-----------------------------------------------------------------------------
	AssetID
-----------------------------------------------------------------------------
*/
bool AssetId_IsNull( const AssetID& assetId ) {
	return assetId.d.IsEmpty();
}

bool AssetId_IsValid( const AssetID& assetId ) {
	return !AssetId_IsNull( assetId );
}

bool AssetIds_AreEqual( const AssetID& assetIdA, const AssetID& assetIdB ) {
	return assetIdA.d == assetIdB.d;
}

AssetID AssetId_GetNull() {
	return AssetID();
}

U32 AssetId_GetHash32( const AssetID& assetId ) {
	return assetId.d.hash();
}

const char* AssetId_ToChars( const AssetID& assetId )
{
	mxASSERT(AssetId_IsValid(assetId));
	return assetId.d.c_str();
}

const AssetID MakeAssetID( const char* _name )
{
	AssetID result;
	make_AssetID_from_filepath( result, _name );
	return result;
}

ERet check_AssetID_string( const char* name )
{
	mxENSURE(strlen( name ) > 0, ERR_INVALID_PARAMETER,
		"Asset IDs cannot be empty: '%s'", name
		);

	mxENSURE(NULL == strchr( name, '/' ), ERR_INVALID_PARAMETER,
		"Asset IDs must not have a path component: '%s'", name
		);
	mxENSURE(NULL == Str::FindExtensionS( name ), ERR_INVALID_PARAMETER,
		"Asset IDs must not have an extension: '%s'", name
		);

	return ALL_OK;
}

ERet make_AssetID_from_string( AssetID &asset_id_
								, const char* asset_name
								)
{
	asset_id_.d = NameID();

	mxTRY(check_AssetID_string( asset_name ));

	asset_id_.d = NameID( asset_name );
	return ALL_OK;
}

#if 1||MX_DEVELOPER

ERet make_AssetID_from_filepath( AssetID &asset_id_
								, const char* filepath
								)
{
	String256	asset_name;

	mxDO(Str::CopyS( asset_name, filepath ));
	Str::StripPath( asset_name );
	Str::StripFileExtension( asset_name );

	//
	mxTRY(make_AssetID_from_string( asset_id_, asset_name.c_str() ));

	return ALL_OK;
}

AssetID make_AssetID_from_raw_string( const char* filepath )
{
	mxASSERT(strlen( filepath ) > 0);

	AssetID	asset_id;
	asset_id.d = NameID( filepath );
	return asset_id;
}

mxDEPRECATED
const AssetID AssetID_from_FilePath( const char* _file )
{
	String128 assetId;
	Str::CopyS(assetId, _file);
	Str::StripPath( assetId );
	Str::StripFileExtension(assetId);
	mxASSERT(!assetId.IsEmpty());
	return MakeAssetID(assetId.c_str());
}

#endif // MX_DEVELOPER

ERet readAssetID( AssetID &asset_id_, AReader& stream )
{
	U16 len;
	mxTRY(stream.Get( len ));

	if( len > 0 )
	{
		char buffer[ AssetID::MAX_LENGTH ];
		if( len >= mxCOUNT_OF(buffer) ) {
			mxASSERT(false);
			return ERR_BUFFER_TOO_SMALL;
		}
		len = smallest(len, mxCOUNT_OF(buffer)-1);
		mxTRY(stream.Read( buffer, len ));
		buffer[ len ] = '\0';

		asset_id_.d = NameID( buffer );
	}
	else
	{
		asset_id_.d = NameID();
	}

	return ALL_OK;
}

// NOTE: does not align the write cursor!
ERet writeAssetID( const AssetID& assetId, AWriter &stream )
{
	const U16 length = assetId.d.size();

	mxTRY(stream.Put( length ));

	mxTRY(stream.Write( assetId.d.raw(), length ));

	return ALL_OK;
}

/*
--------------------------------------------------------------
	AssetKey
--------------------------------------------------------------
*/
mxDEFINE_CLASS( AssetKey );
mxBEGIN_REFLECTION( AssetKey )
	mxMEMBER_FIELD( id ),
	mxMEMBER_FIELD( type ),
mxEND_REFLECTION

bool AssetKey::IsOk() const
{
	return AssetId_IsValid(id) && (type != mxNULL_TYPE_ID);
}

const char* AssetType_To_Chars( AssetTypeT classGUID )
{
	return TypeRegistry::FindClassByGuid( classGUID )->GetTypeName();
}

AWriter& operator << ( AWriter& stream, const AssetID& o )
{
	writeAssetID( o, stream );
	return stream;
}

AReader& operator >> ( AReader& stream, AssetID &o_ )
{
	readAssetID( o_, stream );
	return stream;
}

ATextStream & operator << ( ATextStream & log, const AssetID& o )
{
	log.VWrite( o.d.c_str(), o.d.Length() );
	return log;
}
