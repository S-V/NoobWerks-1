/*
=============================================================================
	File:	TypeDescriptor.cpp
	Desc:	
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/ArrayDescriptor.h>
#include <Base/Object/Reflection/PointerType.h>

#if MX_DEVELOPER

	const char* ETypeKind_To_Chars( ETypeKind typeKind )
	{
		switch( typeKind )
		{
		//case ETypeKind::Type_Unknown :

		case ETypeKind::Type_Void :		return "Void";

		case ETypeKind::Type_Integer :	return "Integer";
		case ETypeKind::Type_Float :	return "Float";

		case ETypeKind::Type_Bool :		return "Boolean";

		case ETypeKind::Type_Enum :		return "Enum";

		case ETypeKind::Type_Flags :	return "Flags";

		case ETypeKind::Type_String :	return "String";

		case ETypeKind::Type_Class :	return "Class";

		case ETypeKind::Type_Pointer :	return "Pointer";
		case ETypeKind::Type_AssetReference :	return "AssetReference";

		case ETypeKind::Type_ClassId :	return "ClassId";
		case ETypeKind::Type_AssetId :	return "AssetID";

		case ETypeKind::Type_UserData :	return "UserData";
		case ETypeKind::Type_Blob	:	return "Blob";

		case ETypeKind::Type_Array :	return "Array";
			
		mxNO_SWITCH_DEFAULT;
		}

		return "Unknown";
	}

#endif // MX_DEVELOPER

mxNO_EMPTY_FILE
