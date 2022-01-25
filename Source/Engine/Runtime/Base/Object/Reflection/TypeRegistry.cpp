/*
=============================================================================
	File:	TypeRegistry.cpp
	Desc:	Object factory for run-time class instancing and type information.
	ToDo:	check inheritance depth and warn about pathologic cases?
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/BaseType.h>
#include <Base/Object/Reflection/TypeRegistry.h>

/*
-------------------------------------------------------------------------
	TypeRegistry
-------------------------------------------------------------------------
*/
TbMetaClass *	TypeRegistry::m_heads[TABLE_SIZE] = { NULL };

ERet TypeRegistry::Initialize()
{
	int class_index = 0;

	TbMetaClass* current = TbMetaClass::m_head;
	while( PtrToBool(current) )
	{
		TbMetaClass* next = current->m_next;

		const char* class_name = current->GetTypeName();
		const TypeGUID name_hash = current->GetTypeGUID();

#if MX_DEBUG
		//DBGOUT("Register class %s (ID=%d, GUID=0x%x)",
		//	class_name, class_index, (int)name_hash
		//	);
		const TbMetaClass* existing = FindClassByGuid( name_hash );
		if( existing ) {
			ptERROR("class name collision: '%s' and '%s' -> 0x%x\n",
				class_name, existing->GetTypeName(), (int)name_hash);
			return ERR_NAME_ALREADY_TAKEN;
		}
#endif

		const U32 bucket = Hash32Bits_2(name_hash) % TABLE_SIZE;

		current->m_link = m_heads[ bucket ];
		m_heads[ bucket ] = current;

		class_index++;

		current = next;
	}

#if MX_DEBUG
	int num_free_slots = 0;
	int num_collisions = 0;
	int max_collisions = 0;
	for( int i = 0; i < mxCOUNT_OF(m_heads); i++ )
	{
		const TbMetaClass* p = m_heads[i];
		if( p )
		{
			int items_in_bucket = 0;
			while( p ) {
				items_in_bucket++;
				p = p->m_link;
			}
			max_collisions = Max( max_collisions, items_in_bucket );
			if( items_in_bucket > 1 ) {
				num_collisions++;
			}
		}
		else
		{
			num_free_slots++;
		}
	}
	DBGOUT("TypeRegistry::Initialize(): %d classes (%d collisions/%d max, %d free slots)",
		class_index, num_collisions, max_collisions, num_free_slots);
#endif

	return ALL_OK;
}

void TypeRegistry::Destroy()
{
}

const TbMetaClass* TypeRegistry::FindClassByGuid( const TypeGUID& typeCode )
{
	const U32 bucket = Hash32Bits_2(typeCode) % TABLE_SIZE;
	const TbMetaClass* current = m_heads[ bucket ];
	while( PtrToBool(current) )
	{
		const TbMetaClass* next = current->m_link;
		if( current->GetTypeGUID() == typeCode ) {
			return current;
		}
		current = next;
	}
	return NULL;
}

const TbMetaClass* TypeRegistry::FindClassByName( const char* className )
{
	const TypeGUID& name_hash = GetDynamicStringHash( className );
	return FindClassByGuid( name_hash );
}

AObject* TypeRegistry::CreateInstance( const TypeGUID& typeCode )
{
	AObject* pObjectInstance = ObjectUtil::Create_Object_Instance( typeCode );
	mxASSERT_PTR( pObjectInstance );
	return pObjectInstance;
}

void TypeRegistry::EnumerateDescendants( const TbMetaClass& baseClass, TArray<const TbMetaClass*> &OutClasses )
{
	TbMetaClass* current = TbMetaClass::m_head;

	while( PtrToBool(current) )
	{
		TbMetaClass* next = current->m_next;

		if( current != &baseClass && current->IsDerivedFrom( baseClass ) )
		{
			OutClasses.add( current );
		}

		current = next;
	}
}

void TypeRegistry::EnumerateConcreteDescendants( const TbMetaClass& baseClass, TArray<const TbMetaClass*> &OutClasses )
{
	TbMetaClass* current = TbMetaClass::m_head;

	while( PtrToBool(current) )
	{
		TbMetaClass* next = current->m_next;

		if( current != &baseClass && current->IsDerivedFrom( baseClass ) && current->IsConcrete() )
		{
			OutClasses.add( current );
		}

		current = next;		
	}
}

void TypeRegistry::ForAllClasses( F_ClassIterator* visitor, void* userData )
{
	mxASSERT_PTR(visitor);

	TbMetaClass* current = TbMetaClass::m_head;

	while( PtrToBool(current) )
	{
		TbMetaClass* next = current->m_next;

		if( (*visitor)( *current, userData ) ) {
			break;
		}

		current = next;		
	}
}
