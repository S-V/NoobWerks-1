/*
=============================================================================
	File:	Integer.h
	Desc:	Integer type.
	Note:	most of these things work only with integers in two's complement representation.
	ToDo:	use intrinsics
=============================================================================
*/
#pragma once

// using bit hacks obscures programmer's intent and good compilers can usually optimize the code better than humans
#define MX_USE_BIT_TRICKS	(0)

#define INT32_SIGN_BIT_SET(i)		(((const U64)(i)) >> 31)
#define INT32_SIGN_BIT_NOT_SET(i)	((~((const U64)(i))) >> 31)
#define INT64_SIGN_BIT_SET(i)		(((const U64)(i)) >> 64)
#define INT64_SIGN_BIT_NOT_SET(i)	((~((const U64)(i))) >> 64)





//
// https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT
#if defined(__SSE4_2__)
	#define vxPOPCNT32(X)	__popcnt((X))
#else
	#define vxPOPCNT32(X)	BitCount((X))
#endif


static mxFORCEINLINE
U32 CountTrailingZeros32(U32 mask)
{
	mxASSERT(mask != 0);
	DWORD	num_trailing_zeros;
	_BitScanForward(&num_trailing_zeros, mask);
	return num_trailing_zeros;
}

/// removes the first set bit from the given bitmask starting from the least significant bit (LSB)
static mxFORCEINLINE
U32 TakeNextTrailingBit32( U32 & mask )
{
	U32 trailing_bit_index = CountTrailingZeros32(mask);
	mask &= ~(1u << trailing_bit_index);	// remove the set MSB bit from the mask
	return trailing_bit_index;
}




//--------------------------------------------------------------------------------

mxBIBREF("Game Engine Gems 2 [2014], 'Bit Hacks for Games', Eric Lengyel, PP.403-414");

// alternative min function for signed 32-bit integers
template<>
mxFORCEINLINE INT32 Min< INT32 >( INT32 x, INT32 y )
{
#if MX_USE_BIT_TRICKS
	INT32 a = x - y;
	return x - (a & ~(a >> 31));
#else
	return (x < y) ? x : y;
#endif
}

// alternative max function for signed 32-bit integers
template<>
mxFORCEINLINE INT32 Max< INT32 >( INT32 x, INT32 y )
{
#if MX_USE_BIT_TRICKS
	INT32 a = x - y;
	return x - (a & (a >> 31));
#else
	return (x > y) ? x : y;
#endif
}

// alternative min function for signed 64-bit integers
template<>
mxFORCEINLINE INT64 Min< INT64 >( INT64 x, INT64 y )
{
#if MX_USE_BIT_TRICKS
	INT64 a = x - y;
	return x - (a & ~(a >> 63));
#else
	return (x < y) ? x : y;
#endif
}

// alternative max function for signed 64-bit integers
template<>
mxFORCEINLINE INT64 Max< INT64 >( INT64 x, INT64 y )
{
#if MX_USE_BIT_TRICKS
	INT64 a = x - y;
	return x - (a & (a >> 63));
#else
	return (x > y) ? x : y;
#endif
}

/*

Average of Integers

This is actually an extension of the "well known" fact that for binary integer values x and y,
(x+y) equals ((x&y)+(x|y)) equals ((x^y)+2*(x&y)). 
Given two integer values x and y, the (floor of the) average normally would be computed by (x+y)/2;
unfortunately, this can yield incorrect results due to overflow.
A very sneaky alternative is to use (x&y)+((x^y)/2).
If we are aware of the potential non-portability due to the fact that C does not specify if shifts are signed,
this can be simplified to (x&y)+((x^y)>>1). In either case, the benefit is that this code sequence cannot overflow.
*/
template<>
mxFORCEINLINE INT32 Mean< INT32 >( INT32 x, INT32 y )
{
#if MX_USE_BIT_TRICKS
	return (x & y) + ((x ^ y)/2);
#else
	return (x + y) / 2;
#endif
}
template<>
mxFORCEINLINE INT64 Mean< INT64 >( INT64 x, INT64 y )
{
#if MX_USE_BIT_TRICKS
	return (x & y) + ((x ^ y)/2);
#else
	return (x + y) / 2;
#endif
}


// returns the absolute value of a signed 32-bit integer value (for reference only)
template<>
mxFORCEINLINE INT32 Abs< INT32 >( INT32 x )
{
#if MX_USE_BIT_TRICKS
	#if 1
	   INT32 y = x >> 31;
	   return ( ( x ^ y ) - y );
	#else
		INT32 y = x >> (sizeof(INT) * CHAR_BIT - 1);
		return ((x + y) ^ y);
	#endif
#else
	return (x > 0) ? x : -x;
#endif
}

// returns the absolute value of a signed 64-bit integer value (for reference only)
template<>
mxFORCEINLINE INT64 Abs< INT64 >( INT64 x )
{
#if MX_USE_BIT_TRICKS
   INT64 y = x >> 63;
   return ( ( x ^ y ) - y );
#else
	return (x > 0) ? x : -x;
#endif
}

