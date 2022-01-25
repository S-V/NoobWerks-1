/*
=============================================================================
	File:	Queue.h
	Desc:	A FIFO circular queue.
	ToDo:	Stop reinventing the wheel.
=============================================================================
*/

#ifndef __MX_CONTAINTERS_FIFO_QUEUE_H__
#define __MX_CONTAINTERS_FIFO_QUEUE_H__



//
//	TQueue< TYPE >
//
template< typename TYPE >
class TQueue
	: public TArrayBase< TYPE, TQueue<TYPE> >
{
	// queue memory management
	TArray< TYPE >	mData;

public:
	typedef TQueue
	<
		TYPE
	>
	THIS_TYPE;

	typedef
	TYPE
	ITEM_TYPE;

	typedef TypeTrait
	<
		TYPE
	>
	TYPE_TRAIT;

public:
	// Creates a zero length queue.
	mxFORCEINLINE TQueue()
	{}

	// Creates a zero length queue and sets the memory manager.
	mxFORCEINLINE explicit TQueue( HMemory hMemoryMgr )
		: mData( hMemoryMgr )
	{}

	// Use it only if you know what you're doing.
	mxFORCEINLINE explicit TQueue(ENoInit)
		: mData( EMemHeap::DefaultHeap )
	{}

	// Deallocates queue memory.
	mxFORCEINLINE ~TQueue()
	{
		this->clear();
	}

	// Returns the total capacity of the queue storage.
	mxFORCEINLINE UINT capacity() const
	{
		return mData.capacity();
	}

	// Convenience function to get the number of elements in this queue.
	mxFORCEINLINE UINT Num() const
	{
		return mData.num();
	}

	mxFORCEINLINE void reserve( UINT numElements )
	{
		mData.reserve( numElements );
	}

	// Convenience function to empty the queue. Doesn't release allocated memory.
	// Invokes objects' destructors.
	mxFORCEINLINE void empty()
	{
		mData.empty();
	}

	// Releases allocated memory (calling destructors of elements) and empties the queue.
	mxFORCEINLINE void clear()
	{
		mData.clear();
	}

	// Accesses the element at the front of the queue but does not remove it.
	mxFORCEINLINE TYPE& Peek()
	{
		return mData.getFirst();
	}
	mxFORCEINLINE const TYPE& Peek() const
	{
		return mData.getFirst();
	}

	// Places a new element to the back of the queue and expand storage if necessary.
	mxFORCEINLINE void Enqueue( const TYPE& newOne )
	{
		mData.add( newOne );
	}

	// Fills in the data with the element at the front of the queue
	// and removes the element from the front of the queue.
	mxFORCEINLINE void Deque( TYPE &element )
	{
		element = mData.getFirst();
		mData.RemoveAt( 0 );
	}
	mxFORCEINLINE void Deque()
	{
		mData.RemoveAt( 0 );
	}

	mxFORCEINLINE void add( const TYPE& newOne )
	{
		Enqueue( newOne );
	}
	mxFORCEINLINE TYPE& add()
	{
		return mData.add();
	}

	mxFORCEINLINE TYPE* Raw() {
		return mData.raw();
	}
	mxFORCEINLINE const TYPE* Raw() const {
		return mData.raw();
	}


public_internal:
	void SetNum_Unsafe( UINT newNum )
	{
		mData.SetNum_Unsafe( newNum );
	}

private:
	NO_ASSIGNMENT(THIS_TYPE);
	NO_COPY_CONSTRUCTOR(THIS_TYPE);
	NO_COMPARES(THIS_TYPE);
};



#endif // ! __MX_CONTAINTERS_QUEUE_H__

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
