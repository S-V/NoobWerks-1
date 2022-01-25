/*
=============================================================================
	File:	TIndexedPool.h
	Desc:	Index-based growing free list.
=============================================================================
*/
#pragma once

#include <Base/Memory/MemoryBase.h>
#include <Base/Template/Containers/Array/DynamicArray.h>

template< class TYPE, typename INDEX >
void TBuildFreeList(
					TYPE * array_start,	// the start of the whole array
					U32 first_index,	// number of 'live' elements (>=0)
					U32 array_count,	// the size of the array
					INDEX null_handle = -1
					)
{
	mxSTATIC_ASSERT(sizeof(array_start[0]) >= sizeof(INDEX));
	struct UPtr {
		INDEX	next;
	};
	U32 i = first_index;
	while( i < array_count )
	{
		((UPtr&)array_start[i]).next = INDEX( i + 1 );
		i++;
	}
	// The tail of the free list points to NULL.
	((UPtr&)array_start[i-1]).next = null_handle;
}

template< class TYPE >	// where TYPE is POD
struct TIndexedPool
	: public TArrayMixin< TYPE, TIndexedPool< TYPE > >
{
	typedef DynamicArray< TYPE > BufferT;	// 'Storage' type
	typedef U32 IndexT;	// 'Handle' type

	BufferT		m_items;		// memory for storing items
	U32			m_liveCount;	// number of active objects (they may not be stored contiguously)
	IndexT		m_firstFree;	// index of the first free item (head of the linked list)

	static const IndexT NIL_HANDLE = (IndexT)~0Ul;

	struct UPtr {
		IndexT	nextFree;
	};
	static inline UPtr& AsUPtr( TYPE& o ) {
		return *c_cast(UPtr*) &o;
	}
	mxSTATIC_ASSERT(sizeof(TYPE) >= sizeof(UPtr));

public:
	TIndexedPool( AllocatorI & heap )
		: m_items( heap )
	{
		m_liveCount = 0;
		m_firstFree = NIL_HANDLE;
	}

	~TIndexedPool()
	{
		mxASSERT2( m_liveCount == 0, "Some handles haven't been released!" );
	}

	//=== TArrayBase
	// NOTE: returns the array capacity, not the number of live objects!
	inline UINT num() const			{ return m_items.num(); }
	inline TYPE * raw()				{ return m_items.raw(); }
	inline const TYPE* raw() const	{ return m_items.raw(); }

	inline UINT num_live_items() const	{ return m_liveCount; }

	mxFORCEINLINE TYPE& GetAt(const IndexT index)
	{
		return m_items[ index ];
	}
	mxFORCEINLINE const TYPE& GetAt(const IndexT index) const
	{
		return m_items[ index ];
	}

	ERet resize( UINT num_elements )
	{
		mxASSERT(m_liveCount == 0 && m_firstFree == NIL_HANDLE);

		mxDO(m_items.setCountExactly( num_elements ));

		TBuildFreeList( m_items.raw(), 0, m_items.num(), NIL_HANDLE );

		m_liveCount = 0;

		m_firstFree = 0;

		return ALL_OK;
	}

	void empty()
	{
		m_liveCount = 0;
		if( m_items.num() ) {
			TBuildFreeList( m_items.raw(), 0, m_items.num(), NIL_HANDLE );
			m_firstFree = 0;
		} else {
			mxASSERT( m_firstFree == NIL_HANDLE );
		}
	}

	// Tries to find a free slot, doesn't allocate memory.
	IndexT GrabFreeIndexIfAvailable()
	{
		if( m_firstFree != NIL_HANDLE )
		{
			const IndexT freeIndex = m_firstFree;
			const UPtr& listNode = AsUPtr( m_items[ freeIndex ] );
			m_firstFree = listNode.nextFree;
			m_liveCount++;
			//DBGOUT("Live count <- %d, m_firstFree <- %d", m_liveCount, m_firstFree);
			return freeIndex;
		}
		return NIL_HANDLE;
	}

	///
	IndexT GrabFreeIndex()
	{
		const IndexT freeIndex = this->GrabFreeIndexIfAvailable();
		mxASSERT( freeIndex != NIL_HANDLE );
		return freeIndex;
	}

	/// Tries to find a free slot, allocates memory if needed.
	IndexT Alloc()
	{
		// First check if the array needs to grow.
		const U32 oldCount = m_items.num();
		if(mxUNLIKELY( m_liveCount >= oldCount ))
		{
			mxASSERT2( m_liveCount < NIL_HANDLE, "Too many objects" );
			mxASSERT2( m_firstFree == NIL_HANDLE, "Tail free list must be NULL" );

			// Grow the array by doubling its capacity.
			const U32 newCount = NextPowerOfTwo( oldCount );
			m_items.setNum( newCount );

			TYPE* a = m_items.raw();

			TBuildFreeList( a, oldCount, newCount, NIL_HANDLE );

			// Free list head now points to the first added item.
			m_firstFree = oldCount;
		}
		// Now that we have ensured enough space, there must be an available slot.
		return this->GrabFreeIndex();
	}

	void ReleaseItemAtIndex( const IndexT id )
	{
		mxASSERT2(m_liveCount > 0, "Cannot delete items from empty pool");

		TYPE* a = m_items.raw();

		TYPE& o = a[ id ];
		//o.~TYPE();
		AsUPtr( o ).nextFree = m_firstFree;

		m_firstFree = id;

		--m_liveCount;
	}

	size_t GetUsedAllocatedMemory() const
	{
		return m_liveCount * sizeof(TYPE);
	}

public:	// Binary Serialization.

	friend AWriter& operator << ( AWriter& file, const TIndexedPool< TYPE >& o )
	{
		//const U32 numAllocated = o.m_items.num();
		const U32 num_live_items = o.m_liveCount;
		const U32 firstFreeIdx = o.m_firstFree;
		file << num_live_items;
		file << firstFreeIdx;
		file << o.m_items;
		return file;
	}

	friend AReader& operator >> ( AReader& file, TIndexedPool< TYPE >& o )
	{
		U32 num_live_items;
		file >> num_live_items;
		o.m_liveCount = num_live_items;

		U32 firstFreeIdx;
		file >> firstFreeIdx;
		o.m_firstFree = firstFreeIdx;

		file >> o.m_items;

		return file;
	}
};