// returns the sign of the given 32-bit integer
mxFORCEINLINE INT32 Sgn( INT32 x )
{
#if MX_USE_BIT_TRICKS
	return (INT32) ((((U32) -x) >> 31) | (x >> 31));
#else
	return (x > 0) ? 1 : -1;
#endif
}

// returns the sign of the given 64-bit integer
mxFORCEINLINE INT64 Sgn64( INT64 x )
{
#if MX_USE_BIT_TRICKS
	return (INT64) ((((U64) -x) >> 63) | (x >> 63));
#else
	return (x > 0) ? 1 : -1;
#endif
}

mxFORCEINLINE INT32 MinZero( INT32 x )
{
#if MX_USE_BIT_TRICKS
	return (x & (x >> 31));
#else
	return smallest( x, 0 );
#endif
}

mxFORCEINLINE INT64 MinZero64( INT64 x )
{
#if MX_USE_BIT_TRICKS
	return (x & (x >> 63));
#else
	return smallest( x, 0 );
#endif
}

mxFORCEINLINE INT32 MaxZero( INT32 x )
{
#if MX_USE_BIT_TRICKS
	return (x & ~(x >> 31));
#else
	return largest( x, 0 );
#endif
}

mxFORCEINLINE INT64 MaxZero64( INT64 x )
{
#if MX_USE_BIT_TRICKS
	return (x & ~(x >> 63));
#else
	return largest( x, 0 );
#endif
}

#if 0

	// if x > 0 then return 1
	// if x <= 0 then return 0
	mxFORCEINLINE INT32 MapTo01( INT32 x )
	{
	#if MX_USE_BIT_TRICKS
		return (x & ((U32)-x >> 31));
	#else
		return (x > 0) ? 1 : 0;
	#endif
	}
	mxFORCEINLINE U32 MapTo01( U32 x )
	{
		return (x > 0) ? 1 : 0;
	}
	mxFORCEINLINE INT64 MapTo01( INT64 x )
	{
	#if MX_USE_BIT_TRICKS
		return (x & ((U64)-x >> 63));
	#else
		return (x > 0) ? 1 : 0;
	#endif
	}

#else

	#ifdef mxCOMPILER_MSVC
		#pragma warning ( disable: 4806 )	// unsafe operation: no value of type 'bool' promoted to type X can equal the given constant
	#endif

	/// Converts any non-zero number into 1, otherwise, into 0.
	/// Quite efficient, compiles into few machine instructions.
	#define MapTo01(x)		(!!(x))

#endif


/// returns 1 if the expression is positive or -1 if it is negative
#define MapToNegPos1(x)		(((x) > 0) ? 1 : -1)



mxSTOLEN("Darkplaces engine (Nexuiz)")
/// LordHavoc: this function never returns exactly MIN or exactly MAX, because
/// of a QuakeC bug in id1 where the line
/// self.nextthink = self.nexthink + random() * 0.5;
/// can result in 0 (self.nextthink is 0 at this point in the code to begin
/// with), causing "stone monsters" that never spawned properly, also MAX is
/// avoided because some people use random() as an index into arrays or for
/// loop conditions, where hitting exactly MAX may be a fatal error
#define lhrandom(MIN,MAX) (((double)(rand() + 0.5) / ((double)RAND_MAX + 1)) * ((MAX)-(MIN)) + (MIN))


/// returns log base 2 of "n"
/// \WARNING: "n" MUST be a power of 2!
#define log2i(n) ((((n) & 0xAAAAAAAA) != 0 ? 1 : 0) | (((n) & 0xCCCCCCCC) != 0 ? 2 : 0) | (((n) & 0xF0F0F0F0) != 0 ? 4 : 0) | (((n) & 0xFF00FF00) != 0 ? 8 : 0) | (((n) & 0xFFFF0000) != 0 ? 16 : 0))

/// boolean XOR (why doesn't C have the ^^ operator for this purpose?)
#define boolxor(a,b) (!(a) != !(b))




template< typename TInteger >
mxFORCEINLINE bool IsEven( TInteger x )
{
	return !(x & 1);
}


//these arise frequently with operation on triples (e.g. triangle indices, axis ids)

mxFORCEINLINE UINT IncMod3( UINT x )
{
	return (((x << 1) & 2) | (((x - 1) >> 1) & 1));
}
mxFORCEINLINE UINT DecMod3( UINT x )
{
	return (((x - 1) & 2) | (x >> 1));
}

// These tricks should not be used anymore, and they are preserved here only for their curious technology.

// classical XOR swap, shouldn't be used anymore
mxFORCEINLINE void XORSwap( UINT & x, UINT & y )
{
	x ^= y;
	y ^= x;
	x ^= y;
}

// returns 'true' if one of the bytes in a 4-byte integer is zero.
mxFORCEINLINE BOOL HasNullByte( U32 x )
{
	return ( (x + 0xFEFEFEFF) & (~x) & 0x80808080 );
	// alternative:
	//return (((x) - 0x01010101) & ( ~(x) & 0x80808080 ) )
}

// finds the smallest 1 bit in x, e.g.: ~~~~~~10---0    =>    0----010---0.
mxFORCEINLINE UINT LowestOneBit( UINT x )
{
	return ( (x) & (~(x)+1) );
}

