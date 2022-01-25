/*
=============================================================================
	File:	TLinearHashTable.h
	Desc:	
=============================================================================
*/
#pragma once


/// Implements a closed ("open addressing") hash-table
/// which maps integer keys to data indices.
/// Does not support deletion and resizing.
/// This hash table is basically a linear search with a smart starting index guess.
///
template
<
	/// KEY must be an integer type (32-bit or 64-bit),
	/// with zero usually denoting an invalid key.
	typename KEY,

	typename VALUE,

	class KEY_HASHER = THashTrait< KEY >,

	/// invalid key, indicates a free slot
	const KEY NIL_KEY = 0
>
class TLinearHashTable: NonCopyable
{
	KEY *			_keys;
	VALUE *			_values;
	U32				_hashmod;	//!< mod = capacity - 1 (capacity is always a power of two)
	AllocatorI &	_allocator;

public:
	TLinearHashTable(
		AllocatorI & storage
		)
		: _keys(nil)
		, _values(nil)
		, _hashmod(0)
		, _allocator(storage)
	{
	}

	~TLinearHashTable()
	{
		this->Clear();
	}

	///
	ERet Initialize( const U32 max_item_count )
	{
		mxASSERT( _keys == nil && _hashmod == 0 );

		//
		const U32 capacity = CeilPowerOfTwo( max_item_count * 2 );

		const size_t size_of_allocated_keys = sizeof(_keys[0]) * capacity;
		const size_t size_of_allocated_values = sizeof(_values[0]) * capacity;

		void* keys_and_values = _allocator.Allocate(
			size_of_allocated_keys + size_of_allocated_values
			, EFFICIENT_ALIGNMENT
			);
		mxENSURE( keys_and_values, ERR_OUT_OF_MEMORY, "" );

		//
		_hashmod = capacity - 1;

		//
		_keys = (KEY*) keys_and_values;
		_values = (VALUE*) mxAddByteOffset( keys_and_values, size_of_allocated_keys );

		mxASSERT(IS_ALIGNED_BY( _values, mxALIGNOF(VALUE) ));

		// Mark all slots as free.
		TConstructN_IfNonPOD( _keys, NIL_KEY, capacity );

		// Default-construct values.
		TConstructN_IfNonPOD( _values, capacity );

		return ALL_OK;
	}

	mxFORCEINLINE U32 Capacity() const { return _hashmod + 1; }

	mxFORCEINLINE bool IsInitialized() const { return _keys != nil; }

	///
	void Clear()
	{
		if( _keys )
		{
			const U32 capacity = this->Capacity();

			TDestructN_IfNonPOD( _keys, capacity );
			TDestructN_IfNonPOD( _values, capacity );

			//
			_allocator.Deallocate( _keys );
			_keys = nil;
			_values = nil;

			_hashmod = 0;
		}
	}

	void RemoveAll()
	{
		if( _keys )
		{
			const U32 capacity = this->Capacity();

			TDestructN_IfNonPOD( _keys, capacity );
			TDestructN_IfNonPOD( _values, capacity );

			// Mark all slots as free.
			TConstructN_IfNonPOD( _keys, NIL_KEY, capacity );
			// Default-construct values.
			TConstructN_IfNonPOD( _values, capacity );
		}
	}

	///
	ERet Insert( const KEY& key, const VALUE& value )
	{
		mxASSERT2(key != NIL_KEY, "invalid key");
		//mxASSERT( m_capacity && m_count < m_capacity - m_capacity / 4 );

		const U32 hashmod = _hashmod;
		U32 bucket = KEY_HASHER::ComputeHash32( key ) & hashmod;

		KEY* __restrict keys = _keys;
		VALUE* __restrict values = _values;

		for( U32 probe = 0; probe <= hashmod; ++probe )
		{
			const KEY& probe_key = keys[ bucket ];

			if( NIL_KEY == probe_key )
			{
				// found an empty spot
				keys[ bucket ] = key;
				values[ bucket ] = value;
				return ALL_OK;
			}

			mxENSURE( probe_key != key, ERR_DUPLICATE_OBJECT, "We assume all keys are different" );
			//if( probe_key.key == key )
			//{
			//	// update an existing slot
			//	return &probe_key.value;
			//}

			// this location in the array is already occupied

			// hash collision, quadratic probing
			bucket = ( bucket + probe + 1 ) & hashmod;
		}

		mxUNREACHABLE2("Hash table is full");
		return ERR_BUFFER_TOO_SMALL;
	}

	/// Returns a pointer to the element if it exists, or NULL if it does not.
	const VALUE* Find( const KEY& key ) const
	{
		mxASSERT2(key != NIL_KEY, "invalid key");

		const U32	hashmod = _hashmod;

		U32 bucket = KEY_HASHER::ComputeHash32( key ) & hashmod;

		const KEY* __restrict keys = _keys;
		const VALUE* __restrict values = _values;

		for( U32 probe = 0; probe <= hashmod; ++probe )
		{
			const KEY& probe_key = keys[ bucket ];

			if( probe_key == key ) {
				return &values[ bucket ];
			}

			if( probe_key == NIL_KEY ) {
				return nil;
			}

			// hash collision, quadratic probing
			bucket = (bucket + probe + 1) & hashmod;
		}

		mxUNREACHABLE2("Hash table is full");
		return nil;
	}

#if 0
	///
	const VALUE* Find( const KEY& key ) const
	{
		return const_cast< TLinearHashTable* >( this )->Find( key );
	}
#endif

	/// for debugging
	const VALUE* Find_LinearSearch( const KEY& key ) const
	{
		mxASSERT2(key != NIL_KEY, "invalid key");

		const U32	hashmod = _hashmod;

		const KEY* __restrict keys = _keys;
		const VALUE* __restrict values = _values;

		for( U32 i = 0; i <= hashmod; i++ )
		{
			if( keys[ i ] == key ) {
				return &values[ i ];
			}
		}

		return nil;
	}

	///
	const VALUE FindRef(
		const KEY& key
		, const VALUE& empty_value = VALUE()
		) const
	{
		mxASSERT2(key != NIL_KEY, "invalid key");

		const U32	hashmod = _hashmod;

		U32 bucket = KEY_HASHER::ComputeHash32( key ) & hashmod;

		const KEY* __restrict keys = _keys;
		const VALUE* __restrict values = _values;

		for( U32 probe = 0; probe <= hashmod; ++probe )
		{
			const KEY& probe_key = keys[ bucket ];

			if( probe_key == key ) {
				return values[ bucket ];
			}

			if( probe_key == NIL_KEY ) {
				return empty_value;
			}

			// hash collision, quadratic probing
			bucket = (bucket + probe + 1) & hashmod;
		}

		mxUNREACHABLE2("Hash table is full");
		return empty_value;
	}

	///
	bool ContainsKey( const KEY& key ) const
	{
		return const_cast<TLinearHashTable*>(this)->Find( key ) != nil;
	}
};
