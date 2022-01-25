/*
=============================================================================
	File:	TSortedMap.h
	Date:	2020 Jan 01
	Desc:	An associative array, map, symbol table, or dictionary
			- an abstract data type composed of a collection of (key, value) pairs,
			- such that each possible key appears at most once in the collection. 
	Note:	Use for keys and values that are cheap to copy.
	ToDo:	
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/MetaDictionary.h>
#include <Base/Template/Algorithm/Sorting/InsertionSort.h>
#include <Base/Template/Containers/Array/ArraysCommon.h>


///
///	TSortedMap< KEY, VALUE > -  An ordered data structure mapping keys to values.
///	
///	Both keys and values are sorted by increasing keys.
///
template<
	typename KEY,
	typename VALUE
>
class TSortedMap
{
public_internal:

	mxOPTIMIZE("store in a single memory block, use only one pointer");

	// sorted in ascending order:
	KEY *			_keys;
	VALUE *			_values;

	U32				_count;
	U32				_capacity_and_flags;

	AllocatorI &	_allocator;

	//20/32

	// One bit is used for indicating if the memory was externally allocated (and the array cannot delete it)
	// and another bit is used for indicating if the array is sorted.
	enum We_Store_Capacity_And_Bit_Flags_In_One_Int
	{
		OWNS_MEMORY_MASK = U32(1<<31),	//!< Indicates that the storage is not the array's to delete.
		ARRAY_IS_SORTED_MASK = U32(1<<30),
		EXTRACT_CAPACITY_MASK = U32((1<<30)-1),
	};

public:
	typedef TSortedMap
	<
		KEY,
		VALUE
	> THIS_TYPE;

	TSortedMap( AllocatorI & allocator )
		: _allocator( allocator )
	{
		_keys = nil;
		_values = nil;
		_count = 0;
		_capacity_and_flags = 0;
	}

	~TSortedMap()
	{
		this->clear();
	}

	mxFORCEINLINE bool IsEmpty() const { return !_count; }

	mxFORCEINLINE U32 Num() const { return _count; }

	/// Returns the current capacity of this array.
	mxFORCEINLINE U32 capacity() const { return _capacity_and_flags & EXTRACT_CAPACITY_MASK; }

	mxFORCEINLINE bool isSorted() const { return _capacity_and_flags & ARRAY_IS_SORTED_MASK; }

	mxFORCEINLINE bool ownsMemory() const { return _capacity_and_flags & OWNS_MEMORY_MASK; }

	/// Ensures no reallocation occurs until at least size 'count'.
	ERet Reserve( U32 count )
	{
		const U32 old_capacity = this->capacity();

		if( count > old_capacity )
		{
			const U32 new_capacity = Arrays::calculateNewCapacity( count, old_capacity );

			mxDO(this->_resize( new_capacity ));
		}

		return ALL_OK;
	}

	ERet ReserveExactly( U32 new_capacity )
	{
		const U32 old_capacity = this->capacity();

		if( new_capacity > old_capacity )
		{
			mxDO(this->_resize( new_capacity ));
		}

		return ALL_OK;
	}

	/// NOTE: make sure to set new Key-Value elements to valid values!
	ERet ResizeToCount( U32 new_count )
	{
		// Change the capacity if necessary.
		mxDO(this->ReserveExactly( new_count ));

		// Call default constructors for new elements.
		const U32 old_count = _count;
		const U32 num_new_elements = new_count - old_count;
		if( num_new_elements )
		{
			TConstructN_IfNonPOD(
				_keys + old_count,
				num_new_elements
				);
			TConstructN_IfNonPOD(
				_values + old_count,
				num_new_elements
				);
			_count = new_count;
		}

		return ALL_OK;
	}

	/// Adds an element to the end. Don't forget to call Sort() before performing any queries!
	ERet add( const KEY& key, const VALUE& value )
	{
		const U32 old_count = _count;

		mxDO(this->Reserve( old_count + 1 ));

		new( &_keys[ old_count ] ) KEY( key );
		new( &_values[ old_count ] ) VALUE( value );

		++_count;

		setUnsorted();

		return ALL_OK;
	}

	void removeAll()
	{
		TDestructN_IfNonPOD( _keys, _count );
		TDestructN_IfNonPOD( _values, _count );
		_count = 0;
		setUnsorted();
	}

	void clear()
	{
		removeAll();
		_releaseMemory();
		_capacity_and_flags = 0;
	}

	/// Resizes the array to exactly the number of elements it contains or frees up memory if empty.
	void shrink()
	{
		// Condense the array.
		if( _count > 0 ) {
			this->_resize( _count );
		} else {
			this->clear();
		}
	}

	VALUE* find( const KEY& key )
	{
		if( _count )
		{
			mxASSERT(this->isSorted());

			const UINT index = LowerBoundAscending( key, _keys, _count );
			return ( _keys[ index ] == key ) ? &_values[index ] : nil;
		}
		return nil;
	}

	const VALUE* find( const KEY& key ) const
	{
		return const_cast< THIS_TYPE* >( this )->find( key );
	}

	bool ContainsKey( const KEY& key ) const
	{
		return this->find( key ) != nil;
	}

	//

	VALUE findRef( const KEY& key, const VALUE& empty_value = VALUE() )
	{
		if( _count )
		{
			mxASSERT(this->isSorted());

			const UINT index = LowerBoundAscending( key, _keys, _count );
			return ( _keys[ index ] == key ) ? _values[index ] : empty_value;
		}
		return empty_value;
	}

	void sort()
	{
		if(_count)
		{
			Sorting::InsertionSortKeysAndValues(
				_keys, _count
				, _values
				, Sorting::ByIncreasingOrder< KEY >()
				);

			mxASSERT2(this->CheckKeysAreSorted(),
				"The keys are not sorted in ascending order or have duplicates!"
				);
		}

		setSorted();
	}

	void setSorted()
	{
		_capacity_and_flags |= ARRAY_IS_SORTED_MASK;
	}

	void setUnsorted()
	{
		CLEAR_BITS( _capacity_and_flags, ARRAY_IS_SORTED_MASK );
	}

	bool CheckKeysAreSorted() const
	{
		return Arrays::CheckIfIsSortedIncreasing(_keys, this->Num());
	}

	const VALUE* findValue_LinearSearch( const KEY& key ) const
	{
		for( U32 i = 0; i < _count; i++ )
		{
			if( _keys[i] == key ) {
				return &_values[i];
			}
		}

		return nil;
	}

public:
	mxFORCEINLINE const TSpan<VALUE> GetValuesSpan()
	{
		return TSpan<VALUE>( _values, _count );
	}

private:
	/// allocates more memory if needed, but doesn't initialize new elements (doesn't change array count)
	ERet _resize( U32 new_capacity )
	{
		const Arrays::MemoryOwnership memory_ownership = this->ownsMemory()
			? Arrays::OWNS_MEMORY
			: Arrays::MEMORY_IS_MANAGED_EXTERNALLY
			;

		const bool is_sorted = this->isSorted();
mxOPTIMIZE("use a single memory block");
		mxDO(Arrays::resizeMemoryBuffer(
			&_keys
			, _count
			, this->capacity()
			, memory_ownership
			, _allocator
			, new_capacity
			));

		mxDO(Arrays::resizeMemoryBuffer(
			&_values
			, _count
			, this->capacity()
			, memory_ownership
			, _allocator
			, new_capacity
			));

		_capacity_and_flags = new_capacity | OWNS_MEMORY_MASK;

		if( is_sorted ) {
			_capacity_and_flags |= ARRAY_IS_SORTED_MASK;
		}

		return ALL_OK;
	}

	void _releaseMemory()
	{
		bool owns_memory = this->ownsMemory();

		if( owns_memory ) {
			_allocator.Deallocate( _keys );
		}
		_keys = nil;

		if( owns_memory ) {
			_allocator.Deallocate( _values );
		}
		_values = nil;

		setUnsorted();
	}

public:	// Reflection.

	///
	class MetaDescriptor: public MetaDictionary
	{
	public:
		MetaDescriptor( const char* type_name )
			: MetaDictionary(
				type_name
				, TbTypeSizeInfo::For_Type< THIS_TYPE >()
				, T_DeduceTypeInfo< KEY >()
				, T_DeduceTypeInfo< VALUE >()
			)
		{}

		virtual void iterateMemoryBlocks(
			void * dictionary_instance
			, IterateMemoryBlockCallback* callback
			, void* user_data
			) const override
		{
			THIS_TYPE* dictionary = static_cast< THIS_TYPE* >( dictionary_instance );

			const U32 capacity = dictionary->capacity();
			const U32 item_count = dictionary->_count;

			if(item_count)
			{
				(*callback)(
					(void**) &dictionary->_keys
					, capacity * sizeof(dictionary->_keys[0])
					, true // is_dynamically_allocated
					, _key_type
					, item_count
					, sizeof(dictionary->_keys[0])
					, user_data
					);

				(*callback)(
					(void**) &dictionary->_values
					, capacity * sizeof(dictionary->_values[0])
					, true // is_dynamically_allocated
					, _value_type
					, item_count
					, sizeof(dictionary->_values[0])
					, user_data
					);
			}
		}

		virtual void markMemoryAsExternallyAllocated(
			void * dictionary_instance
			) const override
		{
			THIS_TYPE* dictionary = static_cast< THIS_TYPE* >( dictionary_instance );
			CLEAR_BITS( dictionary->_capacity_and_flags, OWNS_MEMORY_MASK );
		}
	};
};

//-----------------------------------------------------------------------
// Reflection.
//
template< typename TYPE, typename VALUE >
struct TypeDeducer< TSortedMap< TYPE, VALUE > >
{
	static mxFORCEINLINE const TbMetaType& GetType()
	{
		static TSortedMap< TYPE, VALUE >::MetaDescriptor	static_type_info(
			mxEXTRACT_TYPE_NAME(TSortedMap)
		);
	
		return static_type_info;
	}

	static mxFORCEINLINE ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Dictionary;
	}
};
