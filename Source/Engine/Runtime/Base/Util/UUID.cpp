/*
=============================================================================
	File:	UUID.cpp
	Desc:
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

// for CoCreateGuid()
#include <ObjBase.h>

#include <Base/Util/UUID.h>

/*
-------------------------------------------------------------------------
	mxUUID
-------------------------------------------------------------------------
*/
mxDEFINE_CLASS( mxUUID );

mxBEGIN_REFLECTION( mxUUID )
	mxMEMBER_FIELD( i0 ),
	mxMEMBER_FIELD( i1 ),
	mxMEMBER_FIELD( i2 ),
	mxMEMBER_FIELD( i3 ),
mxEND_REFLECTION

mxUUID::mxUUID()
{
}

bool mxUUID_CreateNew( mxUUID & newUuid )
{
	GUID	guid;
	mxSTATIC_ASSERT( sizeof(newUuid) == sizeof(guid) );
	chkRET_FALSE_IF_NOT( ::CoCreateGuid( &guid ) == S_OK );
	memcpy( &newUuid, &guid, sizeof(guid) );
	return true;
}

bool mxUUID_ToString( const mxUUID& uuid, char *buffer, UINT bufferSize )
{
	enum { STRING_LENGTH = sizeof(uuid) + 1 };
	chkRET_FALSE_IF_NOT( bufferSize >= STRING_LENGTH );
	memcpy( buffer, &uuid, sizeof(uuid) );
	buffer[ STRING_LENGTH ] = '\0';
	return true;
}

bool mxUUID_IsNull( const mxUUID& uuid )
{
	return uuid.q0 == 0
		&& uuid.q1 == 0
		;
}

bool mxUUID_IsValid( const mxUUID& uuid )
{
	return !mxUUID_IsNull( uuid );
}

bool mxUUIDs_AreEqual( const mxUUID& uuidA, const mxUUID& uuidB )
{
	return uuidA.q0 == uuidB.q0
		&& uuidA.q1 == uuidB.q1
		;
}

mxUUID mxUUID_GetNull()
{
	mxUUID	result;
	result.q0 = 0;
	result.q1 = 0;
	return result;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
