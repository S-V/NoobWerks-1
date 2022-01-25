/*
=============================================================================
	File:	Misc.h
	Desc:	misc utility code that doesn't fit into any particular category
=============================================================================
*/
#pragma once

#include <Base/Util/Sorting.h>


namespace EndianUtils
{
	//-----------------------------------------------------------------------
	//	ENDIANNESS
	//-----------------------------------------------------------------------
	// Extra comments by Kenny Hoff:
	// Determines the byte ordering of the current machine (little or big endian)
	// by setting an integer value to 1 (so least significant bit is now 1); take
	// the address of the int and cast to a byte pointer (treat integer as an
	// array of four bytes); check the value of the first byte (must be 0 or 1).
	// If the value is 1, then the first byte least significant byte and this
	// implies LITTLE endian. If the value is 0, the first byte is the most
	// significant byte, BIG endian. Examples:
	//      integer 1 on BIG endian: 00000000 00000000 00000000 00000001
	//   integer 1 on LITTLE endian: 00000001 00000000 00000000 00000000
	// Intel/AMD chips tend to be little endian, MAC PPC's and Suns are big endian.
	//-----------------------------------------------------------------------
	inline bool IsLittleEndian()
	{
		INT32 i = 1;
		return *((UINT8*) & i) != 0;
	}
	inline bool IsBigEndian()
	{
		return !IsLittleEndian();
	}
}//namespace EndianUtils

//
//	TObjectPtrTable - a static table of pointers to 'TYPE' instances.
//
//	May be used for accessing objects by indices.
//	Relative ordering of array elements can carry some useful information.
//
template< typename TYPE, UINT MAX >
struct TObjectPtrTable
{
public:

	// returns unique index

	mxFORCEINLINE UINT GetUniqueIndex() const
	{
		return m_uniqueIndex;
	}

public:
	mxFORCEINLINE static TYPE* GetPtrByIndex( UINT index )
	{
		//mxASSERT( isValidIndex( index ) );
		return ms_table[ index ];
	}
	mxFORCEINLINE static UINT GetTotalCount()
	{
		return ms_table.num();
	}
	mxFORCEINLINE static UINT GetMaxCapacity()
	{
		return MAX;
	}
	mxFORCEINLINE static bool isValidIndex( UINT index )
	{
		return index >= 0 && index < GetTotalCount();
	}

	mxFORCEINLINE static TYPE** GetPtrArray()
	{
		return ms_table.raw();
	}

protected:

	inline TObjectPtrTable()
	{
		mxASSERT( ms_table.num() < GetMaxCapacity() );

		const UINT oldNum = ms_table.num();
		TYPE* pThis = static_cast< TYPE* >( this );

		m_uniqueIndex = oldNum;

		ms_table.add( pThis );
	}

	inline ~TObjectPtrTable()
	{
		mxASSERT( m_uniqueIndex != INDEX_NONE );
		ms_table[ m_uniqueIndex ] = nil;
		m_uniqueIndex = INDEX_NONE;
	}

private:
	UINT		m_uniqueIndex;

	static TFixedArray< TYPE*, MAX >	ms_table;
};

template< typename TYPE, UINT MAX >
TFixedArray< TYPE*, MAX >		TObjectPtrTable< TYPE, MAX >::ms_table;



//----------------------------------------------------------//


//
//	TSortedPtrTable - a static table of pointers to 'TYPE' instances.
//
//	May be used for accessing objects by indices.
//	Relative ordering of array elements can carry some useful information.
//
template< typename TYPE, UINT MAX >
struct TSortedPtrTable
{
	mxFORCEINLINE UINT GetSortedIndex() const
	{
		mxASSERT(mIsSorted);
		return mSortedIndex;
	}

public:
	mxFORCEINLINE static TYPE* GetPtrBySortedIndex( UINT index )
	{
		mxASSERT(mIsSorted);
		return mSortedPtrArray[ index ];
	}
	mxFORCEINLINE static bool IsTableSorted()
	{
		return mIsSorted;
	}
	static void SetUnsorted()
	{
		mIsSorted = FALSE;
	}

