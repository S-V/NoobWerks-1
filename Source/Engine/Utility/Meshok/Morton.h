// Morton codes for linear octrees.
// NOTE: functions for working with 64-bit Morton codes produce terrible code on 32-bit arch.

/*
basics:
http://en.wikipedia.org/wiki/Z-order_curve
http://stackoverflow.com/questions/18529057/produce-interleaving-bit-patterns-morton-keys-for-32-bit-64-bit-and-128bit
http://stackoverflow.com/questions/1024754/how-to-compute-a-3d-morton-number-interleave-the-bits-of-3-ints
http://code.activestate.com/recipes/577558-interleave-bits-aka-morton-ize-aka-z-order-curve/
http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
http://dmytry.com/texts/collision_detection_using_z_order_curve_aka_Morton_order.html
http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
https://www.fpcomplete.com/user/edwardk/revisiting-matrix-multiplication/part-1
https://www.fpcomplete.com/user/edwardk/revisiting-matrix-multiplication/part-2
Converting to and from Dilated Integers [2007]:
http://www.cs.indiana.edu/~dswise/Arcee/castingDilated-comb.pdf

a very good post:
http://asgerhoedt.dk/?p=276

It turns out we don't have to actually interleave the bits
if all we want is the ability to compare two keys as if they had been interleaved.
What we need to know is where the most significant difference between them occurs.
Given two halves of a key we can exclusive or them together to find the positions at which they differ.
*/

#pragma once

//#include <Base/Template/Algorithm/Search.h>	// LowerBoundDescending()
//#include <intrin.h>	// _BitScanReverse64()

/// Morton code/key, used as a cell's locational code or address in linear octrees.
typedef U32 Morton32;
typedef U64 Morton64;


enum
{
	MORTON_ROOT_DEPTH = 0,

	/// Maximum octree depth/refinement level (XYZ: 3 coords of 10 bits each which fits into a 32-bit integer).
	///NOTE: limited by the number of bits in Morton32
	MORTON32_MAX_OCTREE_DEPTH = ( sizeof(Morton32) * BITS_IN_BYTE ) / 3u,	//=> 10
	MORTON32_MAX_NUM_OCTREE_LEVELS = MORTON32_MAX_OCTREE_DEPTH + 1,

	MORTON64_MAX_OCTREE_DEPTH = ( sizeof(Morton64) * BITS_IN_BYTE ) / 3u,	//=> 21
	MORTON64_MAX_NUM_OCTREE_LEVELS = MORTON64_MAX_OCTREE_DEPTH + 1,

	///
	MORTON_INVALID_OCTREE_CELLCODE = 0
};

const Morton32 MORTON32_OCTREE_ROOT_CODE = 1;	//!< The locational code of the root node = 1

/// A 32-bit Morton code can be calculated by interleaving three 10-bit integers,
/// which therefore supports a maximum 3D resolution of 2^10 = 1024.
const U32 MORTON32_MAX_RESOLUTION = (1UL << MORTON32_MAX_OCTREE_DEPTH);	//!< max octree resolution / side length


/// These masks allow to manipulate coordinates without unpacking Morton codes.
const U64 ISOLATE_X = 0x9249249249249249; //!< 0b1001...01001001
const U64 ISOLATE_Y = 0x2492492492492492; //!< 0b0010...10010010
const U64 ISOLATE_Z = 0x4924924924924924; //!< 0b0100...00100100

const U64 ISOLATE_XY = ISOLATE_X | ISOLATE_Y;
const U64 ISOLATE_XZ = ISOLATE_X | ISOLATE_Z;
const U64 ISOLATE_YZ = ISOLATE_Y | ISOLATE_Z;


