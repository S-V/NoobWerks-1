//
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>

#include <Core/ObjectModel/Clump.h>

//#include <Core/Assets/AssetManagement.h>
//#include <Core/Memory.h>
//#include <Core/Memory/MemoryHeaps.h>
//#include <Core/ObjectModel/NwClump.h>
//#include <Core/VectorMath.h>
#include <Core/Serialization/Serialization.h>

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( NwClump );
mxBEGIN_REFLECTION( NwClump )
	mxMEMBER_FIELD( _object_lists ),
	//mxMEMBER_FIELD( _object_lists_pool ),
mxEND_REFLECTION

NwClump::NwClump()
{
	_object_lists = nil;
	_object_lists_pool.Initialize( sizeof(NwObjectList), 16 );
	//m_referenceCount = 0;
	//m_flags = 0;
}

NwClump::~NwClump()
{
	this->clear();
	_object_lists_pool.releaseMemory();
}

// NOTE: doesn't call constructor!
void* NwClump::allocateObject( const TbMetaClass& type, U32 granularity )
{
	mxASSERT2(type != NwClump::metaClass(), "Clumps cannot be stored within clumps");
	mxASSERT(granularity > 0);
	// First try to reuse deleted item from an object list of the given type
	{
		NwObjectList* current = _object_lists;
		while(PtrToBool( current ))
		{
			if( current->getType() == type )
			{
				CStruct* newItem = current->Allocate();
				if(PtrToBool( newItem ))
				{
					return newItem;
				}
			}
			current = current->_next;
		}
	}

	// Failed to find a free slot in existing object lists - create a new object list

	NwObjectList* newObjectList = this->CreateObjectList( type, granularity );
	chkRET_NIL_IF_NIL(newObjectList);

	CStruct* newItem = newObjectList->Allocate();
	mxASSERT_PTR(newItem);
	return newItem;
}

void NwClump::deallocateObject( const TbMetaClass& type, void* _pointer )
{
	this->FreeOne( type, _pointer );
}

//ERet NwClump::RegisterResource( Resource* _newResource, const AssetID& _id, const TbMetaClass& type )
//{
//	DBGOUT("Registering resource '%s'...", AssetId_ToChars(_id));
//	AssetExport *	newAssetExport;
//	mxDO(this->New( newAssetExport ));
//
//	newAssetExport->o = _newResource;
//	newAssetExport->id = _id;
//	newAssetExport->type = type.GetTypeGUID();
//	return ALL_OK;
//}

ERet NwClump::FreeOne( const TbMetaClass& type, void* o )
{
	mxASSERT_PTR( o );
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		if( current->containsItem( o ) ) {
			mxASSERT(current->getType().IsDerivedFrom( type ));
			return current->FreeItem( o );
		}
		current = current->_next;
	}
	mxASSERT2(false, "object not found");
	return ERR_OBJECT_NOT_FOUND;
}

void NwClump::destroyAllObjectsOfType( const TbMetaClass& type )
{
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		NwObjectList* next = current->_next;

		if( current->getType().IsDerivedFrom( type ) )
		{
			current->RemoveSelfFromList( &_object_lists );
			_object_lists_pool.Delete( current );
		}

		current = next;
	}
}

bool NwClump::hasAddress( const void* pointer ) const
{
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		if( current->hasAddress( pointer ) )
		{
			return true;
		}
		current = current->_next;
	}
	return false;
}

NwObjectList* NwClump::CreateObjectList( const TbMetaClass& type, U32 capacity )
{
	mxASSERT(capacity > 0);
	void* storage = _object_lists_pool.AllocateItem();
	chkRET_NIL_IF_NIL(storage);
	NwObjectList* objectList = new (storage) NwObjectList( type, capacity );
	objectList->PrependSelfToList( &_object_lists );
	return objectList;
}

void NwClump::clear()
{
	this->__UnloadContainedAssets();

	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		NwObjectList* next = current->_next;

		current->RemoveSelfFromList( &_object_lists );
		_object_lists_pool.Delete( current );

		current = next;
	}
}

