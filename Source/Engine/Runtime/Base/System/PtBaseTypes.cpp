/*
=============================================================================
	File:	Common.cpp
	Desc:
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Math/Hashing/HashFunctions.h>

const char* mxSTRING_Unknown = "Unknown";
const char* mxSTRING_UNKNOWN_ERROR = "unknown error";
const char* mxSTRING_DOLLAR_SIGN = "$";
const char* mxSTRING_QUESTION_MARK = "?";
const char* mxEMPTY_STRING = "";

mxINLINE INT SimpleNameHash( const char *name )
{
	INT hash, i;

	hash = 0;
	for ( i = 0; name[i]; i++ ) {
		hash += name[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

UINT mxGetHashCode( const String& _str )
{
	return SimpleNameHash( _str.raw() );
}

U32 THashTrait< Chars >::ComputeHash32( const Chars& key )
{
	return MurmurHash32( key.buffer, key.length );
}
bool TEqualsTrait< Chars >::Equals( const Chars& a, const Chars& b )
{
	return 0 == strcmp( a.buffer, b.buffer );
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