// Interleaves coordinate bits to build a Morton code: spreads bits out to every-third bit.
// Expands a 10-bit integer into 30 bits by inserting 2 zeros after each bit.
// http://stackoverflow.com/a/18528775/1042102
//......................9876543210
//............98765..........43210
//........987....56......432....10
//......98..7..5..6....43..2..1..0
//....9..8..7..5..6..4..3..2..1..0
/// Dilate bits along the 32-bit unsigned integer.
#if 1
inline U32 Morton32_SpreadBits3( U32 x )
{
	x &= 0x3ff;	// zero out the upper 20 bits
	x = (x | x << 16) & 0x30000ff;	// 0b______11 ________ ________ 11111111
	x = (x | x << 8) & 0x300f00f;	// 0b______11 ________ 1111____ ____1111
	x = (x | x << 4) & 0x30c30c3;	// 0b______11 ____11__ __11____ 11____11
	x = (x | x << 2) & 0x9249249;	// 0b____1__1 __1__1__ 1__1__1_ _1__1__1
	return x;
}//widen/dilate/spread/interleave/interlace
#else
// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
inline U32 Morton32_SpreadBits3( U32 v )
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}
#endif

// from http://www.forceflow.be/2012/07/24/out-of-core-construction-of-sparse-voxel-octrees/
// method to separate bits from a given integer 3 positions apart
inline U64 Morton64_SpreadBits3( U32 n )
{
    U64 x = n & 0x1fffff; // we only look at the first 21 bits
    x = (x | x << 32) & 0x1f00000000ffff;  //         ___11111________________________________1111111111111111
    x = (x | x << 16) & 0x1f0000ff0000ff;  //         ___11111________________11111111________________11111111
    x = (x | x << 8) & 0x100f00f00f00f00f; // ___1________1111________1111________1111________1111____________
    x = (x | x << 4) & 0x10c30c30c30c30c3; // ___1____11____11____11____11____11____11____11____11___1________
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}


/*
https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
*/
/// Contract bits along the 32-bit unsigned integer.
inline U32 Morton32_CompactBits3( U32 x )
{
	x &= 0x09249249; // 0b...01001001
	x = (x ^ (x >>  2)) & 0x030c30c3;
	x = (x ^ (x >>  4)) & 0x0300f00f;
	x = (x ^ (x >>  8)) & 0xff0000ff;
	x = (x ^ (x >> 16)) & 0x000003ff;
	return x;
}//shrink/contract/compact


inline U32 Morton64_CompactBits3( U64 M )
{
	M &= 0x1249249249249249;
	M = (M ^ (M >> 2))  & 0x30c30c30c30c30c3;
	M = (M ^ (M >> 4))  & 0xf00f00f00f00f00f;
	M = (M ^ (M >> 8))  & 0x00ff0000ff0000ff;
	M = (M ^ (M >> 16)) & 0x00ff00000000ffff;
	M = (M ^ (M >> 32)) & 0x1fffff;
	return M;
}//shrink/contract/compact


// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
inline Morton32 Morton32_BuildForPoint( float x, float y, float z )
{
	x = minf(maxf(x * 1024.0f, 0.0f), 1023.0f);
	y = minf(maxf(y * 1024.0f, 0.0f), 1023.0f);
	z = minf(maxf(z * 1024.0f, 0.0f), 1023.0f);
	U32 xx = Morton32_SpreadBits3((U32)x);
	U32 yy = Morton32_SpreadBits3((U32)y);
	U32 zz = Morton32_SpreadBits3((U32)z);
	return xx * 4 + yy * 2 + zz;
}


/* add two morton keys (xyz interleaving) 
morton3(4,5,6) + morton3(1,2,3) == morton3(5,7,9);*/
/// Dilated integer addition.

template< typename MortonCode >	// where MortonCode is Morton32 or Morton64
static mxFORCEINLINE
MortonCode Morton_Add( const MortonCode a, const MortonCode b )
{
	const MortonCode xxx = ( a | ISOLATE_YZ ) + ( b & ISOLATE_X );
	const MortonCode yyy = ( a | ISOLATE_XZ ) + ( b & ISOLATE_Y );
	const MortonCode zzz = ( a | ISOLATE_XY ) + ( b & ISOLATE_Z );
	// Masked merge bits https://graphics.stanford.edu/~seander/bithacks.html#MaskedMerge
	return (xxx & ISOLATE_X) | (yyy & ISOLATE_Y) | (zzz & ISOLATE_Z);
}

