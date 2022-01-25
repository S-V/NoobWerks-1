// Space-Filling Curves: Morton/Z-/Lebegue and Hilbert curves.
#pragma once

// Taken from:
// http://and-what-happened.blogspot.ru/2011/08/fast-2d-and-3d-hilbert-curves-and.html
namespace SFC
{
	// The 'bits' parameter in the Morton/Hilbert conversion functions refers to the extent of the encoded space.
	// For instance a bits value of 1 indicates a 2x2(x2) space, 2 is a 4x4(x4) space, 3 is a 8x8(x8) space and so on.

	inline
	U32 MortonToHilbert2D( const U32 morton, const U32 bits )
	{
		U32 hilbert = 0;
		U32 remap = 0xb4;
		U32 block = ( bits << 1 );
		while( block )
		{
			block -= 2;
			U32 mcode = ( ( morton >> block ) & 3 );
			U32 hcode = ( ( remap >> ( mcode << 1 ) ) & 3 );
			remap ^= ( 0x82000028 >> ( hcode << 3 ) );
			hilbert = ( ( hilbert << 2 ) + hcode );
		}
		return( hilbert );
	}

	inline
	U32 HilbertToMorton2D( const U32 hilbert, const U32 bits )
	{
		U32 morton = 0;
		U32 remap = 0xb4;
		U32 block = ( bits << 1 );
		while( block )
		{
			block -= 2;
			U32 hcode = ( ( hilbert >> block ) & 3 );
			U32 mcode = ( ( remap >> ( hcode << 1 ) ) & 3 );
			remap ^= ( 0x330000cc >> ( hcode << 3 ) );
			morton = ( ( morton << 2 ) + mcode );
		}
		return( morton );
	}

	inline
	U32 MortonToHilbert3D( const U32 morton, const U32 bits )
	{
		U32 hilbert = morton;
		if( bits > 1 )
		{
			U32 block = ( ( bits * 3 ) - 3 );
			U32 hcode = ( ( hilbert >> block ) & 7 );
			U32 mcode, shift, signs;
			shift = signs = 0;
			while( block )
			{
				block -= 3;
				hcode <<= 2;
				mcode = ( ( 0x20212021 >> hcode ) & 3 );
				shift = ( ( 0x48 >> ( 7 - shift - mcode ) ) & 3 );
				signs = ( ( signs | ( signs << 3 ) ) >> mcode );
				signs = ( ( signs ^ ( 0x53560300 >> hcode ) ) & 7 );
				mcode = ( ( hilbert >> block ) & 7 );
				hcode = mcode;
				hcode = ( ( ( hcode | ( hcode << 3 ) ) >> shift ) & 7 );
				hcode ^= signs;
				hilbert ^= ( ( mcode ^ hcode ) << block );
			}
		}
		hilbert ^= ( ( hilbert >> 1 ) & 0x92492492 );
		hilbert ^= ( ( hilbert & 0x92492492 ) >> 1 );
		return( hilbert );
	}

	inline
	U32 HilbertToMorton3D( const U32 hilbert, const U32 bits )
	{
		U32 morton = hilbert;
		morton ^= ( ( morton & 0x92492492 ) >> 1 );
		morton ^= ( ( morton >> 1 ) & 0x92492492 );
		if( bits > 1 )
		{
			U32 block = ( ( bits * 3 ) - 3 );
			U32 hcode = ( ( morton >> block ) & 7 );
			U32 mcode, shift, signs;
			shift = signs = 0;
			while( block )
			{
				block -= 3;
				hcode <<= 2;
				mcode = ( ( 0x20212021 >> hcode ) & 3 );
				shift = ( ( 0x48 >> ( 4 - shift + mcode ) ) & 3 );
				signs = ( ( signs | ( signs << 3 ) ) >> mcode );
				signs = ( ( signs ^ ( 0x53560300 >> hcode ) ) & 7 );
				hcode = ( ( morton >> block ) & 7 );
				mcode = hcode;
				mcode ^= signs;
				mcode = ( ( ( mcode | ( mcode << 3 ) ) >> shift ) & 7 );
				morton ^= ( ( hcode ^ mcode ) << block );
			}
		}
		return( morton );
	}

