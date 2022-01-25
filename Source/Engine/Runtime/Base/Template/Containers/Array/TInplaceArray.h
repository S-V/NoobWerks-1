/*
=============================================================================
	TInplaceArray - A dynamic array class with some inline storage.

	It behaves similarly to TArray, except until
	a certain number of elements are reserved it does not use the heap.

	Like standard vector, it is guaranteed to use contiguous memory.
	(So, after it spills to the heap all the elements live in the heap buffer.)

	see:
	https://github.com/facebook/folly/blob/master/folly/docs/small_vector.md
	hkLocalArray, Ubisoft's InplaceArray
=============================================================================
*/
#pragma once

// std::aligned_storage
#include <type_traits>

#include <Base/Object/Reflection/ArrayDescriptor.h>

/// contains an embedded buffer to avoid initial memory allocations
template< typename TYPE, UINT32 RESERVED >
class TInplaceArray: public TArray< TYPE >
{
	//TODO: move
	typedef typename std::aligned_storage
	<
		sizeof(TYPE),
		std::alignment_of<TYPE>::value
	>::type
	StorageType;

	StorageType	m__embeddedStorage[RESERVED];	// uninitialized storage for N objects of TYPE

public:
	TInplaceArray()
	{
		this->initializeWithExternalStorageAndCount(
			(TYPE*)m__embeddedStorage, mxCOUNT_OF(m__embeddedStorage)
			);
	}
	TInplaceArray( const TInplaceArray& _other )
	{
		this->initializeWithExternalStorageAndCount(
			(TYPE*)m__embeddedStorage, mxCOUNT_OF(m__embeddedStorage)
			);
		this->Copy(_other);
	}

	void operator = ( const TInplaceArray& _other )
	{
		this->Copy( _other );
	}

	NO_COMPARES( TInplaceArray );

public:

	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef TYPE* iterator;
	typedef const TYPE* const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;

public:
	typedef TYPE	ITEM_TYPE;
};

template< typename TYPE, UINT32 RESERVED >
struct TypeDeducer< TInplaceArray< TYPE, RESERVED > >
{
	static inline const TbMetaType& GetType()
	{
		static TInplaceArray< TYPE, RESERVED >::ArrayDescriptor
					staticTypeInfo(mxEXTRACT_TYPE_NAME(TArray));
		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};
