/*
=============================================================================
	File:	BaseType.cpp
	Desc:	Common base class.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/BaseType.h>

/*================================
		CStruct
================================*/

/*static*/
TbMetaClass CStruct::ms_staticTypeInfo(
	mxEXTRACT_NAME(CStruct), mxNULL_TYPE_ID,
	nil,
	SClassDescription(),
	TbClassLayout::dummy,
	nil	// Asset Loader
);

/*================================
			AObject
================================*/

/*static*/
TbMetaClass	AObject::ms_staticTypeInfo( 
	mxEXTRACT_NAME(AObject), mxNULL_TYPE_ID+1,
	&CStruct::metaClass(),
	SClassDescription(),
	TbClassLayout::dummy,
	nil	// Asset Loader
);

AObject::~AObject()
{
}

namespace ObjectUtil
{
	AObject* Create_Object_Instance( const TbMetaClass& classInfo )
	{
		chkRET_NIL_IF_NOT( classInfo.IsConcrete() );

		AObject* pNewInstance = classInfo.CreateInstance();
		mxASSERT_PTR( pNewInstance );

		return pNewInstance;
	}
	AObject* Create_Object_Instance( const TypeGUID& classGuid )
	{
		const TbMetaClass* pClass = TypeRegistry::FindClassByGuid( classGuid );
		chkRET_NIL_IF_NIL(pClass);

		AObject* pNewInstance = Create_Object_Instance( *pClass );

		return pNewInstance;
	}

}//namespace ObjectUtil

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