// zeros the least significant '1' bit in x
mxFORCEINLINE void ZeroLeastSetBit( UINT & x )
{
	x &= (x - 1);
}

// sets the least significant N bits in x
mxFORCEINLINE void SetLeastNBits( UINT & x, UINT N )
{
	x |= ~(~0 << N);
}

// conditional set based on mask and arithmetic shift
mxFORCEINLINE U32 if_c_a_else_b( INT32 condition, U32 a, U32 b )
{
	return ( ( -condition >> 31 ) & ( a ^ b ) ) ^ b;
}

// conditional set based on mask and arithmetic shift
mxFORCEINLINE U16 if_c_a_else_b( INT16 condition, U16 a, U16 b )
{
	return ( ( -condition >> 15 ) & ( a ^ b ) ) ^ b;
}

// conditional set based on mask and arithmetic shift
mxFORCEINLINE U32 if_c_a_else_0( INT32 condition, U32 a )
{
	return ( -condition >> 31 ) & a;
}

// if (condition) state |= m; else state &= ~m
mxFORCEINLINE void setbit_cond( U32 &state, INT32 condition, U32 mask )
{
	// 0, or any positive to mask
	//INT32 conmask = -condition >> 31;
	state ^= ( ( -condition >> 31 ) ^ state ) & mask;
}

mxINLINE U32 Log2OfPowerOfTwo( U32 powerOfTwo )
{
    U32 log2 = (powerOfTwo & 0xAAAAAAAA) != 0;	// & 10101010101010101010101010101010
    log2 |= ((powerOfTwo & 0xCCCCCCCC) != 0) << 1;	// & 11001100110011001100110011001100
	log2 |= ((powerOfTwo & 0xF0F0F0F0) != 0) << 2;	// & 11110000111100001111000011110000
	log2 |= ((powerOfTwo & 0xFF00FF00) != 0) << 3;	// & 11111111000000001111111100000000
	log2 |= ((powerOfTwo & 0xFFFF0000) != 0) << 4;	// & 11111111111111110000000000000000
    return log2;
}

mxINLINE INT32 Log2OfPowerOfTwo( INT32 powerOfTwo )
{
    U32 log2 = (powerOfTwo & 0xAAAAAAAA) != 0;
    log2 |= ((powerOfTwo & 0xFFFF0000) != 0) << 4;
    log2 |= ((powerOfTwo & 0xFF00FF00) != 0) << 3;
    log2 |= ((powerOfTwo & 0xF0F0F0F0) != 0) << 2;
    log2 |= ((powerOfTwo & 0xCCCCCCCC) != 0) << 1;
    return (INT32)log2;
}

// "Next Largest Power of 2"
// Given a binary integer value x, the next largest power of 2 can be computed by a SWAR algorithm
// that recursively "folds" the upper bits into the lower bits. This process yields a bit vector with
// the same most significant 1 as x, but all 1's below it. Adding 1 to that value yields the next
// largest power of 2. For a 32-bit value:"
//
// e.g. 0 -> 1
// e.g. 1 -> 2
// e.g. 2 -> 4
// e.g. 13 -> 16
// e.g. 16 -> 32
// NOTE: this doesn't work for x < 2.
//
mxFORCEINLINE U32 NextPowerOfTwo( U32 x )
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x + 1);
}

/*
__inline int NextPow2(int Number)
{ 
	unsigned int RetVal = 1;
	__asm
	{
		xor ecx, ecx
		bsr ecx, Number
		inc ecx
		shl RetVal, cl
	} 
	return(RetVal);
}
uint32 NextPow2( uint32 value )
{
	uint32  count = 0;
	::_BitScanReverse( &count, value );
	return( 1 << count );
}
*/

/// round x up to the nearest power of 2
///
///	e.g. 0 -> 0
///	e.g. 1 -> 1
///	e.g. 2 -> 2
///	e.g. 16 -> 16
///	e.g. 17 -> 32
/// NOTE: this doesn't work for x < 2.
///
mxFORCEINLINE U32 CeilPowerOfTwo( U32 x )
{
	x--;
	// now set all bits below its most significant bit to 1
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x++;
	return x;
}

// round x down to the nearest power of 2
mxFORCEINLINE INT32 FloorPowerOfTwo( INT32 x ) {
	return CeilPowerOfTwo( x ) >> 1;
}


template< typename TInteger >
mxFORCEINLINE bool IsPowerOfTwo( TInteger x )
{
	return (x & (x-1)) == 0;
}

/*
// returns true if the integer x is a power of 2
template< typename TYPE >
mxFORCEINLINE bool IsPowerOfTwo( const TYPE& x ) {
	return ( x & ( x - 1 ) ) == 0 && x > 0;
}

// returns true if a signed integer x is a power of 2
template<>
mxFORCEINLINE bool IsPowerOfTwo<INT>( INT x ) {
	return ( x & ( x - 1 ) ) == 0 && x > 0;
}

// returns true if an unsigned integer x is a power of 2
template<>
mxFORCEINLINE bool IsPowerOfTwo<UINT>( UINT x ) {
	return ((x) & ((x) - 1)) == 0;
}
*/
// returns true if a signed integer x is a power of 2
// NOTE: this doesn't work for x < 2.
// e.g. 8 -> 1
// e.g. 0 -> 1
// e.g. 1 -> 1
template<>
mxFORCEINLINE bool IsPowerOfTwo<INT>( INT x ) {
	return ( x & ( x - 1 ) ) == 0 && x > 0;
}

