/*
=============================================================================
	File:	TBitField.h
	Desc:	A large static bit array with a size multiple of 32 bits.
	Note:	 (C) 2009 Radon Labs GmbH
=============================================================================
*/

#ifndef __MX_BIT_FIELD_H__
#define __MX_BIT_FIELD_H__
mxSTOLEN("Nebula3");


template< unsigned int NUMBITS >
class TBitField
{
	static const int size = ((NUMBITS + 31) / 32);
	U32 mBits[ size ];

public:
	/// constructor
	TBitField();
	/// copy constructor
	TBitField(const TBitField<NUMBITS>& rhs);


	/// Returns (in the lowest bit position) the bit at the given index.
	mxFORCEINLINE UINT Get( UINT iBit ) const
	{
		return mBits[ iBit >> 5 ] & (1 << (iBit & 31));
	}

	/// Set or clear the bit at the given index.
	mxFORCEINLINE void SetCond( UINT iBit, bool bSet )
	{
		if( bSet ) {
			this->SetBit( iBit );
		} else {
			this->ClearBit( iBit );
		}
	}

	/// assignment operator
	void operator=(const TBitField<NUMBITS>& rhs);
	/// equality operator
	bool operator==(const TBitField<NUMBITS>& rhs) const;
	/// inequality operator
	bool operator!=(const TBitField<NUMBITS>& rhs) const;

	mxFORCEINLINE bool operator [] ( UINT iBit ) const
	{
		return this->Get( iBit );
	}

	/// clear content
	void ClearAllBits();
	/// return true if all bits are 0
	bool IsNil() const;
	/// set a bit by index
	void SetBit(UINT bitIndex);
	/// clear a bit by index
	void ClearBit(UINT bitIndex);

	/// set bitfield to OR combination
	static TBitField<NUMBITS> Or(const TBitField<NUMBITS>& b0, const TBitField<NUMBITS>& b1);
	/// set bitfield to AND combination
	static TBitField<NUMBITS> And(const TBitField<NUMBITS>& b0, const TBitField<NUMBITS>& b1);
};

template<unsigned int NUMBITS>
TBitField<NUMBITS>::TBitField()
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        mBits[i] = 0;
    }
}

template<unsigned int NUMBITS>
TBitField<NUMBITS>::TBitField(const TBitField<NUMBITS>& rhs)
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        mBits[i] = rhs.mBits[i];
    }
}

template<unsigned int NUMBITS> void
TBitField<NUMBITS>::operator=(const TBitField<NUMBITS>& rhs)
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        mBits[i] = rhs.mBits[i];
    }
}

template<unsigned int NUMBITS> bool
TBitField<NUMBITS>::operator==(const TBitField<NUMBITS>& rhs) const
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        if (mBits[i] != rhs.mBits[i])
        {
            return false;
        }
    }
    return true;
}

template<unsigned int NUMBITS> bool
TBitField<NUMBITS>::operator!=(const TBitField<NUMBITS>& rhs) const
{
    return !(*this == rhs);
}

template<unsigned int NUMBITS> void
TBitField<NUMBITS>::ClearAllBits()
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        mBits[i] = 0;
    }
}

template<unsigned int NUMBITS> bool
TBitField<NUMBITS>::IsNil() const
{
    UINT i;
    for (i = 0; i < size; i++)
    {
        if (mBits[i] != 0)
        {
            return false;
        }
    }
    return true;
}

template<unsigned int NUMBITS> void
TBitField<NUMBITS>::SetBit(UINT i)
{
    mxASSERT(i < NUMBITS);
    mBits[i / 32] |= (1 << (i % 32));
}

template<unsigned int NUMBITS> void
TBitField<NUMBITS>::ClearBit(UINT i)
{
    mxASSERT(i < NUMBITS);
    mBits[i / 32] &= ~(1 << (i % 32));
}

template<unsigned int NUMBITS> TBitField<NUMBITS>
TBitField<NUMBITS>::Or(const TBitField<NUMBITS>& b0, const TBitField<NUMBITS>& b1)
{
    TBitField<NUMBITS> res;
    UINT i;
    for (i = 0; i < size; i++)
    {
        res.mBits[i] = b0.mBits[i] | b1.mBits[i];
    }
    return res;
}

template<unsigned int NUMBITS> TBitField<NUMBITS>
TBitField<NUMBITS>::And(const TBitField<NUMBITS>& b0, const TBitField<NUMBITS>& b1)
{
    TBitField<NUMBITS> res;
    UINT i;
    for (i = 0; i < size; i++)
    {
        res.mBits[i] = b0.mBits[i] & b1.mBits[i];
    }
    return res;
}



#endif // ! __MX_BIT_FIELD_H__

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
