/*
=============================================================================
	File:	DefaultHeap.h
	Desc:	
=============================================================================
*/
#pragma once

/// wraps _aligned_malloc/_aligned_free
class DefaultHeap: public AllocatorI
{
public:
	DefaultHeap();
	virtual void* Allocate( U32 size, U32 alignment ) override;
	virtual void Deallocate( const void* memory_block ) override;
	virtual U32	GetUsableSize( const void* memory_block ) const override;
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