//void NwClump::empty()
//{
//	this->__UnloadContainedAssets();
//
//	NwObjectList* current = _object_lists;
//	while(PtrToBool( current ))
//	{
//		NwObjectList* next = current->_next;
//
//		current->RemoveSelfFromList( &_object_lists );
//		current->empty();
//		_object_lists_pool.ReleaseItem( current );
//
//		current = next;
//	}
//}

NwObjectList::Head NwClump::GetObjectLists() const
{
	return _object_lists;
}

bool NwClump::IsEmpty() const
{
	return _object_lists == nil;
}

void NwClump::IterateObjects( Reflection::AVisitor* visitor, void* userData ) const
{
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		current->IterateItems( visitor, userData );
		current = current->_next;
	}
}
void NwClump::IterateObjects( Reflection::AVisitor2* visitor, void* userData ) const
{
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		current->IterateItems( visitor, userData );
		current = current->_next;
	}
}

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
NwClump::IteratorBase::IteratorBase( const NwClump& clump, const TbMetaClass& type )
	: m_type( type )
{
	m_currentList = clump._object_lists;
	this->MoveToFirstObject();
}

bool NwClump::IteratorBase::IsValid() const
{
	return m_objectIterator.IsValid();
}

void NwClump::IteratorBase::MoveToNext()
{
	mxASSERT2(m_objectIterator.IsValid(), "Iterator has run off the end of the list");
	m_objectIterator.MoveToNext();
	if( !m_objectIterator.IsValid() )
	{
		this->MoveToNextList();
	}
}

void NwClump::IteratorBase::Skip( U32 count )
{
	return m_objectIterator.Skip( count );
}
U32 NwClump::IteratorBase::NumContiguousObjects() const
{
	return m_objectIterator.NumContiguousObjects();
}

void* NwClump::IteratorBase::ToVoidPtr() const
{
	return m_objectIterator.ToVoidPtr();
}

void NwClump::IteratorBase::Initialize( const NwClump& clump )
{
	m_currentList = clump._object_lists;
	this->MoveToFirstObject();
}
void NwClump::IteratorBase::Reset()
{
	m_objectIterator.Reset();
	m_currentList = nil;
}

void NwClump::IteratorBase::SetFromCurrentList()
{
	if(PtrToBool( m_currentList ))
	{
		m_objectIterator.Initialize( *m_currentList );
	}
	else
	{
		m_objectIterator.Reset();
	}
}

bool NwClump::IteratorBase::IsCurrentListValid() const
{
#if 0
	if( m_currentList == nil )
	{
		// this is a valid state, need to move on to the next list
		// or reset the object iterator to null
		return true;
	}
	return m_currentList->GetType().IsDerivedFrom(m_type) && (m_currentList->num() > 0);
#else
	// rewritten for speed
	return !m_currentList
		|| (m_currentList->getType().IsDerivedFrom(m_type) && m_currentList->count() > 0)
		;
#endif
}

void NwClump::IteratorBase::MoveToFirstObject()
{
	while( !this->IsCurrentListValid() )
	{
		m_currentList = m_currentList->_next;
	}
	this->SetFromCurrentList();
}

void NwClump::IteratorBase::MoveToNextList()
{
	do
	{
		m_currentList = m_currentList->_next;
	}
	while( !this->IsCurrentListValid() );
	this->SetFromCurrentList();
}

void NwClump::DbgCheckPointers()
{
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		current->DbgCheckPointers();
		current = current->_next;
	}
}

