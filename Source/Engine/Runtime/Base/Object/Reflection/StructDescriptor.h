#pragma once

#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/ClassDescriptor.h>

/*
-------------------------------------------------------------------------
	mxStruct

	C-style structure
	(i.e. inheritance is not allowed, only member fields are reflected)
-------------------------------------------------------------------------
*/


//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- can only be used on C-like structs
//!=- should be placed into header files
//
#define mxDECLARE_STRUCT( STRUCT )\
	extern TbClassLayout& PP_JOIN( Reflect_Struct_, STRUCT )();\
	template<>\
	struct TypeDeducer< STRUCT >\
	{\
		static inline ETypeKind GetTypeKind()\
		{\
			return ETypeKind::Type_Class;\
		}\
		static inline const TbMetaType& GetType()\
		{\
			return GetClass();\
		}\
		static inline const TbMetaClass& GetClass()\
		{\
			static TbMetaClass staticTypeInfo(\
								mxEXTRACT_TYPE_NAME( STRUCT ),\
								mxEXTRACT_TYPE_GUID( STRUCT ),\
								nil /*parent class*/,\
								SClassDescription::For_Class_With_Default_Ctor< STRUCT >(),\
								PP_JOIN( Reflect_Struct_, STRUCT )(),\
								TbAssetTypeTraits<STRUCT>::getAssetLoader()\
							);\
			return staticTypeInfo;\
		}\
	};

//!----------------------------------------------------------------------




//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- can only be used on C-like structs
//!=- should be placed into source files
//
#define mxBEGIN_STRUCT( STRUCT )\
	TbClassLayout& PP_JOIN( Reflect_Struct_, STRUCT )() {\
		typedef STRUCT OuterType;\
		static MetaField fields[] = {\

// use mxMEMBER_FIELD*
// and mxEND_REFLECTION

//!----------------------------------------------------------------------





//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- can only be used on C-like structs
//!=- which inherit from CStruct class
//!=- should be placed into source files
//
#define mxREFLECT_STRUCT_VIA_STATIC_METHOD( STRUCT )\
	TbClassLayout& PP_JOIN( Reflect_Struct_, STRUCT )() {\
		return STRUCT::staticLayout();\
	}\

//!----------------------------------------------------------------------





//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- can only be used on C-like structs
//!=- should be placed into source files
//!=- basically allows to reinterpret the struct as another struct
//!=- (e.g.this can be used to alias __m128 as XMFLOAT4)
//
#define mxREFLECT_STRUCT_AS_ANOTHER_STRUCT( THIS_STRUCT, OTHER_STRUCT )\
	TbClassLayout& PP_JOIN( Reflect_Struct_, THIS_STRUCT )() {\
		mxSTATIC_ASSERT( sizeof(THIS_STRUCT) == sizeof(OTHER_STRUCT) );\
		return PP_JOIN( Reflect_Struct_, OTHER_STRUCT )();\
	}\
//!----------------------------------------------------------------------



//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
