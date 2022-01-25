/*
=============================================================================
	File:	ObjectModel.h
	Desc:	Based on PhyreEngine's PCluster.
	Basically, NwClump is a grouping of arrays of objects for iteration and serialization.
=============================================================================
*/
#pragma once

#include <Core/ObjectModel/ObjectList.h>


/*
-----------------------------------------------------------------------------
	A 'NwClump' groups together related objects.
	This is mainly used for easy & efficient serialization and fast iteration.

	Each 'NwClump' stores objects in object lists - contiguous arrays of objects.
	There can be several object lists for the same type (class) of objects
	(except for serialized/optimized clumps).
-----------------------------------------------------------------------------
*/
class NwClump
	: public NwResource	// Clumps are used in streaming
	, public ObjectAllocatorI
	, public TDoublyLinkedList< NwClump >	// for linking & cross-referencing
	//, public ReferenceCounted
{
public:
	NwObjectList::Head	_object_lists;	//!< head of the linked list of object lists
	FreeListAllocator	_object_lists_pool;	//!< used for recycling object lists
	char *				m_userData;		//!< for debugging
	//U32				m_referenceCount;
	//U32				m_flags;
public:
	mxDECLARE_CLASS(NwClump,NwResource);
	mxDECLARE_REFLECTION;

	NwClump();
	~NwClump();
	NwClump(_FinishedLoadingFlag) {}

	// ObjectAllocatorI:
	// NOTE: simply allocates storage, doesn't call ctor
	// NOTE: the returned memory block is uninitialized!
	virtual void* allocateObject( const TbMetaClass& type, U32 granularity ) override;
	virtual void deallocateObject( const TbMetaClass& type, void* _pointer ) override;

	//ERet RegisterResource( Resource* _newResource, const AssetID& _id, const TbMetaClass& type );

	ERet FreeOne( const TbMetaClass& type, void* o );
	void destroyAllObjectsOfType( const TbMetaClass& type );

	bool hasAddress( const void* pointer ) const;

	/// Creates an empty object list.
	NwObjectList* CreateObjectList( const TbMetaClass& type, U32 capacity );

	void clear();	// remove all items and release allocated memory
	//void empty();	// doesn't free allocated memory

	NwObjectList::Head GetObjectLists() const;

	U32 numAliveObjectLists() const { return _object_lists_pool.NumValidItems(); }

	bool IsEmpty() const;

	// walk over all object lists
	void IterateObjects( Reflection::AVisitor* visitor, void* userData ) const;
	void IterateObjects( Reflection::AVisitor2* visitor, void* userData ) const;

public:
	// creates a new object (may reuse existing free-listed items)
	template< class CLASS >
	ERet New( CLASS *&_o, U32 granularity = 1 )
	{
		_o = nil;
		const TbMetaClass& type = T_DeduceClass< CLASS >();
		void* storage = this->allocateObject( type, granularity );
		if( storage ) {
			_o = new (storage) CLASS();
			return ALL_OK;
		}
		return ERR_OUT_OF_MEMORY;
	}

	/// adds the object to the free list
	/// doesn't call the object's destructor!
	template< class CLASS >
	ERet Free( CLASS* o )
	{
		return this->FreeOne( T_DeduceClass< CLASS >(), o );
	}

	template< class CLASS >
	ERet Destroy( CLASS *& _o )
	{
		_o->~CLASS();
		const ERet result = this->FreeOne( T_DeduceClass< CLASS >(), _o );
		_o = nil;
		return result;
	}

	// NOTE: the returned memory block is uninitialized!
	template< class CLASS >
	CLASS* CreateObjectList( U32 capacity )
	{
		NwObjectList* objectList = this->CreateObjectList( T_DeduceClass< CLASS >(), capacity );
		if( objectList ) {
			return static_cast< CLASS* >( objectList->raw() );
		}
		return NULL;
	}

public:
	/// Base class for iterating over object lists within a clump.
	class ObjectListIterator
	{
		NwObjectList *	m_currentList;	//!< current object list being iterated
		const TbMetaClass &	m_baseClass;	//!< base type of objects that should be iterated

	public:
		ObjectListIterator( const NwClump& clump, const TbMetaClass& type );

		bool IsValid() const;
		void MoveToNext();

		U32 NumObjects() const;

		NwObjectList& value() const;
	};
	/// Base class for iterating over objects stored in object lists within a clump.
	class IteratorBase
	{
	protected:
		NwObjectList::IteratorBase	m_objectIterator;	//!< Iterator for the objects in the current object list.
		NwObjectList *				m_currentList;
		const TbMetaClass &				m_type;	//!< for iterating over objects of a specific type

	public:
		IteratorBase( const NwClump& clump, const TbMetaClass& type );

		bool IsValid() const;
		void MoveToNext();
		void Skip( U32 count );
		U32 NumContiguousObjects() const;

		void* ToVoidPtr() const;

		void Initialize( const NwClump& clump );
		void Reset();

	protected:
		void SetFromCurrentList();
		bool IsCurrentListValid() const;
		void MoveToFirstObject();
		void MoveToNextList();
	};
	/// Iterator for iterating over objects stored in object lists within a clump.
	template< class CLASS >
	class Iterator: public NwClump::IteratorBase
	{
	public:
		Iterator( const NwClump& clump )
			: NwClump::IteratorBase( clump, CLASS::metaClass() )
		{}
		inline CLASS* Raw() const
		{
			mxASSERT(this->IsValid());
			return static_cast< CLASS* >( m_objectIterator.ToVoidPtr() );
		}
		//@todo: rename to ToRef()?
		inline CLASS& Value() const
		{
			mxASSERT(this->IsValid());
			return *static_cast< CLASS* >( m_objectIterator.ToVoidPtr() );
		}
	};

public:	// Utilities

	CStruct* FindFirstObjectOfType( const TbMetaClass& type ) const;

	template< class CLASS >
	CLASS* FindFirstObjectOfType() const {
		return static_cast< CLASS* >( this->FindFirstObjectOfType( CLASS::metaClass() ) );
	}



	CStruct* FindSingleInstance( const TbMetaClass& type ) const;

	template< class CLASS >
	CLASS* FindSingleInstance() const {
		return static_cast< CLASS* >( this->FindSingleInstance( CLASS::metaClass() ) );
	}

	template< class CLASS >
	ERet GetSingleInstance( CLASS *&_o ) const
	{
		_o = this->FindSingleInstance< CLASS >();
		return _o ? ALL_OK : ERR_OBJECT_NOT_FOUND;
	}

public:	// Resource management

	static TbAssetLoaderI* staticResourceLoader();

	//// this function must be called after loading
	//// to fixup references to external assets
	//// (create 'live' resource objects from external data)
	//ERet LoadAssets( U32 timeOutMilliseconds = INFINITE );

	//static NwClump* Create( const CreateContext& _ctx );
	//static ERet Load( NwClump *_o, const LoadContext& _ctx );
	//static void Unload( NwClump * _o );
	//static void Destroy( NwClump *_o, const DestroyContext& _ctx );

	enum { FOURCC = MCHAR4('C','L','M','P') };

public:	// Debugging
	void DbgCheckPointers();

private:
	void __UnloadContainedAssets();
};