// returns true if an unsigned integer x is a power of 2.
// NOTE: this doesn't work for x < 2.
// e.g. 8 -> 1
// e.g. 0 -> 1
// e.g. 1 -> 1
template<>
mxFORCEINLINE bool IsPowerOfTwo<UINT>( UINT x ) {
	return ((x) & ((x) - 1)) == 0;
}

// returns the number of 1 bits in x
//
// Calculates the number of '1' bits in a 32-bit integer value (Hamming weight).
//
mxFORCEINLINE INT32 BitCount( INT32 x )
{
#if 0
	// This relies of the fact that the count of x bits can NOT overflow 
	// an x bit integer. EG: 1 bit count takes a 1 bit integer, 2 bit counts
	// 2 bit integer, 3 bit count requires only a 2 bit integer.
	// So we add all bit pairs, then each nible, then each byte etc...
	x = (x & 0x55555555) + ((x & 0xAAAAAAAA) >> 1);
	x = (x & 0x33333333) + ((x & 0xCCCCCCCC) >> 2);
	x = (x & 0x0F0F0F0F) + ((x & 0xF0F0F0F0) >> 4);
	x = (x & 0x00FF00FF) + ((x & 0xFF00FF00) >> 8);
	x = (x & 0x0000FFFF) + ((x & 0xFFFF0000) >> 16);
	// Etc for larger integers (64 bits in Java).
	// NOTE: the >> operation must be unsigned! (>>> in Java).
	return x;
#elif 0
	/* 32-bit recursive reduction using SWAR...
	but first step is mapping 2-bit values
	into sum of 2 1-bit values in sneaky way
	*/
	x -= ( ( x >> 1 ) & 0x55555555 );
	x = ( ( ( x >> 2 ) & 0x33333333 ) + ( x & 0x33333333 ) );
	x = ( ( ( x >> 4 ) + x ) & 0x0F0F0F0F );
	x += ( x >> 8 );
	return ( ( x + ( x >> 16 ) ) & 0x0000003F );
#else
	// http://stackoverflow.com/a/15979139/4232223
	x = x - ((x >> 1) & 0x55555555);	// put count of each 2 bits into those 2 bits
	x = ((x >> 2) & 0x33333333) + (x & 0x33333333);// put count of each 4 bits into those 4 bits
	x = ((x >> 4) + x) & 0x0F0F0F0F;
	return (x * 0x01010101) >> 24;
#endif
}
//http://stackoverflow.com/a/3026418/4232223

// returns the bit reverse of x
//
// Reverses all the bits in a 32-bit integer value.
// (Each line can be done in any order.)
//
mxFORCEINLINE INT32 BitReverse( INT32 x )
{
#if 1
	x = ( ( ( x >> 1 ) & 0x55555555 ) | ( ( x & 0x55555555 ) << 1 ) );
	x = ( ( ( x >> 2 ) & 0x33333333 ) | ( ( x & 0x33333333 ) << 2 ) );
	x = ( ( ( x >> 4 ) & 0x0F0F0F0F ) | ( ( x & 0x0F0F0F0F ) << 4 ) );
	x = ( ( ( x >> 8 ) & 0x00FF00FF ) | ( ( x & 0x00FF00FF ) << 8 ) );
	return ( ( x >> 16 ) | ( x << 16 ) );
#elif 0
	U32 y = 0x55555555;
	x = (((x >> 1) & y) | ((x & y) << 1));
	y = 0x33333333;
	x = (((x >> 2) & y) | ((x & y) << 2));
	y = 0x0f0f0f0f;
	x = (((x >> 4) & y) | ((x & y) << 4));
	y = 0x00ff00ff;
	x = (((x >> 8) & y) | ((x & y) << 8));
	return((x >> 16) | (x << 16));
#else
	// UINT x
	x = ((x >>  1) & 0x55555555) | ((x <<  1) & 0xAAAAAAAA);
	x = ((x >>  2) & 0x33333333) | ((x <<  2) & 0xCCCCCCCC);
	x = ((x >>  4) & 0x0F0F0F0F) | ((x <<  4) & 0xF0F0F0F0);
	x = ((x >>  8) & 0x00FF00FF) | ((x <<  8) & 0xFF00FF00);
	x = ((x >> 16) & 0x0000FFFF) | ((x << 16) & 0xFFFF0000);
	// Etc for larger integers (64 bits in Java).
	// NOTE: the >> operation must be unsigned! (>>> in Java).
#endif
}