	template< typename U >	// where U : TYPE and U has operator < (const U&, const U&)
	static void SortIndices( U** source, UINT num )
	{
		if( num > 1 )
		{
			memcpy( mSortedPtrArray.raw(), source, num * sizeof(source[0]) );

			U ** pStart = c_cast(U**) mSortedPtrArray.raw();
			U ** pEnd = pStart + num;

			NxQuickSort< U*, U >( pStart, pEnd );

			for( UINT i=0; i<num; i++ )
			{
				TYPE* p = mSortedPtrArray[i];
				p->mSortedIndex = i;
			}
		}

		mIsSorted = true;
	}

protected:

	inline TSortedPtrTable()
	{
		mSortedIndex = 0;
	}

	inline ~TSortedPtrTable()
	{
	}

private:

	UINT	mSortedIndex;

	static BOOL mIsSorted;

	static TStaticArray_InitZeroed< TYPE*, MAX >	mSortedPtrArray;
};

template< typename TYPE, UINT MAX >
BOOL TSortedPtrTable< TYPE, MAX >::mIsSorted = false;

template< typename TYPE, UINT MAX >
TStaticArray_InitZeroed< TYPE*, MAX >	TSortedPtrTable< TYPE, MAX >::mSortedPtrArray;

//----------------------------------------------------------//

//
//	TSingleton_LazyInit< TYPE >
//
template< typename TYPE >
class TSingleton_LazyInit {
public:
	static TYPE & Get()
	{
		ConstructInstance();
		return (*gInstance);
	}
	static TYPE* GetInstance()
	{
		ConstructInstance();
		return gInstance;
	}
	static bool HasInstance()
	{
		return (gInstance != nil);
	}

	virtual ~TSingleton_LazyInit()
	{
		mxASSERT(HasInstance());
		gInstance = nil;
	}

protected:
	TSingleton_LazyInit()
	{
		mxASSERT2(!HasInstance(),"Singleton class has already been instantiated.");
		gInstance = static_cast< TYPE* >( this );
	}

private:
	typedef TSingleton_LazyInit<TYPE> THIS_TYPE;
	PREVENT_COPY( THIS_TYPE );

private:
	static void ConstructInstance()
	{
		if(!gInstance)
		{
			gInstance = new_one(TYPE());
		}
	}

private:
	mxTHREAD_LOCAL static TYPE * gInstance;
};

template< typename TYPE >
mxTHREAD_LOCAL TYPE * TSingleton_LazyInit< TYPE >::gInstance = nil;

//----------------------------------------------------------//

//
//	TIndirectObjectTable - a large static table of pointers to 'TYPE' instances.
//
//	May be useful when accessing objects by indices.
//	Even relative ordering of elements can carry some useful information.
//	Usually the first element (with zero index) is the one used by default.
//
template< typename TYPE, UINT MAX = 1024 >
struct TIndirectObjectTable
{
public:
	mxFORCEINLINE UINT GetUniqueIndex() const
	{
		return mUniqueIndex;
	}

public:
	mxFORCEINLINE static TYPE* GetPtrByIndex( UINT index )
	{
//		mxASSERT_PTR( mTable );
		mxASSERT( index >= 0 && index < mStaticCounter );
		return mTable[ index ];
	}
	mxFORCEINLINE static UINT GetTotalCount()
	{
		return mStaticCounter;
	}
	mxFORCEINLINE static UINT GetMaxCapacity()
	{
		return MAX;
	}

public_internal:

	// table should be large enough to hold MAX pointers to instances of TYPE
	static void InitTable( TYPE** table )
	{
		mxASSERT(nil == mTable);
		mTable = table;
	}

