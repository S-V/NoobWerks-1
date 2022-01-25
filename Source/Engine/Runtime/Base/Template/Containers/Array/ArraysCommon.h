/*
=============================================================================
	File:	ArraysCommon.h
	Desc:	
=============================================================================
*/
#pragma once

#include <Base/Template/Templates.h>

template< typename TYPE >
struct TSpan;

/*
-------------------------------------------------------------------------
	TArrayBase< TYPE, DERIVED >

	this mixin implements common array functions,
	it must not contain any member variables.

	the derived class must have the following functions
	(you'll get a compile-time error or stack overflow if it does't):

	'UINT num() const'- returns number of elements in the array
	'TYPE* raw()' - returns a pointer to the contents
	'const TYPE* raw() const' - returns a pointer to the contents

	@todo: check if it causes any code bloat
-------------------------------------------------------------------------
*/
template
<
	typename ItemType,
	class DerivedClass	// where DerivedClass : TArrayMixin< ItemType, DerivedClass >
>
struct TArrayMixin
{
	typedef ItemType ItemType;

private:
	mxFORCEINLINE DerivedClass* asDerived()
	{
		return static_cast< DerivedClass* >( this );
	}
	mxFORCEINLINE const DerivedClass* asDerived() const
	{
		return static_cast< const DerivedClass* >( this );
	}

/// Returns the size (the number of elements in the array).
#define NUM()	((static_cast< const DerivedClass* >(this))->num())
/// Returns a raw pointer to the array.
#define RAW()	((const_cast< DerivedClass* >(static_cast< const DerivedClass* >(this)))->raw())

public:	//

	/// Convenience function to get the number of elements in this array.
	mxFORCEINLINE U32 num() const { return NUM(); }

	mxFORCEINLINE ItemType * raw()				{ return RAW(); }
	mxFORCEINLINE const ItemType* raw() const	{ return RAW(); }

public:

	/// Checks if the size is zero.
	mxFORCEINLINE bool IsEmpty() const
	{
		return !NUM();
	}

	mxFORCEINLINE bool nonEmpty() const
	{
		return NUM();
	}

	/// Returns the size of a single element, in bytes.
	mxFORCEINLINE U32 stride() const
	{
		return sizeof(ItemType);
	}

	/// Returns the total size of stored elements, in bytes.
	mxFORCEINLINE size_t rawSize() const
	{
		return NUM() * sizeof(ItemType);
	}

	/// Returns true if the index is within the boundaries of this array.
	mxFORCEINLINE bool isValidIndex( U32 index ) const
	{
		return index < NUM();
	}

public:	// Operator overloads.

	/// Read only access to the i'th element.
	mxFORCEINLINE const ItemType& operator [] ( U32 i ) const
	{
		mxASSERT2(
			this->isValidIndex( i ),
			"Array index '%u' out of range (%u)", i, NUM()
			);
		return RAW()[ i ];
	}

	/// Read/write access to the i'th element.
	mxFORCEINLINE ItemType & operator [] ( U32 i )
	{
		mxASSERT2(
			this->isValidIndex( i ),
			"Array index '%u' out of range (%u)", i, NUM()
			);
		return RAW()[ i ];
	}

	/// Range slicing.

	mxFORCEINLINE operator TSpan< ItemType >()				{ return TSpan< ItemType >( RAW(), NUM() ); }
	mxFORCEINLINE operator TSpan< const ItemType >() const	{ return TSpan< const ItemType >( RAW(), NUM() ); }

public:

	mxFORCEINLINE ItemType & getFirst()
	{
		mxASSERT(NUM()>0);
		return RAW()[ 0 ];
	}
	mxFORCEINLINE const ItemType& getFirst() const
	{
		mxASSERT(NUM()>0);
		return RAW()[ 0 ];
	}

	mxFORCEINLINE ItemType & getLast()
	{
		mxASSERT(NUM()>0);
		return RAW()[ NUM()-1 ];
	}
	mxFORCEINLINE const ItemType& getLast() const
	{
		mxASSERT(NUM()>0);
		return RAW()[ NUM()-1 ];
	}

