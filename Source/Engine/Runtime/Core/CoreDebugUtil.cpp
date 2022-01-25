//
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/CoreDebugUtil.h>

#include <Core/Assets/AssetID.h>

namespace Dbg
{

	bool isDebugAsset( const AssetID& assetId )
	{
		return !strcmp( AssetId_ToChars( assetId ),
			"models/weapons/rocketlauncher/rocketlauncher_mflashb"
			//"models/weapons/rocketlauncher/rocketlauncher_mflash"
			);
	}

	bool isDebugModel( const AssetID& assetId )
	{
		return strstr(AssetId_ToChars(assetId),
			"rocketlauncher"
			);
	}

	V3f g_mdl_offset = CV3f(0,0,0);

}//namespace Dbg