// returns the number of one bits in x
//
// "Population Count (Ones Count)
// The population count of a binary integer value x is the number of one bits in the value. Although many machines have
// single instructions for this, the single instructions are usually microcoded loops that test a bit per cycle; a log-time
// algorithm coded in C is often faster. The following code uses a variable-precision SWAR algorithm to perform a tree
// reduction adding the bits in a 32-bit value:"
//
mxFORCEINLINE U32 OnesCount( U32 x )
{
	/* 32-bit recursive reduction using SWAR...
	but first step is mapping 2-bit values
	into sum of 2 1-bit values in sneaky way
	*/
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0F0F0F0F);
	x += (x >> 8);
	x += (x >> 16);
	return (x & 0x0000003F);
	// "It is worthwhile noting that the SWAR population count algorithm given above can be improved upon for the case of
	// counting the population of multi-word bit sets. How? The last few steps in the reduction are using only a portion
	// of the SWAR width to produce their results; thus, it would be possible to combine these steps across multiple words
	// being reduced.
}

// returns the number of trailing zeros in x
mxFORCEINLINE UINT TrailingZeros( INT x )
{
	// Given the Least Significant 1 Bit and Population Count (Ones Count) algorithms, it is trivial to combine them to
	// construct a trailing zero count (as pointed-out by Joe Bowbeer):"
	return OnesCount( (x & -x) - 1 );
}


// The following C function will compute the Hamming distance of two integers
// (considered as binary values, that is, as sequences of bits).
// The running time of this procedure is proportional
// to the Hamming distance rather than to the number of bits in the inputs.
// It computes the bitwise exclusive or of the two inputs,
// and then finds the Hamming weight of the result (the number of nonzero bits)
// using an algorithm of Wegner (1960) that repeatedly finds
// and clears the lowest-order nonzero bit.
inline
unsigned hamdist( unsigned x, unsigned y )
{
	unsigned dist = 0, val = x ^ y;

	// Count the number of set bits
	while( val )
	{
		++dist; 
		val &= val - 1;
	}

	return dist;
}


/*
        The purpose of this function is to convert an unsigned
        binary number to reflected binary Gray code.
*/
inline
unsigned short binaryToGray(unsigned short num)
{
        return (num>>1) ^ num;
}
 
/*
        The purpose of this function is to convert a reflected binary
        Gray code number to a binary number.
*/
inline
unsigned short grayToBinary(unsigned short num)
{
        unsigned short temp = num ^ (num>>8);
        temp ^= (temp>>4);
        temp ^= (temp>>2);
        temp ^= (temp>>1);
        return temp;
}



mxSTOLEN("Havok");
mxFORCEINLINE U32 mxBitmask(int width)
{
	// return (1<<width) - 1,
	// except it must work for width = 32
	// (bear in mind that shift by >=32 places is undefined)
	U32 maskIfNot32 = (1 << (width & 31)) - 1; // ans if width != 32, else 0
	U32 is32 = ((width & 32) >> 5); // 1 if width = 32, else 0
	return maskIfNot32 - is32; // the answer in the case width=32 is -1
}

#if 0
// Helper class for rounding up integer values to 2^N values.
mxSTOLEN("Nebula 3");

class Round
{
public:
	/// round up to nearest 2 bytes boundary
	mxFORCEINLINE static UINT RoundUp2( UINT val )
	{
		return ((UINT)val + (2 - 1)) & ~(2 - 1);
	}
	/// round up to nearest 4 bytes boundary
	mxFORCEINLINE static UINT RoundUp4( UINT val )
	{
		return ((UINT)val + (4 - 1)) & ~(4 - 1);
	}
	/// round up to nearest 8 bytes boundary
	mxFORCEINLINE static UINT RoundUp8( UINT val )
	{
		return ((UINT)val + (8 - 1)) & ~(8 - 1);
	}
	/// round up to nearest 16 bytes boundary
	mxFORCEINLINE static UINT RoundUp16( UINT val )
	{
		return ((UINT)val + (16 - 1)) & ~(16 - 1);
	}
	/// round up to nearest 32 bytes boundary
	mxFORCEINLINE static UINT RoundUp32( UINT val )
	{
		return ((UINT)val + (32 - 1)) & ~(32 - 1);
	}
	/// generic roundup
	mxFORCEINLINE static UINT RoundUp( UINT val, UINT boundary )
	{
		mxASSERT(0 != boundary);
		return (((val - 1) / boundary) + 1) * boundary;
	}
};
#endif


#if 0
/*
	Increment And Decrement Wrapping Values.

	See:http://cellperformance.beyond3d.com/articles/2006/07/increment-and-decrement-wrapping-values.html#more

	Occasionally you have a set of values that you want to wrap around as you increment and decrement them.
	For example, in a GUI where the user keys right or left and you want to wrap around the menu.
	A typical implementation: 

	static inline int wrap_inc( int value, int min, int max )
	{
		return ( value == max ) ? min : value + 1;
	}

	static inline int wrap_dec( int value, int min, int max )
	{
		return ( value == min ) ? max : value - 1;
	}

	But on processors (such as the PowerPC) where compare and branch is very costly
	these small one-liners can have a significant impact on performance when used in critical code.
	They also make optimization more difficult for the compiler for the surrounding code.
*/
//
// Increment wrapping value      
//
// val = { ( val == max ), min
//     = { otherwise,      val + 1
//
// UINT8_t  wrap_inc_u8 ( const UINT8_t  val, const UINT8_t  min, const UINT8_t  max );
// UINT16_t wrap_inc_u16( const UINT16_t val, const UINT16_t min, const UINT16_t max );
// UINT32_t wrap_inc_u32( const UINT32_t val, const UINT32_t min, const UINT32_t max );
// UINT64_t wrap_inc_u64( const UINT64_t val, const UINT64_t min, const UINT64_t max );
// int8_t   wrap_inc_s8 ( const int8_t   val, const int8_t   min, const int8_t   max );
// int16_t  wrap_inc_s16( const int16_t  val, const int16_t  min, const int16_t  max );
// int32_t  wrap_inc_s32( const int32_t  val, const int32_t  min, const int32_t  max );
// int64_t  wrap_inc_s64( const int64_t  val, const int64_t  min, const int64_t  max );