	mxFORCEINLINE ItemType* getFirstItemPtrOrNil()
	{
		return ( NUM() > 0 ) ? RAW() : nil;
	}
	mxFORCEINLINE const ItemType* getFirstItemPtrOrNil() const
	{
		return ( NUM() > 0 ) ? RAW() : nil;
	}

	/// Returns the index of the first occurrence of the given element, or INDEX_NONE if not found.
	/// Slow! (uses linear search)
	U32 findIndexOf( const ItemType& element ) const
	{
		const U32 num = NUM();
		const ItemType* data = RAW();
		for( U32 i = 0; i < num; i++ ) {
			if( data[ i ] == element ) {
				return i;
			}
		}
		return INDEX_NONE;
	}

	bool contains( const ItemType& element ) const
	{
		const U32 num = NUM();
		const ItemType* data = RAW();
		for( U32 i = 0; i < num; i++ ) {
			if( data[ i ] == element ) {
				return true;
			}
		}
		return false;
	}

public:	// Standard C++ library stuff (e.g. for compatibility with <algorithm>).

    mxFORCEINLINE const ItemType *	begin() const	{ return RAW(); }
	mxFORCEINLINE ItemType *		begin()			{ return RAW(); }
    mxFORCEINLINE const ItemType *	end() const		{ return RAW() + NUM(); }
	mxFORCEINLINE ItemType *		end()			{ return RAW() + NUM(); }

//
#undef NUM
#undef RAW

};



struct TSpan_Untyped
{
	void *	_data;
	U32		_count;
};

/*
==========================================================
Represents a non-owning "slice", "range", "Range" or "view" of contiguous data,
i.e. a start pointer and a length.
It allows various APIs to take consecutive elements easily and conveniently.

This class does not own the underlying data, it is expected to be used in
situations where the data resides in some other buffer, whose lifetime
extends past that of the TSpan. For this reason, it is not in general
safe to store an TSpan.

This is intended to be trivially copyable, so it should be passed by
value.

https://en.cppreference.com/w/cpp/container/span

span: the best span
https://brevzin.github.io/c++/2018/12/03/span-best-span/

https://www.boost.org/doc/libs/1_68_0/libs/range/doc/html/range/reference/adaptors/reference/sliced.html
==========================================================
*/

template< typename ItemType >
struct TSpan: public TArrayMixin< ItemType, TSpan<ItemType> >
{
	/// The start of the array, in an external buffer.
	ItemType * 	_data;

	/// The number of elements.
	U32			_count;

public:
	mxFORCEINLINE TSpan()
		: _data( nil ), _count( 0 )
	{dbg_check_size();}

	mxFORCEINLINE TSpan( const TSpan& other )
		: _data( other._data ), _count( other._count )
	{dbg_check_size();}

	mxFORCEINLINE TSpan( ItemType* items, U32 count )
		: _data( items ), _count( count )
	{dbg_check_size();}

public:	// TArrayBase interface

	mxFORCEINLINE ItemType* raw()				{ return _data; }
	mxFORCEINLINE const ItemType* raw() const	{ return _data; }
	mxFORCEINLINE U32 num() const				{ return _count; }

public:	// Standard C++ library stuff (e.g. for compatibility with <algorithm>).

    mxFORCEINLINE const ItemType *	begin() const	{ return _data; }
	mxFORCEINLINE ItemType *		begin()			{ return _data; }
    mxFORCEINLINE const ItemType *	end() const		{ return _data + _count; }
	mxFORCEINLINE ItemType *		end()			{ return _data + _count; }

public:	// These are needed for STL iterators and Boost foreach (BOOST_FOREACH):

	typedef ItemType *		iterator;
	typedef const ItemType*	const_iterator;
	typedef ItemType &		reference;
	typedef const ItemType&	const_reference;

private:
	NO_COMPARES(TSpan);

	void dbg_check_size()
	{
		ASSERT_SIZEOF( TSpan< ItemType >, sizeof(TSpan_Untyped) );
	}
};

typedef TSpan< BYTE >	ByteSpanT;


