/*
=============================================================================
	File:	StaticString.h
	Desc:	Compile-time strings.
	Note:	Don't use this with empty strings ("") !
			empty strings are usually not needed, anyway.
=============================================================================
*/

#ifndef __MX_COMMON_STATIC_STRING_H__
#define __MX_COMMON_STATIC_STRING_H__


// enable this to check for hash collisions (i.e. different strings with equal hash keys)
#define MX_CHECK_STATIC_STR_HASH_COLLISIONS	(MX_DEBUG)




mxSTOLEN("The code is based on work done by Sergey Makeev, http://mail@sergeymakeev.com");

namespace StaticString_Util
{
	const UINT MX_STATIC_STR_HASH_MAGIC_NUMBER = 1315423911;

	mxFORCEINLINE void HashCombine( UINT & hash, char c )
	{
		hash ^= ((hash << 5) + c + (hash >> 2));
	}

	mxFORCEINLINE UINT mxVPCALL StringHashRunTime( const char* _str, ULONG & length )
	{
		length = 0;

		mxASSERT_PTR(_str);

		UINT hash = MX_STATIC_STR_HASH_MAGIC_NUMBER;
		const char * char_ptr = _str;

		while( *char_ptr )
		{
			char c = *char_ptr++;
			HashCombine( hash, c );
		}

		length = (ULONG)(char_ptr - _str);

		return hash;
	}

	template< INT LENGTH >
	class StringHashCompileTime
	{
	public:
		mxFORCEINLINE static void mxVPCALL Build( UINT & hash, INT strLen, const char* input )
		{
			char c = input[ strLen - LENGTH ];
			HashCombine( hash, c );
			StringHashCompileTime< LENGTH-1 >::Build( hash, strLen, input );
		}
	};
	template<>
	class StringHashCompileTime< 0 >
	{
	public:
		mxFORCEINLINE static void mxVPCALL Build( UINT & hash, INT strLen, const char* input )
		{
			char c = input[ strLen ];
			HashCombine( hash, c );
		}
	};

}//StaticString_Util

/*
-------------------------------------------------------------------------
	StaticString
-------------------------------------------------------------------------
*/
class StaticString
{
protected_internal:
	char const* const	mPtr;
	const ULONG			mSize;
	UINT				mHash;

public_internal:

	mxFORCEINLINE StaticString( const StaticString& other )
		: mPtr( other.mPtr )
		, mSize( other.mSize )
		, mHash( other.mHash )
	{}

	// 'run-time' constructor
	mxFORCEINLINE explicit StaticString( const char* _str, UINT length, UINT hash )
		: mPtr( _str )
		, mSize( length )
		, mHash( hash )
	{}

public:

	// 'compile-time' constructor
	template< INT N >
	mxFORCEINLINE StaticString( const char (&_str)[N] )
		: mPtr( _str )
		, mSize( N-1 )
	{
		mxSTATIC_ASSERT( N > 0 );	// disallow empty string ("")

		using namespace StaticString_Util;
		mHash = MX_STATIC_STR_HASH_MAGIC_NUMBER;

		// a small fix for strings which are less than 2 characters in length (e.g. "!")
		if( N < 2 ) {
			StringHashCompileTime< N-1 >::Build( mHash, N-1, _str );
		} else {
			StringHashCompileTime< N-2 >::Build( mHash, N-2, _str );
		}
	}

	mxFORCEINLINE ULONG GetSize() const
	{
		return mSize;
	}

	mxFORCEINLINE const char* c_str() const
	{
		return mPtr;
	}
	mxFORCEINLINE const char* Raw() const
	{
		return mPtr;
	}

	mxFORCEINLINE UINT GetHash() const
	{
		return mHash;
	}

	mxFORCEINLINE bool Equals( const StaticString& other ) const
	{
	#if MX_CHECK_STATIC_STR_HASH_COLLISIONS

		if( (GetSize() != other.GetSize()) || (GetHash() != other.GetHash()) )
		{
			return false;
		}

		// If a hash collision did occur then try using another hash function or gperf.
		// See:
		// http://blog.gamedeff.com/?p=115
		// http://gnuwin32.sourceforge.net/packages/gperf.htm

		mxASSERT2( mxStrEquAnsi( c_str(), other.c_str() ), "Need to update hash function, collisions must be resolved!" );
		return true;

	#else // !MX_CHECK_STATIC_STR_HASH_COLLISIONS

		// No need to check for hash collisions in release mode because static (hardcoded, compile-time) strings cannot change.
		return (GetSize() == other.GetSize()) && (GetHash() == other.GetHash());

	#endif // MX_CHECK_STATIC_STR_HASH_COLLISIONSMX_CHECK_STATIC_STR_HASH_COLLISIONS
	}
};

mxFORCEINLINE bool operator == ( const StaticString& first, const StaticString& second )
{
	return first.Equals( second );
}

template<>
struct THashTrait< StaticString >
{
	mxFORCEINLINE static UINT ComputeHash32( const StaticString& s )
	{
		return s.GetHash();
	}
};

template<>
struct TEqualsTrait< StaticString >
{
	mxFORCEINLINE static bool Equals( const StaticString& a, const StaticString& b )
	{
		return (a == b);
	}
};




/*

Standard tests:

using namespace StaticString_Util;

ULONG len;
UINT h0 = StringHashRunTime( "DS_Normal", len );

StaticString	s( "DS_Normal" );
mxASSERT(h0==s.GetHash());

UINT h2 = StringHashRunTime( "mxFORCEINLINE operator const", len );
StaticString	s2( "mxFORCEINLINE operator const" );
mxASSERT(h2==s2.GetHash());

UINT h3 = StringHashRunTime( "A", len );
StaticString	s3( "A" );
mxASSERT(h3==s3.GetHash());


//UINT h4 = StringHashRunTime( "", len );
//UINT lll = strlen("");	// is zero
//StaticString	s4( "" );// compiler goes into infinite loop?
//mxASSERT(h4==s4.GetHash());


*/

#endif // !__MX_COMMON_STATIC_STRING_H__

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
