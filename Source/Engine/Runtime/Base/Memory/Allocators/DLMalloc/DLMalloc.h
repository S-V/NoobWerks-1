/*
=============================================================================
	File:	DLMalloc.h
	Desc:	
=============================================================================
*/
#pragma once

#if 0
/// Doug Lea Allocator (aka dlmalloc)
class DLMalloc: public AllocatorI
{
	mspace	m_mspace;
	size_t	m_initial_space_size;
public:
	DLMalloc()
		: m_mspace( nil )
		, m_initial_space_size( 0 )
	{}
	~DLMalloc()
	{
		mxASSERT(nil == m_mspace);
	}
	ERet Initialize( void* _memory, size_t _capacity )
	{
		m_mspace = create_mspace_with_base( _memory, _capacity, 0 );
		mxENSURE( m_mspace != nil, ERR_OUT_OF_MEMORY, "create_mspace_with_base() failed" );

		// no additional memory shall be obtained from the system
		// when this initial space is exhausted
		mspace_set_footprint_limit( m_mspace, 0 );	// set hard limit

		m_initial_space_size = mspace_mallinfo( m_mspace ).uordblks;	// total allocated space
		return ALL_OK;
	}
	void Shutdown()
	{
		mxASSERT2( mspace_mallinfo( m_mspace ).uordblks == m_initial_space_size, "HeapAllocator detected a Memory Leak!" );
		destroy_mspace( m_mspace );
		m_mspace = nil;
		m_initial_space_size = 0;
	}
	virtual void* Allocate( U32 _bytes, U32 alignment ) override
	{
		//extern void* mspace_memalign(mspace msp, size_t alignment, size_t bytes);
		return mspace_memalign( m_mspace, (size_t)alignment, (size_t)_bytes );
	}
	virtual void Deallocate( const void* _memory ) override
	{
		//extern void* mspace_free(mspace msp, void* mem);
		mspace_free( m_mspace, _memory );
	}
	virtual U32	usableSize( const void* _memory ) const
	{
		//extern size_t mspace_usable_size(const void*);
		return (U32)mspace_usable_size( _memory );
	}
};
#endif

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
