/*
=============================================================================
	File: BitArray.h
	Desc: Bit array.
=============================================================================
*/
#pragma once

///
/// BitArray - should be used instead of an array of bools.
///
struct BitArray: NonCopyable
{
	U32 *	_words;
	U32		_num_words;
	AllocatorI & _allocator;

public:
	BitArray( AllocatorI & _storage )
		: _allocator( _storage )
	{
		_words = 0;
		_num_words = 0;
	}

	/// move ctor for C++03
	BitArray( BitArray & other, TagStealOwnership )
		: _words( other._words )
		, _num_words( other._num_words )
		, _allocator( other._allocator )
	{
		other._words = nil;
		other._num_words = nil;
	}

	~BitArray()
	{
		this->clear();
	}

	///NOTE: doesn't initialize the bit array!
	ERet setup( U32 num_bits )
	{
		this->clear();

		const U32 num_words = BitsToUInts( num_bits );
		_words = (U32*) _allocator.Allocate(
			num_words * sizeof(_words[0]),
			mxALIGNOF(U32)
			);

		if( !_words ) { return ERR_OUT_OF_MEMORY; }

		_num_words = num_words;

		return ALL_OK;
	}

	void clear()
	{
		if( _words ) {
			_allocator.Deallocate( _words );
			_words = nil;
		}
		_num_words = 0;
	}

	mxFORCEINLINE FASTBOOL isSetBit( U32 iBit ) const
	{
		return _words[ iBit >> 5u ] & ( 1u << ( iBit & 31 ) );
	}

	mxFORCEINLINE void setBit( U32 iBit )
	{
		// the same as
		// _words[i / 32] |= (1 << (i % 32));
		_words[ iBit >> 5u ] |= 1u << ( iBit & 31 );
	}

	mxFORCEINLINE void clearBit( U32 iBit )
	{
		// the same as
		// _words[i / 32] &= ~(1 << (i % 32));
		_words[ iBit >> 5u ] &= ~( 1u << ( iBit & 31 ) );
	}

	mxFORCEINLINE void toggleBit( U32 iBit )
	{
		_words[ iBit >> 5u ] ^= 1u << ( iBit & 31 );
	}

	mxFORCEINLINE void clearAllBits()
	{
		for( UINT i = 0; i < _num_words; i++ ) {
			_words[i] = 0;
		}
	}

	mxFORCEINLINE void setAllBits()
	{
		for( UINT i = 0; i < _num_words; i++ ) {
			_words[i] = ~0;
		}
	}

	mxFORCEINLINE const U32* words() const { return _words; }
	mxFORCEINLINE U32   num_words() const { return _num_words; }
};


mxSTOLEN("ICE, by Pierre Terdiman");

// - We consider square symmetric N*N matrices
// - A N*N symmetric matrix has N(N+1)/2 elements
// - A boolean version needs N(N+1)/16 bytes
//  N  NbBits NbBytes
//  4  10  -
//  8  36  4.5
//  16  136  17  <= the one we select
//  32  528  66
static const BYTE BitMasks[]	= { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
static const BYTE NegBitMasks[]	= { 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F };

//
// BoolSquareSymmetricMatrix16
//
struct BoolSquareSymmetricMatrix16
{
	inline UINT32 Index( UINT32 x, UINT32 y ) const { if(x>y) TSwap(x,y); return x + (y ? ((y-1)*y)>>1 : 0);   }

	inline void Set( UINT32 x, UINT32 y )   { UINT32 i = Index(x, y); mBits[i>>3] |= BitMasks[i&7];    }
	inline void clear( UINT32 x, UINT32 y )   { UINT32 i = Index(x, y); mBits[i>>3] &= NegBitMasks[i&7];   }
	inline void Toggle( UINT32 x, UINT32 y )  { UINT32 i = Index(x, y); mBits[i>>3] ^= BitMasks[i&7];    }
	inline bool IsSet( UINT32 x, UINT32 y ) const { UINT32 i = Index(x, y); return (mBits[i>>3] & BitMasks[i&7])!=0; }

	inline void clearAll()	{ ZeroMemory(mBits, 17);  }
	inline void setAll()	{ FillMemory(mBits, 17, 0xff); }

	BYTE mBits[17];
};

//----------------------------------------------------------//
//    End Of File.												//
//----------------------------------------------------------//