	// deletes each entry
	static void Purge()
	{
		if( !mTable ) {
			return;
		}
		for( UINT iObj = 0; iObj < GetMaxCapacity(); iObj++ )
		{
			if( mTable[iObj] ) {
				free_one( mTable[iObj] );
			}
		}
	}

protected:
	inline TIndirectObjectTable()
		: mUniqueIndex( mStaticCounter++ )
	{
		mxASSERT( mStaticCounter < GetMaxCapacity() );
		mTable[ mUniqueIndex ] = static_cast< TYPE* >( this );
	}
	virtual ~TIndirectObjectTable()
	{
		mTable[ mUniqueIndex ] = nil;
	}

protected://private:
	/*const*/ UINT		mUniqueIndex;
	static UINT		mStaticCounter;	// always increases
	static TYPE **	mTable;
};

template< typename TYPE, UINT MAX >
UINT TIndirectObjectTable< TYPE, MAX >::mStaticCounter = 0;

template< typename TYPE, UINT MAX >
TYPE** TIndirectObjectTable< TYPE, MAX >::mTable = nil;

// stolen from ANGLE
template< typename HANDLE_TYPE = UINT16 >
class HandleAllocator: NonCopyable
{
	HANDLE_TYPE				m_baseValue;
	HANDLE_TYPE				m_nextValue;
	TArray< HANDLE_TYPE >	m_freeValues;

public:
	HandleAllocator()
	{
		m_baseValue = 1;
		m_nextValue = 1;
	}
	~HandleAllocator()
	{
	}

	void SetBaseHandle( HANDLE_TYPE value )
	{
		mxASSERT(m_baseValue == m_nextValue);
		m_baseValue = value;
		m_nextValue = value;
	}
	HANDLE_TYPE Allocate()
	{
		if( m_freeValues.num() )
		{
			GLuint handle = m_freeValues.getLast();
			m_freeValues.PopLast();
			return handle;
		}
		return m_nextValue++;
	}
	void Release( HANDLE_TYPE handle )
	{
		if( handle == m_nextValue - 1 )
		{
			// Don't drop below base value
			if( m_nextValue > m_baseValue )
			{
				m_nextValue--;
			}
		}
		else
		{
			// Only free handles that we own - don't drop below the base value
			if( handle >= m_baseValue )
			{
				m_freeValues.add( handle );
			}
		}
	}
};


/** 
 * C++ utilities for variable-length integer encoding
 *
 * Licensed under the public domain
 * Originally published on http://techoverflow.net
 * Copyright (c) 2015 Uli Koehler
 */
#ifndef __VARIABLE_LENGTH_INTEGER_H
#define __VARIABLE_LENGTH_INTEGER_H

/**
 * Encodes an unsigned variable-length integer using the MSB algorithm.
 * @param value The input value. Any standard integer type is allowed.
 * @param output A pointer to a piece of reserved memory. Must have a minimum size dependent on the input size (32 bit = 5 bytes, 64 bit = 10 bytes).
 * @param outputSizePtr A pointer to a single integer that is set to the number of bytes used in the output memory.
 */
template<typename int_t = uint64_t>
void encodeVarint(int_t value, uint8_t* output, uint8_t* outputSizePtr) {
    uint8_t outputSize = 0;
    //While more than 7 bits of data are left, occupy the last output byte
    // and set the next byte flag
    while (value > 127) {
        //|128: Set the next byte flag
        output[outputSize] = ((uint8_t)(value & 127)) | 128;
        //Remove the seven bits we just wrote
        value >>= 7;
        outputSize++;
    }
    output[outputSize++] = ((uint8_t)value) & 127;
    *outputSizePtr = outputSize;
}

/**
 * Decodes an unsigned variable-length integer using the MSB algorithm.
 * @param value A variable-length encoded integer of arbitrary size.
 * @param inputSize How many bytes are 
 */
template<typename int_t = uint64_t>
int_t decodeVarint(uint8_t* input, uint8_t inputSize) {
    int_t ret = 0;
    for (uint8_t i = 0; i < inputSize; i++) {
        ret |= (input[i] & 127) << (7 * i);
        //If the next-byte flag is set
        if(!(input[i] & 128)) {
            break;
        }
    }
    return ret;
}
#endif //__VARIABLE_LENGTH_INTEGER_H

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