/*
==========================================================
	ARRAY INDEXING
==========================================================
*/
namespace Arrays
{
	template< class ARRAY >
	mxFORCEINLINE U32 GetStoredItemIndex( const ARRAY& a, const typename ARRAY::ItemType* o )
	{
		const ARRAY::ItemType* start = a.raw();
		const U32 index = o - start;
		mxASSERT2( a.isValidIndex( index ), "Invalid index of '%xp': '%u'", o, index );
		// @fixme: ideally, it should be:
		// (ptrdiff_t)((size_t)o - (size_t)start) / sizeof(TYPE);
		return index;
	}
}//namespace Arrays

/*
==========================================================
	ARRAY QUERIES
==========================================================
*/
namespace Arrays
{
	template< typename T >
	const T* Find_LinearSearch(
		const T* values
		, const UINT count
		, const T& searched_value
		)
	{
		for( UINT i = 0; i < count; i++ ) {
			if( values[ i ] == searched_value ) {
				return &values[ i ];
			}
		}
		return nil;	// Not found
	}

	template< typename T >
	UINT FindIndex_LinearSearch(
		const T* values
		, const UINT count
		, const T& searched_value
		)
	{
		for( UINT i = 0; i < count; i++ ) {
			if( values[ i ] == searched_value ) {
				return i;
			}
		}
		return INDEX_NONE;	// Not found
	}

	/// Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.
	/// NOTE: This function can only be called on lists containing pointers. Calling it
	/// on non-pointer lists will cause a compiler error.
	template< class ARRAY >
	UINT FindNull( const ARRAY& a )
	{
		const UINT array_count = a.num();
		const ARRAY::ItemType* items = a.raw();
		for( UINT i = 0; i < array_count; i++ ) {
			if( items[ i ] == nil ) {
				return i;
			}
		}
		return INDEX_NONE;	// Not found
	}

	/// Removes the element at the specified index
	/// and moves all data following the element down to fill in the gap.
	template< class ARRAY >
	void RemoveAndShift( ARRAY & _array, U32 _startIndex, U32 _itemsToRemove = 1 )
	{
		const U32 array_count = _array.num();
		ARRAY::ItemType* items = _array.raw();
		mxASSERT( _startIndex + _itemsToRemove <= array_count );
		for( U32 i = _startIndex; i < array_count - _itemsToRemove; i++ )
		{
			items[i] = items[ i + _itemsToRemove ];
		}
		_array.setNum( array_count - _itemsToRemove );
	}




	template< class ITEM >
	bool IsSortedInDecreasingOrder_AllowDuplicates( const ITEM* items, UINT count )
	{
		for( U32 i = 1; i < count; i++ ) {
			if( items[ i-1 ] < items[ i ] ) {
				return false;
			}
		}
		return true;
	}

	template< class ITEM >
	bool CheckIfIsSortedDecreasing( const ITEM* items, UINT count )
	{
		for( U32 i = 1; i < count; i++ ) {
			if( items[ i-1 ] <= items[ i ] ) {
				return false;
			}
		}
		return true;
	}

	template< class ARRAY >
	bool CheckIfIsSortedDecreasing( const ARRAY& a )
	{
		return CheckIfIsSortedDecreasing( a.raw(), a.num() );
	}




	template< class ITEM >
	bool CheckIfIsSortedIncreasing_AllowDuplicates( const ITEM* items, UINT count )
	{
		for( U32 i = 1; i < count; i++ ) {
			if( items[ i-1 ] > items[ i ] ) {
				return false;
			}
		}
		return true;
	}

	template< class ITEM >
	bool CheckIfIsSortedIncreasing( const ITEM* items, UINT count )
	{
		for( U32 i = 1; i < count; i++ ) {
			if( items[ i-1 ] >= items[ i ] ) {
				return false;
			}
		}
		return true;
	}

	template< class ARRAY >
	bool CheckIfIsSortedIncreasing( const ARRAY& a )
	{
		return CheckIfIsSortedIncreasing( a.raw(), a.num() );
	}




	/// checks if the sorted array contains duplicate items
	template< class ITEM >
	U32 CheckIfSortedArrayHasDupes( const ITEM* _items, UINT _count )
	{
		ITEM previous = _items[0];
		for( U32 i = 1; i < _count; i++ )
		{
			const ITEM& current = _items[ i ];
			if( current == previous ) {
				return true;
			}
			previous = current;
		}
		return false;
	}

