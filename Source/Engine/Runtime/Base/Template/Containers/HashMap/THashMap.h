/*
=============================================================================
	File:	THashMap.h
	Desc:	A very simple (closed) hash map - maps (unique) keys to values.
	Note:	Programmer uses hash table size that is power of 2
			because address calculation can be performed very quickly.
			The integer hash function can be used to post condition the output
			of a marginal quality hash function before the final address calculation is done. 
			Using the inlined version of the integer hash function is faster
			than doing a remaindering operation with a prime number!
			An integer remainder operation may take up to 18 cycles or longer to complete,
			depending on machine architecture.
	ToDo:	Try tables with a non-power-of-two sizes (using prime numbers);
			some say that by AND-ing with (tableSize-1) you effectively limit
			the range of possible values of hash keys.
			Stop reinventing the wheel.
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/Array/DynamicArray.h>

/// out-of-line methods to reduce memory bloat
/// (also, brings convenience - all memory functions in one place)
namespace HashMapUtil
{
	/// hashMod = tableSize - 1
	UINT GetHashTableSize( const void* buckets, UINT hashMod );

	enum { DEFAULT_HASH_TABLE_SIZE = 16 };

	UINT CalcHashTableSize( UINT numItems, UINT defaultTableSize = DEFAULT_HASH_TABLE_SIZE );
}//namespace HashMapUtil

///
///	THashMap< KEY, VALUE > -  An unordered data structure mapping keys to values.
///
///	NOTE: KEY should have member function 'UINT ComputeHash32( const KEY& key )'
///	and 'bool operator == (const KEY& other) const'.
///
///	This is a closed hash table with linear probing.
///	Periodically check that DbgGetLoad() is low (> 0.1) and DbgGetSpread() is high.
///	When DbgGetLoad() gets near 1.0 your hash function is badly designed
///	and maps too many inputs to the same output.
///
template<
	typename KEY,
	typename VALUE,
	class HASH_FUNC = THashTrait< KEY >,
	class EQUALS_FUNC = TEqualsTrait< KEY >
>
class THashMap: NonCopyable
{
public:
	typedef U32 HashType_t;
	typedef U32 IndexType_t;

	struct Pair
	{
		KEY			key;
		VALUE		value;
		IndexType_t	next;

	public:
		mxFORCEINLINE Pair()
			: next(INDEX_NONE)
		{}
		mxFORCEINLINE Pair( const KEY& k, const VALUE& v )
			: key( k ), value( v )
			, next(INDEX_NONE)
		{}
		mxFORCEINLINE Pair( const KEY& k, const VALUE& v, INT next_pair_index )
			: key( k ), value( v )
			, next( next_pair_index )
		{}
		friend AWriter& operator << ( AWriter& file, const Pair& o )
		{
			file << o.key << o.value;
			return file;
		}
		friend AReader& operator >> ( AReader& file, Pair& o )
		{
			file >> o.key >> o.value;
			return file;
		}
		friend mxArchive& operator && ( mxArchive& archive, Pair& o )
		{
			return archive && o.key && o.value;
		}
	};

	typedef DynamicArray< Pair >	PairsArray;

private:
	IndexType_t *	_table;
	IndexType_t		_table_mask;	// = tableSize - 1, tableSize must be a power of two
	PairsArray		_pairs;

public:
	typedef THashMap
	<
		KEY,
		VALUE,
		HASH_FUNC,
		EQUALS_FUNC
	> THIS_TYPE;

	THashMap( AllocatorI & allocator )
		: _pairs( allocator )
	{
		_table = NULL;
		_table_mask = 0;
	}

	~THashMap()
	{
		this->Clear();
	}

	ERet Initialize(
		const UINT table_size_pow_of_two = HashMapUtil::DEFAULT_HASH_TABLE_SIZE,	// must be a power of two
		const UINT reserved_element_count = 0
		)
	{
		mxASSERT(table_size_pow_of_two > 1 && IsPowerOfTwo(table_size_pow_of_two));
		mxASSERT(NULL == _table && _pairs.IsEmpty());

		const size_t table_size_in_bytes = table_size_pow_of_two * sizeof(_table[0]);

		_table = c_cast(IndexType_t*) this->allocator().Allocate(
			table_size_in_bytes
			, EFFICIENT_ALIGNMENT
			);
		mxENSURE( _table, ERR_OUT_OF_MEMORY, "malloc(%d) failed", table_size_in_bytes );

		memset( _table, INDEX_NONE, table_size_in_bytes );

		_table_mask = table_size_pow_of_two - 1;

		if( reserved_element_count ) {
			mxDO(_pairs.reserve( reserved_element_count ));
		}

		return ALL_OK;
	}

	// Removes all elements from the table.
	void RemoveAll()
	{
		const UINT tableSize = this->GetTableSize();
		if( tableSize )
		{
			const UINT numBytes = tableSize * sizeof(_table[0]);
			memset( _table, INDEX_NONE, numBytes );
		}
		_pairs.RemoveAll();
	}

	// Removes all elements from the table and releases allocated memory.
	void Clear()
	{
		if( _table ) {
			const U32 bytesToFree = this->GetTableSize() * sizeof(_table[0]);
			mxUNUSED( bytesToFree );
			this->allocator().Deallocate( _table );
			_table = NULL;
		}
		_table_mask = 0;
		_pairs.clear();
	}

	// assuming that VALUEs are pointers, deletes them and empties the table
	void DeleteValues()
	{
		const UINT tableSize = GetTableSize();

		const UINT numBytes = tableSize * sizeof(_table[0]);
		memset( _table, INDEX_NONE, numBytes );

		for( IndexType_t i = 0; i < _pairs.num(); i++ )
		{
			delete _pairs[i].value;
		}
		_pairs.RemoveAll();
	}

	// Returns a pointer to the element if it exists, or NULL if it does not.

	VALUE* FindValue( const KEY& key )
	{
		mxASSERT_PTR(_table);
		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;
		for( IndexType_t i = _table[hash]; i != INDEX_NONE; i = _pairs[i].next )
		{
			if(EQUALS_FUNC::Equals( _pairs[i].key, key )) {
				return &(_pairs[i].value);
			}
		}
		return NULL;
	}

	// Returns a pointer to the element if it exists, or NULL if it does not.

	const VALUE* FindValue( const KEY& key ) const
	{
		return const_cast< THIS_TYPE* >( this )->FindValue( key );
	}

	// Returns a reference to the element if it exists, or a default-constructed value if it does not.

	VALUE FindRef( const KEY& key, const VALUE& _default = VALUE() )
	{
		mxASSERT_PTR(_table);
		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;
		for( IndexType_t i = _table[hash]; i != INDEX_NONE; i = _pairs[i].next )
		{
			if(EQUALS_FUNC::Equals( _pairs[i].key, key )) {
				return _pairs[i].value;
			}
		}
		return _default;
	}

	const VALUE FindRef( const KEY& key, const VALUE& _default = VALUE() ) const
	{
		return const_cast< THIS_TYPE* >( this )->FindRef( key, _default );
	}

	bool Contains( const KEY& key ) const
	{
		mxASSERT_PTR(_table);
		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;
		for( IndexType_t i = _table[hash]; i != INDEX_NONE; i = _pairs[i].next )
		{
			if(EQUALS_FUNC::Equals( _pairs[i].key, key )) {
				return true;
			}
		}
		return false;
	}

	// Returns key by value. Returns NULL pointer if it's not in the table. Slow!

	KEY* FindKeyByValue( const VALUE& value )
	{
		for( IndexType_t i = 0; i < _pairs.num(); i++ )
		{
			if( _pairs[i].value == value ) {
				return &(_pairs[i].key);
			}
		}
		return NULL;
	}

	UINT FindKeyIndex( const KEY& key ) const
	{
		for( IndexType_t i = 0; i < _pairs.num(); i++ )
		{
			if( _pairs[i].key == key ) {
				return i;
			}
		}
		return INDEX_NONE;
	}

	// Inserts a (key,value) pair into the table.

	ERet Insert( const KEY& key, const VALUE& value )
	{
		const Pair* new_pair = InsertEx( key, value );
		return new_pair ? ALL_OK : ERR_OUT_OF_MEMORY;
	}

	// NOTE: expensive operation!
	// Returns the number of removed items.

	UINT Remove( const KEY& key )
	{
		mxASSERT_PTR(_table);
		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;

		Pair* pair = this->_FindPair( key, hash );
		if( !PtrToBool( pair ) ) {
			return 0;
		}

		// index of pair being removed in the _pairs array
		const IndexType_t thisPairIndex = Arrays::GetStoredItemIndex( _pairs, pair );

		// Remove the pair from the hash table.

		this->_UnlinkPair( thisPairIndex, hash );

		// We now move the last pair into spot of the pair being removed
		// (to avoid unused holes in the array of pairs).
		// We need to fix the hash table indices to support the move.
		{
			const IndexType_t lastPairIndex = _pairs.num() - 1;

			// If the removed pair is the last pair, we are done.
			if( thisPairIndex == lastPairIndex )
			{
				_pairs.PopLastValue();
				return 1;
			}

			// Remove the last pair from the hash table.
			const HashType_t lastPairHash = HASH_FUNC::ComputeHash32( _pairs[ lastPairIndex ].key ) & _table_mask;

			this->_UnlinkPair( lastPairIndex, lastPairHash );

			// Copy the last pair into the removed pair's spot.
			_pairs[ thisPairIndex ] = _pairs[ lastPairIndex ];

			// Insert the last pair into the hash table.
			_pairs[ thisPairIndex ].next = _table[ lastPairHash ];
			_table[ lastPairHash ] = thisPairIndex;

			// Remove and destroy the last pair.
			_pairs.PopLastValue();
		}

		return 1;
	}

	// Returns the number of buckets.
	mxFORCEINLINE UINT GetTableSize() const
	{
		return HashMapUtil::GetHashTableSize( _table, _table_mask );
	}

	// Returns the number of key-value pairs stored in the table.
	mxFORCEINLINE UINT NumEntries() const
	{
		return _pairs.num();
	}

	mxFORCEINLINE bool IsEmpty() const
	{
		return !this->NumEntries();
	}

	mxFORCEINLINE ERet resize( UINT new_table_size_pow_of_two )
	{
		mxASSERT(IsPowerOfTwo( new_table_size_pow_of_two ));
		mxDO(this->_Rehash( new_table_size_pow_of_two ));
		return ALL_OK;
	}

	friend AWriter& operator << ( AWriter& file, const THIS_TYPE& o )
	{
		file << o._pairs;
		return file;
	}
	friend AReader& operator >> ( AReader& file, THIS_TYPE& o )
	{
		file >> o._pairs;
		o._Rehash();
		return file;
	}

	friend mxArchive& operator && ( mxArchive& archive, THIS_TYPE& o )
	{
		archive && o._pairs;
		if( archive.IsReading() )
		{
			o._Rehash();
		}
		return archive;
	}

public_internal:

	Pair* FindPair( const KEY& key )
	{
		mxASSERT_PTR(_table);
		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;
		return this->_FindPair( key, hash );
	}

	Pair* InsertEx( const KEY& key, const VALUE& value )
	{
		mxASSERT_PTR(_table);

		const HashType_t hash = HASH_FUNC::ComputeHash32( key ) & _table_mask;

		for( IndexType_t i = _table[hash]; i != INDEX_NONE; i = _pairs[i].next )
		{
			if(EQUALS_FUNC::Equals( _pairs[i].key, key )) {
				_pairs[i].value = value;
				return &_pairs[i];
			}
		}

		// Create a new entry.

		const IndexType_t new_entry_index = _pairs.num();

		Pair* new_pair = _pairs.AllocateUninitialized();

		if( new_pair )
		{
			new (new_pair) Pair(
				key
				, value
				, _table[ hash ]
			);

			_table[ hash ] = new_entry_index;
		}

		return new_pair;
	}

	mxFORCEINLINE PairsArray & GetPairs()
	{
		return _pairs;
	}
	mxFORCEINLINE const PairsArray & GetPairs() const
	{
		return _pairs;
	}

	void Relax()
	{
		// calculate new table size
		UINT tableSize = _table_mask + 1;
		while( tableSize > _pairs.num()*2 + DEFAULT_HASH_TABLE_SIZE )
		{
			tableSize /= 2;
		}
		tableSize = CeilPowerOfTwo( tableSize );
		this->_Rehash( tableSize );
	}

	void _Rehash()
	{
		const UINT oldNum = _pairs.num();
		UINT tableSize = oldNum ? (oldNum * 2) : DEFAULT_HASH_TABLE_SIZE;
		tableSize = CeilPowerOfTwo( tableSize );
		this->_Rehash( tableSize );
	}

	void GrowIfNeeded()
	{
		const UINT numPairs = _pairs.num();
		const UINT tableSize = this->GetTableSize();
		if( tableSize * 2 + DEFAULT_HASH_TABLE_SIZE < numPairs )
		{
			const UINT new_table_size_pow_of_two = CeilPowerOfTwo( numPairs+1 );
			this->_Rehash( new_table_size_pow_of_two );
		}
	}

public:	// Iterators, algorithms, ...

	friend class Iterator;
	class Iterator {
	public:
		mxINLINE Iterator( THashMap& map )
			: _map( map )
			, _pairs( map._pairs )
			, _index( 0 )
		{}
	
		mxFORCEINLINE bool IsValid() const
		{
			return _index < _pairs.num();
		}
		mxFORCEINLINE void MoveToNext()
		{
			++_index;
		}

		mxFORCEINLINE KEY & Key() const
		{
			return _pairs[ _index ].key;
		}
		mxFORCEINLINE VALUE & Value() const
		{
			return _pairs[ _index ].value;
		}

		// Pre-increment.
		mxFORCEINLINE void operator ++ ()
		{
			this->MoveToNext();
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		mxFORCEINLINE operator bool () const
		{
			return this->IsValid();
		}

	private:
		THashMap &		_map;
		PairsArray &	_pairs;
		IndexType_t		_index;
	};

	friend class ConstIterator;
	class ConstIterator {
	public:
		mxINLINE ConstIterator( const THashMap& map )
			: _map( map )
			, _pairs( map._pairs )
			, _index( 0 )
		{}

		mxFORCEINLINE bool IsValid() const
		{
			return _index < _pairs.num();
		}
		mxFORCEINLINE void MoveToNext()
		{
			++_index;
		}

		mxFORCEINLINE const KEY& Key() const
		{
			return _pairs[ _index ].key;
		}
		mxFORCEINLINE const VALUE& Value() const
		{
			return _pairs[ _index ].value;
		}

		// Pre-increment.
		mxFORCEINLINE void operator ++ ()
		{
			this->MoveToNext();
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		mxFORCEINLINE operator bool () const
		{
			return this->IsValid();
		}

	private:
		const THashMap &	_map;
		const PairsArray &	_pairs;
		IndexType_t			_index;
	};

public:	// Testing & Debugging.

	UINT DbgGetDeepestBucketSize() const
	{
		const UINT numBuckets = this->GetTableSize();

		UINT deepest = 0;

		for(UINT iBucket = 0; iBucket < numBuckets; iBucket++)
		{
			UINT numItemsInBucket = 0;

			for(INT iPair = _table[ iBucket ];
				iPair != INDEX_NONE;
				iPair = _pairs[ iPair ].next)
			{
				numItemsInBucket++;
			}
			deepest = Max( deepest, numItemsInBucket );
		}

		return deepest;
	}

	// returns the hash table's load factor - a number in range [0..1]; the less number means the better

	FLOAT DbgGetLoad() const
	{
		return (FLOAT)this->DbgGetDeepestBucketSize() / this->NumEntries();
	}

	/// returns a number in range [0..100]; the greater number means the better
	UINT DbgGetSpread() const
	{
		const UINT numEntries = this->NumEntries();
		const UINT tableSize = this->GetTableSize();

		// if this table is empty
		if( !numEntries ) {
			return 100;
		}

		UINT average = numEntries / tableSize;

		UINT error = 0;

		for( UINT iBucket = 0; iBucket < tableSize; iBucket++ )
		{
			UINT numItemsInBucket = 0;

			for(INT iPair = _table[ iBucket ];
				iPair != INDEX_NONE;
				iPair = _pairs[ iPair ].next)
			{
				numItemsInBucket++;
			}

			UINT e = Abs( numItemsInBucket - average );
			if( e > 1 ) {
				error += e - 1;
			}
		}

		return 100 - (error * 100 / numEntries);
	}

private:

	// Resizing takes O(n) time to complete, where n is a number of entries in the table.
	ERet _Rehash( const UINT new_table_size_pow_of_two )
	{
		mxASSERT(new_table_size_pow_of_two > 1 && IsPowerOfTwo(new_table_size_pow_of_two));

		const UINT numBytes = new_table_size_pow_of_two * sizeof(_table[0]);
		IndexType_t* new_table = (IndexType_t*) this->allocator().Allocate(
			numBytes
			, EFFICIENT_ALIGNMENT
			);
		mxENSURE( new_table, ERR_OUT_OF_MEMORY, "" );

		memset( new_table, INDEX_NONE, numBytes );

		const UINT old_table_size_pow_of_two = this->GetTableSize();
		_table_mask = new_table_size_pow_of_two - 1;

		for( IndexType_t i = 0; i < _pairs.num(); i++ )
		{
			Pair & pair = _pairs[ i ];
			const HashType_t hash = HASH_FUNC::ComputeHash32( pair.key ) & _table_mask;
			pair.next = new_table[ hash ];
			new_table[ hash ] = i;
		}

		if( _table )
		{
			const UINT numBytes = old_table_size_pow_of_two * sizeof(_table[0]);
			mxUNUSED( numBytes );
			this->allocator().Deallocate( _table );
		}

		_table = new_table;

		return ALL_OK;
	}

	inline Pair* _FindPair( const KEY& key, HashType_t hash )
	{
		mxASSERT_PTR(_table);
		for( IndexType_t i = _table[hash]; i != INDEX_NONE; i = _pairs[i].next )
		{
			if(EQUALS_FUNC::Equals( _pairs[i].key, key )) {
				return &(_pairs[i]);
			}
		}
		return NULL;
	}

	// Removes the pair from the hash table.
	void _UnlinkPair( IndexType_t pairIndex, HashType_t hash )
	{
		// find the index of pair that references the pair being deleted
		// i.e. find 'prevIndex' that '_pairs[ prevIndex ].next == pairIndexInArray'
		//
		IndexType_t prevPairIndex = INDEX_NONE;

		// this could be removed if we had 'Pair::prev' along with 'Pair::next'
		{
			IndexType_t currPairIndex = _table[ hash ];
			mxASSERT( currPairIndex != INDEX_NONE );

			while( currPairIndex != pairIndex )
			{
				prevPairIndex = currPairIndex;
				currPairIndex = _pairs[ currPairIndex ].next;
			}
		}

		if( prevPairIndex != INDEX_NONE )
		{
			mxASSERT( _pairs[ prevPairIndex ].next == pairIndex );
			// remove this pair from the linked list
			_pairs[ prevPairIndex ].next = _pairs[ pairIndex ].next;
		}
		else
		{
			// if this pair was the first one
			_table[ hash ] = _pairs[ pairIndex ].next;
		}
	}

	mxFORCEINLINE AllocatorI& allocator() { return _pairs.allocator(); }
};

/*
interesting:
http://www.reedbeta.com/blog/2015/01/12/data-oriented-hash-table/
*/
