#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/Reflection.h>
#include <Base/Object/Reflection/EnumType.h>

/*
-------------------------------------------------------------------------
	MetaFlags
	TBits< ENUM, STORAGE > - 8,16,32 bits of named values.
-------------------------------------------------------------------------
*/
struct MetaFlags: public MetaEnum
{
public:
	inline MetaFlags(
		const char* typeName
		, const ItemList& _items
		, GetterF* const _getter
		, SetterF* const _setter
		, const TbTypeSizeInfo& info
		)
		: MetaEnum( ETypeKind::Type_Flags, typeName, _items, _getter, _setter, info )
	{}
};

//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- ENUM - name of enumeration (e.g. EMyEnum)
//!=- STORAGE - type of storage (e.g. U32)
//!=- ALIAS - typedef of TBits<ENUM,STORAGE> (e.g. MyEnumF)
//
#define mxDECLARE_FLAGS( ENUM, STORAGE, ALIAS )\
	typedef TBits< ENUM, STORAGE > ALIAS;\
	mxDECLARE_POD_TYPE( ALIAS );\
	\
	extern const MetaFlags::ItemList& PP_JOIN(GetMembersOf_,ALIAS)();\
	\
	struct PP_JOIN(ALIAS,_Type): public MetaFlags\
	{\
		PP_JOIN(ALIAS,_Type)()\
			: MetaFlags(\
				mxEXTRACT_TYPE_NAME(ALIAS),\
				PP_JOIN(GetMembersOf_,ALIAS)(),\
				&(Converter< STORAGE >::Get),\
				&(Converter< STORAGE >::Set),\
				TbTypeSizeInfo::For_Type< ALIAS >() )\
		{}\
		static const PP_JOIN(ALIAS,_Type)	s_instance;\
	};\
	const PP_JOIN(ALIAS,_Type)& PP_JOIN(GetTypeOf_,ALIAS)();\
	\
	extern const PP_JOIN(ALIAS,_Type)& PP_JOIN(GetTypeOf_,ALIAS)();\
	\
	template<>\
	struct TypeDeducer< ALIAS >\
	{\
		static inline const TbMetaType& GetType()\
		{\
			return PP_JOIN(GetTypeOf_,ALIAS)();\
		}\
		static inline ETypeKind GetTypeKind()\
		{\
			return ETypeKind::Type_Flags;\
		}\
	};

//!----------------------------------------------------------------------


//!=- MACRO -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!=- should be placed into source files
//
#define mxBEGIN_FLAGS( ALIAS )\
	const PP_JOIN(ALIAS,_Type)& PP_JOIN(GetTypeOf_,ALIAS)()\
	{\
		static const PP_JOIN(ALIAS,_Type)	s_instance;\
		return s_instance;\
	}\
	const MetaFlags::ItemList& PP_JOIN( GetMembersOf_, ALIAS )() {\
		static MetaFlags::Item s_items[] = {\


//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxREFLECT_BIT( NAME, VALUE )\
	{\
		mxEXTRACT_NAME(NAME),\
		VALUE\
	}


//!=- MACRO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
#define mxEND_FLAGS\
		};\
		static MetaFlags::ItemList s_itemList = { s_items, mxCOUNT_OF(s_items) };\
		return s_itemList;\
	}


//!----------------------------------------------------------------------
void Dbg_FlagsToString( const void* _o, const MetaFlags& _type, String &_string );

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
