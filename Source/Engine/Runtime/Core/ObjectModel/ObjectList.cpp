//
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>

#include <Core/ObjectModel/ObjectList.h>


mxOPTIMIZE("use index-based free list to waste less memory when stride is less than pointer size");
static mxFORCEINLINE
U32 GetStrideForFreeList(const TbMetaClass& type)
{
	return largest(type.m_size, sizeof(void*));
}

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS_NO_DEFAULT_CTOR( NwObjectList );
mxBEGIN_REFLECTION( NwObjectList )
	mxMEMBER_FIELD( _next ),
	mxMEMBER_FIELD( m_type ),
	mxMEMBER_FIELD( m_freeList ),
	mxMEMBER_FIELD( m_liveCount ),
	mxMEMBER_FIELD( m_capacity ),
	mxMEMBER_FIELD( m_flags ),
mxEND_REFLECTION

NwObjectList::NwObjectList( const TbMetaClass& type, U32 capacity )
{
	//DBGOUT("Creating object buffer (type: %s, num: %u)\n", type.GetTypeName(), capacity);
	{
		const U32 itemSize = type.GetInstanceSize();
		const U32 alignment = type.m_align;
		mxASSERT( itemSize > 0 );
		mxASSERT( alignment > 0 );
		mxASSERT2( itemSize % alignment == 0, "Object size is not a multiple of its alignment!" );
		mxASSERT( itemSize == AlignUp( itemSize, alignment ) );
	}

	m_type = &type;

	const U32 stride = GetStrideForFreeList(type);

	// allocate a new memory buffer
	const size_t bufferSize = capacity * stride;
	void* allocatedBuffer = MemoryHeaps::clumps().Allocate( bufferSize, EFFICIENT_ALIGNMENT );
	mxASSERT(IS_16_BYTE_ALIGNED(allocatedBuffer));

	if(MX_DEBUG)
	{
		memset(allocatedBuffer, mxDBG_UNINITIALIZED_MEMORY_TAG, bufferSize);
	}

	// add all items to the free list
	m_freeList.Initialize( allocatedBuffer, stride, capacity );

	m_capacity = capacity;
	m_liveCount = 0;

	m_flags = NwObjectList::CanFreeMemory;
}

NwObjectList::~NwObjectList()
{
	this->clear();
	m_flags = 0;
}

const TbMetaClass& NwObjectList::getType() const
{
	return *m_type;
}

U32 NwObjectList::stride() const
{
	return GetStrideForFreeList(*m_type);
}

bool NwObjectList::hasAddress( const void* ptr ) const
{
	return m_freeList.hasAddress( ptr );
}

bool NwObjectList::hasValidItem( const void* item ) const
{
	return this->containsItem( item ) && !m_freeList.ItemInFreeList( item );
}

bool NwObjectList::containsItem( const void* item ) const
{
	return this->IndexOfContainedItem( item ) != INDEX_NONE;
}

U32 NwObjectList::IndexOfContainedItem( const void* item ) const
{
	if( m_freeList.hasAddress( item ) )
	{
		const U32 itemSize = this->stride();
		const U32 byteOffset = mxGetByteOffset32( m_freeList.m_start, item );
		mxASSERT( byteOffset % itemSize == 0 );
		return byteOffset % itemSize;
	}
	return INDEX_NONE;
}

ERet NwObjectList::FreeItem( void* item )
{
	mxASSERT(this->hasValidItem( item ));
	mxASSERT(m_liveCount > 0);

	// assume that the object has already been destructed
//	DBGOUT( "~%s( 0x%p ) [%u]", m_type->GetTypeName(), item, m_liveCount-1 );
	//m_type->DestroyInstance( item );

	if(MX_DEBUG) { memset(item, mxDBG_UNINITIALIZED_MEMORY_TAG, this->stride()); }

	m_liveCount--;

	// insert the destroyed object into the free list
	m_freeList.releaseSorted( item );	// the free list should be sorted by increasing addresses

	return ALL_OK;
}

CStruct* NwObjectList::Allocate()
{
	CStruct* newItem = c_cast(CStruct*) m_freeList.allocItem();
	if( newItem != nil ) {
		m_liveCount++;
		return newItem;
	}
	return nil;
}

U32 NwObjectList::count() const
{
	return m_liveCount;
}

U32 NwObjectList::capacity() const
{
	return m_capacity;
}

CStruct* NwObjectList::raw() const
{
	return c_cast(CStruct*) m_freeList.m_start;
}

CStruct* NwObjectList::itemAtIndex( U32 itemIndex ) const
{
	mxASSERT( itemIndex < m_capacity );
	void* o = mxAddByteOffset( this->raw(), itemIndex * this->stride() );
	return c_cast(CStruct*) o;
}

