/*
=============================================================================
	File:	Set.h
	Desc:	Simple array-based set
=============================================================================
*/
#pragma once

template< typename TYPE >
class TSet
{
	TArray< TYPE >	m_elems;

public:
	enum { MAX_CAPACITY = MAX_UINT16-1 };

	explicit TSet()
	{
	}

	UINT Num() const
	{
		return m_elems.num();
	}

	TYPE & add( const TYPE& newOne )
	{
		return m_elems.add( newOne );
	}

	void Insert( const TYPE& item )
	{
		m_elems.AddUnique( item );
	}

	bool contains( const TYPE& newOne )
	{
		return m_elems.contains( newOne );
	}
};

template< typename VALUE >
class SortedSet: NonCopyable
{
	DynamicArray< VALUE >	_items;

public:
	SortedSet( AllocatorI & allocator )
		: _items( allocator )
	{
	}
	~SortedSet()
	{
	}

	ERet initialize( const U32 max_items )
	{mxTODO("how many items to allocate?");
		const U32 capacity = CeilPowerOfTwo( max_items * 2 );

		_items = static_cast< ITEM* >( m_allocator.Allocate( sizeof(ITEM) * capacity, EFFICIENT_ALIGNMENT ) );
		mxENSURE( _items, ERR_OUT_OF_MEMORY, "" );

		// Mark all slots as free.
		memset( _items, NIL, sizeof(ITEM) * capacity );

		m_hashmod = capacity - 1;

		return ALL_OK;
	}

	ERet add( const VALUE& item )
	{
		const VALUE* start = _items.begin();
		const VALUE* end = _items.end();
		const VALUE* ptr = std::lower_bound( start, end, item );
		if( (ptr != end) && (*ptr == item) ) {
			ptERROR("Trying to insert duplicate element!");
			return ERR_DUPLICATE_OBJECT;
		}
		int index = int( ptr - start );
        this->valueArray.Insert(index, val);
		return ALL_OK;
	}
	void addUnique( const VALUE& _item )
	{
	}

	bool contains( const U32 _key ) const
	{
		//mxASSERT(_key != NIL);
		//if( !m_capacity ) {
		//	return 0;
		//}
		//mxASSERT(m_capacity);

		const U32 hashmod = m_hashmod;
		U32 bucket = ComputeHash(_key) & hashmod;

		for( U32 probe = 0; probe <= hashmod; ++probe )
		{
			const ITEM& probe_item = _items[ bucket ];

			if( probe_item.key == _key ) {
				return probe_item.value;
			}

			if( probe_item.key == NIL ) {
				return _invalidValue;
			}

			// hash collision, quadratic probing
			bucket = (bucket + probe + 1) & hashmod;
		}

		mxUNREACHABLE2("Hash table is full");
		return _invalidValue;
	}

private:
	static mxFORCEINLINE U32 ComputeHash( const U32 _key )
	{
		U32 h = _key;

		// MurmurHash3 32-bit finalizer
		h ^= h >> 16;
		h *= 0x85ebca6bu;
		h ^= h >> 13;
		h *= 0xc2b2ae35u;
		h ^= h >> 16;

		return h;
	}
};
