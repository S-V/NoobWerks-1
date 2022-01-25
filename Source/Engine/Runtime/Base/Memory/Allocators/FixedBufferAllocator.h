/*
=============================================================================
	File:	FixedBufferAllocator.h
	Desc:	
=============================================================================
*/
#pragma once

#include <Base/Memory/Allocator.h>

mxSTOLEN("Bitsquid foundation library");

/// A temporary memory allocator that primarily allocates memory from a
/// local stack buffer of size BUFFER_SIZE. If that memory is exhausted it will
/// use the backing allocator (typically a scratch allocator).
///
/// Memory allocated with a FixedBufferAllocator does not have to be deallocated. It is
/// automatically deallocated when the FixedBufferAllocator is destroyed.
template <int BUFFER_SIZE>
class FixedBufferAllocator: public AllocatorI
{
	char			_buffer[BUFFER_SIZE];	//< Local stack buffer for allocations.
	AllocatorI *	_backing_allocator;		//< Backing allocator if local memory is exhausted.
	char *			_start;				//< Start of current allocation region
	char *			_cursor;					//< Current allocation pointer.
	char *			_end;					//< End of current allocation region
	unsigned		_chunk_size;		//< Chunks to allocate from backing allocator

public:
	/// Creates a new temporary allocator using the specified backing allocator.
	FixedBufferAllocator( AllocatorI * backing = nil );
	virtual ~FixedBufferAllocator();

	virtual void* Allocate( U32 _bytes, U32 alignment );
	
	/// Deallocation is a NOP for the FixedBufferAllocator. The memory is automatically
	/// deallocated when the FixedBufferAllocator is destroyed.
	virtual void Deallocate(const void*) override {}

	/// Returns SIZE_NOT_TRACKED.
	virtual U32 GetUsableSize(const void*) const override {return SIZE_NOT_TRACKED;}

	/// Returns SIZE_NOT_TRACKED.
	virtual U32 total_allocated() const override {return SIZE_NOT_TRACKED;}
};

// If possible, use one of these predefined sizes for the FixedBufferAllocator to avoid
// unnecessary template instantiation.
typedef FixedBufferAllocator<64> TempAllocator64;
typedef FixedBufferAllocator<128> TempAllocator128;
typedef FixedBufferAllocator<256> TempAllocator256;
typedef FixedBufferAllocator<512> TempAllocator512;
typedef FixedBufferAllocator<1024> TempAllocator1024;
typedef FixedBufferAllocator<2048> TempAllocator2048;
typedef FixedBufferAllocator<4096> TempAllocator4096;
typedef FixedBufferAllocator<8192> TempAllocator8192;

// -----------------------------------------------------------
// Inline function implementations
// -----------------------------------------------------------

template <int BUFFER_SIZE>
FixedBufferAllocator<BUFFER_SIZE>::FixedBufferAllocator(AllocatorI * backing)
	: _backing_allocator(backing)
	, _chunk_size(4*1024)
{
	_cursor = _start = _buffer;
	_end = _start + BUFFER_SIZE;
	*(void **)_start = nil;
	_cursor += sizeof(void *);
}

template <int BUFFER_SIZE>
FixedBufferAllocator<BUFFER_SIZE>::~FixedBufferAllocator()
{
	void *p = *(void **)_buffer;
	while (p) {
		void *next = *(void **)p;

		if( p != _buffer ) {
			_backing_allocator->Deallocate(p);
		}

		p = next;
	}
}

template <int BUFFER_SIZE>
void * FixedBufferAllocator<BUFFER_SIZE>::Allocate( U32 _bytes, U32 alignment )
{
	_cursor = (char *)memory::align_forward( _cursor, alignment );

	const U32 bytes_available = ( _end - _cursor );

	if( _bytes > bytes_available )
	{
		if( !_backing_allocator )
		{
			UNDONE;
			return nil;
		}

		U32 to_allocate = sizeof(void *) + _bytes + alignment;
		if (to_allocate < _chunk_size)
			to_allocate = _chunk_size;
		_chunk_size *= 2;
		void *p = _backing_allocator->Allocate( to_allocate, alignment );
		*(void **)_start = p;
		_cursor = _start = (char *)p;
		_end = _start + to_allocate;
		*(void **)_start = 0;
		_cursor += sizeof(void *);
		_cursor = (char *)memory::align_forward(_cursor, alignment);
	}

	//
	void *result = _cursor;
	_cursor += _bytes;
	return result;
}
