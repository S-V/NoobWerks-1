/*
=============================================================================
	File:	TMemoryPool.h
	Desc:	Memory pool allocator template.
=============================================================================
*/

#ifndef MX_TEMPLATED_MEMORY_POOL_H__
#define MX_TEMPLATED_MEMORY_POOL_H__




mxSTOLEN("lifted and modified from CPool that was posted on flipcode");
mxBUG("memory leaks");

template< typename TYPE >
class TMemoryPool
{
	BYTE *			mPool;		// The allocation pool of items.
	BYTE *			mLast;		// End of the pool.
	BYTE *			mAvailable;	// Next available item.
	size_t			mPoolSize;	// Maximum number of items in the pool.
	size_t			mTOS;		// Top of the free stack.
	BYTE **			mFreeStack;	// The stack of deleted items.

	// New allocation pools are created and inserted into a doubly-linked list.
	TMemoryPool* const	mPrev;
	TMemoryPool*		mNext;

	const HMemory	mMemory;	// Handle to the memory manager

public:
	TMemoryPool()
		: mPrev( nil )
		, mNext( nil )
		, mMemory( EMemHeap::DefaultHeap )
	{
		clear();
	}
	TMemoryPool( HMemory hMemoryMgr )
		: mPrev( nil )
		, mNext( nil )
		, mMemory( hMemoryMgr )
	{
		clear();
	}
	~TMemoryPool()
	{
		Close();
	}
	void Setup( size_t maxPoolSize )
	{
		mxASSERT(maxPoolSize > 0);
		mPoolSize = maxPoolSize;

		size_t totalDataSize = maxPoolSize * sizeof(TYPE);

		mPool = AllocateMemory( totalDataSize );
		mFreeStack = c_cast(BYTE**) AllocateMemory( maxPoolSize );

		mAvailable = mPool;

		mLast = mPool + totalDataSize;
	}
	void Close()
	{
		if( mNext ) {
			mNext->~TMemoryPool();
			_releaseMemory( mNext );
			mNext = nil;
		}

		if( mPool ) {
			_releaseMemory( mPool );
			mPool = nil;
		}

		if( mFreeStack ) {
			_releaseMemory( mFreeStack );
			mFreeStack = nil;
		}

		clear();
	}

	void* New( const size_t size )
	{
		mxASSERT( sizeof(TYPE) == size );

		// If there are any holes in the free stack then fill them up.
		if ( mTOS == 0 )
		{
			// If there is any space left in this pool then use it, otherwise move
			// on to the next in the linked list.
			if ( mAvailable < mLast )
			{
				BYTE*	pReturn = mAvailable;
				mAvailable += sizeof(TYPE);
				return reinterpret_cast< void* >( pReturn );
			}
			else
			{
				// If there is another pool in the list then pass the request on to
				// it, otherwise try to create a new pool.
				if ( mNext ) {
					return mNext->New( size );
				} else {
					mNext = new_one(TMemoryPool( this ));
					if ( mNext ) {
						return mNext->New(size);
					}
				}
			}
		}
		return reinterpret_cast< void* >( mFreeStack[ --mTOS ] );
	}

	void Delete( void* pVoid )
	{
		if ( pVoid )
		{
			BYTE*	pItem = reinterpret_cast< BYTE* >( pVoid );

			// Check if the item being deleted is within this pool's memory range.
			if ( pItem < mPool || pItem >= mLast )
			{
				// If there is another pool in the list then try to delete from it,
				// otherwise call the generalised delete operator.
				if ( mNext ) {
					mNext->Delete( pItem );
				} else {
					delete( pVoid );
				}
			}
			else
			{
				// add a hole to the free stack.
				mFreeStack[ mTOS++ ] = pItem;

				// If this pool is empty and it is the last in the linked list
				// then delete it and set the previous pool to the last.
				if ( mPrev && !mNext &&
					static_cast< long >( mTOS * sizeof(TYPE) ) == mAvailable - mPool )
				{
					mPrev->mNext = 0;
					free_one( this );
				}
			}
		}
	}

private:

	void clear()
	{
		mLast = nil;
		mAvailable = mPool;
		mPoolSize = 0;
		mTOS = 0;

		// Reset all the pointers and indices, effectively deleting the allocated
		// items without actually calling their destructors.  This function should
		// be used with extreme caution since all sorts of horrible things can result
		// from it's misuse.
		//
		if ( mNext ) {
			mNext->clear();
		}
	}

	// add a new pool to the end of the linked list, this can only be called from
	// another memory pool.
	//
	TMemoryPool( TMemoryPool* const pPrev )
		: mPrev( pPrev )
		, mNext( nil )
		, mPoolSize( pPrev->mPoolSize )
		, mTOS( 0 )
		, mMemory( pPrev->mMemory )
	{
		Setup( mPoolSize );
	}

	mxFORCEINLINE BYTE* AllocateMemory( size_t numBytes )
	{
		return c_cast(BYTE*) F_GetMemoryManager( mMemory )->Allocate( numBytes );
	}
	mxFORCEINLINE void _releaseMemory( void* ptr )
	{
		F_GetMemoryManager( mMemory )->Free( ptr );
	}
};





// global overloaded operators 'new' and 'delete'

template< class T >
void * operator new( size_t size,  TMemoryPool<T> & rMemPool )
{
	return rMemPool.New( size );
}

template< class T >
void operator delete( void* p,  TMemoryPool<T> & rMemPool )
{
	return rMemPool.Delete( p );
}

#endif // MX_TEMPLATED_MEMORY_POOL_H__

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
