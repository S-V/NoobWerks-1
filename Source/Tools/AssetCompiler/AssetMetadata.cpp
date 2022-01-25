#include "stdafx.h"
#pragma hdrstop

#include <Base/Util/PathUtils.h>

#include <AssetCompiler/AssetMetadata.h>


namespace AssetBaking
{

ERet composePathToAssetMetadata(
	String& path_to_asset_metadata,
	const char* absolute_path_to_source_file
	)
{
	mxDO(Str::CopyS( path_to_asset_metadata, absolute_path_to_source_file ));
	Str::StripFileExtension( path_to_asset_metadata );
	mxDO(Str::setFileExtension( path_to_asset_metadata, "meta" ));
	return ALL_OK;
}

}//namespace AssetBaking