void NwClump::__UnloadContainedAssets()
{
#if 0
	{
		Clump::Iterator< AssetImport >	it( *_clump );
		while( it.IsValid() )
		{
			const AssetImport& assetImport = it.Value();

			const TbMetaClass* type = TypeRegistry::FindClassByGuid( assetImport.type );
			mxASSERT(type);
			if( type )
			{
				Resource * resource_that_must_be_loaded = nil;
				mxDO(Resources::Load( resource_that_must_be_loaded, assetImport.id, *type, &_ctx.storage ));
			}

			it.MoveToNext();
		}
	}
#endif

#if 0
	mxOPTIMIZE("This is very slow, use AssetExports");
	NwObjectList* current = _object_lists;
	while(PtrToBool( current ))
	{
		NwObjectList* next = current->_next;

		const TbMetaClass& currentType = current->GetType();
		if( currentType.IsDerivedFrom( Resource::metaClass() ) )
		{
			NwObjectList::IteratorBase	it( *current );
			while( it.IsValid() )
			{
				Resource* resource = (Resource*) it.ToVoidPtr();
				Resources::Unload( resource );
				it.MoveToNext();
			}

			current->RemoveSelfFromList( &_object_lists );
			current->empty();

			if( _object_lists_pool.GetAllocatedBlocksList() != nil ) {
				_object_lists_pool.ReleaseItem( current );
			}
		}

		current = next;
	}
#endif
#if 0
	Iterator< AssetExport >	it( *this );
	while( it.IsValid() )
	{
		const AssetExport& assetExport = it.Value();

		const TbMetaClass* type = TypeRegistry::FindClassByGuid( assetExport.type );
		mxASSERT(type);
		if( type )
		{
			Resources::Unload( assetExport.id, *type );
		}

		it.MoveToNext();
	}
#endif
}


namespace Testbed
{
	ClumpLoader::ClumpLoader( ProxyAllocator & allocator )
		: TbMemoryImageAssetLoader( NwClump::metaClass(), allocator )
	{
	}

	ERet ClumpLoader::create( NwResource **new_instance_, const TbAssetLoadContext& context )
	{
		AssetReader	stream;
		mxDO(Resources::OpenFile( context.key, &stream ));

		NwClump * clump = nil;
		mxDO(Serialization::LoadClumpImage( stream, clump, context.raw_allocator ));
		*new_instance_ = clump;
		return ALL_OK;
	}

	ERet ClumpLoader::load( NwResource * instance, const TbAssetLoadContext& context )
	{
		NwClump * clump = static_cast< NwClump* >( instance );

		// Resolve references to external assets (which are not stored within this clump).
		{
			NwClump::Iterator< AssetImport >	it( *clump );
			while( it.IsValid() )
			{
				const AssetImport& assetImport = it.Value();

				const TbMetaClass* type = TypeRegistry::FindClassByGuid( assetImport.type );
				mxASSERT(type);
				if( type )
				{
					NwResource * resource_that_must_be_loaded = nil;
					mxDO(Resources::Load( resource_that_must_be_loaded, assetImport.id, *type, &context.object_allocator ));
				}

				it.MoveToNext();
			}
		}

		// Load assets and register them in the hash table.
		// The memory for storing the assets has already been allocated,
		// they 'live' inside the clump, now they have to be 'finalized'.
		{
			NwClump::Iterator< AssetExport >	it( *clump );
			while( it.IsValid() )
			{
				AssetExport & assetExport = it.Value();
				UNDONE;
#if 0
				// Load the instance and register them in the hash table.
				mxDO(Resources::Finalize( assetExport.o, assetExport ));
#endif
				it.MoveToNext();
			}
		}

		clump->AppendSelfToList( &ClumpList::g_loadedClumps.head );

		return ALL_OK;
	}

	void ClumpLoader::unload( NwResource * instance, const TbAssetLoadContext& context )
	{
		NwClump * clump = static_cast< NwClump* >( instance );
		clump->RemoveSelfFromList();
//		clump->__UnloadContainedAssets();
	}
}//namespace Testbed

//NwClump* NwClump::Create( const CreateContext& _ctx )
//{
//	NwClump * clump = nil;
//	Serialization::LoadClumpImage( _ctx.stream, clump, _ctx.object_allocator );
//	return clump;
//}
//ERet NwClump::Load( NwClump *_clump, const LoadContext& _ctx )
//{
//	// Resolve references to external assets (which are not stored within this clump).
//	{
//		NwClump::Iterator< AssetImport >	it( *_clump );
//		while( it.IsValid() )
//		{
//			const AssetImport& assetImport = it.Value();
//
//			const TbMetaClass* type = TypeRegistry::FindClassByGuid( assetImport.type );
//			mxASSERT(type);
//			if( type )
//			{
//				Resource * resource_that_must_be_loaded = nil;
//				mxDO(Resources::Load( resource_that_must_be_loaded, assetImport.id, *type, &_ctx.object_allocator ));
//			}
//
//			it.MoveToNext();
//		}
//	}
//
//	// Load assets and register them in the hash table.
//	// The memory for storing the assets has already been allocated,
//	// they 'live' inside the clump, now they have to be 'finalized'.
//	{
//		NwClump::Iterator< AssetExport >	it( *_clump );
//		while( it.IsValid() )
//		{
//			AssetExport & assetExport = it.Value();
//UNDONE;
//#if 0
//			// Load the instance and register them in the hash table.
//			mxDO(Resources::Finalize( assetExport.o, assetExport ));
//#endif
//			it.MoveToNext();
//		}
//	}
//
//	_clump->AppendSelfToList( &ClumpList::g_loadedClumps.head );
//
//	return ALL_OK;
//}
//void NwClump::Unload( NwClump * _clump )
//{
//	_clump->__UnloadContainedAssets();
//	_clump->RemoveSelfFromList();
//}
ClumpList ClumpList::g_loadedClumps = { nil };