	inline
	U32 Morton_2D_Encode_5bit( U32 index1, U32 index2 )
	{ // pack 2 5-bit indices into a 10-bit Morton code
		index1 &= 0x0000001f;
		index2 &= 0x0000001f;
		index1 *= 0x01041041;
		index2 *= 0x01041041;
		index1 &= 0x10204081;
		index2 &= 0x10204081;
		index1 *= 0x00108421;
		index2 *= 0x00108421;
		index1 &= 0x15500000;
		index2 &= 0x15500000;
		return( ( index1 >> 20 ) | ( index2 >> 19 ) );
	}

	inline
	void Morton_2D_Decode_5bit( const U32 morton, U32& index1, U32& index2 )
	{ // unpack 2 5-bit indices from a 10-bit Morton code
		U32 value1 = morton;
		U32 value2 = ( value1 >> 1 );
		value1 &= 0x00000155;
		value2 &= 0x00000155;
		value1 |= ( value1 >> 1 );
		value2 |= ( value2 >> 1 );
		value1 &= 0x00000133;
		value2 &= 0x00000133;
		value1 |= ( value1 >> 2 );
		value2 |= ( value2 >> 2 );
		value1 &= 0x0000010f;
		value2 &= 0x0000010f;
		value1 |= ( value1 >> 4 );
		value2 |= ( value2 >> 4 );
		value1 &= 0x0000001f;
		value2 &= 0x0000001f;
		index1 = value1;
		index2 = value2;
	}

	inline
	U32 Morton_2D_Encode_16bit( U32 index1, U32 index2 )
	{ // pack 2 16-bit indices into a 32-bit Morton code
		index1 &= 0x0000ffff;
		index2 &= 0x0000ffff;
		index1 |= ( index1 << 8 );
		index2 |= ( index2 << 8 );
		index1 &= 0x00ff00ff;
		index2 &= 0x00ff00ff;
		index1 |= ( index1 << 4 );
		index2 |= ( index2 << 4 );
		index1 &= 0x0f0f0f0f;
		index2 &= 0x0f0f0f0f;
		index1 |= ( index1 << 2 );
		index2 |= ( index2 << 2 );
		index1 &= 0x33333333;
		index2 &= 0x33333333;
		index1 |= ( index1 << 1 );
		index2 |= ( index2 << 1 );
		index1 &= 0x55555555;
		index2 &= 0x55555555;
		return( index1 | ( index2 << 1 ) );
	}

	inline
	void Morton_2D_Decode_16bit( const U32 morton, U32& index1, U32& index2 )
	{ // unpack 2 16-bit indices from a 32-bit Morton code
		U32 value1 = morton;
		U32 value2 = ( value1 >> 1 );
		value1 &= 0x55555555;
		value2 &= 0x55555555;
		value1 |= ( value1 >> 1 );
		value2 |= ( value2 >> 1 );
		value1 &= 0x33333333;
		value2 &= 0x33333333;
		value1 |= ( value1 >> 2 );
		value2 |= ( value2 >> 2 );
		value1 &= 0x0f0f0f0f;
		value2 &= 0x0f0f0f0f;
		value1 |= ( value1 >> 4 );
		value2 |= ( value2 >> 4 );
		value1 &= 0x00ff00ff;
		value2 &= 0x00ff00ff;
		value1 |= ( value1 >> 8 );
		value2 |= ( value2 >> 8 );
		value1 &= 0x0000ffff;
		value2 &= 0x0000ffff;
		index1 = value1;
		index2 = value2;
	}

	inline
	U32 Morton_3D_Encode_5bit( U32 index1, U32 index2, U32 index3 )
	{ // pack 3 5-bit indices into a 15-bit Morton code
		index1 &= 0x0000001f;
		index2 &= 0x0000001f;
		index3 &= 0x0000001f;
		index1 *= 0x01041041;
		index2 *= 0x01041041;
		index3 *= 0x01041041;
		index1 &= 0x10204081;
		index2 &= 0x10204081;
		index3 &= 0x10204081;
		index1 *= 0x00011111;
		index2 *= 0x00011111;
		index3 *= 0x00011111;
		index1 &= 0x12490000;
		index2 &= 0x12490000;
		index3 &= 0x12490000;
		return( ( index1 >> 16 ) | ( index2 >> 15 ) | ( index3 >> 14 ) );
	}

