/*
=============================================================================
	File:	BitSet32.h
	Desc:
=============================================================================
*/
#pragma once

struct BitSet32
{
	U32		_value;

public:
	mxFORCEINLINE BitSet32()
	{}

	mxFORCEINLINE BitSet32( U32 value )
		: _value( value )
	{}

	// Returns (in the lowest bit position) the bit at the given index.
	mxFORCEINLINE int	get( int index )
	{
		return _value & (1 << index);
		//_bittest( _value, index );
	}

	// Set the bit at the given index to 1.
	mxFORCEINLINE void set( int index )
	{
		_value |= (1 << index);
	}

	// clear the bit at the given index to 0.
	mxFORCEINLINE void clear( int index )
	{
		_value &= ~(1 << index);
	}

	// Flips the bit at the given index.
	mxFORCEINLINE void flip( int index )
	{
		_value ^= (1 << index);
	}

	// Returns the index of the first set bit.
	mxFORCEINLINE unsigned firstOneBit()
	{
		DWORD index;
		// Search the mask data from least significant bit (LSB) to the most significant bit (MSB) for a set bit (1).
		_BitScanForward( &index, _value );
		return index;
	}
	
	// Returns the index of the first zero bit. Slow.
	mxFORCEINLINE unsigned firstZeroBit()
	{
		DWORD inv = ~this->_value;
		DWORD index;
		_BitScanForward( &index, inv );
		return index;
	}

	mxFORCEINLINE void setAll( int value )
	{
		_value = value;
	}
	
	mxFORCEINLINE void clearAll()
	{
		_value = 0;
	}

	mxFORCEINLINE UINT capacity() const
	{
		return BYTES_TO_BITS(sizeof(*this));
	}
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