/*
-----------------------------------------------------------------------------
	ObjectListIterator
-----------------------------------------------------------------------------
*/

static inline bool IsValidObjectList( NwObjectList* objectList, const TbMetaClass& baseType )
{
	mxASSERT_PTR(objectList);
	return objectList->getType().IsDerivedFrom( baseType )
		&& objectList->count() > 0
		;
}

NwClump::ObjectListIterator::ObjectListIterator( const NwClump& clump, const TbMetaClass& type )
	: m_baseClass( type )
{
	m_currentList = clump._object_lists;
	// move to the first valid object list
	while( m_currentList && !IsValidObjectList(m_currentList, type) )
	{
		m_currentList = m_currentList->_next;
	}
}

bool NwClump::ObjectListIterator::IsValid() const
{
	return m_currentList != nil;
}

void NwClump::ObjectListIterator::MoveToNext()
{
	mxASSERT(this->IsValid());
	do 
	{
		m_currentList = m_currentList->_next;
		if( !m_currentList ) {
			break;
		}
	}
	while( !IsValidObjectList(m_currentList, m_baseClass) );
}

U32 NwClump::ObjectListIterator::NumObjects() const
{
	mxASSERT(this->IsValid());
	return m_currentList->count();
}

NwObjectList& NwClump::ObjectListIterator::value() const
{
	mxASSERT(this->IsValid());
	return *m_currentList;
}

NwObjectList* FindFirstObjectListOfType( const NwClump& clump, const TbMetaClass& type )
{
	NwObjectList* current = clump.GetObjectLists();
	while(PtrToBool( current ))
	{
		if( current->getType() == type )
		{
			return current;
		}
		current = current->_next;
	}
	return nil;
}

CStruct* NwClump::FindFirstObjectOfType( const TbMetaClass& type ) const
{
	NwClump::IteratorBase	it( *this, type );

	return it.IsValid()
		? (CStruct*) it.ToVoidPtr()
		: nil
		;
}

CStruct* NwClump::FindSingleInstance( const TbMetaClass& type ) const
{
	NwObjectList* objectList = FindFirstObjectListOfType( *this, type );
	if( PtrToBool(objectList) ) {
		mxASSERT(objectList->count() == 1);
		return objectList->itemAtIndex( 0 );
	}
	return nil;
}

//AssetExport* FindAssetExportByPointer( const NwClump& clump, const void* assetInstance )
//{
//	NwClump::Iterator< AssetExport >	it( clump );
//	while( it.IsValid() )
//	{
//		AssetExport& assetExport = it.Value();
//		if( assetExport.o == assetInstance ) {
//			return &assetExport;
//		}
//		it.MoveToNext();
//	}
//	return nil;
//}

