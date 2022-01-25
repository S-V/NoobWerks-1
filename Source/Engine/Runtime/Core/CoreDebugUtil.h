/*
=============================================================================
	File:	Client.h
	Desc:	
=============================================================================
*/
#pragma once

struct AssetID;

namespace Dbg
{

	bool isDebugAsset( const AssetID& assetId );
	bool isDebugModel( const AssetID& assetId );

	extern V3f g_mdl_offset;

}//namespace Dbg
