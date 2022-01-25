/*
=============================================================================
	File:	StringsCommon.h
	Desc:	
=============================================================================
*/
#pragma once


//-----------------------------------------------------------------------
// Static string that caches the length of the string.
//-----------------------------------------------------------------------

// provides array interface around raw string pointers
template< typename CHAR_TYPE, class STRING_TYPE >	// where STRING_TYPE : TStringBase< CHAR_TYPE, STRING_TYPE >
struct TStringBase: public TArrayMixin< CHAR_TYPE, STRING_TYPE >
{
	// returns a *read-only* null-terminated string
	inline const CHAR_TYPE* c_str() const
	{ return this->SafeGetPtr(); }

	// returns "" if the string is empty
	inline const CHAR_TYPE* SafeGetPtr() const {
		return this->IsEmpty() ? mxEMPTY_STRING : raw();
	}

	// case sensitive comparison
	template< UINT N >
	bool operator == ( const CHAR_TYPE (&_str)[N] ) const {
		return strncmp(c_str(), _str, Min(num(), N-1)) == 0;
	}
	template< UINT N >
	bool operator != ( const CHAR_TYPE (&_str)[N] ) const {
		return !(*this == _str);
	}

	bool operator == ( const CHAR_TYPE c ) const {
		return num() == 1 && c_str()[0] == c;
	}
	bool operator != ( const CHAR_TYPE c ) const {
		return !(*this == c);
	}
};

struct Chars: public TStringBase< char, Chars >
{
	const char *	buffer;	// pointer to the null-terminated string
	const U32		length;	// length (NOT including the terminator)

public:
	template< UINT N >
	inline Chars( const char (&_str)[N] )
		: buffer( _str )
		, length( N-1 )
	{}
	inline Chars( const char* text_, UINT length_ )
		: buffer( text_ ), length( length_ )
	{}
	inline explicit Chars( const char* _str, EInitSlow )
		: buffer( _str ), length( strlen(_str) )
	{}
	inline Chars()
		: buffer( nil ), length( 0 )
	{}
	inline Chars( const Chars& other )
		: buffer( other.buffer ), length( other.length )
	{}
	//=== TArrayBase
	inline UINT num() const			{ return length; }
	inline char * raw()				{ return const_cast< char* >( buffer ); }
	inline const char* raw() const	{ return buffer; }
};

template<>
struct THashTrait< Chars >
{
	static U32 ComputeHash32( const Chars& key );
};
template<>
struct TEqualsTrait< Chars >
{
	static bool Equals( const Chars& a, const Chars& b );
};
