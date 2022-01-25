/*
=============================================================================
	File:	PointerType.h
	Desc:	
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>

struct MetaPointer: public TbMetaType
{
	const TbMetaType &	pointee;	// type of the referenced object

public:
	inline MetaPointer( const char* typeName, const TbTypeSizeInfo& typeInfo, const TbMetaType& pointeeType )
		: TbMetaType( ETypeKind::Type_Pointer, typeName, typeInfo )
		, pointee( pointeeType )
	{
	}
};

template< typename TYPE >
struct TypeDeducer< TYPE* >
{
	static inline const TbMetaType& GetType()
	{
		const TbMetaType& pointeeType = T_DeduceTypeInfo< TYPE >();

		static MetaPointer staticTypeInfo(
			mxEXTRACT_TYPE_NAME(Pointer),
			TbTypeSizeInfo::For_Type< TYPE* >(),
			pointeeType
		);

		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Pointer;
	}
};

template< typename TYPE >
struct TypeDeducer< TYPE& >
{
	static inline const TbMetaType& GetType()
	{
		const TbMetaType& pointeeType = T_DeduceTypeInfo< TYPE >();

		static MetaPointer staticTypeInfo(
			mxEXTRACT_TYPE_NAME(Pointer),
			TbTypeSizeInfo::For_Type< TYPE* >(),
			pointeeType
		);

		return staticTypeInfo;
	}
	static inline ETypeKind GetTypeKind()
	{
		return ETypeKind::Type_Pointer;
	}
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
