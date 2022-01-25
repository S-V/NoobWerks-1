/*
=============================================================================
	File:	NetProtocol.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>

#include <Core/NetProtocol.h>

mxDEFINE_CLASS( NetCmd_Base );
mxBEGIN_REFLECTION( NetCmd_Base )
	mxMEMBER_FIELD( m_type ),
mxEND_REFLECTION

mxDEFINE_CLASS( NetCmd_AssetChanged );
mxBEGIN_REFLECTION( NetCmd_AssetChanged )
	mxMEMBER_FIELD( m_assetName ),
mxEND_REFLECTION

mxDEFINE_CLASS( EdCmd_AssetDatabaseInfo );
mxBEGIN_REFLECTION( EdCmd_AssetDatabaseInfo )
	mxMEMBER_FIELD( assetDbName ),
mxEND_REFLECTION

mxDEFINE_CLASS( EdCmd_ReloadAsset );
mxBEGIN_REFLECTION( EdCmd_ReloadAsset )
	mxMEMBER_FIELD( assetId ),
	mxMEMBER_FIELD( assetPath ),
	mxMEMBER_FIELD( assetData ),
mxEND_REFLECTION

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
