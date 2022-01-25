// Utility functions for loading asset metadata file.
#pragma once

#include <Core/Assets/AssetID.h>
//
#include <AssetCompiler/AssetCompiler.h>
#include <AssetCompiler/AssetDatabase.h>	// CompiledAssetData


namespace AssetBaking
{

	ERet composePathToAssetMetadata(
		String& path_to_asset_metadata,
		const char* absolute_path_to_source_file
		);

	template< typename ASSET_METADATA >
	ERet LoadAssetMetadataFromFile(
		ASSET_METADATA &asset_metadata_
		, const AssetCompilerInputs& inputs
		, AllocatorI & temporary_allocator
		)
	{
		FilePathStringT	path_to_asset_metadata;
		mxDO(composePathToAssetMetadata( path_to_asset_metadata, inputs.path.c_str() ));

		return SON::LoadFromFile(
			path_to_asset_metadata.c_str()
			, asset_metadata_
			, temporary_allocator
			);
	}

}//namespace AssetBaking