CStruct* NwObjectList::getFirstFreeItem() const
{
	return c_cast(CStruct*) m_freeList._first_free;
}

size_t NwObjectList::usableSize() const
{
	return this->stride() * m_liveCount;
}

void NwObjectList::empty()
{
	IteratorBase	it( *this );
	while( it.IsValid() )
	{
		void* o = it.ToVoidPtr();
		m_type->DestroyInstance( o );
		it.MoveToNext();
	}
	m_liveCount = 0;

	// add all items to the free list
	m_freeList.Initialize( m_freeList.GetBuffer(), this->stride(), m_capacity );
}

void NwObjectList::clear()
{
	// call destructors
	IteratorBase	it( *this );
	while( it.IsValid() )
	{
		void* o = it.ToVoidPtr();
		m_type->DestroyInstance( o );
		it.MoveToNext();
	}
	m_liveCount = 0;

	// release allocated memory (if we own it)
	if( m_flags & NwObjectList::CanFreeMemory )
	{
		void* allocatedBuffer = m_freeList.GetBuffer();
		MemoryHeaps::clumps().Deallocate( allocatedBuffer/*, m_capacity * this->stride()*/ );
		m_freeList.clear();
	}
	m_capacity = 0;

	m_flags = 0;
}

void NwObjectList::IterateItems( Reflection::AVisitor* visitor, void* userData ) const
{
	const TbMetaClass& itemType = this->getType();

	IteratorBase	it( *this );
	while( it.IsValid() )
	{
		void* o = it.ToVoidPtr();
		Reflection::Walker::Visit( o, itemType, visitor, userData );
		it.MoveToNext();
	}
}
void NwObjectList::IterateItems( Reflection::AVisitor2* visitor, void* userData ) const
{
	const TbMetaClass& itemType = this->getType();

	IteratorBase	it( *this );
	while( it.IsValid() )
	{
		void* o = it.ToVoidPtr();
		Reflection::Walker2::Visit( o, itemType, visitor, userData );
		it.MoveToNext();
	}
}

void NwObjectList::DbgCheckPointers()
{
#if MX_DEBUG
	Reflection::PointerChecker	pointerChecker;
	this->IterateItems( &pointerChecker, nil );
#endif // MX_DEBUG
}

//---------------------------------------------------------------------
//	NwObjectList::IteratorBase
//---------------------------------------------------------------------
//
NwObjectList::IteratorBase::IteratorBase()
{
	this->Reset();
}

NwObjectList::IteratorBase::IteratorBase( const NwObjectList& objectList )
{
	this->Initialize( objectList );
}

bool NwObjectList::IteratorBase::IsValid() const
{
	return m_remaining > 0;
}

/// Moves the iterator to the next valid object.
void NwObjectList::IteratorBase::MoveToNext()
{
	mxASSERT(m_remaining > 0);
	do
	{
		// move to the next live object
		m_current = mxAddByteOffset( m_current, m_stride );
		m_remaining--;
		// move forward until we hit an active object
		if( m_current != m_nextFree ) {
			break;	// the current object is live (i.e. not in the free list)
		}
		m_nextFree = *(void**)m_nextFree;
	}
	while( m_remaining );
}

// Skips over the given number of objects.
void NwObjectList::IteratorBase::Skip( U32 count )
{
	mxASSERT(count <= m_remaining);
	while( count-- )
	{
		this->MoveToNext();
	}
}

U32 NwObjectList::IteratorBase::NumRemaining() const
{
	return m_remaining;
}

U32 NwObjectList::IteratorBase::NumContiguousObjects() const
{
	return ( m_current < m_nextFree ) ?
		mxGetByteOffset32( m_current, m_nextFree ) / m_stride
		:
		m_remaining
		;
}

void NwObjectList::IteratorBase::Initialize( const NwObjectList& objectList )
{
	m_current		= objectList.raw();
	m_nextFree		= objectList.getFirstFreeItem();
	m_stride		= objectList.stride();
	m_remaining		= objectList.capacity();

	// skip free-listed elements and move to the first valid object
	while( m_remaining )
	{
		// move forward until we hit a live object
		if( m_current != m_nextFree ) {
			break;	// the current object is live (i.e. not in the free list)
		}
		// move past free-listed objects
		m_current = mxAddByteOffset( m_current, m_stride );
		m_nextFree = *(void**)m_nextFree;
		m_remaining--;
	}
}

void NwObjectList::IteratorBase::Reset()
{
	m_current	= nil;
	m_nextFree	= nil;
	m_stride	= 0;
	m_remaining	= 0;
}

void* NwObjectList::IteratorBase::ToVoidPtr() const
{
	return m_current;
}
