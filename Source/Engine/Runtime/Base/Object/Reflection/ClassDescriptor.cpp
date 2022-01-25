/*
=============================================================================
	File:	TbMetaClass.cpp
	Desc:	Classes for run-time type checking
			and run-time instancing of objects.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/BaseType.h>
#include <Base/Object/Reflection/TypeRegistry.h>
#include <Base/Object/Reflection/ClassDescriptor.h>

/*================================
		TbMetaClass
================================*/
//tbPRINT_SIZE_OF(TbMetaClass);
// static
TbMetaClass* TbMetaClass::m_head = nil;

TbMetaClass::TbMetaClass(
	const char* class_name,
	const TypeGUID& class_guid,
	const TbMetaClass* const base_class,
	const SClassDescription& class_info,
	const TbClassLayout& reflected_members,
	TbAssetLoaderI* asset_loader
	)
	: TbMetaType( ETypeKind::Type_Class, class_name, class_info )
	, m_uid( class_guid )
	, m_base( base_class )
	, m_members( reflected_members )
{
	mxASSERT( m_base != this );	// but parentType can be NULL for root classes

	m_creator = class_info.creator;
	m_constructor = class_info.constructor;
	m_destructor = class_info.destructor;

	// Insert this object into the linked list
	m_next = m_head;
	m_head = this;
	m_link = nil;

	loader = asset_loader;

	//this->loader = loader;
	//fallbackInstance = nil;	
	//editorInfo = nil;
	//allocationGranularity = 1;
}

bool TbMetaClass::IsDerivedFrom( const TbMetaClass& other ) const
{
	for ( const TbMetaClass * current = this; current != nil; current = current->GetParent() )
	{
		if ( current == &other )
		{
			return true;
		}
	}
	return false;
}

bool TbMetaClass::IsDerivedFrom( const TypeGUID& typeCode ) const
{
	mxASSERT(typeCode != mxNULL_TYPE_ID);
	for ( const TbMetaClass * current = this; current != nil; current = current->GetParent() )
	{
		if ( current->GetTypeGUID() == typeCode )
		{
			return true;
		}
	}
	return false;
}

bool TbMetaClass::IsDerivedFrom( PCSTR class_name ) const
{
	mxASSERT_PTR(class_name);
	for ( const TbMetaClass * current = this; current != nil; current = current->GetParent() )
	{
		if ( 0 == strcmp( m_name, class_name ) )
		{
			return true;
		}
	}
	return false;
}

AObject* TbMetaClass::CreateInstance() const
{
	mxASSERT( this->IsConcrete() );

	F_CreateObject* pCreateFunction = this->GetCreator();
	mxASSERT_PTR( pCreateFunction );

	AObject* pObjectInstance = (AObject*) (*pCreateFunction)();
	mxASSERT_PTR( pObjectInstance );

	return pObjectInstance;
}

bool TbMetaClass::ConstructInPlace( void* o ) const
{
	F_ConstructObject* constructor = this->GetConstructor();
	chkRET_FALSE_IF_NIL(constructor);
	(*constructor)( o );
	return true;
}

bool TbMetaClass::DestroyInstance( void* o ) const
{
	F_DestructObject* destructor = this->GetDestructor();
	chkRET_FALSE_IF_NIL(destructor);
	(*destructor)( o );
	return true;
}

bool TbMetaClass::IsEmpty() const
{
	UINT numFields = 0;
	for ( const TbMetaClass * current = this; current != nil; current = current->GetParent() )
	{
		numFields += current->GetLayout().numFields;
	}
	return numFields == 0;
}

//-----------------------------------------------------------------
//	STypedObject
//-----------------------------------------------------------------
//
STypedObject::STypedObject()
	: o( nil )
	, type( &CStruct::metaClass() )
{}

//-----------------------------------------------------------------
//	SClassIdType
//-----------------------------------------------------------------
//
SClassIdType SClassIdType::ms_staticInstance;
