/*
=============================================================================
	File:	ObjectModel.h
	Desc:	Based on PhyreEngine's PInstanceList.
	Basically, Clump is a grouping of arrays of objects for iteration and serialization.
=============================================================================
*/
#pragma once

#include <Base/Memory/FreeList/FreeList.h>

#include <Core/Core.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory.h>

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
-----------------------------------------------------------------------------
	Object lists store arrays of objects of a certain type (derived from CStruct).

	Storing related data together improves locality of reference
	and allows for more efficient cache usage.

	Note:
	very good for storing fixed-size POD structs;
	doesn't support polymorphic types.

	Implementation:
		Once created, an object list cannot be resized.
		Dead (deleted) objects are kept in a free list sorted by objects' addresses
		so that we can iterate over 'live' objects while skipping deleted items.
	The memory allocated for storing data objects of a certain type is never released
	for reuse upon subsequent allocations of objects of the same type.

	We sacrifice element contiguity in favor of stability; references and iterators
	to a given element remain valid as long as the element is not erased.
-----------------------------------------------------------------------------
*/
struct NwObjectList: CStruct, public TLinkedList< NwObjectList >
{
	TPtr< const TbMetaClass >	m_type;	//!< The class of objects stored in this list.

	FreeList	m_freeList;	//!< The (sorted) free list for reusing dead objects.

	U32	m_liveCount;//!< The number of valid (live) objects in this list.
	U32	m_capacity;	//!< The maximum number of objects that can be stored in this list.
	U32	m_flags;	//!< Internal flags specifying memory ownership.

	enum Flags
	{
		CanFreeMemory = BIT(0),	//!< Does this container own memory?
	};

public:
	mxDECLARE_CLASS(NwObjectList,CStruct);
	mxDECLARE_REFLECTION;
	NwObjectList( const TbMetaClass& type, U32 capacity );
	~NwObjectList();

	const TbMetaClass& getType() const;
	U32 stride() const;

	bool hasAddress( const void* ptr ) const;

	// returns true if the given item is live in this object list (O(N))
	bool hasValidItem( const void* item ) const;

	// returns true if this list owns memory block for the given object (O(1))
	bool containsItem( const void* item ) const;

	U32 IndexOfContainedItem( const void* item ) const;

	/// releases the object and adds it to the free list (O(N))
	/// NOTE: doesn't call the object's destructor!
	ERet FreeItem( void* item );

	// tries to allocate a new item, doesn't call a constructor (O(1))
	CStruct* Allocate();

	/// returns the number of valid objects
	U32 count() const;

	/// returns the maximum amount of objects which can be stored in this list
	U32 capacity() const;

	/// returns a pointer to the array of stored elements
	CStruct* raw() const;

	CStruct* itemAtIndex( U32 itemIndex ) const;

	CStruct* getFirstFreeItem() const;

	size_t usableSize() const;

	/// doesn't release allocated memory
	void empty();

	/// destroys all live objects and frees allocated memory
	void clear();

	void IterateItems( Reflection::AVisitor* visitor, void* userData ) const;
	void IterateItems( Reflection::AVisitor2* visitor, void* userData ) const;

	void DbgCheckPointers();

public:
	/// Base class for iterating over live objects inside an NwObjectList.
	class IteratorBase
	{
	protected:
		void *	m_current;		//!< pointer to the current element
		void *	m_nextFree;		//!< next free object in the sorted free list
		U32		m_stride;		//!< byte distance between objects, in bytes
		U32		m_remaining;	//!< number of remaining objects to iterate over

	public:
		IteratorBase();
		IteratorBase( const NwObjectList& objectList );

		bool IsValid() const;
		void MoveToNext();
		void Skip( U32 count );
		U32 NumRemaining() const;
		U32 NumContiguousObjects() const;

		void Initialize( const NwObjectList& objectList );
		void Reset();

		void* ToVoidPtr() const;

	private:PREVENT_COPY(IteratorBase);
	};

private:
	PREVENT_COPY(NwObjectList);
};

// a type-safe wrapper around NwObjectList
template< class CLASS >
class TObjectList
{
	NwObjectList & m_list;
public:
	TObjectList( NwObjectList & _list ) : m_list( _list )
	{
		mxASSERT(_list.getType() == CLASS::metaClass());
	}
	ERet New( CLASS *& _o )
	{
		void* mem = m_list.Allocate();
		if( mem ) {
			_o = new(mem) CLASS();
			return ALL_OK;
		}
		return ERR_OUT_OF_MEMORY;
	}
};
