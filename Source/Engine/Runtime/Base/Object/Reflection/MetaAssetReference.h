/*
=============================================================================
	File:	MetaAssetReference.h
	Desc:	reflection asset/resource pointers/references (TResPtr)
=============================================================================
*/
#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>

///
struct MetaAssetReference: public TbMetaType
{
	const TbMetaClass &	asset_type;	// type of the referenced object

public:
	inline MetaAssetReference(
		const char* type_name
		, const TbTypeSizeInfo& size_info
		, const TbMetaClass& asset_type
		)
		: TbMetaType(
			ETypeKind::Type_AssetReference
			, type_name
			, size_info
		)
		, asset_type( asset_type )
	{
	}

	/// must be synced with TResPtr<>
	enum { ASSET_ID_OFFSET = sizeof(void*) };
};
