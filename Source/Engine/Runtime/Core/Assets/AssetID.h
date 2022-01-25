/*
=============================================================================
	File:	AssetID.h
	Desc:	Asset/Resource management.
	ToDo:	use numerical asset ids (32-bit ints or 128-bit GUIDs),
			strings should only used in editor/development mode!
=============================================================================
*/
#pragma once

#include <type_traits>	// std::is_base_of

#include <Core/Core.h>
#include <Core/Memory.h>
#include <Core/Text/NameTable.h>


/*
--------------------------------------------------------------
	AssetID - (static)(globally) unique resource identifier,
	usually assigned by resource compiler
	and known before running the program
	and used for locating the asset data.

	NOTE: asset id is just the file name (ASCII) without path, always in lowercase.
--------------------------------------------------------------
*/
struct AssetID: CStruct
{
	NameID	d;	//!< e.g. "material_default"
	enum { MAX_LENGTH = 256 };

public:
	mxFORCEINLINE bool operator == ( const AssetID& other ) const throw()
	{
		return d == other.d;
	}
	mxFORCEINLINE bool operator != ( const AssetID& other ) const throw()
	{
		return d != other.d;
	}

	// binary search
	mxFORCEINLINE bool operator <= ( const AssetID& other ) const throw()
	{
		return strcmp( this->d.c_str(), other.d.c_str() ) <= 0;
	}

	mxFORCEINLINE const char *c_str() const { return d.c_str(); }

	mxFORCEINLINE bool IsValid() const { return !d.IsEmpty(); }
};

bool AssetId_IsNull( const AssetID& assetId );
bool AssetId_IsValid( const AssetID& assetId );
bool AssetIds_AreEqual( const AssetID& assetIdA, const AssetID& assetIdB );
AssetID AssetId_GetNull();
U32 AssetId_GetHash32( const AssetID& assetId );
const char* AssetId_ToChars( const AssetID& assetId );
const AssetID MakeAssetID( const char* _name );

ERet check_AssetID_string( const char* name );

// used at run-time
ERet make_AssetID_from_string( AssetID &asset_id_
							  , const char* asset_name
							  );

#if 1||MX_DEVELOPER
// used by Tools
ERet make_AssetID_from_filepath( AssetID &asset_id_
								, const char* filepath
								);

// is used to create asset IDs from strings containing path components, e.g. "models/md5/monsters/zcc/chaingun_wallstepleft_A.md5anim"
AssetID make_AssetID_from_raw_string( const char* filepath );

mxDEPRECATED
const AssetID AssetID_from_FilePath( const char* _file );
#endif // MX_DEVELOPER

ERet readAssetID( AssetID &asset_id_, AReader& stream );
ERet writeAssetID( const AssetID& assetId, AWriter &stream );



template<>
struct THashTrait< AssetID > {
	mxFORCEINLINE static UINT ComputeHash32( const AssetID& k ) {
		return AssetId_GetHash32( k );
	}
};
template<>
struct TEqualsTrait< AssetID > {
	mxFORCEINLINE static bool Equals( const AssetID& a, const AssetID& b ) {
		return AssetIds_AreEqual( a, b );
	}
};
template<>
inline const TbMetaType& T_DeduceTypeInfo< AssetID >() {
	static mxBuiltInType< AssetID >	assetIdType( ETypeKind::Type_AssetId, mxEXTRACT_TYPE_NAME(AssetID) );
	return assetIdType;
}
template<>
inline ETypeKind T_DeduceTypeKind< AssetID >() {
	return ETypeKind::Type_AssetId;
}

// this is actually a GUID (32-bit hash) of the corresponding TbMetaClass's name
typedef TypeGUID AssetTypeT;

const char* AssetType_To_Chars( AssetTypeT classGUID );

/*
--------------------------------------------------------------
	AssetKey
	used for uniquely identifying each asset instance
	(and for sharing asset instances by mapping asset ids to pointers).
	Should be lightweight as it's stored as a key in the resource dictionary.
--------------------------------------------------------------
*/
struct AssetKey: CStruct
{
	AssetID		id;		//!< unique id of asset (can be null if pointing to fallback asset instance)
	AssetTypeT	type;	//!< type of asset, must always be valid
public:
	mxDECLARE_CLASS( AssetKey, CStruct );
	mxDECLARE_REFLECTION;

	AssetKey(
		const AssetID& asset_id = AssetId_GetNull(),
		const AssetTypeT asset_type = mxNULL_TYPE_ID
		)
		: id( asset_id )
		, type( asset_type )
	{}

	bool IsOk() const;

	template< class ASSET >
	static AssetKey make( const AssetID& asset_id )
	{
		AssetKey	key;
		key.id = asset_id;
		key.type = ASSET::metaClass().GetTypeGUID();
		return key;
	}
};

template<>
struct THashTrait< AssetKey > {
	static UINT ComputeHash32( const AssetKey& k ) {
		return AssetId_GetHash32( k.id ) ^ long( k.type );
	}
};
template<>
struct TEqualsTrait< AssetKey > {
	static bool Equals( const AssetKey& a, const AssetKey& b ) {
		return AssetIds_AreEqual( a.id, b.id ) && a.type == b.type;
	}
};

AWriter& operator << ( AWriter& stream, const AssetID& o );
AReader& operator >> ( AReader& stream, AssetID &o_ );

ATextStream & operator << ( ATextStream & log, const AssetID& o );
