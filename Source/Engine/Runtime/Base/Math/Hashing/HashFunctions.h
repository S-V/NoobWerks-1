/*
=============================================================================
	File:	HashFunctions.h
	Desc:	Useful hash functions.
=============================================================================
*/
#pragma once

mxSTOLEN("idLib, Doom3/Prey/Quake4 SDKs")
mxINLINE INT SimpleStrHash( const char *_str )
{
	INT hash = 0;
	for ( INT i = 0; *_str != '\0'; i++ ) {
		hash += ( *_str++ ) * ( i + 119 );
	}
	return hash;
}
mxSTOLEN("idLib, Doom3/Prey/Quake4 SDKs")
mxINLINE UINT SimpleUIntHash( UINT key )
{
	return UINT( ( key >> ( 8 * sizeof( UINT ) - 1 ) ) ^ key );
}

mxSTOLEN("Quake 3 source code")
mxINLINE INT FileNameHash( const char *name, UINT len )
{
	char letter;
	long hash = 0;
	UINT i = 0;
	while( i < len ) 
	{
		letter = ::tolower( name[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                      // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	return hash;
}
mxINLINE INT FileNameHash( const char *name ) {
	return FileNameHash( name, ::strlen( name ) );
}

//
//	HashPjw - has been successfully used in symbol tables (in old compilers).
//
mxINLINE INT HashPjw( const char* s )
{
	enum { PRIME = 211 };

#if 0
	// generates warning : assignment within conditional expression
	UINT h = 0, g;
	for ( const char *p = s; *p; p++ ) {
		if ( g = (h = (h << 4) + *p) & 0xF0000000 ) {
			h ^= g >> 24 ^ g;
		}
	}
#else
	UINT h = 0, g;
	for ( const char *p = s; *p; p++ ) {
		h = (h << 4) + *p;
		g = h & 0xF0000000;
		if ( g ) {
			h ^= g >> 24 ^ g;
		}
	}
#endif
	return h % PRIME;
}

//
//	String hashing functions from D compiler (by Walter Bright).
//
UINT CalcStringHash( const char* _str, UINT len );
//	Case insensitive version:
UINT CalcStringHashI( const char* _str, UINT len );

UINT StringHash2( const char* _str, UINT length = 0 );

/*
SuperFastHash from Paul Hsieh site, that is very fast hash to variable string size. 
Mode information here: http://www.azillionmonkeys.com/qed/hash.html 
*/

UINT32 SuperFastHash_1( const char* data, int len );

//
// MurmurHash is a non-cryptographic hash function suitable for general hash-based lookup.
//
UINT32 MurmurHash32( const void* buffer, UINT32 sizeBytes, UINT32 seed = 0 );
UINT64 MurmurHash64( const void* buffer, UINT32 sizeBytes, UINT64 seed = 0 );

// Fowler / Noll / Vo (FNV) Hash
// See: http://isthe.com/chongo/tech/comp/fnv/
//
UINT32 FNV32_StringHash( const char* _str );


// Quick hashing function
inline UINT32 Adler32_Hash( const char *data )
{
	static const UINT32 MOD_ADLER = 65521;
	UINT32 a = 1, b = 0;

	/* Loop over each byte of data, in order */
	for (size_t index = 0; data[index]; ++index)
	{
		a = (a + data[index]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}

	return (b << 16) | a;
}

/*
==================================================================
	Integer hash functions.

	An integer hash function accepts an integer hash key
	and returns an integer hash result with uniform distribution.
==================================================================
*/

// Thomas Wang's hash
// see:http://www.concentric.net/~Ttwang/tech/inthash.htm
mxINLINE INT32 Hash32Bits_0( INT32 key )
{
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key;
}
mxINLINE INT64 Hash64Bits_0( INT64 key )
{
	key += ~(key << 32);
	key ^= (key >> 22);
	key += ~(key << 13);
	key ^= (key >> 8);
	key += (key << 3);
	key ^= (key >> 15);
	key += ~(key << 27);
	key ^= (key >> 31);
	return key;
}

// https://jfdube.wordpress.com/2011/10/12/hashing-strings-and-pointers-avoiding-common-pitfalls/
mxFORCEINLINE U32 HashPointer32( const void* ptr )
{
	U32 Value = (U32)ptr;
	Value = ~Value + (Value << 15);
	Value = Value ^ (Value >> 12);
	Value = Value + (Value << 2);
	Value = Value ^ (Value >> 4);
	Value = Value * 2057;
	Value = Value ^ (Value >> 16);
	return Value;
}


// Bob Jenkin's hash
// http://burtleburtle.net/bob/hash/integer.html
//
mxINLINE INT32 Hash32Bits_1( INT32 key )
{
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);
	return key;
}
mxINLINE INT64 Hash64Bits_1( INT64 key )
{
	INT64 c1 = 0x6e5ea73858134343L;
	INT64 c2 = 0xb34e8f99a2ec9ef5L;
	key ^= ((c1 ^ key) >> 32);
	key *= c1;
	key ^= ((c2 ^ key) >> 31);
	key *= c2;
	key ^= ((c1 ^ key) >> 32);
	return key;
}
mxINLINE UINT32 Hash32Bits_2( UINT32 a )
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

// multiplicative hash function
mxINLINE INT32 Hash32ShiftMult( INT32 key )
{
	INT32 c2 = 0x27d4eb2d; // a prime or an odd constant
	key = (key ^ 61) ^ ((UINT32)key >> 16);
	key = key + (key << 3);
	key = key ^ ((UINT32)key >> 4);
	key = key * c2;
	key = key ^ ((UINT32)key >> 15);
	return key;
}

// 64 bit to 32 bit hash function
mxINLINE INT32 hash6432shift( INT64 key )
{
	key = (~key) + (key << 18); // key = (key << 18) - key - 1;
	key = key ^ ((UINT64)key >> 31);
	key = key * 21; // key = (key + (key << 2)) + (key << 4);
	key = key ^ ((UINT64)key >> 11);
	key = key + (key << 6);
	key = key ^ ((UINT64)key >> 22);
	return key;
}

inline static UINT32 MurmurHash3( UINT32 i )
{
    i ^= i >> 16;
    i *= 0x85EBCA6B;
    i ^= i >> 13;
    i *= 0xC2B2AE35;
    i ^= i >> 16;
    return i;
}

mxSTOLEN("Havok");
//
//
// mxPointerHash - uses multiplication for hashing.
// In Knuth's "The Art of Computer Programming", section 6.4,
// a multiplicative hashing scheme is introduced as a way to write hash function.
// The key is multiplied by the golden ratio of 2^32 (2654435761) to produce a hash result. 
// Since 2654435761 and 2^32 has no common factors in common,
// the multiplication produces a complete mapping of the key to hash result with no overlap.
// This method works pretty well if the keys have small values.
// Bad hash results are produced if the keys vary in the upper bits.
// As is true in all multiplications, variations of upper digits
// do not influence the lower digits of the multiplication result.
//
mxFORCEINLINE ULONG mxPointerHash( const void* ptr )
{
	const ULONG addr = (ULONG) ptr;
	// We ignore the lowest four bits on the address, since most addresses will be 16-byte aligned.
	return (addr >> 4) * 2654435761U;	// Knuth's multiplicative golden hash.
}

struct mxPointerHasher
{
	mxFORCEINLINE static UINT ComputeHash32( const void* ptr )
	{
		#if (mxPOINTER_SIZE == 4)
		{
			return Hash32Bits_0( (INT32)ptr );
		}
		#elif (mxPOINTER_SIZE == 8)
		{
			return Hash64Bits_0( (INT64)ptr );
		}
		#else
		{
			return mxPointerHash( ptr );
		}
		#endif
	}
};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template< typename POD >
UINT64 CalcPodHash( const POD& o, UINT64 seed = 0 )
{
	return MurmurHash64( &o, sizeof(o), seed );
}
