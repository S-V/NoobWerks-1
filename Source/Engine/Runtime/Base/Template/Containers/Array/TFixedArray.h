/*
=============================================================================
	File:	TFixedArray.h
	Desc:	A non-growing, memset-able, one dimensional fixed-size
			array template using no memory allocation (cannot be resized).
	Note:	This array can never grow and reallocate memory
			so it should be safe to store pointers to its contents.
			Nice thing is that it doesn't move/copy its contents around.
	ToDo:	Stop reinventing the wheel.
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/ArraysCommon.h>

template< typename TYPE, const U32 SIZE >
class TFixedArray
	: public TArrayMixin< TYPE, TFixedArray< TYPE, SIZE > >
{
public_internal:
	/// The number of elements, this should be inside the range [0..SIZE).
	U32		_count;

	TYPE	_data[ SIZE ];

public:
	typedef TFixedArray<TYPE,SIZE> THIS_TYPE;

	typedef
	TYPE
	ITEM_TYPE;

	static const U32 CAPACITY = SIZE;

public:
	inline TFixedArray()
	{
		_count = 0;
	}
	inline explicit TFixedArray(EInitZero)
	{
		_count = 0;
		mxZERO_OUT(_data);
	}
	inline explicit TFixedArray( const THIS_TYPE& other )
	{
		_count = other._count;
		TCopyConstructArray( _data, other._data, other._count );
	}

	// Returns the number of elements currently contained in the list.
	inline U32 num() const
	{
		return _count;
	}

	inline ERet setNum( U32 newNum )
	{
		mxASSERT(newNum <= Max());
		_count = newNum;
		return ALL_OK;
	}

	// Returns the maximum number of elements in the list.
	inline U32 Max() const
	{
		return SIZE;
	}
	inline U32 capacity() const
	{
		return SIZE;
	}

	inline void setEmpty()
	{
		_count = 0;
	}
	inline void clear()
	{
		_count = 0;
	}

	inline bool isFull() const
	{
		return _count == SIZE;
	}

	// Returns a pointer to the beginning of the array.  Useful for iterating through the list in loops.
	inline TYPE * raw()
	{
		return _data;
	}
	inline const TYPE* raw() const
	{
		return _data;
	}

	// Implicit type conversion - intentionally disabled:
	//inline operator TYPE* ()
	//{ return _data; }
	//inline operator const TYPE* () const
	//{ return _data; }

	inline ERet add( const TYPE& newItem )
	{
		mxENSURE( _count < SIZE, ERR_BUFFER_TOO_SMALL, "" );
		_data[ _count++ ] = newItem;
		return ALL_OK;
	}
	inline TYPE & add()
	{
		mxASSERT( _count < SIZE );
		return _data[ _count++ ];
	}

	// Slow!
	bool AddUnique( const TYPE& item )
	{
		for( U32 i = 0; i < _count; i++ )
		{
			if( _data[i] == item ) {
				return false;
			}
		}
		add( item );
		return true;
	}

	// Slow!
	inline bool removeFirst( const TYPE& item )
	{
		const U32 index = this->findIndexOf( item );
		if( INDEX_NONE == index ) {
			return false;
		}
		this->RemoveAt( index );
		return true;
	}

	// Slow!
	inline void RemoveAt( U32 index )
	{
		mxASSERT( isValidIndex( index ) );
		//TDestructOne_IfNonPOD( &_data[index] );
		--_count;
		for( U32 i = index; i < _count; i++ )
		{
			_data[ i ] = _data[ i + 1 ];
		}
	}

	// Doesn't preserve the relative order of elements.
	inline void RemoveAt_Fast( U32 index )
	{
		mxASSERT( isValidIndex( index ) );
		//TDestructOne_IfNonPOD( &_data[index] );
		--_count;
		if ( index != _count ) {
			_data[ index ] = _data[ _count ];
		}
	}

	// deletes the last element
	inline TYPE Pop()
	{
		mxASSERT(_count > 0);
		--_count;
		TYPE result = _data[ _count ];
		// remove and destroy the last element
		TDestructOne_IfNonPOD( _data[ _count ] );
		return result;
	}

public:	// Unsafe Utilities.

	/// If the array is full, overwrites the first item.
	mxFORCEINLINE void AddWithWraparoundOnOverflow( const TYPE& new_item )
	{
		const U32 old_count = _count;
		const U32 new_item_index = old_count % CAPACITY;
		_count = smallest( old_count + 1, CAPACITY );
		_data[ new_item_index ] = new_item;
	}

	/// If the array is full, overwrites the first item and returns true.
	mxFORCEINLINE bool AddWithWraparoundOnOverflowEx(
		const TYPE& new_item
		, TYPE *overwritten_item_ = nil
		)
	{
		const U32 old_count = _count;
		const bool will_overwrite_old_item = (old_count >= CAPACITY);

		const U32 new_item_index = old_count % CAPACITY;
		_count = smallest( old_count + 1, CAPACITY );

		if( will_overwrite_old_item && overwritten_item_ ) {
			*overwritten_item_ = _data[ new_item_index ];
		}
		_data[ new_item_index ] = new_item;

		return will_overwrite_old_item;
	}

	// Use it only if you know what you're doing.
	// This only works if array capacity is a power of two.
	inline TYPE & addFastUnsafe()
	{
		mxSTATIC_ASSERT( TIsPowerOfTwo< SIZE >::value );
		mxASSERT( _count < SIZE );
		const U32 newIndex = (_count++) & (SIZE-1);//avoid checking for overflow
		return _data[ newIndex ];
	}
	// only valid for lists with power-of-two capacities
	inline void addFastUnsafe( const TYPE& newItem )
	{
		mxSTATIC_ASSERT( TIsPowerOfTwo< SIZE >::value );
		mxASSERT( _count < SIZE );
		const U32 newIndex = (_count++) & (SIZE-1);//avoid checking for overflow
		_data[ newIndex ] = newItem;
	}

	inline void Prepend( const TYPE& newItem )
	{
		this->Insert( newItem, 0 );
	}

	// Insert the element at the given index.
	// Returns the index of the new element.
	inline U32 Insert( const TYPE& newItem, U32 index )
	{
		mxASSERT(!this->isFull());
		mxASSERT(index < this->capacity());
		index = smallest(index, _count);
		for( U32 i = _count; i > index; i-- )
		{
			_data[ i ] = _data[ i - 1 ];
		}
		++_count;
		_data[ index ] = newItem;
		return index;
	}

public:
	// Returns the total amount of occupied memory in bytes.
	inline U32 usedMemorySize() const
	{
		return sizeof(*this);
	}

#if 0
	friend AWriter& operator << ( AWriter& file, const THIS_TYPE &o )
	{
		file << o.mNum;
		file.SerializeArray( o.mData, o.mNum );
		return file;
	}
	friend AReader& operator >> ( AReader& file, THIS_TYPE &o )
	{
		file >> o.mNum;
		mxASSERT( o.mNum <= SIZE );
		file.SerializeArray( o.mData, o.mNum );
		return file;
	}
	friend mxArchive& operator && ( mxArchive & archive, THIS_TYPE & o )
	{
		archive && o.mNum;
		TSerializeArray( archive, o.mData, o.mNum );
		return archive;
	}
#endif

public:
	class OStream: public AWriter
	{
		THIS_TYPE &	mArray;
	public:
		OStream( THIS_TYPE & dest )
			: mArray( dest )
		{}
		virtual ERet Write( const void* pBuffer, size_t numBytes ) override
		{
			const U32 oldSize = mArray.num();
			const U32 newSize = oldSize + numBytes;
			mArray.setNum( newSize );
			memcpy( mArray.raw() + oldSize, pBuffer, numBytes );
			return (newSize == oldSize) ? ALL_OK : ERR_FAILED_TO_WRITE_FILE;
		}
	};
	inline OStream GetOStream()
	{
		return OStream( *this );
	}

public_internal:

	class Descriptor: public MetaArray
	{
	public:
		inline Descriptor( const char* typeName )
			: MetaArray( typeName, TbTypeSizeInfo::For_Type<THIS_TYPE>(), T_DeduceTypeInfo<ITEM_TYPE>(), sizeof(ITEM_TYPE) )
		{}
		//=-- MetaArray
		virtual bool IsDynamic() const override
		{
			return false;
		}
		virtual void* Get_Array_Pointer_Address( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return c_cast(void*) &theArray->mData;
		}
		virtual U32 Generic_Get_Count( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->num();
		}
		virtual ERet Generic_Set_Count( void* pArrayObject, U32 newNum ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			theArray->setNum( newNum );
			return ALL_OK;
		}
		virtual U32 Generic_Get_Capacity( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->capacity();
		}
		virtual ERet Generic_Set_Capacity( void* pArrayObject, U32 newNum ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			mxASSERT(newNum <= theArray->capacity());
			return ALL_OK;
		}
		virtual void* Generic_Get_Data( void* pArrayObject ) const override
		{
			THIS_TYPE* theArray = static_cast< THIS_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual const void* Generic_Get_Data( const void* pArrayObject ) const override
		{
			const THIS_TYPE* theArray = static_cast< const THIS_TYPE* >( pArrayObject );
			return theArray->raw();
		}
		virtual void SetDontFreeMemory( void* pArrayObject ) const override
		{
			mxUNUSED(pArrayObject);
		}
	};

	/// For serialization, we want to initialize the vtables
	/// in classes post data load, and NOT call the default constructor
	/// for the arrays (as the data has already been set).
	inline explicit TFixedArray( _FinishedLoadingFlag )
	{
	}

public:

	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef TYPE* iterator;
	typedef const TYPE* const_iterator;
	typedef TYPE& reference;
	typedef const TYPE& const_reference;

private:
	NO_COMPARES(TFixedArray);
};

template< typename TYPE, const U32 SIZE >
struct TypeDeducer< TFixedArray< TYPE, SIZE > >
{
	static inline const TbMetaType& GetType()
	{
		static TFixedArray< TYPE, SIZE >::Descriptor typeInfo(mxEXTRACT_TYPE_NAME(TFixedArray));
		return typeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Array;
	}
};