///
struct ClumpList
{
	NwClump::Head		head;	//!< head of the linked list of clumps

public:
	// for iterating through all loaded clumps
	static ClumpList g_loadedClumps;

public:
	template< class CLASS >
	class Iterator
	{
		NwClump::Iterator< CLASS >	m_clumpIterator;	//!< Iterator for the clump in the current object list.
		NwClump *						m_currentClump;

	public:
		Iterator( const ClumpList& clumpList )
			: m_clumpIterator( *clumpList.head )
			, m_currentClump( clumpList.head )
		{
			// go to the first valid object
			while( m_currentClump )
			{
				m_clumpIterator.Initialize( *m_currentClump );
				if( m_clumpIterator.IsValid() ) {
					break;
				}
				m_currentClump = m_currentClump->next;
			}
		}

		inline CLASS& Value() const
		{
			mxASSERT(this->IsValid());
			return *(CLASS*)m_clumpIterator.ToVoidPtr();
		}

		bool IsValid() const
		{
			return m_clumpIterator.IsValid();
		}

		/// Moves to the next valid ('live') object.
		void MoveToNext()
		{
			mxASSERT2(m_clumpIterator.IsValid(), "Iterator has run off the end of the list");
			// move to the next object in the current clump
			m_clumpIterator.MoveToNext();
			// if reached the end of the current clump
			if( !m_clumpIterator.IsValid() )
			{
				// move to the next valid clump
				do {
					m_currentClump = m_currentClump->next;
					if( !m_currentClump ) {
						break;
					}
					m_clumpIterator.Initialize( *m_currentClump );
				} while( !m_clumpIterator.IsValid() );
			}
		}
	};
};