	template< class ARRAY >
	U32 CheckIfSortedArrayHasDupes( const ARRAY& a )
	{
		return CheckIfSortedArrayHasDupes( a.raw(), a.num() );
	}

	/// Removes the element at the specified index
	/// and moves all data following the element down to fill in the gap.
	/// Returns the number of unique items.
	template< typename ItemType >
	U32 RemoveDupesFromSortedArray( ItemType* items, const UINT count )
	{
		U32 num_unique_items = 1;
		ItemType previous = items[0];
		for( U32 i = 1; i < count; i++ )
		{
			const ItemType current = items[ i ];
			if( current != previous ) {
				items[ num_unique_items++ ] = current;
				previous = current;
			}
		}
		return num_unique_items;
	}

	template< class ARRAY >
	U32 RemoveDupesFromSortedArray( ARRAY & _array )
	{
		const U32 array_count = _array.num();
		mxASSERT(array_count > 0);
		ARRAY::ItemType * items = _array.raw();

		const U32 num_unique_items = RemoveDupesFromSortedArray( items, array_count );
		//const U32 numDuplicates = array_count - num_unique_items;
		_array.setNum( num_unique_items );
		return num_unique_items;
	}

	template< typename ItemType >
	UINT CountUniqueItemsInSortedArray( ItemType* items, const UINT count )
	{
		UINT num_unique_items = 1;
		ItemType previous = items[0];
		for( U32 i = 1; i < count; i++ )
		{
			const ItemType current = items[ i ];
			if( current != previous ) {
				++num_unique_items;
				previous = current;
			}
		}
		return num_unique_items;
	}

}//namespace Arrays

/*
==========================================================
	ARRAY MODIFICATION
==========================================================
*/
namespace Arrays
{
	/// Ensures that there's a space for at least the given number of elements.
	template< class ARRAY >
	ERet reserveMore( ARRAY & a, U32 item_count )
	{
		return a.reserve( a.num() + item_count );
	}

	/// Set all items of this array to the given value.
	template< class ARRAY >
	void setAll( ARRAY & a, const typename ARRAY::ItemType& new_value )
	{
		const U32 count = a.num();
		ARRAY::ItemType* items = a.raw();

		for( U32 i = 0; i < count; ++i ) {
			items[ i ] = new_value;
		}
	}

	template< class ARRAY >
	void zeroOut( ARRAY & a )
	{
		MemZero( a.raw(), a.rawSize() );
	}

	template< class ARRAY, typename TYPE >
	ERet setFrom( ARRAY & array_, const TYPE* source, UINT count )
	{
		mxDO(array_.setNum( count ));
		TCopyArray( array_.raw(), source, count );
		return ALL_OK;
	}

	template< class ARRAY, typename TYPE, size_t N >
	ERet setFrom( ARRAY & array_, const TYPE (&source)[N] )
	{
		mxDO(array_.setNum( N ));
		TCopyArray( array_, source, N );
		return ALL_OK;
	}

	template< class ARRAY_A, class ARRAY_B >
	ERet Copy( ARRAY_A & destination_, const ARRAY_B& source )
	{
		const UINT item_count = source.num();
		mxDO(destination_.setNum( item_count ));
		TCopyArray( destination_.raw(), source.raw(), item_count );
		return ALL_OK;
	}

	template< class ARRAY, typename TYPE >
	ERet append( ARRAY & destination_
		, const TYPE items_to_add, const UINT num_items_to_add )
	{
		const UINT old_item_count = destination_.num();
		const UINT new_item_count = old_item_count + num_items_to_add;
		mxDO(destination_.setNum( new_item_count ));
		TCopyArray( destination_.raw() + old_item_count, items_to_add, num_items_to_add );
		return ALL_OK;
	}

	template< class ARRAY >
	ERet append( ARRAY & destination_, const ARRAY& source )
	{
		return append( destination_, source.raw(), source.num() );
	}

	/// NOTE: extremely slow! O(N^2)
	template< class ARRAY >
	ERet appendUniqueItems( ARRAY & destination_, const ARRAY& source )
	{
		for( UINT i = 0; i < source.num(); i++ )
		{
			if( !destination_.contains( other[i] ) )
			{
				mxDO(destination_.add( other[i] ));
			}
		}
		return ALL_OK;
	}