	inline
	void Morton_3D_Decode_5bit( const U32 morton,
		U32& index1, U32& index2, U32& index3 )
	{ // unpack 3 5-bit indices from a 15-bit Morton code
		U32 value1 = morton;
		U32 value2 = ( value1 >> 1 );
		U32 value3 = ( value1 >> 2 );
		value1 &= 0x00001249;
		value2 &= 0x00001249;
		value3 &= 0x00001249;
		value1 |= ( value1 >> 2 );
		value2 |= ( value2 >> 2 );
		value3 |= ( value3 >> 2 );
		value1 &= 0x000010c3;
		value2 &= 0x000010c3;
		value3 &= 0x000010c3;
		value1 |= ( value1 >> 4 );
		value2 |= ( value2 >> 4 );
		value3 |= ( value3 >> 4 );
		value1 &= 0x0000100f;
		value2 &= 0x0000100f;
		value3 &= 0x0000100f;
		value1 |= ( value1 >> 8 );
		value2 |= ( value2 >> 8 );
		value3 |= ( value3 >> 8 );
		value1 &= 0x0000001f;
		value2 &= 0x0000001f;
		value3 &= 0x0000001f;
		index1 = value1;
		index2 = value2;
		index3 = value3;
	}

	inline
	U32 Morton_3D_Encode_10bit( U32 index1, U32 index2, U32 index3 )
	{ // pack 3 10-bit indices into a 30-bit Morton code
		index1 &= 0x000003ff;
		index2 &= 0x000003ff;
		index3 &= 0x000003ff;
		index1 |= ( index1 << 16 );
		index2 |= ( index2 << 16 );
		index3 |= ( index3 << 16 );
		index1 &= 0x030000ff;
		index2 &= 0x030000ff;
		index3 &= 0x030000ff;
		index1 |= ( index1 << 8 );
		index2 |= ( index2 << 8 );
		index3 |= ( index3 << 8 );
		index1 &= 0x0300f00f;
		index2 &= 0x0300f00f;
		index3 &= 0x0300f00f;
		index1 |= ( index1 << 4 );
		index2 |= ( index2 << 4 );
		index3 |= ( index3 << 4 );
		index1 &= 0x030c30c3;
		index2 &= 0x030c30c3;
		index3 &= 0x030c30c3;
		index1 |= ( index1 << 2 );
		index2 |= ( index2 << 2 );
		index3 |= ( index3 << 2 );
		index1 &= 0x09249249;
		index2 &= 0x09249249;
		index3 &= 0x09249249;
		return( index1 | ( index2 << 1 ) | ( index3 << 2 ) );
	}

	inline
	void Morton_3D_Decode_10bit( const U32 morton,
		U32& index1, U32& index2, U32& index3 )
	{ // unpack 3 10-bit indices from a 30-bit Morton code
		U32 value1 = morton;
		U32 value2 = ( value1 >> 1 );
		U32 value3 = ( value1 >> 2 );
		value1 &= 0x09249249;
		value2 &= 0x09249249;
		value3 &= 0x09249249;
		value1 |= ( value1 >> 2 );
		value2 |= ( value2 >> 2 );
		value3 |= ( value3 >> 2 );
		value1 &= 0x030c30c3;
		value2 &= 0x030c30c3;
		value3 &= 0x030c30c3;
		value1 |= ( value1 >> 4 );
		value2 |= ( value2 >> 4 );
		value3 |= ( value3 >> 4 );
		value1 &= 0x0300f00f;
		value2 &= 0x0300f00f;
		value3 &= 0x0300f00f;
		value1 |= ( value1 >> 8 );
		value2 |= ( value2 >> 8 );
		value3 |= ( value3 >> 8 );
		value1 &= 0x030000ff;
		value2 &= 0x030000ff;
		value3 &= 0x030000ff;
		value1 |= ( value1 >> 16 );
		value2 |= ( value2 >> 16 );
		value3 |= ( value3 >> 16 );
		value1 &= 0x000003ff;
		value2 &= 0x000003ff;
		value3 &= 0x000003ff;
		index1 = value1;
		index2 = value2;
		index3 = value3;
	}

}//namespace SFC

