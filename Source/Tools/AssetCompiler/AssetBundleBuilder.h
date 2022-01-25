#pragma once

#include <AssetCompiler/AssetPipeline.h>

namespace AssetBaking
{

///
struct StoredAssetData: CompiledAssetData, ReferenceCounted
{
	const TbMetaClass& asset_class;

	StoredAssetData(
		const CompiledAssetData& asset_data
		, const TbMetaClass& asset_class
		)
		: asset_class( asset_class )
	{
		this->object_data = asset_data.object_data;
		this->stream_data = asset_data.stream_data;
	}
};

///
class AssetBundleBuilder: NonCopyable
{
	typedef THashMap< AssetID, TRefPtr<StoredAssetData> >	AssetsMap;
	AssetsMap	_assets;

public:
	AssetBundleBuilder( AllocatorI &	allocator );
	~AssetBundleBuilder();

	ERet initialize( U32 expected_asset_count );

	ERet addAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		);

	ERet addAssetIfNeeded(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		);

	ERet saveToBlob( NwBlob & blob_ );

	NO_COMPARES(AssetBundleBuilder);
};

#if 0
namespace TbDevAssetBundleUtil
{
	ERet initialize(
		NwAssetBundleT & asset_bundle
		, U32 expected_asset_count
		);

	bool assetMustBeCompiled(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, AssetDatabase & asset_database
		);

	ERet addAsset_and_Save(
		NwAssetBundleT & asset_bundle
		, const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		, AssetDatabase & asset_database
		);

	//ERet addAssetIfNotExists_and_SaveToDisk(
	//	NwAssetBundleT & asset_bundle
	//	, const AssetID& asset_id
	//	, const TbMetaClass& asset_class
	//	, const CompiledAssetData& asset_data
	//	, AssetDatabase & asset_database
	//	);

	ERet saveAndClose(
		NwAssetBundleT & asset_bundle
		);
}//namespace TbDevAssetBundleUtil
#endif

/*
----------------------------------------------------------
	DevAssetBundler
----------------------------------------------------------
*/
class DevAssetBundler: public IAssetBundler
{
	NwAssetBundleT &	_asset_bundle;
	const TbMetaClass *		_accepted_asset_class;

public:
	DevAssetBundler(
		NwAssetBundleT & asset_bundle
		, const TbMetaClass* accepted_asset_class = nil
		);
	~DevAssetBundler();

	//
	ERet addAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		, AssetDatabase & asset_database
		) override;
};


/*
----------------------------------------------------------
	AssetBundler_None
----------------------------------------------------------
*/
class AssetBundler_None: public IAssetBundler
{
	const TbMetaClass *		_accepted_asset_class;

public:
	AssetBundler_None(
		const TbMetaClass* accepted_asset_class = nil
		);
	~AssetBundler_None();

	//
	ERet addAsset(
		const AssetID& asset_id
		, const TbMetaClass& asset_class
		, const CompiledAssetData& asset_data
		, AssetDatabase & asset_database
		) override;
};

}//namespace AssetBaking