	/// assumes that the array hasn't allocated any memory
	template< class ARRAY, typename T, size_t N >
	void initializeWithExternalStorage( ARRAY & a, TStorage< T, N >& storage )
	{
		a.initializeWithExternalStorageAndCount( storage.raw(), storage.num() );
	}

	/// assumes that the array hasn't allocated any memory
	template< class ARRAY, typename POD_TYPE, size_t N >
	void initializeWithExternalStorage( ARRAY & a, POD_TYPE (&storage_buffer)[N] )
	{
		a.initializeWithExternalStorageAndCount( storage_buffer, N );
	}

	/// assumes that the array hasn't allocated any memory
	template< class ARRAY, typename POD_TYPE, size_t N >
	void initializeWithExternalStorageAndCount( ARRAY & a, POD_TYPE (&storage_buffer)[N] )
	{
		a.initializeWithExternalStorageAndCount( storage_buffer, N, N );
	}

	/// assuming that the stored elements are pointers,
	/// deletes them.
	/// NOTE: don't forget to empty the array afterwards, if needed.
	///
	template< class ARRAY >
	void deleteContents( ARRAY & a )
	{
		const U32 count = a.num();
		ARRAY::ItemType* items = a.raw();

		for( U32 i = 0; i < count; i++ ) {
			delete items[i];
		}
	}

	/// Invokes objects' destructors.
	template< class ARRAY >
	void destroyContents( ARRAY & a )
	{
		const U32 count = a.num();
		ARRAY::ItemType* items = a.raw();

		TDestructN_IfNonPOD( items, count );
	}

}//namespace Arrays

/*
==========================================================
	ARRAY ITERATORS
==========================================================
*/
namespace Arrays
{
	///
	template< class ARRAY >
	class Iterator
	{
		ARRAY &	_array;
		U32		_current_index;
		typedef typename ARRAY::ItemType TYPE;
	public:
		Iterator( ARRAY& rArray )
			: _array( rArray )
			, _current_index( 0 )
		{}

		// Functions.

		// returns 'true' if this iterator is valid (there are other elements after it)
		bool IsValid() const
		{
			return _current_index < _array.num();
		}
		TYPE& Value() const
		{
			return _array[ _current_index ];
		}
		void MoveToNext()
		{
			++_current_index;
		}
		void Skip( U32 count )
		{
			_current_index += count;
		}
		void Reset()
		{
			_current_index = 0;
		}

		// Overloaded operators.

		// Pre-increment.
		const Iterator& operator ++ ()
		{
			++_current_index;
			return *this;
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		operator bool () const
		{
			return this->IsValid();
		}
		TYPE* operator -> () const
		{
			return &this->Value();
		}
		TYPE& operator * () const
		{
			return this->Value();
		}
	};

	///
	template< class ARRAY >
	class ConstIterator
	{
		const ARRAY &	_array;
		U32				_current_index;
		typedef typename ARRAY::ItemType TYPE;
	public:
		ConstIterator( const ARRAY& rArray )
			: _array( rArray )
			, _current_index( 0 )
		{}

		// Functions.

		// returns 'true' if this iterator is valid (there are other elements after it)
		bool IsValid() const
		{
			return _current_index < _array.num();
		}
		const TYPE& Value() const
		{
			return _array[ _current_index ];
		}
		void MoveToNext()
		{
			++_current_index;
		}
		void Skip( U32 count )
		{
			_current_index += count;
		}
		void Reset()
		{
			_current_index = 0;
		}

		// Overloaded operators.

		// Pre-increment.
		const ConstIterator& operator ++ ()
		{
			++_current_index;
			return *this;
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		operator bool () const
		{
			return this->IsValid();
		}
		const TYPE* operator -> () const
		{
			return &this->Value();
		}
		const TYPE& operator * () const
		{
			return this->Value();
		}
	};

}//namespace Arrays

/*
==========================================================
	ARRAY SLICING
==========================================================
*/
namespace Arrays
{
	template< class ARRAY >
	mxFORCEINLINE const TSpan< const typename ARRAY::ItemType >
	GetSpan( const ARRAY& a )
	{
		return TSpan< const typename ARRAY::ItemType >(
			a.raw(), a.num()
			);
	}

