/*
=============================================================================
	File:	StackAlloc.h
	Desc:	Stack- and scope-based memory allocators.
	Note:	inspired by Brooke Hodgman (author of the Eighth Engine), see:
http://eighthengine.com/2013/08/30/a-memory-management-adventure-2/
https://code.google.com/p/eight/source/browse/include/eight/core/alloc/scope.h
=============================================================================
*/
#pragma once

/*
----------------------------------------------------------
	LinearAllocator
	aka Stack-based/LIFO (last-in-first-out) _allocator
----------------------------------------------------------
*/
class LinearAllocator
{
	char *		m_memory;	//!< start of the memory region
	U32			m_offset;	//!< current allocation offset
	U32			m_capacity;	//!< size of the memory region
	const void *m_owner;	//!< used by MemoryScope
public:
	LinearAllocator();

	ERet Initialize( void* memory, U32 capacity );
	void Shutdown();

	//NOTE: compatible with AllocatorI::Allocate()
	void* Allocate( U32 _bytes, U32 alignment = sizeof(void*) );

	//char* Alloc( U32 size );

	ERet AlignTo( U32 alignment );
	void FreeTo( U32 marker );
	void Reset();

	U32 capacity() const;
	U32 Position() const;

	void* GetBufferPtr() { return m_memory; }

	void TransferOwnership( const void* from, const void* to )
	{
		mxASSERT(from == m_owner);
		m_owner = to;
	}
	const void* Owner() const
	{
		return m_owner;
	}

	bool contains( const void* _memory ) const
	{
		return _memory >= m_memory && _memory < mxAddByteOffset(m_memory, m_capacity);
	}
};

/*
----------------------------------------------------------
	StackAllocator
	frees the memory upon leaving the scope
----------------------------------------------------------
*/
class StackAllocator: public AllocatorI
{
	LinearAllocator &	_allocator;	//!<
	AllocatorI &				_backing;	//!< Backing _allocator if the stack _allocator memory is exhausted.
	const U32			_marker;		//!< for rewinding

public:
	inline StackAllocator(
		LinearAllocator & alloc,
		AllocatorI & backing
		)
		: _allocator( alloc )
		, _marker( alloc.Position() )
		, _backing( backing )
	{}
	inline ~StackAllocator()
	{
		_allocator.FreeTo( _marker );
	}
	virtual void* Allocate( U32 _bytes, U32 alignment ) override
	{
		void* memory = _allocator.Allocate( _bytes, alignment );
		if( memory ) {
			return memory;
		}
		return _backing.Allocate( _bytes, alignment );
	}
	virtual void Deallocate( const void* _memory ) override
	{
		if( !_allocator.contains( _memory ) ) {
			_backing.Deallocate( _memory );
		}
	}
};

/*
----------------------------------------------------------
	MemoryScope - is a trashable heap.
----------------------------------------------------------
*/
class MemoryScope {
public:
	enum { DEFAULT_ALIGNMENT = 16 };

	typedef FCallback FDestruct;

	MemoryScope( LinearAllocator & alloc );
	MemoryScope( MemoryScope * parent );
	~MemoryScope();

	void* Alloc( U32 size, U32 alignment = DEFAULT_ALIGNMENT );

	void Unwind();
	void Seal();
	bool IsSealed() const;

	ERet AddDestructor( void* object, FDestruct fn );

	U32 capacity() const;
	U32 Position() const;

	template< typename TYPE >
	inline TYPE* New()
	{
		FDestruct* dtor = &FDestructorCallback< TYPE >;
		return (TYPE*) this->AllocMany( 1, sizeof(TYPE), dtor );
	}
	template< typename TYPE >
	inline TYPE* NewArray( U32 count )
	{
		FDestruct* dtor = &FDestructorCallback< TYPE >;
		return (TYPE*) this->AllocMany( count, sizeof(TYPE), dtor );
	}

private:
	struct Destructor
	{
		FDestruct *		fun;
		Destructor *	next;
		void *			object;
		void *			_pad16;
	};

	void* AllocMany( U32 count, U32 stride, FDestruct* fun );

	void AddDestructor( Destructor* dtor, void* object, FDestruct* fun );
	void AddDestructors( Destructor* dtors, U32 count, void* objects, U32 stride, FDestruct* fun );

	LinearAllocator &	m_alloc;
	MemoryScope *		m_parent;	//!< optional
	Destructor *		m_destructors;
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
