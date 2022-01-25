/*
=============================================================================
	File:	TConcurrentArray.h
	Desc:	
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/ArrayDescriptor.h>


/*
----------------------------------------------------------
	A very simple concurrent/atomic non-growable array
	which uses an atomic/interlocked counter.

	It's typically used to gather results
	from parallel jobs/tasks / worker threads.
----------------------------------------------------------
*/
template< typename ITEM >
class TConcurrentArray: NonCopyable
{
	ITEM *		_data;
	AtomicInt	_count;
	const U32	_capacity;
	AllocatorI &		_allocator;

public:
	TConcurrentArray(
		const U32 capacity
		, AllocatorI& allocator
		)
		: _data( nil )
		, _count( 0 )
		, _capacity( capacity )
		, _allocator( allocator )
	{
	}

	~TConcurrentArray()
	{
		if( _data )
		{
			TDestructN_IfNonPOD( _data, this->num() );
			_allocator.Deallocate( _data );
		}
	}

	ERet allocateMemoryForMaxItems()
	{
		mxASSERT(nil == _data);
		_data = (ITEM*) _allocator.Allocate(
			sizeof(_data[0]) * _capacity
			, mxALIGNOF(ITEM)
			);
		mxENSURE(_data, ERR_OUT_OF_MEMORY, "");
		return ALL_OK;
	}

	void AppendInterlocked( const ITEM& new_item )
	{
		const U32 new_item_index = AtomicIncrement( &_count ) - 1;
		mxASSERT( new_item_index < _capacity );

		new( &_data[ new_item_index ] ) ITEM( new_item );
	}

	mxFORCEINLINE U32 num() const { return AtomicLoad( _count ); }

	mxFORCEINLINE TSpan< ITEM > GetSpan() { return TSpan< ITEM >( _data, _count ); }

	mxFORCEINLINE bool IsEmpty() const { return this->num() == 0; }
};