	template< class ARRAY >
	mxFORCEINLINE const TSpan< typename ARRAY::ItemType >
	GetSpan( ARRAY & a )
	{
		return TSpan< typename ARRAY::ItemType >(
			a.raw(), a.num()
			);
	}

	template< typename T, unsigned long N >
	mxFORCEINLINE const TSpan< const T >
	GetSpan( const T (&static_array)[N] )
	{
		return TSpan< const T >(
			static_array, N
			);
	}
}//namespace Arrays

/*
==========================================================
	ARRAY COMPARISON
==========================================================
*/
namespace Arrays
{
	/// compares the arrays for equality
	template< typename T >
	bool equal( const T* a, const T* b, size_t count )
	{
		if( TypeTrait<T>::IsPlainOldDataType )
		{
			return memcmp( a, b, count * sizeof(T) ) == 0;
		}
		else
		{
			for( size_t i = 0; i < count; i++ )
			{
				if( a[ i ] != b[ i ] ) {
					return false;
				}
			}
			return true;
		}
	}
}//namespace Arrays

/*
==========================================================
	DYNAMIC ARRAYS MEMORY MANAGEMENT
==========================================================
*/
namespace Arrays
{
	// defined in "Core/Memory/MemoryHeaps.cpp"
	AllocatorI& defaultAllocator();

	/// these flags are usually stored in the two lowest bits of an array pointer
	enum MemoryOwnership
	{
		MEMORY_IS_MANAGED_EXTERNALLY = 0,
		OWNS_MEMORY = 1,	//!< Can the array deallocate the memory block?
		//MEMORY_READ_ONLY = 2,	//!< Can the array write to the pointed memory?
	};

	/// figure out the size for allocating a new buffer
	U32 calculateNewCapacity( const UINT new_element_count, const UINT old_capacity );

	///
	enum ResizeBehavior
	{
		/// Reduce the number of reallocations by preallocating up to twice as much memory as the actual data needs.
		Preallocate,

		/// Resize to the exact size specified irregardless of granularity.
		ResizeExactly,
	};

	/// Allocates more or releases memory if needed,
	/// but doesn't construct/initialize new elements (doesn't change array item_count).
	template< typename TYPE >
	ERet resizeMemoryBuffer(
		TYPE **const p_old_array
		, const U32 item_count	// the number of 'alive' elements
		, const U32 old_capacity
		, const MemoryOwnership memory_ownership
		, AllocatorI & allocator
		, const U32 new_capacity
		, const U32 alignment = mxALIGNOF(TYPE)
		)
	{
		mxASSERT( new_capacity > 0 && new_capacity <= DBG_MAX_ARRAY_CAPACITY );

		if(mxUNLIKELY( new_capacity == old_capacity )) {
			return ALL_OK;
		}

		// allocate a new memory buffer.
		TYPE * new_array = static_cast< TYPE* >(
			allocator.Allocate(
			new_capacity * sizeof(TYPE),
			alignment
			)
		);
		if(mxUNLIKELY( !new_array )) {
			return ERR_OUT_OF_MEMORY;
		}

		TYPE* old_array = *p_old_array;
		if( old_array )
		{
			// Copy-construct the new elements.
			TCopyConstructArray( new_array, old_array, item_count );
			// Destroy the old contents.
			TDestructN_IfNonPOD( old_array, item_count );
			// deallocate the old memory buffer.
			if( memory_ownership == OWNS_MEMORY ) {
				allocator.Deallocate( old_array );
			}
		}

		// NOTE: newly created elements are allocated, but not constructed!

		*p_old_array = new_array;

		return ALL_OK;
	}

	template< typename TYPE >
	void freeMemoryBuffer(
		TYPE ** p_array
		, const U32 capacity
		, AllocatorI & allocator
		)
	{
		//TODO: pass buffer size?: this->capacity() * sizeof(TYPE)
		allocator.Deallocate( *p_array );
		*p_array = nil;
	}

}//namespace Arrays
