/*
=============================================================================
	File:	TSimpleHashTable.h
	Desc:	A simple hash table that doesn't own memory.
=============================================================================
*/
#pragma once


/// A simple "closed"/"open addressing"/"linear" hash-table
/// that uses linear probing and power-of-two capacity.
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
struct TSimpleHashTable
{
	KEY *	_keys;
	VALUE *	_values;
	U32		_hashmod;	//!< mod = capacity - 1 (capacity is always a power of two)

public:
	TSimpleHashTable()
		: _keys(nil)
		, _values(nil)
		, _hashmod(0)
	{
	}

	~TSimpleHashTable()
	{
		this->Clear();
	}

	static size_t CalcTotalUsedMemoryForMaxEntries(
		const U32 max_item_count
		)
	{
		const U32 capacity = CeilPowerOfTwo( max_item_count * 2 );
		const size_t size_of_allocated_keys = sizeof(KEY) * capacity;
		const size_t size_of_allocated_values = sizeof(VALUE) * capacity;
		return size_of_allocated_keys + size_of_allocated_values;
	}

	ERet Initialize(
		KEY* keys
		, VALUE* values
		, const U32 table_size__pow_of_two
		)
	{
		mxASSERT( _keys == nil && _hashmod == 0 );
		mxASSERT(IsPowerOfTwo(table_size__pow_of_two));

		//
		_hashmod = table_size__pow_of_two - 1;

		//
		_keys = keys;
		_values = values;

		mxASSERT(IS_ALIGNED_BY( _values, mxALIGNOF(VALUE) ));

		// Mark all slots as free.
		TSetArray( _keys, table_size__pow_of_two, (KEY)NIL_KEY );

		// Default-construct values.
		TConstructN_IfNonPOD( _values, table_size__pow_of_two );

		return ALL_OK;
	}

	///
	ERet Initialize(
		const TSpan<BYTE> memory
		, const U32 max_item_count
		)
	{
		mxASSERT( _keys == nil && _hashmod == 0 );

		//
		const U32 capacity_pow2 = CeilPowerOfTwo( max_item_count * 2 );

		const size_t size_of_allocated_keys = sizeof(_keys[0]) * capacity_pow2;
		const size_t total_size_of_allocated_memory = CalcTotalUsedMemoryForMaxEntries(max_item_count);

		mxENSURE( total_size_of_allocated_memory <= memory.rawSize()
			, ERR_BUFFER_TOO_SMALL
			, ""
			);

		KEY* keys = (KEY*) memory._data;
		VALUE* values = (VALUE*) mxAddByteOffset( memory._data, size_of_allocated_keys );

		return Initialize(keys, values, max_item_count);
	}

	mxFORCEINLINE U32 capacity() const { return _hashmod + 1; }

	///
	void Clear()
	{
		if( _keys )
		{
			TDestructN_IfNonPOD( _keys, this->capacity() );
			TDestructN_IfNonPOD( _values, this->capacity() );

			_keys = nil;
			_values = nil;

			_hashmod = 0;
		}
	}

	///
	ERet Insert( const KEY& key, const VALUE& value )
	{
		mxASSERT2(key != NIL_KEY, "invalid key");

		const U32 hashmod = _hashmod;
		U32 bucket = KEY_HASHER::ComputeHash32( key ) & hashmod;

		KEY* __restrict keys = _keys;
		VALUE* __restrict values = _values;

		for( U32 probe_index = 0; probe_index <= hashmod; probe_index++ )
		{
			const KEY& probe_key = keys[ bucket ];

			if(
				// empty slot, insert a new entry:
				NIL_KEY == probe_key
				// update an existing slot:
				//|| probe_key.key == key
				)
			{
				// found an empty spot
				keys[ bucket ] = key;
				values[ bucket ] = value;
				return ALL_OK;
			}

			mxENSURE( probe_key != key, ERR_DUPLICATE_OBJECT, "We assume all keys are different" );

			// this location in the array is already occupied

			// hash collision, linear probing
			bucket = ( bucket + 1 ) & hashmod;
		}
		mxUNREACHABLE2("Hash table is full");
		return ERR_TOO_MANY_OBJECTS;
	}

	/// Returns a pointer to the element if it exists, or NULL if it does not.
	VALUE* FindValue( const KEY& key )
	{
		mxASSERT2(key != NIL_KEY, "invalid key");

		const U32	hashmod = _hashmod;

		U32 bucket = KEY_HASHER::ComputeHash32( key ) & hashmod;

		KEY* __restrict keys = _keys;
		VALUE* __restrict values = _values;

		for( U32 probe_index = 0; probe_index <= hashmod; probe_index++ )
		{
			const KEY& probe_key = keys[ bucket ];

			if( probe_key == key ) {
				return &values[ bucket ];
			}
			else if( probe_key == NIL_KEY ) {
				return nil;
			}

			bucket = ( bucket + 1 ) & hashmod;
		}

		mxUNREACHABLE2("Hash table is full");
		return nil;
	}

	///
	const VALUE* FindValue( const KEY& key ) const
	{
		return const_cast< TSimpleHashTable* >( this )->FindValue( key );
	}

	ERet Remove( const KEY& key )
	{
		const VALUE* existing_value = this->FindValue(key);
		mxENSURE(existing_value, ERR_OBJECT_NOT_FOUND, "");

		const U32 hashmod = _hashmod;
		const U32 bucket_of_removed_item = existing_value - _values;

		KEY* __restrict keys = _keys;
		VALUE* __restrict values = _values;

		//
		keys[ bucket_of_removed_item ] = NIL_KEY;
		values[ bucket_of_removed_item ] = VALUE();

		// Fix up collisions after this entry.
		U32 bucket = ( bucket_of_removed_item + 1 ) & hashmod;
		while( keys[ bucket ] != NIL_KEY )
		{
			const KEY key2 = keys[ bucket ];
			const VALUE value2 = values[ bucket ];

			keys[ bucket ] = NIL_KEY;
			values[ bucket ] = VALUE();

			Insert( key2, value2 );

			bucket = ( bucket + 1 ) & hashmod;
		}

		return ALL_OK;
	}

	///
	bool ContainsKey( const KEY& key ) const
	{
		return this->FindValue( key ) != nil;
	}
};