#define Morton32_Add( a, b )	Morton_Add( (a), (b) )


/// Dilated integer subtraction.
static mxFORCEINLINE
Morton32 Morton32_Sub( const Morton32 a, const Morton32 b )
{
	const Morton32 xxx = ( a & ISOLATE_X ) - ( b & ISOLATE_X );
	const Morton32 yyy = ( a & ISOLATE_Y ) - ( b & ISOLATE_Y );
	const Morton32 zzz = ( a & ISOLATE_Z ) - ( b & ISOLATE_Z );
	// Masked merge bits https://graphics.stanford.edu/~seander/bithacks.html#MaskedMerge
	return (xxx & ISOLATE_X) | (yyy & ISOLATE_Y) | (zzz & ISOLATE_Z);
}

inline const UInt3 Morton32_Decode( const Morton32 _code )
{
	return UInt3(
		Morton32_CompactBits3(_code >> 0),
		Morton32_CompactBits3(_code >> 1),
		Morton32_CompactBits3(_code >> 2)
	);
}
/// Encode three 10-bit integers into a 32-bit Morton code
inline Morton32 Morton32_Encode( const U32 _x, const U32 _y, const U32 _z )
{
	mxASSERT( _x < MORTON32_MAX_RESOLUTION );
	mxASSERT( _y < MORTON32_MAX_RESOLUTION );
	mxASSERT( _z < MORTON32_MAX_RESOLUTION );
	return
		(Morton32_SpreadBits3(_x) << 0)|
		(Morton32_SpreadBits3(_y) << 1)|
		(Morton32_SpreadBits3(_z) << 2);
}

inline Morton32 Morton32_Encode_NoOverflowCheck( const U32 _x, const U32 _y, const U32 _z )
{
	return
		(Morton32_SpreadBits3(_x) << 0)|
		(Morton32_SpreadBits3(_y) << 1)|
		(Morton32_SpreadBits3(_z) << 2);
}

/// Encodes three 21-bit integers into a 63-bit Morton code.
inline Morton64 Morton64_Encode( const U32 _x, const U32 _y, const U32 _z )
{
	return
		(Morton64_SpreadBits3(_x) << 0)|
		(Morton64_SpreadBits3(_y) << 1)|
		(Morton64_SpreadBits3(_z) << 2);
}

template< class XYZ >
inline Morton32 Morton32_Encode( const XYZ& _xyz )
{
	return Morton32_Encode( _xyz.x, _xyz.y, _xyz.z );
}

template< class XYZ >
inline Morton64 Morton64_Encode( const XYZ& _xyz )
{
	return Morton64_Encode( _xyz.x, _xyz.y, _xyz.z );
}

// from https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/