void DBG_DumpFields_Recursive( const void* _memory, const TbMetaType& type, ATextStream &_log, int _depth )
{
	switch( type.m_kind )
	{
	case ETypeKind::Type_Integer :
		{
			const INT64 value = GetInteger( _memory, type.m_size );
			_log << value;
		}
		break;

	case ETypeKind::Type_Float :
		{
			const double value = GetDouble( _memory, type.m_size );
			_log << value;
		}
		break;
	case ETypeKind::Type_Bool :
		{
			const bool value = TPODCast<bool>::GetConst( _memory );
			_log << value;
		}
		break;

	case ETypeKind::Type_Enum :
		{
			const MetaEnum& enumType = type.UpCast< MetaEnum >();
			const U32 enumValue = enumType.GetValue( _memory );
			const char* valueName = enumType.FindString( enumValue );
			_log << valueName;
		}
		break;

	case ETypeKind::Type_Flags :
		{
			const MetaFlags& flagsType = type.UpCast< MetaFlags >();
			String512 temp;
			Dbg_FlagsToString( _memory, flagsType, temp );
			_log << temp;
		}
		break;

	case ETypeKind::Type_String :
		{
			const String & stringReference = TPODCast< String >::GetConst( _memory );
			_log << stringReference;
		}
		break;

	case ETypeKind::Type_Class :
		{
			const TbMetaClass& classType = type.UpCast< TbMetaClass >();

			const TbMetaClass* parentType = classType.GetParent();
			while( parentType != nil )
			{
				DBG_DumpFields_Recursive( _memory, *parentType, _log, _depth+1 );
				if( ObjectUtil::Class_Has_Fields( *parentType ) ) {
					_log << "; ";
				}
				parentType = parentType->GetParent();
			}

			const TbClassLayout& layout = classType.GetLayout();
			for( int fieldIndex = 0 ; fieldIndex < layout.numFields; fieldIndex++ )
			{
				const MetaField& field = layout.fields[ fieldIndex ];

				const void* memberVarPtr = mxAddByteOffset( _memory, field.offset );
				_log << field.name << ':';
				DBG_DumpFields_Recursive( memberVarPtr, field.type, _log, _depth+1 );
				if( fieldIndex < layout.numFields-1 ) {
					_log << ", ";
				} else {
					if( _depth == 0 ) {
						_log << '.';
					}
				}
			}
		}
		break;

	case ETypeKind::Type_Pointer :
		{
			_log << "(Pointers not impl)";
		}
		break;

	case ETypeKind::Type_AssetId :
		{
			const AssetID& assetId = *static_cast< const AssetID* >( _memory );
			_log << AssetId_ToChars( assetId );
		}
		break;

	case ETypeKind::Type_ClassId :
		{
			_log << "(ClassId not impl)";
		}
		break;

	case ETypeKind::Type_UserData :
		{
			_log << "(UserData not impl)";
		}
		break;

	case ETypeKind::Type_Blob :
		{
			_log << "(Blobs not impl)";
		}
		break;

	case ETypeKind::Type_Array :
		{
			_log << "(Arrays not impl)";
		}
		break;

		mxNO_SWITCH_DEFAULT;
	}//switch
}
void DBG_DumpFields( const void* _memory, const TbMetaType& type, ATextStream &_log )
{
	DBG_DumpFields_Recursive( _memory, type, _log, 0 );
}
void DBG_PrintClump( const NwClump& _clump )
{
	NwObjectList::Head currentList = _clump.GetObjectLists();
	while( currentList != nil )
	{
		const U32 objectCount = currentList->count();
		const TbMetaClass& object_type = currentList->getType();

		DBGOUT("NwObjectList: count=%u, type='%s'",objectCount,object_type.GetTypeName());

		//CStruct* objectsArray = currentList->GetArrayPtr();
		//const U32 arrayStride = currentList->stride();

		currentList = currentList->_next;
	}
}


/*
Useful references:
Fast Memory Pool Allocators: Boost, Nginx & Tempesta FW
Memory Pools and Region-based Memory Management
http://natsys-lab.blogspot.ru/2015/09/fast-memory-pool-allocators-boost-nginx.html?spref=tw

Improving Real-Time Performance by Utilizing Cache Allocation Technology
(Enhancing Performance via Allocation of the Processor’s Cache) [April 2015]
(http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/cache-allocation-technology-white-paper.pdf

https://github.com/nasa/eefs/blob/master/inc/eefs_fileapi.h

Generic 'struct-of-arrays' container, data oriented design:
https://github.com/eXistence/Arrays

Data structures for traversable memory pool
http://stackoverflow.com/questions/23345476/data-structures-for-traversable-memory-pool

What is the most efficient container to store dynamic game objects in?
https://gamedev.stackexchange.com/questions/33888/what-is-the-most-efficient-container-to-store-dynamic-game-objects-in
*/

