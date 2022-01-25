/*
=============================================================================
	File:	DebugHeap.h
	Desc:	
=============================================================================
*/
#pragma once

#include <Base/Memory/Allocator.h>

/// wraps Insomniac Games' DebugHeap
class IGDebugHeap: public AllocatorI, public TSingleInstance<IGDebugHeap>
{
	TPtr< struct DebugHeap >	pimpl;
	mutable SpinWait			CS;

public:
	IGDebugHeap();

	void Initialize( size_t debugHeapSize );
	void Shutdown();
	bool isInitialized() const { return pimpl.IsValid(); }

	virtual void* Allocate( U32 numBytes, U32 alignment ) override;
	virtual void Deallocate( const void* memory ) override;
	virtual U32	GetUsableSize( const void* memory ) const override;
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
