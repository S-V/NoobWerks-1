/*
=============================================================================
	File:	DefaultHeap.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Memory/DefaultHeap/DefaultHeap.h>

namespace
{
	struct MemoryBlockHeader
	{
		U32	usable_size;
		U32	alignment;
	};
}//namespace

DefaultHeap::DefaultHeap()
{
}

void* DefaultHeap::Allocate( U32 _bytes, U32 alignment )
{

#if MX_ENABLE_MEMORY_TRACKING

	const U32	padded_header_size = tbALIGN( sizeof(MemoryBlockHeader), alignment );
	const U32	bytes_to_allocate_including_header = padded_header_size + _bytes;

	void *	allocated_memory_block = ::_aligned_malloc( bytes_to_allocate_including_header, alignment );

	if( allocated_memory_block )
	{
		MemoryBlockHeader *	memory_block_header = (MemoryBlockHeader*) allocated_memory_block;

		memory_block_header->usable_size = _bytes;
		memory_block_header->alignment = alignment;

		void *	returned_address = mxAddByteOffset( allocated_memory_block, padded_header_size );

		*( ((void**)returned_address) - 1 ) = allocated_memory_block;

		return returned_address;
	}

	return nil;

#else

	return ::_aligned_malloc( _bytes, alignment );

#endif

}

void DefaultHeap::Deallocate( const void* memory_block )
{

#if MX_ENABLE_MEMORY_TRACKING

	if( memory_block )
	{
		void *	allocated_memory_block = *( ((void**)memory_block) - 1 );

		MemoryBlockHeader *	memory_block_header = (MemoryBlockHeader*) allocated_memory_block;
		const U32	padded_header_size = tbALIGN( sizeof(MemoryBlockHeader), memory_block_header->alignment );

		::_aligned_free( allocated_memory_block );
	}

#else

	::_aligned_free( (void*) memory_block );

#endif

}

U32	DefaultHeap::GetUsableSize( const void* memory_block ) const
{

#if MX_ENABLE_MEMORY_TRACKING

	void *	allocated_memory_block = *( ((void**)memory_block) - 1 );

	MemoryBlockHeader *	memory_block_header = (MemoryBlockHeader*) allocated_memory_block;

	return memory_block_header->usable_size;

#else

	return (U32)::_aligned_msize( (void*)memory_block, EFFICIENT_ALIGNMENT, 0 /*offset*/ );

#endif

}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