// Taken from:
// http://threadlocalmutex.com/?p=126
namespace SFC
{
	U32 deinterleave(U32 x)
	{
	  x = x & 0x55555555;
	  x = (x | (x >> 1)) & 0x33333333;
	  x = (x | (x >> 2)) & 0x0F0F0F0F;
	  x = (x | (x >> 4)) & 0x00FF00FF;
	  x = (x | (x >> 8)) & 0x0000FFFF;
	  return x;
	}

	U32 interleave(U32 x)
	{
	  x = (x | (x << 8)) & 0x00FF00FF;
	  x = (x | (x << 4)) & 0x0F0F0F0F;
	  x = (x | (x << 2)) & 0x33333333;
	  x = (x | (x << 1)) & 0x55555555;
	  return x;
	}

	U32 prefixScan(U32 x)
	{
	  x = (x >> 8) ^ x;
	  x = (x >> 4) ^ x;
	  x = (x >> 2) ^ x;
	  x = (x >> 1) ^ x;
	  return x;
	}

	U32 descan(U32 x)
	{
	  return x ^ (x >> 1);
	}

	void hilbertIndexToXY(U32 n, U32 i, U32& x, U32& y)
	{
	  i = i << (32 - 2 * n);

	  U32 i0 = deinterleave(i);
	  U32 i1 = deinterleave(i >> 1);

	  U32 t0 = (i0 | i1) ^ 0xFFFF;
	  U32 t1 = i0 & i1;

	  U32 prefixT0 = prefixScan(t0);
	  U32 prefixT1 = prefixScan(t1);

	  U32 a = (((i0 ^ 0xFFFF) & prefixT1) | (i0 & prefixT0));

	  x = (a ^ i1) >> (16 - n);
	  y = (a ^ i0 ^ i1) >> (16 - n);
	}

	U32 hilbertXYToIndex(U32 n, U32 x, U32 y)
	{
	  x = x << (16 - n);
	  y = y << (16 - n);

	  U32 A, B, C, D;

	  // Initial prefix scan round, prime with x and y
	  {
		U32 a = x ^ y;
		U32 b = 0xFFFF ^ a;
		U32 c = 0xFFFF ^ (x | y);
		U32 d = x & (y ^ 0xFFFF);

		A = a | (b >> 1);
		B = (a >> 1) ^ a;

		C = ((c >> 1) ^ (b & (d >> 1))) ^ c;
		D = ((a & (c >> 1)) ^ (d >> 1)) ^ d;
	  }

	  {
		U32 a = A;
		U32 b = B;
		U32 c = C;
		U32 d = D;

		A = ((a & (a >> 2)) ^ (b & (b >> 2)));
		B = ((a & (b >> 2)) ^ (b & ((a ^ b) >> 2)));

		C ^= ((a & (c >> 2)) ^ (b & (d >> 2)));
		D ^= ((b & (c >> 2)) ^ ((a ^ b) & (d >> 2)));
	  }

	  {
		U32 a = A;
		U32 b = B;
		U32 c = C;
		U32 d = D;

		A = ((a & (a >> 4)) ^ (b & (b >> 4)));
		B = ((a & (b >> 4)) ^ (b & ((a ^ b) >> 4)));

		C ^= ((a & (c >> 4)) ^ (b & (d >> 4)));
		D ^= ((b & (c >> 4)) ^ ((a ^ b) & (d >> 4)));
	  }

	  // Final round and projection
	  {
		U32 a = A;
		U32 b = B;
		U32 c = C;
		U32 d = D;

		C ^= ((a & (c >> 8)) ^ (b & (d >> 8)));
		D ^= ((b & (c >> 8)) ^ ((a ^ b) & (d >> 8)));
	  }

	  // Undo transformation prefix scan
	  U32 a = C ^ (C >> 1);
	  U32 b = D ^ (D >> 1);

	  // Recover index bits
	  U32 i0 = x ^ y;
	  U32 i1 = b | (0xFFFF ^ (i0 | a));

	  return ((interleave(i1) << 1) | interleave(i0)) >> (32 - 2 * n);
	}

}//namespace SFC

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