#define DECL_WRAP_INC( type_name, type, stype, bit_mask )                                  \
static inline type wrap_inc_##type_name( const type val, const type min, const type max )  \
{                                                                                          \
	const type result_inc   = val + 1;                                                       \
	const type max_diff     = max - val;                                                     \
	const type max_diff_nz  = (type)( (stype)( max_diff | -max_diff ) >> bit_mask );         \
	const type max_diff_eqz = ~max_diff_nz;                                                  \
	const type result       = ( result_inc & max_diff_nz ) | ( min & max_diff_eqz );         \
	\
	return (result);                                                                         \
}

DECL_WRAP_INC( U64,  U8,  INT8,  7  ); 
DECL_WRAP_INC( U16, U16, INT16, 15 ); 
DECL_WRAP_INC( U32, U32, INT32, 31 ); 
DECL_WRAP_INC( U64, U64, INT64, 63 ); 
DECL_WRAP_INC( INT64,  INT8,   INT8,  7  ); 
DECL_WRAP_INC( S16, INT16,  INT16, 15 ); 
DECL_WRAP_INC( S32, INT32,  INT32, 31 ); 
DECL_WRAP_INC( S64, INT64,  INT64, 63 ); 

//
// Decrementing wrapping value      
//
// val = { ( val == min ), max
//     = { otherwise,      val - 1
//
// UINT8_t  wrap_dec_u8 ( const UINT8_t  val, const UINT8_t  min, const UINT8_t  max );
// UINT16_t wrap_dec_u16( const UINT16_t val, const UINT16_t min, const UINT16_t max );
// UINT32_t wrap_dec_u32( const UINT32_t val, const UINT32_t min, const UINT32_t max );
// UINT64_t wrap_dec_u64( const UINT64_t val, const UINT64_t min, const UINT64_t max );
// int8_t   wrap_dec_s8 ( const int8_t   val, const int8_t   min, const int8_t   max );
// int16_t  wrap_dec_s16( const int16_t  val, const int16_t  min, const int16_t  max );
// int32_t  wrap_dec_s32( const int32_t  val, const int32_t  min, const int32_t  max );
// int64_t  wrap_dec_s64( const int64_t  val, const int64_t  min, const int64_t  max );

#define DECL_WRAP_DEC( type_name, type, stype, bit_mask )                                  \
	static inline type wrap_dec_##type_name( const type val, const type min, const type max )  \
{                                                                                          \
	const type result_dec   = val - 1;                                                       \
	const type min_diff     = min - val;                                                     \
	const type min_diff_nz  = (type)( (stype)( min_diff | -min_diff ) >> bit_mask );         \
	const type min_diff_eqz = ~min_diff_nz;                                                  \
	const type result       = ( result_dec & min_diff_nz ) | ( max & min_diff_eqz );         \
	\
	return (result);                                                                         \
}

DECL_WRAP_DEC( U64,  U8,  INT8,  7  ); 
DECL_WRAP_DEC( U16, U16, INT16, 15 ); 
DECL_WRAP_DEC( U32, U32, INT32, 31 ); 
DECL_WRAP_DEC( U64, U64, INT64, 63 ); 
DECL_WRAP_DEC( INT64,  INT8,   INT8,  7  ); 
DECL_WRAP_DEC( S16, INT16,  INT16, 15 ); 
DECL_WRAP_DEC( S32, INT32,  INT32, 31 ); 
DECL_WRAP_DEC( S64, INT64,  INT64, 63 );

#endif









mxSTOLEN("CryEngine 3, CryEngine/CryCommon/branchmask.h")

///////////////////////////////////////////
// helper functions for branch elimination
//
// msb/lsb - most/less significant byte
//
// mask - 0xFFFFFFFF
// nz   - not zero
// zr   - is zero

mxFORCEINLINE const U32 nz2msb(const U32 x)
{
	return -(INT32)x | x;
}

mxFORCEINLINE const U32 msb2mask(const U32 x)
{
	return (INT32)(x) >> 31;
}

mxFORCEINLINE const U32 nz2one(const U32 x)
{
	return nz2msb(x) >> 31; // int((bool)x);
}

mxFORCEINLINE const U32 nz2mask(const U32 x)
{
	return (INT32)msb2mask(nz2msb(x)); // -(INT32)(bool)x;
}


