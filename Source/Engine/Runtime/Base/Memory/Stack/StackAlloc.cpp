/*
=============================================================================
	File:	StackAlloc.cpp
	Desc:	Stack-based memory allocator.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Memory/Stack/StackAlloc.h>

/*
----------------------------------------------------------
	LinearAllocator
----------------------------------------------------------
*/
LinearAllocator::LinearAllocator()
{
	m_offset = 0;
	m_capacity = 0;
	m_memory = nil;
	m_owner = nil;
}

ERet LinearAllocator::Initialize( void* memory, U32 capacity )
{
	chkRET_X_IF_NIL(memory, ERR_NULL_POINTER_PASSED);
	chkRET_X_IF_NOT(capacity > 0, ERR_INVALID_PARAMETER);
	chkRET_X_IF_NOT(IS_16_BYTE_ALIGNED(memory), ERR_INVALID_PARAMETER);
	m_memory = (char*) memory;
	m_offset = 0;
	m_capacity = capacity;
	return ALL_OK;
}

void LinearAllocator::Shutdown()
{
	m_memory = nil;
	m_offset = 0;
	m_capacity = 0;
}

void* LinearAllocator::Allocate( U32 _bytes, U32 alignment )
{
	mxASSERT(alignment >= MINIMUM_ALIGNMENT && IsPowerOfTwo(alignment));
	// align current allocation offset
	const U32 alignedOffset = tbALIGN( m_offset, alignment );

	//const U32 paddedSize = tbALIGN( _bytes, alignment );
	const U32 paddedSize = _bytes;

	const U32 newOffset = m_offset + paddedSize;

	// check if there's enough space
	if( newOffset + paddedSize <= m_capacity )
	{
		m_offset = newOffset;
		char* alignedAddress = mxAddByteOffset( m_memory, alignedOffset );
//#if MX_DEBUG
//	memset(alignedAddress, 0xCDCDCDCD, size);	// 0xCDCDCDCD is MVC++-specific (uninitialized heap memory)
//#endif
		return alignedAddress;
	}

	// this is non-standard behaviour ("by the book", we should throw an out-of-memory exception)
	return nil;
}

ERet LinearAllocator::AlignTo( U32 alignment )
{
	mxASSERT(IsValidAlignment(alignment));

	//const U32 alignedOffset = ((m_offset + alignment-1) / alignment) * alignment;
	const U32 alignedOffset = AlignUp( m_offset, alignment );
	if( alignedOffset >= m_capacity ) {
		return ERR_OUT_OF_MEMORY;
	}
	m_offset = alignedOffset;

	return ALL_OK;
}

void LinearAllocator::FreeTo( U32 marker )
{
	mxASSERT( marker <= m_offset );
	// simply reset the pointer to the old position
	m_offset = marker;
#if MX_DEBUG
	U32 size = m_offset - marker;
	void* start = mxAddByteOffset(m_memory, marker);
	memset(start, 0xFEEEFEEE, size);	// 0xFEEEFEEE is MVC++-specific (freed memory)
#endif
}

void LinearAllocator::Reset()
{
	this->FreeTo( 0 );
}

U32 LinearAllocator::capacity() const
{
	return m_capacity;
}

U32 LinearAllocator::Position() const
{
	return m_offset;
}

/*
----------------------------------------------------------
	MemoryScope
----------------------------------------------------------
*/
MemoryScope::MemoryScope( LinearAllocator & alloc )
	: m_alloc( alloc )
	, m_parent( nil )
{
}

MemoryScope::MemoryScope( MemoryScope * parent )
	: m_alloc( parent->m_alloc )
	, m_parent( parent )
{
}

MemoryScope::~MemoryScope()
{
	Destructor* current = m_destructors;
	while( current )
	{
		(*current->fun)(current->object);
		current = current->next;
	}
	if( m_alloc.Owner() == this )
	{
		m_alloc.Reset();
		m_alloc.TransferOwnership( this, m_parent );
	}
}

void* MemoryScope::Alloc( U32 size, U32 alignment )
{
	mxASSERT( !this->IsSealed() );
	return m_alloc.Allocate( size, alignment );
}

void MemoryScope::Unwind()
{
	mxASSERT( m_alloc.Owner() == this );
	Destructor* current = m_destructors;
	while( current )
	{
		(*current->fun)(current->object);
		current = current->next;
	}
	m_destructors = nil;
	m_alloc.Reset();
}

void MemoryScope::Seal()
{
	mxASSERT( !this->IsSealed() );
	m_alloc.TransferOwnership( this, m_parent );
}

bool MemoryScope::IsSealed() const
{
	return m_alloc.Owner() != this;
}

ERet MemoryScope::AddDestructor( void* object, FDestruct fn )
{
	mxASSERT( object );
	mxASSERT_PTR( fn );
	const U32 top = m_alloc.Position();
	Destructor* dtor = (Destructor*) m_alloc.Allocate( sizeof(Destructor) );
	if( dtor )
	{
		this->AddDestructor( dtor, object, fn );
		return ALL_OK;
	}
	m_alloc.FreeTo(top);
	// NOTE: this is non-standard behaviour
	return ERR_OUT_OF_MEMORY;
}

U32 MemoryScope::capacity() const
{
	return m_alloc.capacity();
}

U32 MemoryScope::Position() const
{
	return m_alloc.Position();
}

void* MemoryScope::AllocMany( U32 count, U32 stride, FDestruct* fun )
{
	mxASSERT( m_alloc.Owner() == this );
	const U32 top = m_alloc.Position();
	m_alloc.AlignTo(DEFAULT_ALIGNMENT);

	Destructor* dtors = nil;
	if( fun )
	{
		dtors = (Destructor*) m_alloc.Allocate( count * sizeof(Destructor) );
		if( !dtors ) {
			goto L_Error;
		}
	}

	void* objects = m_alloc.Allocate( count * stride );
	if( !objects ) {
		goto L_Error;
	}

	if( fun ) {
		this->AddDestructors( dtors, count, objects, stride, fun );
	}

	return objects;

L_Error:
	m_alloc.FreeTo(top);
	// NOTE: this is non-standard behaviour
	return nil;
}

void MemoryScope::AddDestructor( Destructor* dtor, void* object, FDestruct* fun )
{
	dtor->fun = fun;
	dtor->next = m_destructors;
	dtor->object = object;
	m_destructors = dtor;
}

void MemoryScope::AddDestructors( Destructor* dtors, U32 count, void* objects, U32 stride, FDestruct* fun )
{
	Destructor* current = m_destructors;
	for( U32 iDtor = 0; iDtor < count; iDtor++ )
	{
		dtors[iDtor].fun = fun;
		dtors[iDtor].next = current;
		dtors[iDtor].object = objects;
		current = m_destructors + iDtor;
		objects = mxAddByteOffset(objects, stride);
	}
	m_destructors = current;
}

/*
Virtual memory aware linear allocator
https://nfrechette.github.io/2015/06/11/vmem_linear_allocator/
*/

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