NwObjectList* FindFirstObjectListOfType( const NwClump& clump, const TbMetaClass& type );
//AssetExport* FindAssetExportByPointer( const NwClump& clump, const void* assetInstance );






template< class TYPE >	// where TYPE has member 'name' of type 'String'
class NamesEqual {
	const String& name;
public:
	NamesEqual( const String& _name ) : name( _name )
	{}
	bool operator() ( const TYPE& other ) const
	{ return Str::Equal( this->name, other.name ); }
};
template< class TYPE >	// where TYPE has member 'hash' of type 'String'
class NameHashesEqual {
	const NameHash32 name_hash;
public:
	NameHashesEqual( const NameHash32 _name_hash ) : name_hash( _name_hash )
	{}
	bool operator() ( const TYPE& other ) const
	{ return this->name_hash == other.hash; }
};

void DBG_DumpFields( const void* _memory, const TbMetaType& type, ATextStream &_log );

void DBG_PrintClump( const NwClump& _clump );


template< typename TYPE, class CONTAINER, class PREDICATE >
TYPE* LinearSearch( CONTAINER& _container, const PREDICATE& _predicate )
{
	CONTAINER::Iterator<TYPE>	it( _container );
	while( it.IsValid() ) {
		TYPE & o = it.Value();
		if( _predicate( o ) ) {
			return &o;
		}
		it.MoveToNext();
	}
	return nil;
}
template< class TYPE, class CONTAINER >
ERet GetByHash( TYPE *&_o, const NameHash32 _name_hash, CONTAINER& _container )
{
	_o = LinearSearch< TYPE >( _container, NameHashesEqual< TYPE >(_name_hash) );
	return _o ? ALL_OK : ERR_OBJECT_NOT_FOUND;
}

template< class TYPE, class CONTAINER >
ERet GetByName( TYPE *&_o, const String& _name, CONTAINER& _container )
{
	_o = LinearSearch< TYPE >( _container, NamesEqual< TYPE >(_name) );
	return _o ? ALL_OK : ERR_OBJECT_NOT_FOUND;
}

mxREMOVE_OLD_CODE
///// Loads the resource and creates the corresponding AssetExport for safe unloading.
//template< typename RESOURCE >
//ERet LoadNewResource(
//	RESOURCE *&_o,
//	const AssetID& _id,
//	NwClump * _storage
//	)
//{
//	mxSTATIC_ASSERT( (std::is_base_of< Resource, RESOURCE >::value) );
//	Resource* new_resource = nil;	// in case RESOURCE has a vtable
//	const TbMetaClass& resource_type = RESOURCE::metaClass();
//	mxTRY(Resources::Load( new_resource, _id, resource_type, _storage ));
////	mxTRY(_storage->RegisterResource( new_resource, _id, resource_type ));
//	_o = static_cast< RESOURCE* >( new_resource );
//	return ALL_OK;
//}



namespace Testbed
{
	class ClumpLoader: public TbMemoryImageAssetLoader
	{
	public:
		ClumpLoader( ProxyAllocator & allocator );
		virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
		virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
		virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	};
}//namespace Testbed