mxFORCEINLINE const U32 iselmask(const U32 mask, U32 x, const U32 y)// select integer with mask (0xFFFFFFFF or 0x0 only!!!)
{
	return (x & mask) | (y & ~mask);
}


mxFORCEINLINE const U32 mask_nz_nz(const U32 x, const U32 y)// mask if( x != 0 && y != 0)
{
	return msb2mask(nz2msb(x) & nz2msb(y));
}

mxFORCEINLINE const U32 mask_nz_zr(const U32 x, const U32 y)// mask if( x != 0 && y == 0)
{
	return msb2mask(nz2msb(x) & ~nz2msb(y));
}


mxFORCEINLINE const U32 mask_zr_zr(const U32 x, const U32 y)// mask if( x == 0 && y == 0)
{
	return ~nz2mask(x | y);
}





//----------------------------------------------------------------------------
mxSTOLEN("CryEngine 3");



// Description: various integer bit fiddling hacks

#if defined(LINUX)
#define countLeadingZeros32(x)              __builtin_clz(x)
#elif defined(XENON)
#define countLeadingZeros32(x)              _CountLeadingZeros(x)
#elif defined(PS3)
#if defined(__SPU__)
#define countLeadingZeros32(x)              (spu_extract( spu_cntlz( spu_promote((x),0) ),0) )
#else // #if defined(__SPU__)
// SDK3.2 linker errors when using __cntlzw
mxINLINE U8 countLeadingZeros32( U32 x)
{
	long result;
	__asm__ ("cntlzw %0, %1" 
			/* outputs:  */ : "=r" (result) 
			/* inputs:   */ : "r" (x));
	return result;
}

#endif // #if defined(__SPU__)
#else		// Windows implementation
mxINLINE U32 countLeadingZeros32(U32 x)
{
    DWORD result;
	_BitScanReverse(&result,x);
	result^=31;								// needed because the index is from LSB (whereas all other implementations are from MSB)
    return result;
}
#endif

class CBitIter
{
public:
	mxINLINE CBitIter( U8 x ) : m_val(x) {}
	mxINLINE CBitIter( U16 x ) : m_val(x) {}
	mxINLINE CBitIter( U32 x ) : m_val(x) {}

	template <class T>
	mxINLINE bool Next( T& rIndex )
	{
		bool r = (m_val != 0);
		U32 maskLSB;

		// Rewritten to use clz technique. Note the iterator now works from lsb to msb but that shouldn't matter - this way we avoid microcoded instructions
		maskLSB = m_val & (~(m_val-1));
		rIndex = 31-countLeadingZeros32(maskLSB);
		m_val &= ~maskLSB;
		return r;
	}

private:
	U32 m_val;
};

// this function returns the integer logarithm of various numbers without branching
#define IL2VAL(mask,shift) \
	c |= ((x&mask)!=0) * shift; \
	x >>= ((x&mask)!=0) * shift



inline U8 IntegerLog2( U8 x )
{
	U8 c = 0;
	IL2VAL(0xf0,4);
	IL2VAL(0xc,2);
	IL2VAL(0x2,1);
	return c;
}
inline U16 IntegerLog2( U16 x )
{
	U16 c = 0;
	IL2VAL(0xff00,8);
	IL2VAL(0xf0,4);
	IL2VAL(0xc,2);
	IL2VAL(0x2,1);
	return c;
}

inline U32 IntegerLog2( U32 x )
{
	return 31 - countLeadingZeros32(x);
}

inline U64 IntegerLog2( U64 x )
{
	U64 c = 0;
	IL2VAL(0xffffffff00000000ull,32);
	IL2VAL(0xffff0000u,16);
	IL2VAL(0xff00,8);
	IL2VAL(0xf0,4);
	IL2VAL(0xc,2);
	IL2VAL(0x2,1);
	return c;
}
#undef IL2VAL

template <typename TInteger>
inline TInteger IntegerLog2_RoundUp( TInteger x )
{
	return 1 + IntegerLog2(x-1);
}

static mxINLINE U8 BitIndex( U8 v )
{
	U32 vv=v;
	return 31-countLeadingZeros32(vv);
}

static mxINLINE U8 BitIndex( U16 v )
{
	U32 vv=v;
	return 31-countLeadingZeros32(vv);
}

static mxINLINE U8 BitIndex( U32 v )
{
	return 31-countLeadingZeros32(v);
}

static mxINLINE U8 CountBitsInByte_ALU( U8 v )
{
	U8 c = v;
	c = ((c>>1) & 0x55) + (c&0x55);
	c = ((c>>2) & 0x33) + (c&0x33);
	c = ((c>>4) & 0x0f) + (c&0x0f);
	return c;
}

static mxINLINE U8 CountBitsInByte_LUT( U8 v )
{
	static const U8 NIBBLE_LOOKUP[16] =
	{
		0, 1, 1, 2, 1, 2, 2, 3, 
		1, 2, 2, 3, 2, 3, 3, 4
	};
	return NIBBLE_LOOKUP[v & 0xF] + NIBBLE_LOOKUP[v >> 4];
}

static mxINLINE U8 CountBits( U16 v )
{
	return	CountBits((U8)(v	 &0xff)) + 
		CountBits((U8)((v>> 8)&0xff));
}

