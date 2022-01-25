/*
=============================================================================
	File:	NetProtocol.h
	Desc:	
=============================================================================
*/
#pragma once

// for AssetID
#include <Core/Assets/AssetManagement.h>

struct NetCmd_Base: CStruct
{
	U32	m_type;
public:
	mxDECLARE_CLASS( NetCmd_Base, CStruct );
	mxDECLARE_REFLECTION;
	//NetCmd_Base( U32 _type ) : m_command_type( _type ) {}
};

struct NetCmd_AssetChanged : NetCmd_Base
{
	char	m_assetName[64];
public:
	mxDECLARE_CLASS( NetCmd_AssetChanged, CStruct );
	mxDECLARE_REFLECTION;
	//NetCmd_AssetChanged() : NetCmd_Base(metaClass().GetTypeID()) {}
};


struct EdCmd_AssetDatabaseInfo : NetCmd_Base
{
	String		assetDbName;
public:
	mxDECLARE_CLASS( EdCmd_AssetDatabaseInfo, CStruct );
	mxDECLARE_REFLECTION;
};

struct EdCmd_ReloadAsset : NetCmd_Base
{
	AssetID				assetId;
	String				assetPath;
	TBuffer< BYTE >		assetData;
public:
	mxDECLARE_CLASS( EdCmd_ReloadAsset, NetCmd_Base );
	mxDECLARE_REFLECTION;
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