// "Insert" a 0 bit after each of the 16 low bits of x
inline U32 Part1By1(U32 x)
{
  x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
inline U32 Part1By2(U32 x)
{
  x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

inline U32 PartBy4(U32 x)
{
    x &= 0x0000007f;                  // x = ---- ---- ---- ---- ---- ---- -654 3210
    x = (x ^ (x << 16)) & 0x0070000F; // x = ---- ---- -654 ---- ---- ---- ---- 3210
    x = (x ^ (x <<  8)) & 0x40300C03; // x = -6-- ---- --54 ---- ---- 32-- ---- --10
    x = (x ^ (x <<  4)) & 0x42108421; // x = -6-- --5- ---4 ---- 3--- -2-- --1- ---0
    return x;
}

// Inverse of Part1By1 - "delete" all odd-indexed bits
inline U32 Compact1By1(U32 x)
{
  x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
inline U32 Compact1By2(U32 x)
{
  x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
  return x;
}

inline U32 EncodeMorton2(U32 x, U32 y)
{
  return (Part1By1(y) << 1) + Part1By1(x);
}

inline U32 EncodeMorton3(U32 x, U32 y, U32 z)
{
  return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

inline U32 DecodeMorton2X(U32 code)
{
  return Compact1By1(code >> 0);
}

inline U32 DecodeMorton2Y(U32 code)
{
  return Compact1By1(code >> 1);
}

inline U32 DecodeMorton3X(U32 code)
{
  return Compact1By2(code >> 0);
}

inline U32 DecodeMorton3Y(U32 code)
{
  return Compact1By2(code >> 1);
}

inline U32 DecodeMorton3Z(U32 code)
{
  return Compact1By2(code >> 2);
}


template< typename MortonCode >
struct MortonMaximalNeighbors
{
	const MortonCode	neighhor_pos_X;
	const MortonCode	neighhor_pos_Y;
	const MortonCode	neighhor_pos_Z;

	const MortonCode	neighhor_pos_XY;
	const MortonCode	neighhor_pos_YZ;
	const MortonCode	neighhor_pos_XZ;

	const MortonCode	neighhor_pos_XYZ;

public:
	mxFORCEINLINE MortonMaximalNeighbors(const MortonCode code)
		: neighhor_pos_X( Morton_Add<MortonCode>( code, BIT(0) ) )
		, neighhor_pos_Y( Morton_Add<MortonCode>( code, BIT(1) ) )
		, neighhor_pos_Z( Morton_Add<MortonCode>( code, BIT(2) ) )
		//
		, neighhor_pos_XY( Morton_Add<MortonCode>( code, BIT(1)|BIT(0) ) )
		, neighhor_pos_YZ( Morton_Add<MortonCode>( code, BIT(2)|BIT(1) ) )
		, neighhor_pos_XZ( Morton_Add<MortonCode>( code, BIT(2)|BIT(0) ) )
		//
		, neighhor_pos_XYZ( Morton_Add<MortonCode>( code, BIT(2)|BIT(1)|BIT(0) ) )
	{}
};

/*
Faster 16/32bit 2D Morton Code encode/decode functions 
https://gist.github.com/JarkkoPFC/0e4e599320b0cc7ea92df45fb416d79a

Based on some micro benchmarking, these functions are ~50% faster than classic bit twiddling implementations using separate2/compact2 for each coordinate. It's also better to use 16-bit version for performance if xy-coordinates are in range 0-255, which gains further ~30% boost.

uint16_t encode16_morton2(uint8_t x_, uint8_t y_)
{
  uint32_t res=x_|(uint32_t(y_)<<16);
  res=(res|(res<<4))&0x0f0f0f0f;
  res=(res|(res<<2))&0x33333333;
  res=(res|(res<<1))&0x55555555;
  return uint16_t(res|(res>>15));
}
//----

uint32_t encode32_morton2(uint16_t x_, uint16_t y_)
{
  uint64_t res=x_|(uint64_t(y_)<<32);
  res=(res|(res<<8))&0x00ff00ff00ff00ff;
  res=(res|(res<<4))&0x0f0f0f0f0f0f0f0f;
  res=(res|(res<<2))&0x3333333333333333;
  res=(res|(res<<1))&0x5555555555555555;
  return uint32_t(res|(res>>31));
}
//----

void decode16_morton2(uint8_t &x_, uint8_t &y_, uint16_t mc_)
{
  uint32_t res=(mc_|(uint32_t(mc_)<<15))&0x55555555;
  res=(res|(res>>1))&0x33333333;
  res=(res|(res>>2))&0x0f0f0f0f;
  res=res|(res>>4);
  x_=uint8_t(res);
  y_=uint8_t(res>>16);
}
//----

void decode32_morton2(uint16_t &x_, uint16_t &y_, uint32_t mc_)
{
  uint64_t res=(mc_|(uint64_t(mc_)<<31))&0x5555555555555555;
  res=(res|(res>>1))&0x3333333333333333;
  res=(res|(res>>2))&0x0f0f0f0f0f0f0f0f;
  res=(res|(res>>4))&0x00ff00ff00ff00ff;
  res=res|(res>>8);
  x_=uint16_t(res);
  y_=uint16_t(res>>32);
}
*/