static mxINLINE U8 CountBits( U32 v )
{
	return
		CountBitsInByte_ALU((U8)(v      &0xff)) + 
		CountBitsInByte_ALU((U8)((v>> 8)&0xff)) + 
		CountBitsInByte_ALU((U8)((v>>16)&0xff)) + 
		CountBitsInByte_ALU((U8)((v>>24)&0xff));
}

// Branchless version of return v < 0 ? alt : v;
mxINLINE INT32 Isel32(INT32 v, INT32 alt)
{
	return ((static_cast<INT32>(v) >> 31) & alt) | ((static_cast<INT32>(~v) >> 31) & v);
}

template <U32 ILOG>
struct CompileTimeIntegerLog2
{
	static const U32 result = 1 + CompileTimeIntegerLog2<(ILOG>>1)>::result;
};
template <>
struct CompileTimeIntegerLog2<0>
{
	static const U32 result = 0;
};

template <U32 ILOG>
struct CompileTimeIntegerLog2_RoundUp
{
	static const U32 result = CompileTimeIntegerLog2<ILOG>::result + ((ILOG & (ILOG-1))>0);
};

// Character-to-bitfield mapping

inline U32 AlphaBit(char c)
{
	return c >= 'a' && c <= 'z' ? 1 << (c-'z'+31) : 0;
}

inline U32 AlphaBits(U32 wc)
{
	// Handle wide multi-char constants, can be evaluated at compile-time.
	return AlphaBit((char)wc) 
		| AlphaBit((char)(wc>>8))
		| AlphaBit((char)(wc>>16))
		| AlphaBit((char)(wc>>24));
}

inline U32 AlphaBits(const char* s)
{
	// Handle string of any length.
	U32 n = 0;
	while (*s)
		n |= AlphaBit(*s++);
	return n;
}

// is a bit on in a new bit field, but off in an old bit field
static mxINLINE bool TurnedOnBit( unsigned bit, unsigned oldBits, unsigned newBits )
{
	return (newBits & bit) != 0 && (oldBits & bit) == 0;
}

inline U32 cellUtilCountLeadingZero(U32 x)
{
	U32 y;
	U32 n = 32;

	y = x >> 16; if (y != 0) { n = n - 16; x = y; }
	y = x >>  8; if (y != 0) { n = n -  8; x = y; }
	y = x >>  4; if (y != 0) { n = n -  4; x = y; }
	y = x >>  2; if (y != 0) { n = n -  2; x = y; }
	y = x >>  1; if (y != 0) { return n - 2; }
	return n - x;
}

inline U32 cellUtilLog2(U32 x)
{
	return 31 - cellUtilCountLeadingZero(x);
}

/**
 * For signed numbers, we use an encoding called zig-zag encoding
 * that maps signed numbers to unsigned numbers so they can be
 * efficiently encoded using the normal variable-int encoder.
 * /

uint32 EncodeZigZag32(sint32 n)
{
	return (n << 1) ^ (n >> 31);
}

sint32 DecodeZigZag32(uint32 n)
{
	return (n >> 1) ^ -(sint32)(n & 1);
}

uint64 EncodeZigZag64(sint64 n)
{
	return (n << 1) ^ (n >> 63);
}

sint64 DecodeZigZag64(uint64 n)
{
	return (n >> 1) ^ -(sint64)(n & 1);
}
*/
inline U32 UINT32_Replicate( U8 _byte )
{
	U32 ch0 = U32(_byte) << 0;
	U32 ch1 = U32(_byte) << 8;
	U32 ch2 = U32(_byte) << 16;
	U32 ch3 = U32(_byte) << 24;
	return ch0|ch1|ch2|ch3;
}


template< typename UnsignedInteger >
static mxFORCEINLINE
UnsignedInteger rotateRight( UnsignedInteger x, UINT shift_in_bits )
{
	return ( x >> shift_in_bits )
		| ( x << ( BYTES_TO_BITS(sizeof(UnsignedInteger)) - shift_in_bits ) )
		;
}

/// Bit Array utilities.
namespace BitArrays
{
	template< typename INTEGER >
	mxFORCEINLINE INTEGER BitsToInts( INTEGER numBits )
	{
		enum : U32 { bitsInInt = BYTES_TO_BITS(sizeof(INTEGER)) };
		return (numBits / bitsInInt) + ((numBits % bitsInInt) ? 1 : 0);
	}

	template< typename INTEGER >
	mxFORCEINLINE void SetBit( INTEGER *pBits, U32 iBit )
	{
		enum : U32 { bitsInInt = BYTES_TO_BITS(sizeof(INTEGER)) };
		pBits[ iBit / bitsInInt ] |= 1U << (iBit % bitsInInt);
	}

	template< typename INTEGER >
	mxFORCEINLINE BOOL IsSet( const INTEGER* pBits, U32 iBit )
	{
		enum : U32 { bitsInInt = BYTES_TO_BITS(sizeof(INTEGER)) };
		return pBits[ iBit / bitsInInt ] & (1U << (iBit % bitsInInt));
	}

}//namespace BitArrays
