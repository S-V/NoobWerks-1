/*
=============================================================================
	File:	UUID.h
	Desc:	
=============================================================================
*/
#pragma once

/*
----------------------------------------------------------
	mxUUID

	128-bit universally (globally) unique identifier

	See:
	http://en.wikipedia.org/wiki/Universally_unique_identifier
	http://en.wikipedia.org/wiki/Globally_unique_identifier
	http://wiki.secondlife.com/wiki/Asset
----------------------------------------------------------
*/
struct mxUUID: CStruct
{
	union
	{
		struct
		{
			UINT32	i0;
			UINT32	i1;
			UINT32	i2;
			UINT32	i3;
		};
		struct
		{
			UINT64	q0;
			UINT64	q1;
		};
	};

public:
	mxDECLARE_CLASS( mxUUID, CStruct );
	mxDECLARE_REFLECTION;

	mxUUID();
};

bool mxUUID_CreateNew( mxUUID & newUuid );
bool mxUUID_ToString( const mxUUID& uuid, char *buffer, UINT bufferSize );
bool mxUUID_IsNull( const mxUUID& uuid );
bool mxUUID_IsValid( const mxUUID& uuid );
bool mxUUIDs_AreEqual( const mxUUID& uuidA, const mxUUID& uuidB );
mxUUID mxUUID_GetNull();

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
